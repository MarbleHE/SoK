#define PROFILE

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>

#include "cryptocontextgen.h"
#include "palisade.h"

using namespace std;
using namespace lbcrypto;

typedef std::chrono::microseconds TARGET_TIME_UNIT;

int main(int argc, char *argv[]) {
  std::stringstream ss_params;

  usint plaintextModulus = 536903681;
  ss_params << "plaintextModulus: " << plaintextModulus << std::endl;
  double sigma = 3.2;
  ss_params << "sigma: " << sigma << std::endl;
  SecurityLevel securityLevel = HEStd_128_classic;
  ss_params << "securityLevel: " << securityLevel << std::endl;

  // Parameter generation
  EncodingParams encodingParams(
      std::make_shared<EncodingParamsImpl>(plaintextModulus));

  // Set Crypto Parameters
  // # of evalMults = 3 (first 3) is used to support the multiplication of 7
  // ciphertexts, i.e., ceiling{log2{7}} Max depth is set to 3 (second 3) to
  // generate homomorphic evaluation multiplication keys for s^2 and s^3
  CryptoContext<DCRTPoly> cryptoContext =
      CryptoContextFactory<DCRTPoly>::genCryptoContextBFVrns(
          encodingParams, securityLevel, sigma, 0, 3, 0, OPTIMIZED, 3);

  // enable features that you wish to use
  cryptoContext->Enable(ENCRYPTION);
  cryptoContext->Enable(SHE);

  ss_params << "p = "
            << cryptoContext->GetCryptoParameters()->GetPlaintextModulus()
            << std::endl;
  ss_params << "n = "
            << cryptoContext->GetCryptoParameters()
                       ->GetElementParams()
                       ->GetCyclotomicOrder() /
                   2
            << std::endl;
  ss_params << "log2 q = "
            << log2(cryptoContext->GetCryptoParameters()
                        ->GetElementParams()
                        ->GetModulus()
                        .ConvertToDouble())
            << std::endl;

  // Initialize Public Key Containers
  LPKeyPair<DCRTPoly> keyPair;

  ////////////////////////////////////////////////////////////
  // Perform Key Generation Operation
  ////////////////////////////////////////////////////////////

  keyPair = cryptoContext->KeyGen();
  if (!keyPair.good()) {
    std::cout << "Key generation failed!" << std::endl;
    exit(1);
  }

  cryptoContext->EvalMultKeysGen(keyPair.secretKey);

  // write ss_params into file
  std::ofstream fheParamsFile;
  fheParamsFile.open("fhe_parameters_microbenchmark.txt", std::ios_base::app);
  fheParamsFile << ss_params.str() << std::endl;
  fheParamsFile.close();

  const int NUM_REPETITIONS{250};
  std::stringstream ss_time;

  // =======================================================
  // Ctxt-Ctxt Multiplication with new ciphertext
  // =======================================================

  std::cout << "== Ctxt-Ctxt Multiplication with new ciphertext" << std::endl;
  size_t total_time = 0;

  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto pa = cryptoContext->MakeIntegerPlaintext(4214);
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeIntegerPlaintext(28);
    auto cb = cryptoContext->Encrypt(keyPair.publicKey, pb);

    auto start = std::chrono::high_resolution_clock::now();
    auto ciphertextMult = cryptoContext->EvalMultAndRelinearize(ca, cb);
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

  std::cout << "== Ctxt-Ptxt Multiplication with new ciphertext" << std::endl;
  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto pa = cryptoContext->MakeIntegerPlaintext(4214);
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeIntegerPlaintext(28);

    auto start = std::chrono::high_resolution_clock::now();
    auto ciphertextMult = cryptoContext->EvalMult(ca, pb);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

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
    auto pa = cryptoContext->MakeIntegerPlaintext(4214);
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeIntegerPlaintext(28);
    auto cb = cryptoContext->Encrypt(keyPair.publicKey, pb);

    auto start = std::chrono::high_resolution_clock::now();
    cryptoContext->EvalAdd(ca, cb);
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

  std::cout << "== Ctxt-Ptxt Addition with new ciphertext" << std::endl;
  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto pa = cryptoContext->MakeIntegerPlaintext(4214);
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeIntegerPlaintext(28);

    auto start = std::chrono::high_resolution_clock::now();
    auto ciphertextMult = cryptoContext->EvalAdd(pb, ca);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

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
    auto pa = cryptoContext->MakeIntegerPlaintext(23213);
    cryptoContext->Encrypt(keyPair.publicKey, pa);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Pk Encryption Time
  // =======================================================

  std::cout << "== Pk Encryption time" << std::endl;
  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    auto pa = cryptoContext->MakeIntegerPlaintext(23213);
    cryptoContext->Encrypt(keyPair.secretKey, pa);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Decryption time
  // =======================================================

  std::cout << "== Decryption time" << std::endl;
  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto pa = cryptoContext->MakeIntegerPlaintext(23213);
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto start = std::chrono::high_resolution_clock::now();
    Plaintext ptxt;
    cryptoContext->Decrypt(keyPair.secretKey, ca, &ptxt);
    auto end = std::chrono::high_resolution_clock::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Rotation (native, i.e. single-key)
  // =======================================================

  // Get BGVrns crypto context as BFV does not support EvalFastRotation
  auto cc = CryptoContextFactory<DCRTPoly>::genCryptoContextBGVrns(1, 65537);
  cc->Enable(ENCRYPTION);
  cc->Enable(SHE);

  auto keys = cc->KeyGen();
  cc->EvalAtIndexKeyGen(keys.secretKey, {4});

  std::vector<int64_t> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  Plaintext ptxt = cc->MakePackedPlaintext(vec);
  auto ctxt = cc->Encrypt(keys.publicKey, ptxt);

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    //   auto precomp = cc->EvalFastRotationPrecompute(ctxt);
    //   auto cRot = cc->EvalFastRotation(ctxt, 1, M, precomp);

    auto cPrecomp = cc->EvalFastRotationPrecompute(ctxt);
    uint32_t N = cc->GetRingDimension();
    uint32_t M = 2 * N;
    auto cRot1 = cc->EvalFastRotation(ctxt, 4, M, cPrecomp);

    auto end = std::chrono::high_resolution_clock::now();
    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS);

  // write ss_time into file
  std::ofstream myfile;
  auto out_filename = std::getenv("OUTPUT_FILENAME");
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();

  return 0;
}
