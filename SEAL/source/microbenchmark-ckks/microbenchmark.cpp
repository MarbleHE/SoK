#include "microbenchmark.h"

#include "../common.h"

typedef std::chrono::microseconds TARGET_TIME_UNIT;

void Microbenchmark::setup_context_ckks(std::size_t poly_modulus_degree) {
  seal::EncryptionParameters params(seal::scheme_type::CKKS);
  params.set_poly_modulus_degree(poly_modulus_degree);
  params.set_coeff_modulus(seal::CoeffModulus::Create(
      poly_modulus_degree,
      {60, 32})); //to match log2 q = 92 in Palisade CKKS

  // Instantiate context
  context = seal::SEALContext::Create(params);

  // Define initial ciphertext scale
  initial_scale = std::pow(2.0, 40);

  // Create keys
  seal::KeyGenerator keyGenerator(context);

  publicKey = std::make_unique<seal::PublicKey>(keyGenerator.public_key());
  // std::ofstream ofs_pk("public_key.dat", std::ios::binary);
  // publicKey->save(ofs_pk);
  // ofs_pk.close();

  secretKey = std::make_unique<seal::SecretKey>(keyGenerator.secret_key());
  // std::ofstream ofs_sk("secret_key.dat", std::ios::binary);
  // secretKey->save(ofs_sk);
  // ofs_sk.close();

  relinKeys =
      std::make_unique<seal::RelinKeys>(keyGenerator.relin_keys_local());
  // std::ofstream ofs_rk("relin_keys.dat", std::ios::binary);
  // relinKeys->save(ofs_rk);
  // ofs_rk.close();

  // Only generate those keys that are actually required/used
  std::vector<int> steps = {-4, 4};
  galoisKeys =
      std::make_unique<seal::GaloisKeys>(keyGenerator.galois_keys_local(steps));
  // std::ofstream ofs_gk("galois_keys.dat", std::ios::binary);
  // galoisKeys->save(ofs_gk);
  // ofs_gk.close();

  // Provide both public and secret key, however, we will use public-key
  // encryption as this is the one used in a typical client-server scenario.
  encryptor =
      std::make_unique<seal::Encryptor>(context, *publicKey, *secretKey);
  evaluator = std::make_unique<seal::Evaluator>(context);
  decryptor = std::make_unique<seal::Decryptor>(context, *secretKey);
  encoder = std::make_unique<seal::CKKSEncoder>(context);
  // std::cout << "Number of slots: " << encoder->slot_count() << std::endl;
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

seal::Ciphertext Microbenchmark::encode_and_encrypt(double numbers) {
  seal::Plaintext ptxt;
  encoder->encode(numbers, initial_scale, ptxt);
  seal::Ciphertext encrypted_numbers;
  encryptor->encrypt(ptxt, encrypted_numbers);
  return encrypted_numbers;
}

void Microbenchmark::run_benchmark() {
  const int NUM_REPETITIONS{100};
  std::stringstream ss_time;

  // set up the CKKS scheme
  auto t0 = Time::now();
  setup_context_ckks(65536);
  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  // =======================================================
  // Ctxt-Ctxt Multiplication with new ciphertext
  // =======================================================

  size_t total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Ciphertext ctxtB = encode_and_encrypt(28);

    auto start = Time::now();
    seal::Ciphertext ctxtC;
    evaluator->multiply(ctxtA, ctxtB, ctxtC);
    evaluator->relinearize_inplace(ctxtC, *relinKeys);
    evaluator->rescale_to_next_inplace(ctxtC);
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
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Ciphertext ctxtB = encode_and_encrypt(28);

    auto start = Time::now();
    evaluator->multiply_inplace(ctxtA, ctxtB);
    evaluator->relinearize_inplace(ctxtA, *relinKeys);
    evaluator->rescale_to_next_inplace(ctxtA);
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
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Plaintext ptxtA;
    encoder->encode(28, initial_scale, ptxtA);

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
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Plaintext ptxtA;
    encoder->encode(28, initial_scale, ptxtA);

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
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Ciphertext ctxtB = encode_and_encrypt(28);

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
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Ciphertext ctxtB = encode_and_encrypt(28);

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
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Plaintext ptxtA;
    encoder->encode(28, initial_scale, ptxtA);

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
    seal::Ciphertext ctxtA = encode_and_encrypt(4214);
    seal::Plaintext ptxtA;
    encoder->encode(28, initial_scale, ptxtA);

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
    seal::Plaintext ptxtA;
    encoder->encode(2321, initial_scale, ptxtA);
    seal::Ciphertext ctxt;
    encryptor->encrypt_symmetric(ptxtA, ctxt);
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
    seal::Plaintext ptxtA;
    encoder->encode(2321, initial_scale, ptxtA);
    seal::Ciphertext ctxt;
    encryptor->encrypt(ptxtA, ctxt);
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
    seal::Ciphertext ctxt = encode_and_encrypt(2321);

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

  total_time = 0;
  for (size_t i = 0; i < NUM_REPETITIONS; i++) {
    seal::Plaintext ptxt;
    std::vector<double> data = {43, 23, 54, 31, 341, 43, 34};
    encoder->encode(data, context->first_parms_id(), initial_scale, ptxt);
    seal::Ciphertext ctxt;
    encryptor->encrypt(ptxt, ctxt);

    auto start = Time::now();
    evaluator->rotate_vector_inplace(ctxt, 4, *galoisKeys);
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
  write_parameters_to_file(context, "fhe_parameters_microbenchmark_ckks.txt");
}

int main(int argc, char *argv[]) {
  std::cout << "Starting 'microbenchmark-ckks'..." << std::endl;
  Microbenchmark().run_benchmark();
  return 0;
}
