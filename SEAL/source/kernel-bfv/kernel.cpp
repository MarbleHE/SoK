#include "kernel.h"

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

void Evaluation::apply_kernel(VecInt2D &img) {
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
  seal::KeyGenerator keyGenerator(context);
  secret_key = std::make_unique<seal::SecretKey>(keyGenerator.secret_key());
  public_key = std::make_unique<seal::PublicKey>(keyGenerator.public_key());
  galois_keys =
      std::make_unique<seal::GaloisKeys>(keyGenerator.galois_keys_local());
  relin_keys =
      std::make_unique<seal::RelinKeys>(keyGenerator.relin_keys_local());

  // Create helper objects
  encoder = std::make_unique<seal::BatchEncoder>(context);
  encryptor =
      std::make_unique<seal::Encryptor>(context, *public_key, *secret_key);
  decryptor = std::make_unique<seal::Decryptor>(context, *secret_key);
  evaluator = std::make_unique<seal::Evaluator>(context);

  // Encrypt input
  std::vector<int64_t> img_as_vec;
  img_as_vec.reserve(img.size() * img.size());
  for (auto row : img) {
    for (auto elem : row) {
      img_as_vec.push_back(elem);
    }
  }

  seal::Plaintext img_ptxt;
  encoder->encode(img_as_vec, img_ptxt);
  seal::Ciphertext img_ctxt;
  encryptor->encrypt_symmetric(img_ptxt, img_ctxt);

  // Naive way: very similar to plain C++
  VecInt2D weightMatrix = {{1, 1, 1}, {1, -8, 1}, {1, 1, 1}};
  // Can be default constructed because this is overriden in each loop
  // seal::Ciphertext output_img;
  seal::Ciphertext img2_ctxt = img_ctxt;

  for (int x = 1; x < img.size() - 1; ++x) {
    for (int y = 1; y < img.at(x).size() - 1; ++y) {
      std::cout << "== x: " << x << ", y: " << y << std::endl;
      seal::Ciphertext value(context);
      for (int j = -1; j < 2; ++j) {
        for (int i = -1; i < 2; ++i) {
          std::cout << "x+i: " << x + i << ", y+j: " << y + j
                    << ", i+1: " << i + 1 << ", j+1: " << j + 1 << std::endl
                    << "(y+j) + (x+i)*img.size(): "
                    << (y + j) + (x + i) * img.size() << std::endl;

          // set w at index where input is to the value of weightMatrix
          seal::Plaintext w;
          std::vector<int64_t> data(img.size() * img.size(), 0);
          data[(y + j) + (x + i) * img.size()] =
              weightMatrix.at(i + 1).at(j + 1);
          encoder->encode(data, w);

          // encoder->encode(std::vector<int64_t>
          //      (encoder->slot_count(), weightMatrix.at(i + 1).at(j + 1)), w);
          // int target_x = (x + i);
          // int target_y = (y + j);
          // int steps = target_x * img.size() + target_y;
          // if (steps >= DEFAULT_NUM_SLOTS / 2) {
          //   steps = DEFAULT_NUM_SLOTS / 2 - steps;
          // }

          // evaluator.rotate_rows(img_ctxt, steps, *galois_keys, temp);

          // temp = img_ctxt * w
          seal::Ciphertext temp;
          evaluator->multiply_plain(img_ctxt, w, temp);
          evaluator->relinearize_inplace(temp, *relin_keys);

          // rotate ciphertext temp so that the respective index (i+1, j+1) is
          // at index 0 afterward
          evaluator->rotate_rows_inplace(temp, ((j + 1) + (i + 1) * img.size()),
                                         *galois_keys);

          // value = value + temp
          evaluator->add_inplace(value, temp);
        }
      }

      // temp = img_ctxt * 2
      seal::Plaintext two;
      // encoder->encode(std::vector<int64_t>(encoder->slot_count(), 2), two);
      std::vector<int64_t> two_data(img.size() * img.size(), 0);
      two_data[y + x * img.size()] = 2;
      encoder->encode(two_data, two);
      seal::Ciphertext temp;
      evaluator->multiply_plain(img_ctxt, two, temp);
      evaluator->relinearize_inplace(temp, *relin_keys);

      // rotate 'value' so that it is at position (x,y) afterwards (UNCHECKED)
      evaluator->rotate_rows_inplace(value, -(y + x * img.size()),
                                     *galois_keys);

      // temp = temp - value
      evaluator->sub_inplace(temp, value);

      // Use masking to merge img2 and the new computed value for pixel (x,y)
      // img2_ctxt = "all 1, except (x,y)" * img2_ctxt + temp
      // (1) masked_img2 = img2_ctxt * mask
      seal::Plaintext mask;
      std::vector<int64_t> data(img.size() * img.size(), 1);
      data[y + x * img.size()] = 0;
      encoder->encode(data, mask);
      seal::Ciphertext masked_img2;
      evaluator->multiply_plain(img2_ctxt, mask, masked_img2);
      evaluator->relinearize_inplace(masked_img2, *relin_keys);

      // (2) img2_ctxt = masked_img2 + temp
      evaluator->add(masked_img2, temp, img2_ctxt);

      // if (x == 2 && y == 1) {
      //   std::cout << "masked_img2 :::" << std::endl;
      //   auto vals = decrypt_and_decode(masked_img2);
      //   print_all(vals);

      //   std::cout << "img2_ctxt :::" << std::endl;
      //   auto vals2 = decrypt_and_decode(img2_ctxt);
      //   print_all(vals2);
      //   return;
      // }
    }
  }

  // decrypt ciphertext
  seal::Plaintext decrypted;
  decryptor->decrypt(img2_ctxt, decrypted);
  std::vector<int64_t> values;
  encoder->decode(decrypted, values);
  int row_length = img.size();
  for (size_t i = 0; i < values.size() && i < PRINT_LIMIT; i++) {
    int x = i % row_length;
    int y = i / row_length;
    std::cout << "(" << x << "," << y << "): " << values[i] << std::endl;
  }
}

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'kernel-bfv'..." << std::endl;
  std::cout << "image_size,time_ms" << std::endl;

  // std::vector<int> image_sizes = {8, 16, 32, 64, 96, 128};
  std::vector<int> image_sizes = {8};

  for (auto img_size : image_sizes) {
    std::vector<int> vec(img_size);
    std::iota(vec.begin(), vec.end(), 0);
    std::vector<std::vector<int>> img(img_size, vec);
    Timepoint t_start = std::chrono::high_resolution_clock::now();

    Evaluation().apply_kernel(img);
    Timepoint t_end = std::chrono::high_resolution_clock::now();
    Duration duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start)
            .count();
    std::cout << img_size << "," << duration_ms << std::endl;
  }
}
