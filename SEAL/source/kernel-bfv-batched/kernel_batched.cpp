#include "kernel_batched.h"

namespace {
void log_time(std::stringstream &ss,
              std::chrono::time_point<std::chrono::high_resolution_clock> start,
              std::chrono::time_point<std::chrono::high_resolution_clock> end,
              bool last = false) {
  ss << std::chrono::duration_cast<ms>(end - start).count();
  if (!last) ss << ",";
}
}  // namespace

Evaluation::Evaluation(int image_size) : image_size(image_size){};

void Evaluation::check_results(VecInt2D img,
                               std::vector<int64_t> computed_values) {
  // run the original algorithm derived from Ramparts' paper
  VecInt2D expected_values = img;
  for (int x = 1; x < img.size() - 1; x++) {
    for (int y = 1; y < img.at(x).size() - 1; y++) {
      int value = 0;
      for (int j = -1; j < 2; ++j) {
        for (int i = -1; i < 2; ++i) {
          value += weight_matrix.at((i + 1) * std::log2(weight_matrix.size()) +
                                    (j + 1)) *
                   img.at(x + i).at(y + j);
        }
      }
      // original: img2[x][y] = img[x][y] - (value / 2);
      // modified variant as we cannot divide in FHE:
      expected_values[x][y] = (2 * img[x][y]) - value;
    }
  }

  // compare FHE computation result with original algorithm's result
  for (size_t x = 0; x < img.size(); x++) {
    for (size_t y = 0; y < img.at(x).size(); y++) {
      auto exp = expected_values[x][y];
      auto cmp = computed_values[y + x * img.size()];
      if (cmp != exp) {
        std::cerr << "MISMATCH: Expected (" << exp << ") vs computed value ("
                  << cmp << ")." << std::endl;
      }
    }
  }
}

std::vector<int64_t> Evaluation::decrypt_and_decode(seal::Ciphertext &ctxt) {
  seal::Plaintext decrypted;
  decryptor->decrypt(ctxt, decrypted);
  return decode(decrypted);
}

std::vector<int64_t> Evaluation::decode(seal::Plaintext &ptxt) {
  std::vector<int64_t> values;
  encoder->decode(ptxt, values);
  return values;
}

void print_all(std::vector<int64_t> &vector) {
  std::cout << "=== print_all called ==============" << std::endl;
  for (size_t i = 0; (i < vector.size() && i < PRINT_LIMIT); i++) {
    std::cout << i << ": " << vector[i] << std::endl;
  }
  std::cout << "===================================" << std::endl;
}

std::vector<int64_t> Evaluation::generate_border_mask(bool invert) {
  std::vector<int64_t> data(image_size * image_size, invert);
  for (size_t i = 0; i < image_size * image_size; i++) {
    int x = i % image_size;
    int y = i / image_size;
    data[i] = (y == 0)                          // top border
                      || (y == image_size - 1)  // bottom border
                      || (x == 0 || x == 7)     // lhs and rhs borders
                  ? !invert
                  : invert;
  }
  return data;
}

std::vector<int64_t> Evaluation::run_kernel(VecInt2D img) {
  std::stringstream ss_time;
  Timepoint t_start_keygen = Time::now();

  seal::EncryptionParameters parms(seal::scheme_type::BFV);
  parms.set_poly_modulus_degree(DEFAULT_NUM_SLOTS);  // = no. of ctxt slots
  // Let SEAL select a "suitable" coefficient modulus (not necessarily maximal)
  parms.set_coeff_modulus(
      seal::CoeffModulus::BFVDefault(parms.poly_modulus_degree()));
  // Let SEAL select a plaintext modulus that actually supports batching
  parms.set_plain_modulus(
      seal::PlainModulus::Batching(parms.poly_modulus_degree(), 20));
  context = seal::SEALContext::Create(parms);

  /// Create keys
  seal::KeyGenerator keygen(context);
  secret_key = std::make_unique<seal::SecretKey>(keygen.secret_key());
  public_key = std::make_unique<seal::PublicKey>(keygen.public_key());
  galois_keys = std::make_unique<seal::GaloisKeys>(keygen.galois_keys_local());
  relin_keys = std::make_unique<seal::RelinKeys>(keygen.relin_keys_local());

  // Create helper objects
  encoder = std::make_unique<seal::BatchEncoder>(context);
  encryptor =
      std::make_unique<seal::Encryptor>(context, *public_key, *secret_key);
  decryptor = std::make_unique<seal::Decryptor>(context, *secret_key);
  evaluator = std::make_unique<seal::Evaluator>(context);

  Timepoint t_end_keygen = Time::now();
  log_time(ss_time, t_start_keygen, t_end_keygen, false);

  // Encrypt input image
  Timepoint t_start_input_encryption = Time::now();
  std::vector<int64_t> img_as_vec(image_size * image_size, 0);
  for (size_t i = 0; i < img.size(); i++) {
    for (size_t j = 0; j < img[i].size(); j++) {
      img_as_vec[i * image_size + j] = img[i][j];
    }
  }
  seal::Plaintext img_ptxt;
  encoder->encode(img_as_vec, img_ptxt);
  seal::Ciphertext img_ctxt;
  encryptor->encrypt_symmetric(img_ptxt, img_ctxt);  // symm is more efficient

  Timepoint t_end_input_encryption = Time::now();
  log_time(ss_time, t_start_input_encryption, t_end_input_encryption, false);

  Timepoint t_start_computation = Time::now();

  // Create rotated copies of the image and multiplicate by weights
  std::vector<int> rotations = {-0,
                                -1,
                                -2,
                                -image_size,
                                -(image_size + 1),
                                -(image_size + 2),
                                -(2 * image_size),
                                -(2 * image_size + 1),
                                -(2 * image_size + 2)};
  seal::Plaintext w_ptxt;
  std::vector<seal::Ciphertext> img_ctxts(weight_matrix.size(),
                                          seal::Ciphertext(context));
  for (size_t i = 0; i < weight_matrix.size(); ++i) {
    evaluator->rotate_rows(img_ctxt, rotations[i], *galois_keys, img_ctxts[i]);
    encoder->encode(
        std::vector<int64_t>(encoder->slot_count(), weight_matrix[i]), w_ptxt);
    evaluator->multiply_plain_inplace(img_ctxts[i], w_ptxt);
  }

  // Sum up all the ciphertexts
  seal::Ciphertext res_ctxt(context);
  evaluator->add_many(img_ctxts, res_ctxt);

  // Move the computed result to the expected position, e.g., first computed
  // value must be at (1,1) as kernel leaves border untouched
  evaluator->rotate_rows_inplace(res_ctxt, image_size + 1, *galois_keys);

  // result = 2*img_ctxt - value
  // (1) 2*img_ctxt
  seal::Plaintext two;
  std::vector<int64_t> full_two(encoder->slot_count(), 2);
  encoder->encode(full_two, two);
  seal::Ciphertext two_times_img_ctxt;
  evaluator->multiply_plain(img_ctxt, two, two_times_img_ctxt);
  // (2) [2*img_ctxt] - value
  evaluator->sub_inplace(two_times_img_ctxt, res_ctxt);

  // Remove anything except the border from the input image
  std::vector<int64_t> data_border = generate_border_mask(false);
  seal::Plaintext mask_border_only;
  encoder->encode(data_border, mask_border_only);
  evaluator->multiply_plain_inplace(img_ctxt, mask_border_only);
  // Remove the border from the computed result
  std::vector<int64_t> data_inner = generate_border_mask(true);
  seal::Plaintext mask_inner_only;
  encoder->encode(data_inner, mask_inner_only);
  evaluator->multiply_plain_inplace(two_times_img_ctxt, mask_inner_only);
  // Merge the input image (border-only) with the computed kernel (border = 0)
  evaluator->add_inplace(img_ctxt, two_times_img_ctxt);

  Timepoint t_end_computation = Time::now();
  log_time(ss_time, t_start_computation, t_end_computation, false);

  Timepoint t_start_decryption = Time::now();
  auto final_result = decrypt_and_decode(img_ctxt);
  Timepoint t_end_decryption = Time::now();
  log_time(ss_time, t_start_decryption, t_end_decryption, true);

  // write ss_time into file
  std::ofstream myfile;
  auto out_filename = std::getenv("OUTPUT_FILENAME");
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();

  // write FHE parameters into file
  write_parameters_to_file(context, "fhe_parameters_kernel.txt");

  // return decrypted+decoded ciphertext
  return final_result;
}

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'kernel-bfv-batched'..." << std::endl;

  // std::vector<int> image_sizes = { 8, 16, 32, 64, 96, 128 };
  std::vector<int> image_sizes = {8};

  for (auto img_size : image_sizes) {
    // generate input image with dummy data
    std::vector<int> vec(img_size);
    // std::vector<int> vec(img_size, 1);
    std::iota(vec.begin(), vec.end(), 0);

    // run kernel using FHE
    std::vector<std::vector<int>> img(img_size, vec);
    Evaluation eval(img.size());
    auto result = eval.run_kernel(img);

    eval.check_results(img, result);
  }
}
