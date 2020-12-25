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

void write_parameters_into_file(CryptoContext<DCRTPoly> &cryptoContext) {
  std::stringstream ss_params;

  ss_params << "Plaintext Modulus p = "
            << cryptoContext->GetCryptoParameters()->GetPlaintextModulus()
            << std::endl;

  ss_params << "Cyclotomic Order n = "
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

  ss_params << "Max Depth = "
            << cryptoContext->GetCryptoParameters()->GetMaxDepth() << std::endl;

  ss_params << "Ring Dimension = " << cryptoContext->GetRingDimension()
            << std::endl;

  ss_params << "Batch Size = "
            << cryptoContext->GetEncodingParams()->GetBatchSize() << std::endl;

  // write ss_params into file
  std::ofstream fheParamsFile;
  fheParamsFile.open("fhe_parameters_microbenchmark_ckks.txt");
  fheParamsFile << ss_params.str() << std::endl;
  fheParamsFile.close();
}

int main(int argc, char *argv[]) {
  // the multiplicative depth that these paramters should support
  uint32_t multDepth = 1;

  // corresponds to the plaintext modulus
  uint32_t scaleFactorBits = 4000;

  // the number of plaintext/ciphertext slots
  uint32_t batchSize = 1;

  SecurityLevel securityLevel = HEStd_128_classic;

  // create a CKKS crypto context
  CryptoContext<DCRTPoly> cryptoContext =
      CryptoContextFactory<DCRTPoly>::genCryptoContextCKKS(
          multDepth, scaleFactorBits, batchSize, securityLevel);

  cryptoContext->Enable(ENCRYPTION);
  cryptoContext->Enable(SHE);

  // generate the encryption keys
  auto keyPair = cryptoContext->KeyGen();
  if (!keyPair.good()) {
    std::cout << "Key generation failed!" << std::endl;
    exit(1);
  }

  // generate the relinearization keys
  cryptoContext->EvalMultKeyGen(keyPair.secretKey);

  // generate the rotation keys
  cryptoContext->EvalAtIndexKeyGen(keyPair.secretKey, {1, -2});

  // write parameters into file
  write_parameters_into_file(cryptoContext);

  const int NUM_REPETITIONS{250};
  std::stringstream ss_time;

  // =======================================================
  // Ctxt-Ctxt Multiplication with new ciphertext
  // =======================================================

  std::cout << "== Ctxt-Ctxt Multiplication with new ciphertext" << std::endl;
  size_t total_time = 0;

  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto pa = cryptoContext->MakeCKKSPackedPlaintext({14});
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeCKKSPackedPlaintext({28});
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
    auto pa = cryptoContext->MakeCKKSPackedPlaintext({214});
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeCKKSPackedPlaintext({28});

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
    auto pa = cryptoContext->MakeCKKSPackedPlaintext({214});
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeCKKSPackedPlaintext({28});
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
    auto pa = cryptoContext->MakeCKKSPackedPlaintext({214});
    auto ca = cryptoContext->Encrypt(keyPair.publicKey, pa);

    auto pb = cryptoContext->MakeCKKSPackedPlaintext({28});

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
    auto pa = cryptoContext->MakeCKKSPackedPlaintext({3213});
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
    auto pa = cryptoContext->MakeCKKSPackedPlaintext({3213});
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
    auto pa = cryptoContext->MakeCKKSPackedPlaintext({3213});
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

  cryptoContext->EvalAtIndexKeyGen(keyPair.secretKey, {4});

  Plaintext ptxt = cryptoContext->MakeCKKSPackedPlaintext({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  auto ctxt = cryptoContext->Encrypt(keyPair.publicKey, ptxt);

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    auto cPrecomp = cryptoContext->EvalFastRotationPrecompute(ctxt);
    uint32_t N = cryptoContext->GetRingDimension();
    uint32_t M = 2 * N;
    auto cRot1 = cryptoContext->EvalFastRotation(ctxt, 4, M, cPrecomp);

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
