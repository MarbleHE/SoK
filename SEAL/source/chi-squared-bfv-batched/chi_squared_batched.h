#ifndef CHI_SQUARED_BATCHED_H_
#define CHI_SQUARED_BATCHED_H_
#endif

#include <seal/seal.h>

#include <algorithm>
#include <chrono>
#include <math.h>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>
#include <cassert>

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;

struct ResultCiphertexts {
 public:
  seal::Ciphertext alpha;
  seal::Ciphertext beta_1;
  seal::Ciphertext beta_2;
  seal::Ciphertext beta_3;

  ResultCiphertexts(seal::Ciphertext alpha, seal::Ciphertext beta_1,
                    seal::Ciphertext beta_2, seal::Ciphertext beta_3)
      : alpha(alpha), beta_1(beta_1), beta_2(beta_2), beta_3(beta_3){};
};

class ChiSquaredBatched {
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

  seal::Ciphertext encode_all_slots_and_encrypt(int64_t value);

  seal::Plaintext encode_all_slots(int64_t value);

 public:
  void run_chi_squared();

  void setup_context_bfv(std::size_t poly_modulus_degree,
                         std::uint64_t plain_modulus);

  ResultCiphertexts compute_alpha_betas(const seal::Ciphertext &N_0,
                                        const seal::Ciphertext &N_1,
                                        const seal::Ciphertext &N_2);

  int main(int argc, char *argv[]);

  int64_t get_first_decrypted_value(seal::Ciphertext value);
};
