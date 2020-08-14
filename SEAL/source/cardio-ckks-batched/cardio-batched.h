#ifndef CARDIO_BATCHED_CKKS_H_
#define CARDIO_BATCHED_CKKS_H_
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
#define print_info(name) internal_print_info(#name, (name))
#define NUM_BITS 8

typedef std::vector<seal::Ciphertext> CiphertextVector;
typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;

/*
 * Helper function: Prints a vector of floating-point values.
 * Method taken from Microsoft SEAL:
 * https://github.com/microsoft/SEAL/blob/master/native/examples/examples.h.
 */
template <typename T>
inline void print_vector(std::vector<T> vec, std::size_t print_size = 4,
                         int prec = 3) {
  /*
  Save the formatting information for std::cout.
  */
  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);

  std::size_t slot_count = vec.size();

  std::cout << std::fixed << std::setprecision(prec);
  std::cout << std::endl;
  if (slot_count <= 2 * print_size) {
    std::cout << "    [";
    for (std::size_t i = 0; i < slot_count; i++) {
      std::cout << " " << vec[i] << ((i != slot_count - 1) ? "," : " ]\n");
    }
  } else {
    vec.resize(std::max(vec.size(), 2 * print_size));
    std::cout << "    [";
    for (std::size_t i = 0; i < print_size; i++) {
      std::cout << " " << vec[i] << ",";
    }
    if (vec.size() > 2 * print_size) {
      std::cout << " ...,";
    }
    for (std::size_t i = slot_count - print_size; i < slot_count; i++) {
      std::cout << " " << vec[i] << ((i != slot_count - 1) ? "," : " ]\n");
    }
  }
  std::cout << std::endl;

  /*
  Restore the old std::cout formatting.
  */
  std::cout.copyfmt(old_fmt);
}

/*
 * Helper function: Prints a matrix of values.
 * Method taken from Microsoft SEAL:
 * https://github.com/microsoft/SEAL/blob/master/native/examples/examples.h.
 */
template <typename T>
inline void print_matrix(std::vector<T> matrix, std::size_t row_size) {
  /*
  We're not going to print every column of the matrix (there are 2048). Instead
  print this many slots from beginning and end of the matrix.
  */
  std::size_t print_size = 5;

  std::cout << std::endl;
  std::cout << "    [";
  for (std::size_t i = 0; i < print_size; i++) {
    std::cout << std::setw(3) << std::right << matrix[i] << ",";
  }
  std::cout << std::setw(3) << " ...,";
  for (std::size_t i = row_size - print_size; i < row_size; i++) {
    std::cout << std::setw(3) << matrix[i]
              << ((i != row_size - 1) ? "," : " ]\n");
  }
  std::cout << "    [";
  for (std::size_t i = row_size; i < row_size + print_size; i++) {
    std::cout << std::setw(3) << matrix[i] << ",";
  }
  std::cout << std::setw(3) << " ...,";
  for (std::size_t i = 2 * row_size - print_size; i < 2 * row_size; i++) {
    std::cout << std::setw(3) << matrix[i]
              << ((i != 2 * row_size - 1) ? "," : " ]\n");
  }
  std::cout << std::endl;
}

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
  std::unique_ptr<seal::CKKSEncoder> encoder;

  double initial_scale;

  void print_vec(seal::Ciphertext &ctxt);

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

  int ciphertextvector_to_int(CiphertextVector &vec);

  CiphertextVector ctxt_to_ciphertextvector(seal::Ciphertext &ctxt);

  void print_plaintext(seal::Plaintext &ptxt);

  void print_ciphertext(seal::Ciphertext &ctxt);

  seal::Ciphertext XOR(seal::Ciphertext &lhs, seal::Ciphertext &rhs);

  seal::Ciphertext XOR(seal::Ciphertext &lhs, seal::Plaintext &rhs);

  void internal_print_info(std::string variable_name, seal::Ciphertext &ctxt);

 public:
  void setup_context_ckks(std::size_t poly_modulus_degree);

  void run_cardio();

  seal::Ciphertext encode_and_encrypt(std::vector<uint64_t> number);

  seal::Plaintext encode(std::vector<uint64_t> numbers);

  seal::Plaintext encode(std::vector<uint64_t> numbers,
                         seal::parms_id_type parms_id);

  std::unique_ptr<seal::Ciphertext> equal(CiphertextVector &lhs,
                                          CiphertextVector &rhs);

  void shift_right_inplace(CiphertextVector &ctxt);

  void shift_left_inplace(CiphertextVector &ctxt);

  CiphertextVector slice(CiphertextVector ctxt, int idx_begin, int idx_end);

  CiphertextVector slice(CiphertextVector ctxt, int idx_begin);

  std::unique_ptr<seal::Ciphertext> multvect(CiphertextVector bitvec);

  std::unique_ptr<seal::Ciphertext> lower(CiphertextVector &lhs,
                                          CiphertextVector &rhs);

  CiphertextVector add(CiphertextVector lhs, CiphertextVector rhs);

  std::vector<seal::Ciphertext> split_by_binary_rep(seal::Ciphertext &ctxt);

  int main(int argc, char *argv[]);
};
