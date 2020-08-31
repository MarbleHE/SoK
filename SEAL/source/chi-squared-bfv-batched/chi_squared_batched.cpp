#include "chi_squared_batched.h"

#include "../common.h"

void ChiSquaredBatched::setup_context_bfv(std::size_t poly_modulus_degree,
                                          std::uint64_t plain_modulus) {
  /// Wrapper for parameters
  seal::EncryptionParameters params(seal::scheme_type::BFV);
  params.set_poly_modulus_degree(poly_modulus_degree);
  // Cingulata params + an additional moduli (44) as computation otherwise
  // cannot be performed
  params.set_coeff_modulus(seal::CoeffModulus::Create(
      poly_modulus_degree, {30, 40, 44, 50, 54, 60, 60}));
  params.set_plain_modulus(
      seal::PlainModulus::Batching(poly_modulus_degree, 20));

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
  encoder = std::make_unique<seal::BatchEncoder>(context);
}

namespace {
void log_time(std::stringstream &ss,
              std::chrono::time_point<std::chrono::high_resolution_clock> start,
              std::chrono::time_point<std::chrono::high_resolution_clock> end,
              bool last = false) {
  ss << std::chrono::duration_cast<ms>(end - start).count();
  if (!last) ss << ",";
}
}  // namespace

int64_t ChiSquaredBatched::get_first_decrypted_value(seal::Ciphertext value) {
  seal::Plaintext tmp;
  decryptor->decrypt(value, tmp);
  std::vector<int64_t> values;
  encoder->decode(tmp, values);
  return tmp[0];
}

ResultCiphertexts ChiSquaredBatched::compute_alpha_betas(
    const seal::Ciphertext &N_0, const seal::Ciphertext &N_1,
    const seal::Ciphertext &N_2) {
  seal::Plaintext four = encode_all_slots(4);
  seal::Plaintext two = encode_all_slots(2);

  // compute alpha
  seal::Ciphertext alpha;
  evaluator->multiply_plain(N_0, four, alpha);
  evaluator->relinearize_inplace(alpha, *relinKeys);
  evaluator->multiply_inplace(alpha, N_2);
  evaluator->relinearize_inplace(alpha, *relinKeys);
  seal::Ciphertext N_1_pow2;
  evaluator->exponentiate(N_1, 2, *relinKeys, N_1_pow2);
  evaluator->sub_inplace(alpha, N_1_pow2);
  evaluator->exponentiate_inplace(alpha, 2, *relinKeys);

  // compute beta_1
  seal::Ciphertext beta_1;
  seal::Ciphertext N_0_t2;
  evaluator->multiply_plain(N_0, two, N_0_t2);
  evaluator->relinearize_inplace(N_0_t2, *relinKeys);
  seal::Ciphertext twot_N_0__plus__N_1;
  evaluator->add(N_0_t2, N_1, twot_N_0__plus__N_1);
  evaluator->exponentiate(twot_N_0__plus__N_1, 2, *relinKeys, beta_1);
  evaluator->multiply_plain_inplace(beta_1, two);
  evaluator->relinearize_inplace(beta_1, *relinKeys);

  // compute beta_2
  seal::Ciphertext beta_2;
  seal::Ciphertext t2_N_2;
  evaluator->multiply_plain(N_2, two, t2_N_2);
  evaluator->relinearize_inplace(t2_N_2, *relinKeys);
  seal::Ciphertext twot_N_2__plus__N_1;
  evaluator->add(t2_N_2, N_1, twot_N_2__plus__N_1);
  evaluator->multiply(twot_N_0__plus__N_1, twot_N_2__plus__N_1, beta_2);
  evaluator->relinearize_inplace(beta_2, *relinKeys);

  // compute beta_3
  seal::Ciphertext beta_3;
  evaluator->exponentiate(twot_N_2__plus__N_1, 2, *relinKeys, beta_3);
  evaluator->multiply_plain_inplace(beta_3, two);
  evaluator->relinearize_inplace(beta_3, *relinKeys);

  return ResultCiphertexts(alpha, beta_1, beta_2, beta_3);
}

seal::Ciphertext ChiSquaredBatched::encode_all_slots_and_encrypt(
    int64_t value) {
  seal::Ciphertext encrypted;
  encryptor->encrypt(encode_all_slots(value), encrypted);
  return encrypted;
}

seal::Plaintext ChiSquaredBatched::encode_all_slots(int64_t value) {
  std::vector<uint64_t> data(encoder->slot_count(), value);
  seal::Plaintext temp;
  encoder->encode(data, temp);
  return temp;
}

void ChiSquaredBatched::run_chi_squared() {
  std::stringstream ss_time;

  // set up the BFV scheme
  auto t0 = Time::now();
  setup_context_bfv(32768, 4096);
  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  auto t2 = Time::now();
  int64_t n0_val = 2, n1_val = 7, n2_val = 9;
  seal::Ciphertext n0 = encode_all_slots_and_encrypt(n0_val);
  seal::Ciphertext n1 = encode_all_slots_and_encrypt(n1_val);
  seal::Ciphertext n2 = encode_all_slots_and_encrypt(n2_val);
  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

  // perform FHE computation
  auto t4 = Time::now();
  auto result = compute_alpha_betas(n0, n1, n2);
  auto t5 = Time::now();
  log_time(ss_time, t4, t5, false);

  // decrypt results
  auto t6 = Time::now();
  int64_t result_alpha = get_first_decrypted_value(result.alpha);
  int64_t result_beta1 = get_first_decrypted_value(result.beta_1);
  int64_t result_beta2 = get_first_decrypted_value(result.beta_2);
  int64_t result_beta3 = get_first_decrypted_value(result.beta_3);
  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);

  // check results
  auto exp_alpha = std::pow((4 * n0_val * n2_val) - std::pow(n1_val, 2), 2);
  assert(("Unexpected result for 'alpha' encountered!",
          result_alpha == exp_alpha));
  auto exp_beta_1 = 2 * std::pow(2 * n0_val + n1_val, 2);
  assert(("Unexpected result for 'beta_1' encountered!",
          result_beta1 == exp_beta_1));
  auto exp_beta_2 = ((2 * n0_val) + n1_val) * ((2 * n2_val) + n1_val);
  assert(("Unexpected result for 'beta_2' encountered!",
          result_beta2 == exp_beta_2));
  auto exp_beta_3 = 2 * std::pow(2 * n2_val + n1_val, 2);
  assert(("Unexpected result for 'beta_3' encountered!",
          result_beta3 == exp_beta_3));

  // write ss_time into file
  std::ofstream myfile;
  auto out_filename = std::getenv("OUTPUT_FILENAME");
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();

  // write FHE parameters into file
  write_parameters_to_file(context, "fhe_parameters_chi_squared.txt");
}

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'chi-squared-bfv-batched'..." << std::endl;
  ChiSquaredBatched().run_chi_squared();
  return 0;
}
