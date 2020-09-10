#include "kernel.h"

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

Duration Evaluation::compute_duration(Timepoint start, Timepoint end) {
  if (end < start) {
    std::cerr << "ERROR: Timepoint 'end' cannot be smaller than 'start'!"
              << std::endl;
  }
  return std::chrono::duration_cast<ms>(end - start).count();
}

void Evaluation::check_results(VecInt2D img,
                               std::vector<int64_t> computed_values) {
  // run the original algorithm derived from Ramparts' paper
  VecInt2D expected_values = img;
  for (int x = 1; x < img.size() - 1; x++) {
    for (int y = 1; y < img.at(x).size() - 1; y++) {
      int value = 0;
      for (int j = -1; j < 2; ++j) {
        for (int i = -1; i < 2; ++i) {
          value += weight_matrix.at(i + 1).at(j + 1) * img.at(x + i).at(y + j);
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

void Evaluation::print_all(std::vector<int64_t> &vector) {
  std::cout << "=== print_all called ==============" << std::endl;
  for (size_t i = 0; (i < vector.size() && i < PRINT_LIMIT); i++) {
    std::cout << i << ": " << vector[i] << std::endl;
  }
  std::cout << "===================================" << std::endl;
}

std::vector<int64_t> Evaluation::apply_kernel(VecInt2D &img) {
  std::stringstream ss_time;

  Timepoint t_start_keygen = Time::now();

  seal::EncryptionParameters parms =
      seal::EncryptionParameters(seal::scheme_type::BFV);
  parms.set_poly_modulus_degree(DEFAULT_NUM_SLOTS);  // number of slots
  // Let SEAL select a "suitable" coefficient modulus
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
  std::vector<int64_t> img_as_vec;
  img_as_vec.reserve(image_size * image_size);
  for (auto row : img) {
    for (auto elem : row) {
      img_as_vec.push_back(elem);
    }
  }
  seal::Plaintext img_ptxt;
  encoder->encode(img_as_vec, img_ptxt);
  seal::Ciphertext img_ctxt;
  encryptor->encrypt_symmetric(img_ptxt, img_ctxt);

  Timepoint t_end_input_encryption = Time::now();
  log_time(ss_time, t_start_input_encryption, t_end_input_encryption, false);

  Timepoint t_start_computation = Time::now();

  // Mask away (in a single mult) everything except borders because later we
  // need to overwrite these masked-away values using additions
  std::vector<int64_t> data(image_size * image_size, 0);
  for (size_t i = 0; i < image_size * image_size; i++) {
    int x = i % image_size;
    int y = i / image_size;
    data[i] = (y == 0)                  // top border
              || (y == image_size - 1)  // bottom border
              || (x == 0 || x == 7);    // lhs and rhs borders
  }
  seal::Plaintext mask;
  encoder->encode(data, mask);
  seal::Ciphertext img2_ctxt;
  evaluator->multiply_plain(img_ctxt, mask, img2_ctxt);

  for (int x = 1; x < image_size - 1; ++x) {
    for (int y = 1; y < img.at(x).size() - 1; ++y) {
      seal::Ciphertext value;
      seal::Plaintext value_ptxt;
      std::vector<int64_t> zero(image_size * image_size, 0);
      encoder->encode(zero, value_ptxt);
      encryptor->encrypt(value_ptxt, value);

      for (int j = -1; j < 2; ++j) {
        for (int i = -1; i < 2; ++i) {
          // set weight at index where input is
          seal::Plaintext w;
          std::vector<int64_t> data(image_size * image_size, 0);
          data.at((y + j) + (x + i) * image_size) =
              weight_matrix.at(i + 1).at(j + 1);
          encoder->encode(data, w);

          // temp = img_ctxt * w
          seal::Ciphertext temp;
          evaluator->multiply_plain(img_ctxt, w, temp);

          // rotate ciphertext temp so that the value is at index (x,y)
          evaluator->rotate_rows_inplace(temp, (j + i * image_size),
                                         *galois_keys);

          // value = value + temp
          evaluator->add_inplace(value, temp);
        }
      }

      // temp = img_ctxt[x][y] * 2
      seal::Plaintext two;
      std::vector<int64_t> two_data(image_size * image_size, 0);
      two_data.at(y + x * image_size) = 2;
      encoder->encode(two_data, two);
      seal::Ciphertext temp;
      evaluator->multiply_plain(img_ctxt, two, temp);

      // temp = temp - value
      evaluator->sub_inplace(temp, value);

      // img2_ctxt = img2_ctxt + temp
      evaluator->add_inplace(img2_ctxt, temp);
    }
  }

  Timepoint t_end_computation = Time::now();
  log_time(ss_time, t_start_computation, t_end_computation, false);

  Timepoint t_start_decryption = Time::now();
  auto final_result = decrypt_and_decode(img2_ctxt);
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
  std::cout << "Starting benchmark 'kernel-bfv'..." << std::endl;

  // std::vector<int> image_sizes = {8, 16, 32, 64, 96, 128};
  std::vector<int> image_sizes = {8};

  for (auto img_size : image_sizes) {
    // generate input
    std::vector<int> vec(img_size);
    std::iota(vec.begin(), vec.end(), 0);
    std::vector<std::vector<int>> img(img_size, vec);
    
    // perform FHE computation
    Evaluation eval(img.size());
    auto fhe_result = eval.apply_kernel(img);

    // check correctness of results
    eval.check_results(img, fhe_result);
  }
}
