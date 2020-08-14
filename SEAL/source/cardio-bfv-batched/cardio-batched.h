#ifndef CARDIO_BATCHED_BFV_H_
#define CARDIO_BATCHED_BFV_H_
#endif

#include <math.h>
#include <seal/seal.h>

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

typedef std::vector<seal::Ciphertext> CiphertextVector;
#define NUM_BITS 8

typedef std::vector<seal::Ciphertext> CiphertextVector;
typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;

class CardioBatched {
 private:
  /// the seal context, i.e. object that holds params/etc
  std::shared_ptr<seal::SEALContext> context;

  // secret key, also used for (more efficient) encryption
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
  std::unique_ptr<seal::BatchEncoder> encoder;

  void print_vec(seal::Ciphertext &ctxt);

  void print_ciphertext(std::string name, seal::Ciphertext &ctxt);

  void print_ciphertextvector(CiphertextVector &vec);

  int ciphertextvector_to_int(CiphertextVector &vec);

  CiphertextVector ctxt_to_ciphertextvector(seal::Ciphertext &ctxt);

  void print_plaintext(seal::Plaintext &ptxt);

  void print_ciphertext(seal::Ciphertext &ctxt);

  seal::Ciphertext XOR(seal::Ciphertext &lhs, seal::Ciphertext &rhs);

  seal::Ciphertext XOR(seal::Ciphertext &lhs, seal::Plaintext &rhs);

 public:
  void setup_context_bfv(std::size_t poly_modulus_degree);

  void run_cardio();

  seal::Ciphertext encode_and_encrypt(std::vector<uint64_t> number);

  seal::Plaintext encode(std::vector<uint64_t> numbers);

  seal::Plaintext encode(std::vector<uint64_t> numbers,
                         seal::parms_id_type parms_id);

  std::unique_ptr<seal::Ciphertext> equal(CiphertextVector &lhs,
                                          CiphertextVector &rhs);

  CiphertextVector slice(CiphertextVector ctxt, int idx_begin, int idx_end);

  CiphertextVector slice(CiphertextVector ctxt, int idx_begin);

  std::unique_ptr<seal::Ciphertext> multvect(CiphertextVector bitvec);

  std::unique_ptr<seal::Ciphertext> lower(CiphertextVector &lhs,
                                          CiphertextVector &rhs);

  std::vector<seal::Ciphertext> split_by_binary_rep(seal::Ciphertext &ctxt);

  int main(int argc, char *argv[]);
};
