#include "kernel.h"

void Evaluation::encryptedLaplacianSharpeningAlgorithmNaive(VecInt2D img) {
  /// Wrapper for parameters
  seal::EncryptionParameters params(seal::scheme_type::BFV);

  // in BFV, this degree is also the number of slots.
  params.set_poly_modulus_degree(DEFAULT_NUM_SLOTS);

  // Let SEAL select a "suitable" coefficient modulus (not necessarily maximal)
  params.set_coeff_modulus(
      seal::CoeffModulus::BFVDefault(params.poly_modulus_degree()));

  // Let SEAL select a plaintext modulus that actually supports batching
  params.set_plain_modulus(
      seal::PlainModulus::Batching(params.poly_modulus_degree(), 20));

  // Instantiate context
  std::shared_ptr<seal::SEALContext> context =
      seal::SEALContext::Create(params);

  /// Helper object to create keys
  seal::KeyGenerator keyGenerator(context);

  std::unique_ptr<seal::SecretKey> secretKey =
      std::make_unique<seal::SecretKey>(keyGenerator.secret_key());
  std::unique_ptr<seal::PublicKey> publicKey =
      std::make_unique<seal::PublicKey>(keyGenerator.public_key());
  std::unique_ptr<seal::GaloisKeys> galoisKeys =
      std::make_unique<seal::GaloisKeys>(keyGenerator.galois_keys_local());
  std::unique_ptr<seal::RelinKeys> relinKeys =
      std::make_unique<seal::RelinKeys>(keyGenerator.relin_keys_local());

  auto encoder = seal::BatchEncoder(context);
  auto encryptor =
      seal::Encryptor(context, *publicKey,
                      *secretKey);  // secret Key encryptor is more efficient

  // Encrypt input
  std::vector<int64_t> img_as_vec;
  img_as_vec.reserve(img.size() * img.size());
  for (int x = 1; x < img.size() - 1; ++x) {
    for (int y = 1; y < img.at(x).size() - 1; ++y) {
      img_as_vec.push_back(img[x][y]);
    }
  }
  seal::Plaintext img_ptxt;
  encoder.encode(img_as_vec, img_ptxt);
  seal::Ciphertext img_ctxt(context);
  encryptor.encrypt_symmetric(img_ptxt, img_ctxt);

  // Compute sharpening filter
  auto evaluator = seal::Evaluator(context);

  // Naive way: very similar to plain C++
  VecInt2D weightMatrix = {{1, 1, 1}, {1, -8, 1}, {1, 1, 1}};
  // Can be default constructed because this is overriden in each loop
  seal::Ciphertext img2_ctxt;

  for (int x = 1; x < img.size() - 1; ++x) {
    for (int y = 1; y < img.at(x).size() - 1; ++y) {
      seal::Ciphertext value(context);
      for (int j = -1; j < 2; ++j) {
        for (int i = -1; i < 2; ++i) {
          //          std::cout << x << ", " << y << ", " << i << ", " << j <<
          //          std::endl;
          seal::Plaintext w;
          encoder.encode(
              std::vector<int64_t>(1, weightMatrix.at(i + 1).at(j + 1)), w);
          seal::Ciphertext temp;
          int steps = (x + i) * img.size() + (y + j);
          if (steps >= DEFAULT_NUM_SLOTS / 2) {
            steps = DEFAULT_NUM_SLOTS / 2 - steps;
          }
          evaluator.rotate_rows(img_ctxt, steps, *galoisKeys, temp);
          evaluator.multiply_plain_inplace(temp, w);
          evaluator.add_inplace(value, temp);
        }
      }

      seal::Plaintext two;
      encoder.encode(std::vector<int64_t>(1, 2), two);
      seal::Ciphertext temp = img_ctxt;
      evaluator.multiply_plain_inplace(temp, two);
      evaluator.sub_inplace(temp, value);
      // TODO: Add masking and merge masking and mult by two
      // std::vector<int64_t> mask(16384,0);
      // mask[x*img.size()+y] = 1;
      // seal::Plaintext mask_ptxt;
      // encoder.encode(mask,mask_ptxt);
      // evaluator.multiply_plain_inplace(temp,mask_ptxt);
      img2_ctxt = temp;
    }
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
    decltype(std::chrono::high_resolution_clock::now()) t_start;
    decltype(std::chrono::high_resolution_clock::now()) t_end;
    decltype(std::chrono::duration_cast<std::chrono::microseconds>(
        t_end - t_start)) duration_ms;
    t_start = std::chrono::high_resolution_clock::now();
    Evaluation::encryptedLaplacianSharpeningAlgorithmNaive(img);
    t_end = std::chrono::high_resolution_clock::now();
    duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
    std::cout << img_size << "," << duration_ms.count() << std::endl;
  }
}
