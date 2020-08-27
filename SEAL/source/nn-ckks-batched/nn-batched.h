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
#include "matrix_vector.h"
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

class DenseLayer {
 private:
  std::vector<vec> diags;
  vec bias_vec;
 public:
  /// Create random weights and biases for a dense or fully-connected layer
  /// \param units number of units, i.e. output size
  /// \param input_size dimension of input
  /// \throws std::invalid_argument if units != input_size because fast MVP is only defined over square matrices
  DenseLayer(size_t units, size_t input_size);

  /// Get Weights
  /// \return The weights matrix of size input_size x units, represented by its diagonals
  const std::vector<vec> &weights_as_diags();

  /// Get Weights
  /// \return A bias vector of length units
  const vec &bias();
};
/// Create only the required power-of-two rotations
/// This can save quite a bit, for example for poly_modulus_degree = 16384
/// The default galois keys (with zlib compression) are 247 MB large
/// Whereas with dimension = 256, they are only 152 MB
/// For poly_modulus_degree = 32768, the default keys are 532 MB large
/// while with dimension = 256, they are only 304 MB
std::vector<int> custom_steps(size_t dimension);

int main(int argc, char *argv[]);
