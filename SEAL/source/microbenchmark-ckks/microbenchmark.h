#ifndef MICROBENCHMARK_H_
#define MICROBENCHMARK_H_
#endif

#include <seal/seal.h>

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

typedef std::vector<seal::Ciphertext> CiphertextVector;
typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;

class Microbenchmark {
 private:
  double initial_scale;

  /// the seal context, i.e. object that holds params/etc
  std::shared_ptr<seal::SEALContext> context;

  // secret key, also used for (more efficient) encryption
  std::unique_ptr<seal::SecretKey> secretKey;

  /// public key (ptr because PublicKey() segfaults)
  std::unique_ptr<seal::PublicKey> publicKey;

  /// keys required to rotate (ptr because GaloisKeys() segfaults)
  std::unique_ptr<seal::GaloisKeys> galoisKeys;

  /// keys required to relinearize after multiplication (ptr for consistency)
  std::unique_ptr<seal::RelinKeys> relinKeys;

  std::unique_ptr<seal::Encryptor> encryptor;
  std::unique_ptr<seal::Evaluator> evaluator;
  std::unique_ptr<seal::Decryptor> decryptor;
  std::unique_ptr<seal::CKKSEncoder> encoder;

  void pre_computation(std::vector<CiphertextVector> &P,
                       std::vector<CiphertextVector> &G, CiphertextVector &lhs,
                       CiphertextVector &rhs);

  void evaluate_G(std::vector<CiphertextVector> &P,
                  std::vector<CiphertextVector> &G, int row_idx, int col_idx,
                  int step);

  void evaluate_P(std::vector<CiphertextVector> &P,
                  std::vector<CiphertextVector> &G, int row_idx, int col_idx,
                  int step);

  CiphertextVector post_computation(std::vector<CiphertextVector> &P,
                                    std::vector<CiphertextVector> &G, int size);
  
  seal::Ciphertext encode_and_encrypt(double numbers);

 public:
  void setup_context_ckks(std::size_t poly_modulus_degree);

  void run_benchmark();

  int main(int argc, char *argv[]);
};
