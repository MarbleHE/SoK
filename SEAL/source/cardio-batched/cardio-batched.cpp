#include "cardio-batched.h"

/*
 * Batched CKKS implementation for cardio benchmark.
 */

void CardioBatched::setup_context_ckks(std::size_t poly_modulus_degree) {
  seal::EncryptionParameters params(seal::scheme_type::CKKS);
  params.set_poly_modulus_degree(poly_modulus_degree);
  params.set_coeff_modulus(seal::CoeffModulus::Create(
      poly_modulus_degree,
      {60, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 60}));

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
  std::vector<int> steps = {-1, -2, -3, -4, -5, -6, -7, 8, 16, 32, 56, 64, 72};
  galoisKeys =
      std::make_unique<seal::GaloisKeys>(keyGenerator.galois_keys_local(steps));
  // std::ofstream ofs_gk("galois_keys.dat", std::ios::binary);
  // galoisKeys->save(ofs_gk);
  // ofs_gk.close();

  // Provide both public and secret key, however, we will use public-key
  // encryption as this is the one used in a typical client-server scenario.
  encryptor = std::make_unique<seal::Encryptor>(context, *publicKey);
  evaluator = std::make_unique<seal::Evaluator>(context);
  decryptor = std::make_unique<seal::Decryptor>(context, *secretKey);
  encoder = std::make_unique<seal::CKKSEncoder>(context);
  // std::cout << "Number of slots: " << encoder->slot_count() << std::endl;
}

CiphertextVector CardioBatched::slice(CiphertextVector ctxt, int idx_begin,
                                      int idx_end) {
  return CiphertextVector(ctxt.begin() + idx_begin, ctxt.begin() + idx_end);
}

CiphertextVector CardioBatched::slice(CiphertextVector ctxt, int idx_begin) {
  return CiphertextVector(ctxt.begin() + idx_begin, ctxt.end());
}

// return lhs < rhs
std::unique_ptr<seal::Ciphertext> CardioBatched::lower(CiphertextVector &lhs,
                                                       CiphertextVector &rhs) {
  std::unique_ptr<seal::Ciphertext> result =
      std::make_unique<seal::Ciphertext>();

  const int len = lhs.size();
  if (len == 1) {
    // andNY(lhs[0], rhs[0]) = !(lhs[0]) & rhs[0]
    seal::Plaintext one;
    encoder->encode(1.0, lhs[0].parms_id(), lhs[0].scale(), one);
    seal::Ciphertext lhs_neg = XOR(lhs[0], one);
    evaluator->rescale_to_next_inplace(lhs_neg);
    lhs_neg.scale() = initial_scale;
    evaluator->mod_switch_to_inplace(rhs[0], lhs_neg.parms_id());
    // print_info(lhs_neg);
    // print_info(rhs[0]);
    evaluator->multiply(lhs_neg, rhs[0], *result);
    evaluator->relinearize_inplace(*result, *relinKeys);
    // print_info(*result);
    evaluator->rescale_to_next_inplace(*result);
    // print_info(*result);

    (*result).scale() = initial_scale;
    // print_info(*result);
    return result;
  }

  auto get_level = [&](seal::Ciphertext &c) -> std::size_t {
    return context->get_context_data(c.parms_id())->chain_index();
  };

  const int len2 = len >> 1;

  CiphertextVector lhs_l = slice(lhs, 0, len2);
  CiphertextVector lhs_h = slice(lhs, len2);
  CiphertextVector rhs_l = slice(rhs, 0, len2);
  CiphertextVector rhs_h = slice(rhs, len2);

  seal::Ciphertext term1 = *lower(lhs_h, rhs_h);
  // print_info(term1);
  evaluator->mod_switch_to_inplace(lhs_h[0], rhs_h[0].parms_id());
  seal::Ciphertext h_equal = *equal(lhs_h, rhs_h);
  // print_info(h_equal);
  seal::Ciphertext l_equal = *lower(lhs_l, rhs_l);
  // print_info(l_equal);

  seal::Ciphertext term2;

  if (get_level(l_equal) > get_level(h_equal)) {
    evaluator->mod_switch_to_inplace(l_equal, h_equal.parms_id());
  } else if (get_level(l_equal) < get_level(h_equal)) {
    evaluator->mod_switch_to_inplace(h_equal, l_equal.parms_id());
  }

  evaluator->multiply(h_equal, l_equal, term2);
  evaluator->relinearize_inplace(term2, *relinKeys);
  evaluator->rescale_to_next_inplace(term2);
  term2.scale() = initial_scale;
  // print_info(term2);

  evaluator->mod_switch_to_inplace(term1, term2.parms_id());
  *result = XOR(term1, term2);
  evaluator->rescale_to_next_inplace(*result);
  (*result).scale() = initial_scale;
  // print_info(*result);
  return result;
}

void CardioBatched::internal_print_info(std::string variable_name,
                                        seal::Ciphertext &ctxt) {
  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);

  std::cout << variable_name << "\n"
            << "— chain_idx:\t"
            << context->get_context_data(ctxt.parms_id())->chain_index()
            << std::endl
            << std::fixed << std::setprecision(10) << "— scale:\t"
            << log2(ctxt.scale()) << " bits" << std::endl
            << "— size:\t\t" << ctxt.size() << std::endl;
  std::cout.copyfmt(old_fmt);
}

seal::Ciphertext CardioBatched::XOR(seal::Ciphertext &lhs,
                                    seal::Ciphertext &rhs) {
  // computes (a-b)^2 by assuming a,b are binary inputs
  // see https://stackoverflow.com/a/46674398
  seal::Ciphertext result;
  evaluator->sub(lhs, rhs, result);
  evaluator->square_inplace(result);
  evaluator->relinearize_inplace(result, *relinKeys);
  return result;
}

seal::Ciphertext CardioBatched::XOR(seal::Ciphertext &lhs,
                                    seal::Plaintext &rhs) {
  // computes (a-b)^2 by assuming a,b are binary inputs
  // see https://stackoverflow.com/a/46674398
  seal::Ciphertext result;
  evaluator->sub_plain(lhs, rhs, result);
  evaluator->square_inplace(result);
  evaluator->relinearize_inplace(result, *relinKeys);
  return result;
}

void CardioBatched::print_plaintext(seal::Plaintext &ptxt) {
  // decode plaintext
  std::vector<double> decoded_data;
  encoder->decode(ptxt, decoded_data);

  // print header of output
  std::cout << "---------------------------------------------------------"
            << std::endl
            << "SLOT_NO"
            << "  "
            << "8-BIT BIN"
            << "   "
            << "DEC" << std::endl
            << "---------------------------------------------------------"
            << std::endl;

  // a helper to print the current slot number
  auto get_slot_no_string = [](size_t loop_counter) {
    std::stringstream ss;
    ss << std::setfill('0');
    ss << std::setw(5);
    ss << (loop_counter + 1) / NUM_BITS;
    ss << "    ";
    return ss.str();
  };

  std::stringstream bits_current_number;
  const int loop_end = 30 * 8 /*decoded_data.size()*/;  // TODO
  for (size_t i = 0; i < loop_end; i++) {
    // print slot number
    if (i == 0 || (i % NUM_BITS) == 0) {
      std::cout << get_slot_no_string(i);
    }

    // print current bit/ciphertext
    uint64_t value = std::nearbyint(decoded_data[i]);
    if (value != 0 && value != 1) {
      std::cerr << "[ERROR] print_plaintext failed: bit could not be decoded "
                   "as 0 or 1: "
                << decoded_data[i] << ". " << std::endl;
    }
    std::cout << value;
    // store bit for decimal representation
    bits_current_number << value;

    // check if we reached the end a number (consists of NUM_BITS bits)
    if (i > 0 && ((i + 1) % NUM_BITS) == 0) {
      // print decimal representation of number
      std::cout << "   "
                << strtol(bits_current_number.str().c_str(), nullptr, 2)
                << std::endl;
      // reset stringstream to enable reuse
      bits_current_number.str("");
      bits_current_number.clear();
    } else if (((i + 1) % 4) == 0) {
      // print space after each 4 bits to enhance readability
      std::cout << " ";
    }
  }
  std::cout << std::endl;
}

void CardioBatched::print_ciphertext(seal::Ciphertext &ctxt) {
  seal::Plaintext ptxt;
  decryptor->decrypt(ctxt, ptxt);
  print_plaintext(ptxt);
}

seal::Ciphertext CardioBatched::encode_and_encrypt(
    std::vector<uint64_t> numbers) {
  seal::Ciphertext encrypted_numbers(context);
  encryptor->encrypt(encode(numbers), encrypted_numbers);
  return encrypted_numbers;
}

seal::Plaintext CardioBatched::encode(std::vector<uint64_t> numbers,
                                      seal::parms_id_type parms_id) {
  // transform each given number into a sequence of NUM_BITS bits
  std::vector<double> numbers_in_bits;
  for (auto n : numbers) {
    // convert integer to binary
    std::string bin = std::bitset<NUM_BITS>(n).to_string();
    // cast each char of binary string into a double
    for (size_t i = 0; i < NUM_BITS; ++i) {
      numbers_in_bits.push_back((double)bin.at(i) - 48);
    }
  }

  // encode bit sequence into a seal::Plaintext
  seal::Plaintext encoded_numbers;
  encoder->encode(numbers_in_bits, parms_id, initial_scale, encoded_numbers);

  return encoded_numbers;
}

seal::Plaintext CardioBatched::encode(std::vector<uint64_t> numbers) {
  return encode(numbers, context->first_parms_id());
}

void CardioBatched::print_vec(seal::Ciphertext &ctxt) {
  seal::Plaintext p;
  decryptor->decrypt(ctxt, p);
  std::vector<double> dec;
  encoder->decode(p, dec);
  // print the first 64 bits
  for (size_t k = 0; k < 160; ++k) {
    if (k % 8 == 0) {
      std::cout << std::endl;
    } else if (k % 4 == 0) {
      std::cout << "\t";
    } else {
      std::cout << "  ";
    }
    // std::cout << (uint64_t)std::nearbyint(dec[k]);
    std::cout << dec[k];
  }
  std::cout << std::endl;
}

std::vector<seal::Ciphertext> CardioBatched::split_by_binary_rep(
    seal::Ciphertext &ctxt) {
  std::vector<seal::Ciphertext> result;
  for (size_t i = 0; i < NUM_BITS; i++) {
    seal::Ciphertext temp_ctxt;
    evaluator->rotate_vector(ctxt, -i, *galoisKeys, temp_ctxt);
    result.push_back(temp_ctxt);
  }
  return result;
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

void CardioBatched::run_cardio() {
  std::stringstream ss_time;

  auto t0 = Time::now();
  // poly_modulus_degree:
  // - must be a power of two
  // - determines the number of ciphertext slots
  // - determines the max. of the sum of coeff_moduli bits
  setup_context_ckks(32768);

  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  auto t2 = Time::now();

  // encode and encrypt keystream
  // assumption: this keystream is known by client and server
  std::vector<uint64_t> keystream = {121, 58,  242, 156, 29,  94,
                                     136, 91,  227, 68,  251, 70,
                                     212, 155, 223, 154, 221, 251};

  // === client-side computation ====================================

  // define input values
  bool man = false;
  bool antecedent = true;
  bool smoking = true;
  bool diabetic = true;
  bool pressure = true;
  uint64_t age = 55;
  uint64_t hdl = 50;
  uint64_t height = 80;
  uint64_t phy_act = 45;
  uint64_t drinking = 4;
  uint64_t weight = 80;

  // == Conditions ====
  // F  +1  if man                                  && 50 < [age]
  // T  +1  if antecedent                           && 0 < [1]
  // T  +1  if smoking                              && 0 < [1]
  // T  +1  if diabetic                             && 0 < [1]
  // T  +1  if high blood pressure                  && 0 < [1]
  // F  +1  if man                                  && 3 < [alc_consumption]
  // T  +1  if (!man)                               && 2 < [alc_consumption]
  // F  +1  if TRUE                                 && [HDL] < 40
  // T  +1  if TRUE                                 && [height] < [weight+90]
  // F  +1  if TRUE                                 && [phy_act] < 30

  // encode and encrypt the inputs
  std::vector<uint64_t> in;
  in.push_back(man ^ keystream[0]);
  in.push_back(antecedent ^ keystream[1]);
  in.push_back(smoking ^ keystream[2]);
  in.push_back(diabetic ^ keystream[3]);
  in.push_back(pressure ^ keystream[4]);
  in.push_back(man ^ keystream[5]);
  in.push_back(!man ^ keystream[6]);
  in.push_back(age ^ keystream[7]);
  in.push_back(1 ^ keystream[8]);
  in.push_back(1 ^ keystream[9]);
  in.push_back(1 ^ keystream[10]);
  in.push_back(1 ^ keystream[11]);
  in.push_back(drinking ^ keystream[12]);
  in.push_back(drinking ^ keystream[13]);
  in.push_back(hdl ^ keystream[14]);
  in.push_back(height ^ keystream[15]);
  in.push_back(phy_act ^ keystream[16]);
  in.push_back(weight + 90 ^ keystream[17]);

  seal::Ciphertext inputs = encode_and_encrypt(in);

  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

  // // transmit data to server...

  // // === server-side computation ====================================

  auto t4 = Time::now();

  // homomorphically execute the Kreyvium algorithm to decrypt data
  seal::Plaintext ks = encode(keystream);
  seal::Ciphertext result = XOR(inputs, ks);
  evaluator->rescale_to_next_inplace(result);
  result.scale() = initial_scale;
  // print_ciphertext(result);

  // create a copy of the input vector
  seal::Ciphertext bool_flags = result;
  // mask the flags
  seal::Plaintext mask = encode({1, 1, 1, 1, 1, 1, 1}, bool_flags.parms_id());
  evaluator->multiply_plain_inplace(bool_flags, mask);
  evaluator->rescale_to_next_inplace(bool_flags);
  bool_flags.scale() = initial_scale;
  // set 1 at bit positions 63, 71, 78 (required for batching scheme)
  seal::Plaintext addendum =
      encode({0, 0, 0, 0, 0, 0, 0, 1, 1, 1}, bool_flags.parms_id());
  evaluator->add_plain_inplace(bool_flags, addendum);

  // prepare b by adding missing values and extracting values for lhs of smaller
  // equation
  std::vector<double> mask_b(encoder->slot_count(), 0.0);
  for (size_t i = 112; i < 112 + 3 * NUM_BITS; i++) mask_b[i] = 1.0;
  seal::Plaintext mask_b_enc;
  encoder->encode(mask_b, result.parms_id(), initial_scale, mask_b_enc);
  seal::Ciphertext b;
  evaluator->multiply_plain(result, mask_b_enc, b);
  evaluator->relinearize_inplace(b, *relinKeys);
  evaluator->rescale_to_next_inplace(b);
  b.scale() = initial_scale;
  evaluator->rotate_vector_inplace(b, 56, *galoisKeys);
  // merge with the constants that are not given as inputs
  seal::Plaintext const_b = encode({50, 0, 0, 0, 0, 3, 2}, b.parms_id());
  evaluator->add_plain_inplace(b, const_b);

  // prepare c by adding missing values and extracting values for rhs of smaller
  // equation
  std::vector<double> mask_c(encoder->slot_count(), 0);
  for (size_t i = 56; i < 56 + 7 * NUM_BITS; i++) mask_c[i] = 1;
  seal::Plaintext mask_c_enc;
  encoder->encode(mask_c, result.parms_id(), initial_scale, mask_c_enc);
  seal::Ciphertext c;
  evaluator->multiply_plain(result, mask_c_enc, c);
  evaluator->relinearize_inplace(c, *relinKeys);
  evaluator->rescale_to_next_inplace(c);
  c.scale() = initial_scale;
  evaluator->rotate_vector_inplace(c, 56, *galoisKeys);
  // merge with the constants that are not given as inputs
  seal::Plaintext const_c =
      encode({0, 0, 0, 0, 0, 0, 0, 40, 0, 30}, c.parms_id());
  evaluator->add_plain_inplace(c, const_c);

  // extract and merge weight+90 into other values in ciphertext c
  std::vector<double> mask_weight90(encoder->slot_count(), 0);
  for (size_t i = 136; i < 136 + 1 * NUM_BITS; i++) mask_weight90[i] = 1;
  seal::Plaintext mask_weight90_enc;
  encoder->encode(mask_weight90, result.parms_id(), initial_scale,
                  mask_weight90_enc);
  seal::Ciphertext weight90;
  evaluator->multiply_plain(result, mask_weight90_enc, weight90);
  evaluator->relinearize_inplace(weight90, *relinKeys);
  evaluator->rotate_vector_inplace(weight90, 72, *galoisKeys);
  evaluator->rescale_to_next_inplace(weight90);
  weight90.scale() = c.scale();
  evaluator->add_inplace(c, weight90);

  // bool_flags, b, c are the ciphertexts where first nine slots contain actual
  // values, i.e., (index+1) mod 8 == 0 contains index-th input
  std::vector<seal::Ciphertext> b_encoded = split_by_binary_rep(b);
  std::vector<seal::Ciphertext> c_encoded = split_by_binary_rep(c);

  // lower_result := b_encoded < c_encoded
  seal::Ciphertext lower_result = *lower(b_encoded, c_encoded);

  // condition_result := bool_flags & lower_result
  seal::Ciphertext condition_result;
  evaluator->mod_switch_to_inplace(bool_flags, lower_result.parms_id());
  evaluator->multiply(bool_flags, lower_result, condition_result);
  evaluator->relinearize_inplace(condition_result, *relinKeys);

  // perform sum & rotate to compute the result of the cardio program
  seal::Ciphertext rot8, rot4, rot2, final_result;
  evaluator->rotate_vector(condition_result, 8 * NUM_BITS, *galoisKeys, rot8);
  evaluator->add_inplace(rot8, condition_result);
  evaluator->rotate_vector(rot8, 4 * NUM_BITS, *galoisKeys, rot4);
  evaluator->add_inplace(rot4, rot8);
  evaluator->rotate_vector(rot4, 2 * NUM_BITS, *galoisKeys, rot2);
  evaluator->add_inplace(rot2, rot4);
  evaluator->rotate_vector(rot2, 1 * NUM_BITS, *galoisKeys, final_result);
  evaluator->add_inplace(final_result, rot2);

  auto t5 = Time::now();
  log_time(ss_time, t4, t5, false);

  auto t6 = Time::now();

  // retrieve the final result (ciphertext slot 7)
  seal::Plaintext p;
  decryptor->decrypt(final_result, p);
  std::vector<double> dec;
  encoder->decode(p, dec);
  std::cout << "Result: " << (uint64_t)dec[7] << std::endl;

  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);

  // write ss_time into file
  std::ofstream myfile;
  myfile.open("seal_batched_cardio.csv", std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();
}

std::unique_ptr<seal::Ciphertext> CardioBatched::multvect(
    CiphertextVector bitvec) {
  const int size = bitvec.size();
  for (std::size_t k = 1; k < size; k *= 2) {
    for (std::size_t i = 0; i < size - k; i += 2 * k) {
      evaluator->multiply_inplace(bitvec[i], bitvec[i + k]);
      evaluator->relinearize_inplace(bitvec[i], *relinKeys);
      evaluator->rescale_to_next_inplace(bitvec[i]);
      // print_info(bitvec[i]);
    }
  }
  return std::make_unique<seal::Ciphertext>(bitvec[0]);
}

std::unique_ptr<seal::Ciphertext> CardioBatched::equal(CiphertextVector &lhs,
                                                       CiphertextVector &rhs) {
  assert(("equal supports same-sized inputs only!", lhs.size() == rhs.size()));

  CiphertextVector comp;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    seal::Ciphertext tmp;
    // evaluator->add(lhs[i], rhs[i], tmp);
    // print_info(lhs[i]);
    // print_info(rhs[i]);
    tmp = XOR(lhs[i], rhs[i]);
    evaluator->rescale_to_next_inplace(tmp);
    // print_info(tmp);
    tmp.scale() = initial_scale;
    // print_info(tmp);

    seal::Plaintext one;
    encoder->encode(1.0, tmp.parms_id(), tmp.scale(), one);
    // print_info(tmp);

    tmp = XOR(tmp, one);
    evaluator->rescale_to_next_inplace(tmp);
    // print_info(tmp);
    tmp.scale() = initial_scale;
    comp.push_back(tmp);
  }
  std::unique_ptr<seal::Ciphertext> mv_result = multvect(comp);
  // print_info(*mv_result);
  return mv_result;
}

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'cardio-batched'..." << std::endl;
  CardioBatched().run_cardio();
  return 0;
}
