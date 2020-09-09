#ifndef LAPLACIAN_SHARPENING_H_
#define LAPLACIAN_SHARPENING_H_
#endif

#include <seal/seal.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

typedef std::vector<std::vector<int>> VecInt2D;
typedef decltype(std::chrono::high_resolution_clock::now()) Timepoint;
typedef long long Duration;

#define DEFAULT_NUM_SLOTS 16'384
#define PRINT_LIMIT 70

class Evaluation {
 private:
  std::unique_ptr<seal::SecretKey> secret_key;
  std::unique_ptr<seal::PublicKey> public_key;
  std::unique_ptr<seal::GaloisKeys> galois_keys;
  std::unique_ptr<seal::RelinKeys> relin_keys;
  std::shared_ptr<seal::SEALContext> context;

  std::unique_ptr<seal::BatchEncoder> encoder;
  std::unique_ptr<seal::Encryptor> encryptor;
  std::unique_ptr<seal::Decryptor> decryptor;
  std::unique_ptr<seal::Evaluator> evaluator;

  std::vector<int64_t> decrypt_and_decode(seal::Ciphertext &ctxt);

  std::vector<int64_t> decode(seal::Plaintext &ptxt);


  public:
    void apply_kernel(VecInt2D & img);

    int main(int argc, char *argv[]);
  };
