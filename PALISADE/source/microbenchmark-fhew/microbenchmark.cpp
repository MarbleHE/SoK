#define PROFILE

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>

#include "binfhecontext.h"
#include "cryptocontextgen.h"
#include "palisade.h"

using namespace std;
using namespace lbcrypto;

typedef std::chrono::microseconds TARGET_TIME_UNIT;

int main(int argc, char *argv[]) {
  auto cryptoContext = BinFHEContext();
  // generate the crypto context with 128-bit of security
  cryptoContext.GenerateBinFHEContext(STD128);

  // generate the secret key
  auto sk = cryptoContext.KeyGen();

  // generate bootstrapping keys needed for refresh and key switching
  cryptoContext.BTKeyGen(sk);

  // encrypt two ciphertexts representing boolean 1
  auto ct1 = cryptoContext.Encrypt(sk, 1);
  auto ct2 = cryptoContext.Encrypt(sk, 1);

  const int NUM_REPETITIONS{100};
  std::stringstream ss_time;

  // =======================================================
  // Ctxt-Ctxt Multiplication with new ciphertext
  // =======================================================

  std::cout << "== Ctxt-Ctxt Multiplication with new ciphertext" << std::endl;
  size_t total_time = 0;

  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    auto ctAND = cryptoContext.EvalBinGate(AND, ct1, ct2);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ctxt Multiplication in-place
  // =======================================================

  // not supported by PALISADE
  ss_time << 0 << ",";

  // =======================================================
  // Ctxt-Ptxt Multiplication with new ciphertext
  // =======================================================

  // not supported by PALISADE
  ss_time << 0 << ",";

  // =======================================================
  // Ctxt-Ptxt Multiplication in-place
  // =======================================================

  // not supported by PALISADE
  ss_time << 0 << ",";

  // =======================================================
  // Ctxt-Ctxt Addition time with new ciphertext
  // =======================================================

  std::cout << "== Ctxt-Ctxt Addition time with new ciphertext" << std::endl;
  total_time = 0;
  
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    auto ct1 = cryptoContext.Encrypt(sk, 1);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ctxt Addition time in-place
  // =======================================================

  // not supported by PALISADE
  ss_time << 0 << ",";

  // =======================================================
  // Ctxt-Ptxt Addition with new ciphertext
  // =======================================================

  // not supported by PALISADE
  ss_time << 0 << ",";

  // =======================================================
  // Ctxt-Ptxt Addition in-place
  // =======================================================

  // not supported by PALISADE
  ss_time << 0 << ",";

  // =======================================================
  // Sk Encryption time
  // =======================================================

  std::cout << "== Sk Encryption time" << std::endl;
  total_time = 0;
  
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    auto ct1 = cryptoContext.Encrypt(sk, 1);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Pk Encryption Time
  // =======================================================

  // not supported by this scheme
  ss_time << 0 << ",";

  // =======================================================
  // Decryption time
  // =======================================================

  std::cout << "== Decryption time" << std::endl;
  total_time = 0;

  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    LWEPlaintext result;
    cryptoContext.Decrypt(sk, ct1, &result);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Rotation (native, i.e. single-key)
  // =======================================================

  // not supported by FHEW scheme
  ss_time << 0 << std::endl;

  // write ss_time into file
  std::ofstream myfile;
  auto out_filename = std::getenv("OUTPUT_FILENAME");
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();

  return 0;
}
