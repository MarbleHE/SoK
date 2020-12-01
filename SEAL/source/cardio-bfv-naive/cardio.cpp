#include "cardio.h"

#include "../common.h"

#define SEX_FIELD 0
#define ANTECEDENT_FIELD 1
#define SMOKER_FIELD 2
#define DIABETES_FIELD 3
#define PRESSURE_FIELD 4

void Cardio::setup_context_bfv(std::size_t poly_modulus_degree,
                               std::uint64_t plain_modulus) {
  /// Wrapper for parameters
  seal::EncryptionParameters params(seal::scheme_type::BFV);
  params.set_poly_modulus_degree(poly_modulus_degree);

#ifdef MANUALPARAMS
  params.set_coeff_modulus(seal::CoeffModulus::Create(
      poly_modulus_degree,  {30, 60, 60, 60, 60, 60}));
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

CiphertextVector Cardio::encode_and_encrypt(int32_t number) {
  const static int NUM_BITS = 8;

  // convert integer to binary
  std::string bin = std::bitset<NUM_BITS>(number).to_string();

  seal::Plaintext zero_ptxt = encoder->encode(0);
  seal::Ciphertext zero;
  encryptor->encrypt(zero_ptxt, zero);
  CiphertextVector result(NUM_BITS, zero);
  for (int i = 0; i < NUM_BITS; ++i) {
    // transform char -> int32_t
    int32_t val = (int) bin.at(NUM_BITS - 1 - i) - 48;
    // encode bit as integer
    seal::Plaintext b = encoder->encode(val);
    // encrypt bit
    encryptor->encrypt(b, result.at(i));
  }

  return result;
}

CiphertextVector Cardio::ctxt_to_ciphertextvector(seal::Ciphertext &ctxt) {
  const static int NUM_BITS = 8;
  seal::Ciphertext zero;
  encryptor->encrypt(encoder->encode(0), zero);
  CiphertextVector result(8, zero);
  result[0] = seal::Ciphertext(ctxt);

  for (size_t i = 1; i < NUM_BITS; i++) {
    encryptor->encrypt(encoder->encode(0), result.at(i));
  }

  return result;
}

void Cardio::shift_left_inplace(CiphertextVector &ctxt) {
  for (std::size_t i = 1; i < ctxt.size(); ++i) {
    ctxt[i - 1] = ctxt[i];
  }
  seal::Ciphertext zero;
  encryptor->encrypt(encoder->encode(0), zero);
  ctxt[7] = zero;
}

void Cardio::shift_right_inplace(CiphertextVector &ctxt) {
  for (std::size_t i = ctxt.size() - 2; i > 0; --i) {
    ctxt[i + 1] = ctxt[i];
  }
  seal::Ciphertext zero;
  encryptor->encrypt(encoder->encode(0), zero);
  ctxt[0] = zero;
}

std::unique_ptr<seal::Ciphertext> Cardio::multvect(CiphertextVector bitvec) {
  const int size = bitvec.size();
  for (std::size_t k = 1; k < size; k *= 2) {
    for (std::size_t i = 0; i < size - k; i += 2*k) {
      evaluator->multiply(bitvec[i], bitvec[i + k], bitvec[i]);
      evaluator->relinearize(bitvec[i], *relinKeys, bitvec[i]);
    }
  }
  return std::make_unique<seal::Ciphertext>(bitvec[0]);
}

std::unique_ptr<seal::Ciphertext> Cardio::equal(CiphertextVector lhs,
                                                CiphertextVector rhs) {
  assert(("equal supports same-sized inputs only!", lhs.size()==rhs.size()));

  CiphertextVector comp;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    seal::Ciphertext tmp;
    evaluator->add(lhs[i], rhs[i], tmp);
    seal::Ciphertext one;
    encryptor->encrypt(encoder->encode(1), one);
    evaluator->add(tmp, one, tmp); // negate tmp
    comp.push_back(tmp);
  }
  return multvect(comp);
}

void Cardio::print_ciphertext(std::string name, seal::Ciphertext &ctxt) {
  seal::Plaintext p;
  decryptor->decrypt(ctxt, p);
  std::cout << name << ": " << encoder->decode_int32(p) << std::flush
            << std::endl;
}

/// Implements a ripple carry adder.
CiphertextVector Cardio::add(CiphertextVector lhs, CiphertextVector rhs) {
  auto size = lhs.size();

  seal::Ciphertext zero;
  encryptor->encrypt(encoder->encode(0), zero);

  seal::Ciphertext carry;
  encryptor->encrypt(encoder->encode(0), carry);

  CiphertextVector res;

  //std::cout << "lhs: " << ciphertextvector_to_int(lhs) << std::endl;
  //std::cout << "rhs: " << ciphertextvector_to_int(rhs) << std::endl;

  for (std::size_t i = 0; i < size; ++i) {

    // SUM
    //print_ciphertext("lhs[i]:", lhs[i]);
    //print_ciphertext("rhs[i]:", rhs[i]);
    seal::Ciphertext sum;
    encryptor->encrypt(encoder->encode(0), sum);
    evaluator->add(lhs[i], rhs[i], sum);
    //print_ciphertext("internal_sum (a+b):", sum);
    evaluator->add(sum, carry, sum);
    //print_ciphertext("internal_sum (a+b+carry):", sum);
    res.push_back(sum);

    // CARRY
    seal::Ciphertext p;
    encryptor->encrypt(encoder->encode(0), p);
    evaluator->multiply(lhs[i], rhs[i], p);
    evaluator->relinearize(p, *relinKeys, p);

    //print_ciphertext("p (lr):", p);

    seal::Ciphertext temp;
    encryptor->encrypt(encoder->encode(0), temp);
    evaluator->add(lhs[i], rhs[i], temp);

    //print_ciphertext("temp (l+r):", temp);

    seal::Ciphertext temp2;
    encryptor->encrypt(encoder->encode(0), temp2);
    evaluator->multiply(carry, temp, temp2);
    evaluator->relinearize(temp2, *relinKeys, temp2);

    //print_ciphertext("temp2 c(l+r):", temp2);

    evaluator->add(p, temp2, carry);

    //print_ciphertext("internal_carry (p + temp2):", carry);
  }

  //std::cout << "sum: " << ciphertextvector_to_int(res) << std::endl;
  return res;
}

CiphertextVector Cardio::slice(CiphertextVector ctxt, int idx_begin,
                               int idx_end) {
  return CiphertextVector(ctxt.begin() + idx_begin, ctxt.begin() + idx_end);
}

CiphertextVector Cardio::slice(CiphertextVector ctxt, int idx_begin) {
  return CiphertextVector(ctxt.begin() + idx_begin, ctxt.end());
}

// return lhs < rhs
std::unique_ptr<seal::Ciphertext> Cardio::lower(CiphertextVector &lhs,
                                                CiphertextVector &rhs) {
  std::unique_ptr<seal::Ciphertext> result =
      std::make_unique<seal::Ciphertext>();

  const int len = lhs.size();
  if (len==1) {
    seal::Ciphertext one;
    encryptor->encrypt(encoder->encode(1), one);
    seal::Ciphertext lhs_neg;
    // andNY(lhs[0], rhs[0]) = !(lhs[0]) & rhs[0]
    evaluator->add(lhs[0], one, lhs_neg);
    evaluator->multiply(lhs_neg, rhs[0], *result);
    evaluator->relinearize(*result, *relinKeys, *result);
    return result;
  }

  const int len2 = len >> 1;

  CiphertextVector lhs_l = slice(lhs, 0, len2);
  CiphertextVector lhs_h = slice(lhs, len2);

  CiphertextVector rhs_l = slice(rhs, 0, len2);
  CiphertextVector rhs_h = slice(rhs, len2);

  seal::Ciphertext term1 = *lower(lhs_h, rhs_h);
  seal::Ciphertext term2;
  evaluator->multiply(*equal(lhs_h, rhs_h), *lower(lhs_l, rhs_l), term2);
  evaluator->relinearize(term2, *relinKeys, term2);
  evaluator->add(term1, term2, *result);
  return result;
}

void Cardio::print_ciphertextvector(CiphertextVector &vec) {
  std::cout << "size: " << vec.size() << std::endl;

  std::cout << "idx:\t\t";
  for (int i = vec.size() - 1; i >= 0; --i) {
    std::cout << i << " ";
  }

  std::cout << std::endl << "val (bin):\t";
  std::stringstream ss;
  for (int i = vec.size() - 1; i >= 0; --i) {
    seal::Plaintext p;
    decryptor->decrypt(vec[i], p);
    auto value = encoder->decode_int32(p);
    std::cout << value << " " << std::flush;
    ss << value;
  }

  std::cout << std::endl;
  long int decimal_value = strtol(ss.str().c_str(), nullptr, 2);
  std::cout << "val (dec):\t" << decimal_value << std::endl;
}

int Cardio::ciphertextvector_to_int(CiphertextVector &vec) {
  std::stringstream ss;
  for (int i = vec.size() - 1; i >= 0; --i) {
    seal::Plaintext p;
    decryptor->decrypt(vec[i], p);
    ss << encoder->decode_int32(p);
  }
  return strtol(ss.str().c_str(), nullptr, 2);
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

void Cardio::run_cardio() {
  std::stringstream ss_time;

  // set up the BFV schema
  auto t0 = Time::now();
  setup_context_bfv(16384, 2);
  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  auto t2 = Time::now();
  // // encode and encrypt keystream
  // int32_t keystream[] = {241, 210, 225, 219, 92, 43, 197};

  // auto ks0 = encode_and_encrypt(keystream[0]);
  // auto ks1 = encode_and_encrypt(keystream[1]);
  // auto ks2 = encode_and_encrypt(keystream[2]);
  // auto ks3 = encode_and_encrypt(keystream[3]);
  // auto ks4 = encode_and_encrypt(keystream[4]);
  // auto ks5 = encode_and_encrypt(keystream[5]);
  // auto ks6 = encode_and_encrypt(keystream[6]);

  // === client-side computation ====================================

  // encode and encrypt the inputs
  // Cingulata:
  //  flags_0 = 0
  //  flags_1 = 1
  //  flags_2 = 1
  //  flags_3 = 1
  //  flags_4 = 1
  // instead of 15 we encode 30 as the bit order in Cingulata is reversed
  auto flags = encode_and_encrypt(30);  // 30 == 0001 1110
  auto age = encode_and_encrypt(55);
  auto hdl = encode_and_encrypt(50);
  auto height = encode_and_encrypt(80);
  auto weight = encode_and_encrypt(80);
  auto physical_act = encode_and_encrypt(45);
  auto drinking = encode_and_encrypt(4);

  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

  // transmit data to server...

  // === server-side computation ====================================

  auto t4 = Time::now();

  // homomorphically execute the Kreyvium algorithm
  // arithmetic addition of bits corresponds to bitwise XOR
  // for (int i = 0; i < 8; ++i) {
  //   // for (int i = 0; i < 5; i++) { flags[i] ^= keystream[0][i];}
  //   evaluator->add_inplace(flags[i], ks0[i]);
  //   // age ^= keystream[1];
  //   evaluator->add_inplace(age[i], ks1[i]);
  //   // hdl ^= keystream[2];
  //   evaluator->add_inplace(hdl[i], ks2[i]);
  //   // height ^= keystream[3];
  //   evaluator->add_inplace(height[i], ks3[i]);
  //   // weight ^= keystream[4];
  //   evaluator->add_inplace(weight[i], ks4[i]);
  //   // physical_act ^= keystream[5];
  //   evaluator->add_inplace(physical_act[i], ks5[i]);
  //   // drinking ^= keystream[6];
  //   evaluator->add_inplace(drinking[i], ks6[i]);
  // }


  // cardiac risk factor assessment algorithm
  seal::Plaintext zero_ptxt = encoder->encode(0);
  seal::Ciphertext zero;
  encryptor->encrypt(zero_ptxt, zero);

  //TODO: In the "optimized" versions, we can also significantly lower the depth here by doing a tree of additions
  CiphertextVector risk_score(8, zero);

  // (flags[SEX_FIELD] & (50 < age))
  seal::Ciphertext condition1;
  CiphertextVector fifty = encode_and_encrypt(50);
  evaluator->multiply(flags[SEX_FIELD], *lower(fifty, age), condition1);
  evaluator->relinearize_inplace(condition1, *relinKeys);
  risk_score = add(risk_score, ctxt_to_ciphertextvector(condition1));

  // flags[SEX_FIELD]+1 & (60 < age)
  // expected: true
  seal::Ciphertext sex_female;
  // !flags[SEX_FIELD] == flags[SEX_FIELD]+1
  evaluator->add_plain(flags[SEX_FIELD], encoder->encode(1), sex_female);
  CiphertextVector sixty = encode_and_encrypt(60);
  seal::Ciphertext condition2;
  evaluator->multiply(sex_female, *lower(sixty, age), condition2);
  evaluator->relinearize_inplace(condition2, *relinKeys);
  risk_score = add(risk_score, ctxt_to_ciphertextvector(condition2));

  // flags[ANTECEDENT_FIELD]
  // expected: true
  risk_score =
      add(risk_score, ctxt_to_ciphertextvector(flags[ANTECEDENT_FIELD]));

  // flags[SMOKER_FIELD]
  // expected: true
  risk_score = add(risk_score, ctxt_to_ciphertextvector(flags[SMOKER_FIELD]));

  // flags[DIABETES_FIELD]
  // expected: true
  risk_score = add(risk_score, ctxt_to_ciphertextvector(flags[DIABETES_FIELD]));

  // flags[PRESSURE_FIELD]
  // expected: false
  risk_score = add(risk_score, ctxt_to_ciphertextvector(flags[PRESSURE_FIELD]));

  // hdl < 40
  // expected: false
  CiphertextVector fourty = encode_and_encrypt(40);
  seal::Ciphertext condition7 = *lower(hdl, fourty);
  risk_score = add(risk_score, ctxt_to_ciphertextvector(condition7));

  // weight > height-90
  // iff. height < weight+90
  // expected: false
  CiphertextVector ninety = encode_and_encrypt(90);
  CiphertextVector weight90 = add(weight, ninety);
  seal::Ciphertext condition8 = *lower(height, weight90);
  risk_score = add(risk_score, ctxt_to_ciphertextvector(condition8));

  // physical_act < 30
  // expected: false
  CiphertextVector thirty = encode_and_encrypt(30);
  seal::Ciphertext condition9 = *lower(physical_act, thirty);
  risk_score = add(risk_score, ctxt_to_ciphertextvector(condition9));

  // flags[SEX_FIELD] && (3 < drinking)
  // expected: true
  seal::Ciphertext condition10;
  CiphertextVector three = encode_and_encrypt(3);
  evaluator->multiply(flags[SEX_FIELD], *lower(three, drinking), condition10);
  evaluator->relinearize_inplace(condition10, *relinKeys);
  risk_score = add(risk_score, ctxt_to_ciphertextvector(condition10));

  // !flags[SEX_FIELD] && (2 < drinking)
  // expected: true
  CiphertextVector two = encode_and_encrypt(2);
  seal::Ciphertext condition11;
  evaluator->multiply(sex_female, *lower(two, drinking), condition11);
  evaluator->relinearize_inplace(condition11, *relinKeys);
  risk_score = add(risk_score, ctxt_to_ciphertextvector(condition11));

  auto t5 = Time::now();
  log_time(ss_time, t4, t5, false);

  // === client-side computation ====================================

  auto t6 = Time::now();

  // decrypt and check result
  int result = ciphertextvector_to_int(risk_score);
  assert(("Cardio benchmark does not produce expected result!", result==6));
  std::cout << "Result: " << result << std::endl;

  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);

  // write ss_time into file
  std::ofstream myfile;
  auto out_filename = std::getenv("OUTPUT_FILENAME");
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();

  // write FHE parameters into file
  write_parameters_to_file(context, "fhe_parameters.txt");
}

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'cardio-bfv'..." << std::endl;
  Cardio().run_cardio();

  return 0;
}
