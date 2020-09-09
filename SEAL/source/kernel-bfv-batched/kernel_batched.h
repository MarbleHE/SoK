#ifndef LAPLACIAN_SHARPENING_BATCHED_H_
#define LAPLACIAN_SHARPENING_BATCHED_H_
#endif

#include <seal/seal.h>

#include <chrono>
#include <iostream>
#include <vector>

typedef std::vector<std::vector<int>> VecInt2D;

typedef std::vector<std::vector<int>> VecInt2D;
typedef std::chrono::high_resolution_clock Time;
typedef decltype(std::chrono::high_resolution_clock::now()) Timepoint;
typedef long long Duration;
typedef std::chrono::milliseconds ms;

#define DEFAULT_NUM_SLOTS 16'384

class Evaluation {
 private:
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

 public:
  void run_kernel(VecInt2D img);
  int main(int argc, char *argv[]);
};
