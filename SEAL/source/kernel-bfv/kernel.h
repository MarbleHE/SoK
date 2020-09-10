#ifndef LAPLACIAN_SHARPENING_H_
#define LAPLACIAN_SHARPENING_H_
#endif

#include <seal/seal.h>

#include <chrono>
#include <iostream>
#include <vector>

typedef std::vector<std::vector<int>> VecInt2D;
typedef std::chrono::high_resolution_clock Time;
typedef decltype(std::chrono::high_resolution_clock::now()) Timepoint;
typedef long long Duration;
typedef std::chrono::milliseconds ms;

#define DEFAULT_NUM_SLOTS 16'384
#define PRINT_LIMIT 70

class Evaluation {
 private:
  // e.g., image_size=8 corresponds to a 8x8 pixels image
  int image_size;

  const VecInt2D weight_matrix = {{1, 1, 1}, {1, -8, 1}, {1, 1, 1}};

  std::shared_ptr<seal::SEALContext> context;

  std::unique_ptr<seal::SecretKey> secret_key;
  std::unique_ptr<seal::PublicKey> public_key;
  std::unique_ptr<seal::GaloisKeys> galois_keys;
  std::unique_ptr<seal::RelinKeys> relin_keys;

  std::unique_ptr<seal::BatchEncoder> encoder;
  std::unique_ptr<seal::Encryptor> encryptor;
  std::unique_ptr<seal::Decryptor> decryptor;
  std::unique_ptr<seal::Evaluator> evaluator;

  std::vector<int64_t> decrypt_and_decode(seal::Ciphertext &ctxt);

  std::vector<int64_t> decode(seal::Plaintext &ptxt);

  Duration compute_duration(Timepoint start, Timepoint end);

  void print_all(std::vector<int64_t> &vector);

 public:
  int main(int argc, char *argv[]);

  Evaluation(int image_size);

  std::vector<int64_t> apply_kernel(VecInt2D &img);

  void check_results(VecInt2D img, std::vector<int64_t> computed_values);
};
