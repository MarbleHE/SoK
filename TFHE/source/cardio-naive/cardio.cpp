#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>
#include <stdio.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>

typedef std::chrono::milliseconds ms;
typedef std::chrono::high_resolution_clock Time;

namespace {
void log_time(std::stringstream &ss,
              std::chrono::time_point<std::chrono::high_resolution_clock> start,
              std::chrono::time_point<std::chrono::high_resolution_clock> end,
              bool last = false) {
  ss << std::chrono::duration_cast<ms>(end - start).count();
  if (!last) ss << ",";
}
}  // namespace

std::stringstream ss_time;

// Counters for gates
int and_gates = 0;
int xor_gates = 0;

void client();
void cloud();
void verify();

int main() {
  client();
  cloud();
  verify();

  // Report total gate numbers
  std::cout << "and: " << and_gates << std::endl;
  std::cout << "xor: " << xor_gates << std::endl;

  // Print out times:
  std::cout << ss_time.str() << std::endl;

  // write ss_time into file
  std::ofstream myfile;
  auto out_filename = std::getenv("OUTPUT_FILENAME");
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();
  return 0;
}

const int NB_FLAGS = 5;
const int SEX_FIELD = 0;
const int ANTECEDENT_FIELD = 1;
const int SMOKER_FIELD = 2;
const int DIABETES_FIELD = 3;
const int PRESSURE_FIELD = 4;

const int NB_VALUES = 8;

//#define DEBUG

#ifdef DEBUG
/// DEBUG SECRET_KEY
TFheGateBootstrappingSecretKeySet *SECRET_KEY;
#endif

int decrypt_array(const LweSample *array, int nb_size, const TFheGateBootstrappingSecretKeySet *key) {
  uint8_t int_answer = 0;
  for (int i = 0; i < nb_size; i++) {
    int ai = bootsSymDecrypt(&array[i], key);
    int_answer |= (ai << i);
  }
  return int_answer;
}

LweSample *encode_n(int n, const TFheGateBootstrappingCloudKeySet *bk) {
  LweSample *p = new_gate_bootstrapping_ciphertext_array(NB_VALUES, bk->params);
  for (int i = 0; i < NB_VALUES; ++i) {
    bootsCONSTANT(&p[i], (n >> i) & 1, bk);
  }
  return p;
}

void client() {
  auto t0 = Time::now();
  //generate a keyset
  const int minimum_lambda = 100;
  TFheGateBootstrappingParameterSet *params = new_default_gate_bootstrapping_parameters(minimum_lambda);

  //generate a random key
  uint32_t seed[] = {314, 1592, 657};
  tfhe_random_generator_setSeed(seed, 3);
  TFheGateBootstrappingSecretKeySet *key = new_random_gate_bootstrapping_secret_keyset(params);

  //export the secret key to file for later use
  FILE *secret_key = fopen("secret.key", "wb");
  export_tfheGateBootstrappingSecretKeySet_toFile(secret_key, key);
  fclose(secret_key);

  //export the cloud key to a file (for the cloud)
  FILE *cloud_key = fopen("cloud.key", "wb");
  export_tfheGateBootstrappingCloudKeySet_toFile(cloud_key, &key->cloud);
  fclose(cloud_key);

  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  auto t2 = Time::now();

  /// Keystream
  std::vector<int> KS = {241, 210, 225, 219, 92, 43, 197};

  // encrypt the KS
  LweSample *ks[7];
  for (int i = 0; i < 7; ++i) {
    ks[i] = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
    for (int j = 0; j < NB_VALUES; ++j) {
      bootsSymEncrypt(&ks[i][j], (KS[i] >> j) & 1, key);
    }
  }

  //export the ciphertexts to a file (for the cloud)
  FILE *cloud_data = fopen("cloud.data", "wb");
  for (auto &k : ks) {
    for (int j = 0; j < NB_VALUES; ++j) {
      export_gate_bootstrapping_ciphertext_toFile(cloud_data, &k[j], params);
    }
  }
  fclose(cloud_data);

  //clean up all pointers
  for (auto &k : ks) {
    delete_gate_bootstrapping_ciphertext_array(NB_VALUES, k);
  }
#ifndef DEBUG
  delete_gate_bootstrapping_secret_keyset(key);
  delete_gate_bootstrapping_parameters(params);
#else
  SECRET_KEY = key;
#endif

  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

}
/// Simple ripple carry adder
/// \param s        [out]      result
/// \param carry    [in,out]
/// \param a        [in]       lhs (must be array of size nb_bits)
/// \param b        [in]       rhs (must be array of size nb_bits)
/// \param nb_bits  [in]       size of lhs and rhs
/// \param bk       [in]
void ripple_carry_adder(LweSample *s,
                        LweSample *carry,
                        const LweSample *a,
                        const LweSample *b,
                        const int nb_bits,
                        const TFheGateBootstrappingCloudKeySet *bk) {
#ifdef DEBUG
  std::cout << "adding " << decrypt_array(a, nb_bits, SECRET_KEY) << " + " << decrypt_array(b, nb_bits, SECRET_KEY)
            << " with carry in " << bootsSymDecrypt(carry, SECRET_KEY) << std::endl;
#endif

  // Create temp ctxt's
  LweSample *n1 = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *n2 = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *n1_AND_n2 = new_gate_bootstrapping_ciphertext(bk->params);

  for (int i = 0; i < nb_bits; i++) {
    bootsXOR(n1, carry, &a[i], bk);
    bootsXOR(n2, carry, &b[i], bk);
    bootsXOR(&s[i], n1, &b[i], bk);
    xor_gates += 3;
    if (i < nb_bits - 1) {
      bootsAND(n1_AND_n2, n1, n2, bk);
      ++and_gates;
      bootsXOR(carry, n1_AND_n2, carry, bk);
      ++xor_gates;
    }
  }
#ifdef DEBUG
  std::cout << "result: " << decrypt_array(s, nb_bits, SECRET_KEY)
            << " carry out: " << bootsSymDecrypt(carry, SECRET_KEY) << std::endl;
#endif

  // Clean up  temp ctxt's
  delete_gate_bootstrapping_ciphertext(n1);
  delete_gate_bootstrapping_ciphertext(n2);
  delete_gate_bootstrapping_ciphertext(n1_AND_n2);
}

// this function compares two multibit words, and puts (a<=b) into result
void less(LweSample *result,
          const LweSample *a,
          const LweSample *b,
          const int nb_bits,
          const TFheGateBootstrappingCloudKeySet *bk) {
#ifdef DEBUG
  std::cout << "comparing " << decrypt_array(a, nb_bits, SECRET_KEY) << " < "
            << decrypt_array(b, nb_bits, SECRET_KEY)
            << std::endl;
#endif
    //Using adder circuit to perform comparison
    // initialize the carry_in to 1
    bootsCONSTANT(result, 1, bk);
    LweSample *notB = new_gate_bootstrapping_ciphertext_array(nb_bits, bk->params);
    for (int i = 0; i < nb_bits; ++i) {
      bootsNOT(&notB[i], &b[i], bk);
    }
    LweSample *ignoredResult = new_gate_bootstrapping_ciphertext_array(nb_bits, bk->params);
    ripple_carry_adder(ignoredResult, result, a, notB, nb_bits, bk);

#ifdef DEBUG
  std::cout << "result: " << bootsSymDecrypt(result, SECRET_KEY) << std::endl;
#endif

}

void cloud() {
  auto t4 = Time::now();

  //reads the cloud key from file
  FILE *cloud_key = fopen("cloud.key", "rb");
  TFheGateBootstrappingCloudKeySet *bk = new_tfheGateBootstrappingCloudKeySet_fromFile(cloud_key);
  fclose(cloud_key);

  //if necessary, the params are inside the key
  const TFheGateBootstrappingParameterSet *params = bk->params;

  // age, hdl, height, weight, physical_act and drinking are actually
  // not encrypted under FHE, but instead under the KS
  /// This is provided here only for simplicity, in a real deployment the server
  /// would of course only see (<value> ^ KS[i])
  std::vector<int> KS = {241, 210, 225, 219, 92, 43, 197};
  LweSample *flags = new_gate_bootstrapping_ciphertext_array(NB_FLAGS, params);
  LweSample *age = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  LweSample *hdl = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  LweSample *height = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  LweSample *weight = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  LweSample *physical_cat = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  LweSample *drinking = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  for (int i = 0; i < NB_FLAGS; i++) {
    bootsCONSTANT(&flags[i], ((15 ^ KS[0]) >> i) & 1, bk);
  }
  for (int i = 0; i < NB_VALUES; i++) {
    bootsCONSTANT(&age[i], ((55 ^ KS[1]) >> i) & 1, bk);
    bootsCONSTANT(&hdl[i], ((50 ^ KS[2]) >> i) & 1, bk);
    bootsCONSTANT(&height[i], ((80 ^ KS[3]) >> i) & 1, bk);
    bootsCONSTANT(&weight[i], ((80 ^ KS[4]) >> i) & 1, bk);
    bootsCONSTANT(&physical_cat[i], ((45 ^ KS[5]) >> i) & 1, bk);
    bootsCONSTANT(&drinking[i], ((4 ^ KS[6]) >> i) & 1, bk);
  }

  LweSample *ks[7];
  for (auto &k : ks) {
    k = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  }

  //reads the encrypted KS from the cloud file
  FILE *cloud_data = fopen("cloud.data", "rb");
  for (auto &k : ks) {
    for (int i = 0; i < NB_VALUES; ++i) {
      import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &k[i], params);
    }
  }
  fclose(cloud_data);

  // Apply the Keystream
  for (int i = 0; i < NB_FLAGS; ++i) {
    bootsXOR(&flags[i], &flags[i], &ks[0][i], bk);
    ++xor_gates;
  }
  for (int i = 0; i < NB_VALUES; ++i) {
    bootsXOR(&age[i], &age[i], &ks[1][i], bk);
    bootsXOR(&hdl[i], &hdl[i], &ks[2][i], bk);
    bootsXOR(&height[i], &height[i], &ks[3][i], bk);
    bootsXOR(&weight[i], &weight[i], &ks[4][i], bk);
    bootsXOR(&physical_cat[i], &physical_cat[i], &ks[5][i], bk);
    bootsXOR(&drinking[i], &drinking[i], &ks[6][i], bk);
    xor_gates += 6;
  }

#ifdef DEBUG
  std::cout << "flags: " << decrypt_array(flags, NB_FLAGS, SECRET_KEY) << std::endl;
  std::cout << "age: " << decrypt_array(age, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "hdl: " << decrypt_array(hdl, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "height: " << decrypt_array(height, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "weight: " << decrypt_array(weight, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "physical_cat: " << decrypt_array(physical_cat, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "drinking: " << decrypt_array(drinking, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // Compute first complex condition: flags(sex_field) && (50 < age) [should be true]
  LweSample *fifty = encode_n(50, bk);
  LweSample *age_gt_50 = new_gate_bootstrapping_ciphertext(params);
  less(age_gt_50, fifty, age, NB_VALUES, bk);
  LweSample *factor_1 = encode_n(0, bk);
  bootsAND(&factor_1[0], &flags[SEX_FIELD], age_gt_50, bk);
  ++and_gates;
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, fifty);
  delete_gate_bootstrapping_ciphertext(age_gt_50);
#ifdef DEBUG
  std::cout << "factor_1: " << decrypt_array(factor_1, NB_VALUES, SECRET_KEY) << std::endl;
#endif


  // Compute second complex condition: !flags(sex_field) && (60 < age) [should be false]
  LweSample *sixty = encode_n(60, bk);
  LweSample *age_gt_60 = new_gate_bootstrapping_ciphertext(params);
  less(age_gt_60, sixty, age, NB_VALUES, bk);
  LweSample *not_sex_field = new_gate_bootstrapping_ciphertext(params);
  bootsNOT(not_sex_field, &flags[SEX_FIELD], bk);
  LweSample *factor_2 = encode_n(0, bk);
  bootsAND(&factor_2[0], not_sex_field, age_gt_60, bk);
  ++and_gates;
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, sixty);
  delete_gate_bootstrapping_ciphertext(age_gt_60);
  // not sex field is used again later, so not deleted here
#ifdef DEBUG
  std::cout << "factor_2: " << decrypt_array(factor_2, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // factors 3,4,5,6 are just flags [3-5 should be true, 6 false]
  LweSample *factor_3 = encode_n(0, bk);
  bootsCOPY(&factor_3[0], &flags[ANTECEDENT_FIELD], bk);
  LweSample *factor_4 = encode_n(0, bk);
  bootsCOPY(&factor_4[0], &flags[SMOKER_FIELD], bk);
  LweSample *factor_5 = encode_n(0, bk);
  bootsCOPY(&factor_5[0], &flags[DIABETES_FIELD], bk);
  LweSample *factor_6 = encode_n(0, bk);
  bootsCOPY(&factor_6[0], &flags[PRESSURE_FIELD], bk);
#ifdef DEBUG
  std::cout << "factor_3: " << decrypt_array(factor_3, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "factor_4: " << decrypt_array(factor_4, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "factor_5: " << decrypt_array(factor_5, NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "factor_6: " << decrypt_array(factor_6, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // compute 7th factor: hdl < 40 [Should be false]
  LweSample *forty = encode_n(40, bk);
  LweSample *factor_7 = encode_n(0, bk);
  less(&factor_7[0], hdl, forty, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, forty);
#ifdef DEBUG
  std::cout << "factor_7: " << decrypt_array(factor_7, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // compute 8-th factor: weight - 10 > height <=> height + 10 < weight [should be false]
  LweSample *ten = encode_n(10, bk);
  LweSample *height_plus_10 = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  LweSample *carry = new_gate_bootstrapping_ciphertext(params);
  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(height_plus_10, carry, height, ten, NB_VALUES, bk);
  LweSample *factor_8 = encode_n(0, bk);
  less(&factor_8[0], height_plus_10, weight, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, ten);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, height_plus_10);
  delete_gate_bootstrapping_ciphertext(carry);
#ifdef DEBUG
  std::cout << "factor_8: " << decrypt_array(factor_8, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // Compute 9th factor: physical_act < 30 [should be false]
  LweSample *thirty = encode_n(30, bk);
  LweSample *factor_9 = encode_n(0, bk);
  less(&factor_9[0], physical_cat, thirty, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, thirty);
#ifdef DEBUG
  std::cout << "factor_9: " << decrypt_array(factor_9, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // Compute 10th factor: sex && (drinking > 3) [should be true]
  LweSample *three = encode_n(3, bk);
  LweSample *drinking_gt_3 = new_gate_bootstrapping_ciphertext(params);
  less(drinking_gt_3, three, drinking, NB_VALUES, bk);
  LweSample *factor_10 = encode_n(0, bk);
  bootsAND(&factor_10[0], &flags[SEX_FIELD], drinking_gt_3, bk);
  ++and_gates;
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, three);
  delete_gate_bootstrapping_ciphertext(drinking_gt_3);
#ifdef DEBUG
  std::cout << "factor_10: " << decrypt_array(factor_10, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // Compute 11th factor: !sex && (drinking > 2) [should be false]
  LweSample *two = encode_n(2, bk);
  LweSample *drinking_gt_2 = new_gate_bootstrapping_ciphertext(params);
  less(drinking_gt_2, two, drinking, NB_VALUES, bk);
  LweSample *factor_11 = encode_n(0, bk);
  bootsAND(&factor_11[0], not_sex_field, drinking_gt_2, bk);
  ++and_gates;
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, two);
  delete_gate_bootstrapping_ciphertext(drinking_gt_2);
  delete_gate_bootstrapping_ciphertext(not_sex_field);
  //not_sex_field no longer needed
#ifdef DEBUG
  std::cout << "factor_11: " << decrypt_array(factor_11, NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // Start adding up all the factors:
  carry = new_gate_bootstrapping_ciphertext(params);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_2, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_2);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_3, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_3);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_4, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_4);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_5, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_5);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_6, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_6);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_7, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_7);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_8, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_8);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_9, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_9);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_10, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_10);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(factor_1, carry, factor_1, factor_11, NB_VALUES, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_11);

  //export the resulting ciphertext to a file (for the cloud)
  FILE *answer_data = fopen("answer.data", "wb");
  for (int i = 0; i < NB_VALUES; i++) {
    export_gate_bootstrapping_ciphertext_toFile(answer_data, &factor_1[i], params);
  }
  fclose(answer_data);

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(NB_FLAGS, flags);
  for (auto &v : {age, hdl, height, weight, physical_cat, drinking}) {
    delete_gate_bootstrapping_ciphertext_array(NB_VALUES, v);
  }
  delete_gate_bootstrapping_cloud_keyset(bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, factor_1);

  auto t5 = Time::now();
  log_time(ss_time, t4, t5, false);
}

void verify() {

  auto t6 = Time::now();

  //reads the secret key from file
  FILE *secret_key = fopen("secret.key", "rb");
  TFheGateBootstrappingSecretKeySet *key = new_tfheGateBootstrappingSecretKeySet_fromFile(secret_key);
  fclose(secret_key);

  //if necessary, the params are inside the key
  const TFheGateBootstrappingParameterSet *params = key->params;

  //create the ciphertext for the result
  LweSample *answer = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);

  //import the  ciphertexts from the answer file
  FILE *answer_data = fopen("answer.data", "rb");
  for (int i = 0; i < NB_VALUES; i++)
    import_gate_bootstrapping_ciphertext_fromFile(answer_data, &answer[i], params);
  fclose(answer_data);

  //decrypt and rebuild the plaintext answer
  uint8_t int_answer = decrypt_array(answer, NB_VALUES, key);

  printf("And the result is: %d\n", int_answer);
  printf("I hope you remember what was the question!\n");

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, answer);
  delete_gate_bootstrapping_secret_keyset(key);

  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);
}
