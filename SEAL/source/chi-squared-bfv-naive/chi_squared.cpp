#include "chi_squared.h"

#include "../common.h"

void ChiSquared::setup_context_bfv(std::size_t poly_modulus_degree,
                                   std::uint64_t plain_modulus) {
  /// Wrapper for parameters
  seal::EncryptionParameters params(seal::scheme_type::BFV);
  params.set_poly_modulus_degree(poly_modulus_degree);
#ifdef MANUALPARAMS
  params.set_coeff_modulus(seal::CoeffModulus::Create(
      poly_modulus_degree,  {60, 60, 30}));
#endif

#ifdef CINGUPARAM
  params.set_coeff_modulus(seal::CoeffModulus::Create(
      poly_modulus_degree, {30, 40, 44, 50, 54, 60, 60}));
#endif

#ifdef SEALPARAMS
  params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(
      poly_modulus_degree, seal::sec_level_type::tc128));
#endif
  params.set_plain_modulus(plain_modulus);

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
  encoder = std::make_unique<seal::IntegerEncoder>(context);
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

int32_t ChiSquared::get_decrypted_value(seal::Ciphertext value) {
  seal::Plaintext tmp;
  decryptor->decrypt(value, tmp);
  return encoder->decode_int32(tmp);
}

ResultCiphertexts ChiSquared::compute_alpha_betas(const seal::Ciphertext &N_0,
                                                  const seal::Ciphertext &N_1,
                                                  const seal::Ciphertext &N_2) {

  // compute alpha
  std::cout << "Computing alpha" << std::endl;
  seal::Plaintext four;
  encoder->encode(4, four);
  seal::Ciphertext four_n0;
  seal::Ciphertext four_ctxt;
  encryptor->encrypt(four, four_ctxt);
  evaluator->multiply(N_0, four_ctxt, four_n0);
  evaluator->relinearize(four_n0, *relinKeys, four_n0);
  seal::Ciphertext four_n0_n2;
  evaluator->multiply(four_n0, N_2, four_n0_n2);
  evaluator->relinearize(four_n0_n2, *relinKeys, four_n0_n2);
  seal::Ciphertext N_1_pow2;
  evaluator->exponentiate(N_1, 2, *relinKeys, N_1_pow2);
  seal::Ciphertext difference;
  evaluator->sub(four_n0_n2, N_1_pow2, difference);
  seal::Ciphertext alpha;
  evaluator->exponentiate(difference, 2, *relinKeys, alpha);


  // compute beta_1
  std::cout << "Computing beta_1" << std::endl;

  seal::Ciphertext N_0_t2;
  seal::Plaintext two;
  encoder->encode(2, two);
  seal::Ciphertext two_ctxt;
  encryptor->encrypt(two, two_ctxt);
  evaluator->multiply(N_0, two_ctxt, N_0_t2);
  seal::Ciphertext N_0_t2_relin;
  evaluator->relinearize(N_0_t2, *relinKeys, N_0_t2_relin);
  seal::Ciphertext twot_N_0__plus__N_1;
  evaluator->add(N_0_t2_relin, N_1, twot_N_0__plus__N_1);
  seal::Ciphertext beta_1_t1;
  evaluator->exponentiate(twot_N_0__plus__N_1, 2, *relinKeys, beta_1_t1);
  seal::Ciphertext beta_1_t2;
  evaluator->multiply(beta_1_t1, two_ctxt, beta_1_t2);
  seal::Ciphertext beta_1;
  evaluator->relinearize(beta_1_t2, *relinKeys, beta_1);

  // compute beta_2
  std::cout << "Computing beta_2" << std::endl;

  // First, re-compute twot_N_0__plus__N_1
  seal::Ciphertext beta_1_;
  seal::Ciphertext N_0_t2_;
  seal::Ciphertext two_ctxt_;
  encryptor->encrypt(two, two_ctxt_);
  evaluator->multiply(N_0, two_ctxt_, N_0_t2_);
  seal::Ciphertext N_0_t2_relin_;
  evaluator->relinearize(N_0_t2_, *relinKeys, N_0_t2_relin_);
  seal::Ciphertext twot_N_0__plus__N_1_;
  evaluator->add(N_0_t2, N_1, twot_N_0__plus__N_1_);

  seal::Ciphertext t2_N_2_temp;
  evaluator->multiply(N_2, two_ctxt, t2_N_2_temp);
  seal::Ciphertext t2_N_2;
  evaluator->relinearize(t2_N_2_temp, *relinKeys, t2_N_2);
  seal::Ciphertext twot_N_2__plus__N_1;
  evaluator->add(t2_N_2, N_1, twot_N_2__plus__N_1);
  seal::Ciphertext beta_2_temp;
  evaluator->multiply(twot_N_0__plus__N_1, twot_N_2__plus__N_1, beta_2_temp);
  seal::Ciphertext beta_2;
  evaluator->relinearize(beta_2_temp, *relinKeys, beta_2);

  // compute beta_3
  std::cout << "Computing beta_3" << std::endl;

  seal::Ciphertext beta_3_t1;
  evaluator->exponentiate(twot_N_2__plus__N_1, 2, *relinKeys, beta_3_t1);
  seal::Ciphertext beta_3_t2;
  evaluator->multiply(beta_3_t1, two_ctxt, beta_3_t2);
  seal::Ciphertext beta_3;
  evaluator->relinearize(beta_3_t2, *relinKeys, beta_3);

  return ResultCiphertexts(alpha, beta_1, beta_2, beta_3);
}

void ChiSquared::run_chi_squared() {
  std::stringstream ss_time;

  // set up the BFV scheme
  auto t0 = Time::now();
  setup_context_bfv(32768, 4096);
  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  auto t2 = Time::now();
  int32_t n0_val = 2, n1_val = 7, n2_val = 9;
  seal::Ciphertext n0, n1, n2;
  encryptor->encrypt(encoder->encode(n0_val), n0);
  encryptor->encrypt(encoder->encode(n1_val), n1);
  encryptor->encrypt(encoder->encode(n2_val), n2);
  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

  // perform FHE computation
  auto t4 = Time::now();
  auto result = compute_alpha_betas(n0, n1, n2);
  auto t5 = Time::now();
  log_time(ss_time, t4, t5, false);

  // decrypt results
  auto t6 = Time::now();
  int32_t result_alpha = get_decrypted_value(result.alpha);
  int32_t result_beta1 = get_decrypted_value(result.beta_1);
  int32_t result_beta2 = get_decrypted_value(result.beta_2);
  int32_t result_beta3 = get_decrypted_value(result.beta_3);
  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);

  // check results
  auto exp_alpha = std::pow((4*n0_val*n2_val) - std::pow(n1_val, 2), 2);
  assert(("Unexpected result for 'alpha' encountered!",
      result_alpha==exp_alpha));
  auto exp_beta_1 = 2*std::pow(2*n0_val + n1_val, 2);
  assert(("Unexpected result for 'beta_1' encountered!",
      result_beta1==exp_beta_1));
  auto exp_beta_2 = ((2*n0_val) + n1_val)*((2*n2_val) + n1_val);
  assert(("Unexpected result for 'beta_2' encountered!",
      result_beta2==exp_beta_2));
  auto exp_beta_3 = 2*std::pow(2*n2_val + n1_val, 2);
  assert(("Unexpected result for 'beta_3' encountered!",
      result_beta3==exp_beta_3));

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
  std::cout << "Starting benchmark 'chi-squared-bfv-naive'..." << std::endl;
  ChiSquared().run_chi_squared();
  return 0;
}
