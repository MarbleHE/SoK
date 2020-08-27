#ifndef CARDIO_BATCHED_CKKS_H_
#define CARDIO_BATCHED_CKKS_H_
#endif

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>
#include <cmath>

#include "helpers.h"
#include "seal/seal.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;

class NNBatched {
 private:
  /// the seal context, i.e. object that holds params/etc
  std::shared_ptr<seal::SEALContext> context;

  /// secret key, also used for (more efficient) encryption
  std::unique_ptr<seal::SecretKey> secretKey;

  /// public key (ptr because PublicKey() segfaults)
  std::unique_ptr<seal::PublicKey> publicKey;

  /// keys required to rotate (ptr because GaloisKeys() segfaults)
  std::unique_ptr<seal::GaloisKeys> galoisKeys;

  /// keys required to relinearize after multipliction (ptr for consistency)
  std::unique_ptr<seal::RelinKeys> relinKeys;

  std::unique_ptr<seal::Encryptor> encryptor;
  std::unique_ptr<seal::Evaluator> evaluator;
  std::unique_ptr<seal::Decryptor> decryptor;
  std::unique_ptr<seal::CKKSEncoder> encoder;

  double initial_scale;

  void internal_print_info(std::string variable_name, seal::Ciphertext &ctxt);

 public:
  void setup_context_ckks(std::size_t poly_modulus_degree);

  void run_nn();

  seal::Ciphertext encode_and_encrypt(std::vector<double> number);

  seal::Plaintext encode(std::vector<double> numbers);

  seal::Plaintext encode(std::vector<double> numbers, seal::parms_id_type parms_id);
};

int main(int argc, char *argv[]);
