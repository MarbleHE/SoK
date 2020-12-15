#include "microbenchmarks.h"

#include "../common.h"

typedef std::chrono::microseconds TARGET_TIME_UNIT;

void Microbenchmark::setup_context_bfv(std::size_t poly_modulus_degree,
                                       std::uint64_t plain_modulus,
                                       bool use_batching) {
  /// Wrapper for parameters
  seal::EncryptionParameters params(seal::scheme_type::BFV);
  params.set_poly_modulus_degree(poly_modulus_degree);

#ifdef SEALPARAMS
  params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(
      poly_modulus_degree, seal::sec_level_type::tc128));
#endif

  if (use_batching) {
    params.set_plain_modulus(
        seal::PlainModulus::Batching(poly_modulus_degree, 20));
  } else {
    params.set_plain_modulus(plain_modulus);
  }

  // Instantiate context
  context = seal::SEALContext::Create(params);

  /// Create keys
  seal::KeyGenerator keyGenerator(context);
  publicKey = std::make_unique<seal::PublicKey>(keyGenerator.public_key());
  secretKey = std::make_unique<seal::SecretKey>(keyGenerator.secret_key());
  relinKeys =
      std::make_unique<seal::RelinKeys>(keyGenerator.relin_keys_local());

  // Provide both public and secret key, however, we will use public-key
  // encryption as this is the one used in a typical client-server scenario.
  encryptor =
      std::make_unique<seal::Encryptor>(context, *publicKey, *secretKey);
  evaluator = std::make_unique<seal::Evaluator>(context);
  decryptor = std::make_unique<seal::Decryptor>(context, *secretKey);

  if (use_batching) {
    batchEncoder = std::make_unique<seal::BatchEncoder>(context);
    // Only generate those keys that are actually required/used
    std::vector<int> steps = {-4, 4};
    galoisKeys = std::make_unique<seal::GaloisKeys>(
        keyGenerator.galois_keys_local(steps));
  } else {
    intEncoder = std::make_unique<seal::IntegerEncoder>(context);
  }
}

namespace {
void log_time(std::stringstream &ss,
              std::chrono::time_point<std::chrono::high_resolution_clock> start,
              std::chrono::time_point<std::chrono::high_resolution_clock> end,
              bool last = false) {
  ss << std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  if (!last) ss << ",";
}
}  // namespace

void Microbenchmark::run_benchmark() {
  const int NUM_REPETITIONS{250};
  std::stringstream ss_time;

  // set up the BFV scheme
  auto t0 = Time::now();
  setup_context_bfv(16384, 2, false);
  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  // =======================================================
  // Ctxt-Ctxt Multiplication with new ciphertext
  // =======================================================

  size_t total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Ciphertext ctxtB;
    encryptor->encrypt(intEncoder->encode(28), ctxtB);

    auto start = Time::now();
    seal::Ciphertext ctxtC;
    evaluator->multiply(ctxtA, ctxtB, ctxtC);
    evaluator->relinearize_inplace(ctxtC, *relinKeys);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ctxt Multiplication in-place
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Ciphertext ctxtB;
    encryptor->encrypt(intEncoder->encode(28), ctxtB);

    auto start = Time::now();
    evaluator->multiply_inplace(ctxtA, ctxtB);
    evaluator->relinearize_inplace(ctxtA, *relinKeys);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ptxt Multiplication with new ciphertext
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Plaintext ptxtA = intEncoder->encode(28);

    auto start = Time::now();
    seal::Ciphertext ctxtB;
    evaluator->multiply_plain(ctxtA, ptxtA, ctxtB);
    evaluator->relinearize_inplace(ctxtB, *relinKeys);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ptxt Multiplication in-place
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Plaintext ptxtA = intEncoder->encode(28);

    auto start = Time::now();
    evaluator->multiply_plain_inplace(ctxtA, ptxtA);
    evaluator->relinearize_inplace(ctxtA, *relinKeys);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ctxt Addition time with new ciphertext
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Ciphertext ctxtB;
    encryptor->encrypt(intEncoder->encode(28), ctxtB);

    auto start = Time::now();
    seal::Ciphertext ctxtC;
    evaluator->add(ctxtA, ctxtB, ctxtC);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ctxt Addition time in-place
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Ciphertext ctxtB;
    encryptor->encrypt(intEncoder->encode(28), ctxtB);

    auto start = Time::now();
    evaluator->add_inplace(ctxtA, ctxtB);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ptxt Addition with new ciphertext
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Plaintext ptxtA = intEncoder->encode(28);

    auto start = Time::now();
    seal::Ciphertext ctxtB;
    evaluator->add_plain(ctxtA, ptxtA, ctxtB);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Ctxt-Ptxt Addition in-place
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA;
    encryptor->encrypt(intEncoder->encode(4214), ctxtA);
    seal::Plaintext ptxtA = intEncoder->encode(28);

    auto start = Time::now();
    evaluator->add_plain_inplace(ctxtA, ptxtA);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Sk Encryption time
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = Time::now();
    seal::Ciphertext ctxt;
    encryptor->encrypt_symmetric(intEncoder->encode(23213), ctxt);
    auto end = Time::now();
    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Pk Encryption Time
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    auto start = Time::now();
    seal::Ciphertext ctxt;
    encryptor->encrypt(intEncoder->encode(23213), ctxt);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS) << ",";

  // =======================================================
  // Decryption time
  // =======================================================

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxt;
    encryptor->encrypt(intEncoder->encode(23213), ctxt);

    auto start = Time::now();
    seal::Plaintext ptxt;
    decryptor->decrypt(ctxt, ptxt);
    auto end = Time::now();

    total_time +=
        std::chrono::duration_cast<TARGET_TIME_UNIT>(end - start).count();
  }
  ss_time << (total_time / NUM_REPETITIONS);

  // =======================================================
  // Rotation (native, i.e. single-key)
  // =======================================================

  setup_context_bfv(16384, -1, true);

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxt;
    std::vector<uint64_t> data = {43, 23, 54, 31, 341, 43, 34};
    seal::Plaintext ptxt;
    batchEncoder->encode(data, ptxt);
    encryptor->encrypt(ptxt, ctxt);

    auto start = Time::now();
    evaluator->rotate_rows_inplace(ctxt, 4, *galoisKeys);
    auto end = Time::now();

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

  // write FHE parameters into file
  write_parameters_to_file(context, "fhe_parameters_microbenchmark.txt");
}

int main(int argc, char *argv[]) {
  std::cout << "Starting 'microbenchmarks'..." << std::endl;
  Microbenchmark().run_benchmark();
  return 0;
}
