#ifndef CARDIO_H_
#define CARDIO_H_
#endif

#include <seal/seal.h>

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

typedef std::vector<seal::Ciphertext> CiphertextVector;

class Cardio {
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
  std::unique_ptr<seal::IntegerEncoder> encoder;

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

  void print_ciphertext(std::string name, seal::Ciphertext &ctxt);

  void print_ciphertextvector(CiphertextVector &vec);

  CiphertextVector ctxt_to_ciphertextvector(seal::Ciphertext &ctxt);


 public:
  void setup_context_bfv(std::size_t poly_modulus_degree,
                         std::uint64_t plain_modulus);

  void run_cardio();

  CiphertextVector encode_and_encrypt(int32_t number);

  std::unique_ptr<seal::Ciphertext> equal(CiphertextVector lhs, CiphertextVector rhs);

  void shift_right_inplace(CiphertextVector &ctxt);

  void shift_left_inplace(CiphertextVector &ctxt);

  CiphertextVector slice(CiphertextVector ctxt, int idx_begin, int idx_end);

  CiphertextVector slice(CiphertextVector ctxt, int idx_begin);


  std::unique_ptr<seal::Ciphertext> multvect(CiphertextVector bitvec);

  std::unique_ptr<seal::Ciphertext> lower(CiphertextVector &lhs,
                                          CiphertextVector &rhs);

  CiphertextVector add(CiphertextVector lhs, CiphertextVector rhs);

  int main(int argc, char *argv[]);
};
