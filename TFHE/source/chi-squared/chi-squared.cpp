#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>
#include <stdio.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>
#include <assert.h>

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

void client();
void cloud();
void verify();

int main() {
  client();
  cloud();
  verify();

  // write ss_time into file
  std::ofstream myfile;
  const char *out_filename = std::getenv("OUTPUT_FILENAME");
  if (!out_filename) out_filename = "tfhe_chi-squared.csv";
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();
  return 0;
}

/// Number of bits in numerical parameters
const int BIT_SIZE = 8;

// DEBUG SECRET_KEY
TFheGateBootstrappingSecretKeySet *SECRET_KEY;

int decrypt_array(const LweSample *array, int nb_size, const TFheGateBootstrappingSecretKeySet *key) {
  uint32_t int_answer = 0;
  for (int i = 0; i < nb_size; i++) {
    int ai = bootsSymDecrypt(&array[i], key);
    int_answer |= (ai << i);
  }
  return int_answer;
}

void client() {
  auto t0 = Time::now();
  //generate a keyset
  const int minimum_lambda = 110;
  TFheGateBootstrappingParameterSet *params = new_default_gate_bootstrapping_parameters(minimum_lambda);

  //generate a random key
  uint32_t seed[] = {314, 1592, 657};
  tfhe_random_generator_setSeed(seed, 3);
  TFheGateBootstrappingSecretKeySet *key = new_random_gate_bootstrapping_secret_keyset(params);
  SECRET_KEY = key;

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
  //generate and encrypt the three input values
  LweSample *n0 = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
  LweSample *n1 = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
  LweSample *n2 = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
  uint8_t n0_ptxt = 10;
  uint8_t n1_ptxt = 20;
  uint8_t n2_ptxt = 30;
  for (int i = 0; i < BIT_SIZE; i++) {
    bootsSymEncrypt(&n0[i], (n0_ptxt >> i) & 1, key);
    bootsSymEncrypt(&n1[i], (n1_ptxt >> i) & 1, key);
    bootsSymEncrypt(&n2[i], (n2_ptxt >> i) & 1, key);

  }

  printf("Hi there! Today we will calculate a chi-squared test !\n");

  //export the ciphertexts to a file (for the cloud)
  FILE *cloud_data = fopen("cloud.data", "wb");

  for (int i = 0; i < BIT_SIZE; i++)
    export_gate_bootstrapping_ciphertext_toFile(cloud_data, &n0[i], params);

  for (int i = 0; i < BIT_SIZE; i++)
    export_gate_bootstrapping_ciphertext_toFile(cloud_data, &n1[i], params);

  for (int i = 0; i < BIT_SIZE; i++)
    export_gate_bootstrapping_ciphertext_toFile(cloud_data, &n2[i], params);

  fclose(cloud_data);

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, n0);
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, n1);
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, n2);
  //delete_gate_bootstrapping_secret_keyset(key);
  //delete_gate_bootstrapping_parameters(params);

  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

}

// A (very inefficiently formulated) full adder
void full_adder(LweSample *s,
                LweSample *c_out,
                const LweSample *a,
                const LweSample *b,
                const LweSample *c_in,
                const TFheGateBootstrappingCloudKeySet *bk) {
  LweSample *tmp = new_gate_bootstrapping_ciphertext(bk->params);
  bootsXOR(tmp, a, b, bk); // tmp = a XOR b
  bootsXOR(s, tmp, c_in, bk); // s = (a XOR b) XOR c_in

  LweSample *tmp2 = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *tmp3 = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(tmp2, c_in, tmp, bk); // tmp2 = c_in AND (a XOR b)
  bootsAND(tmp3, a, b, bk); // tmp3 = a AND b
  bootsOR(c_out, tmp2, tmp3, bk); // c_out = (a AND b) OR (c_in AND (a XOR b))

  delete_gate_bootstrapping_ciphertext(tmp);
  delete_gate_bootstrapping_ciphertext(tmp2);
  delete_gate_bootstrapping_ciphertext(tmp3);

}

void ripple_carry_adder(LweSample *s,
                        LweSample *c_out,
                        const LweSample *a,
                        const LweSample *b,
                        const int nb_bits,
                        const TFheGateBootstrappingCloudKeySet *bk) {
  //run the elementary comparator gate n times
  LweSample *c_in = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCONSTANT(c_in, 0, bk);

  for (int i = 0; i < nb_bits; i++) {
//    int ptxt_cin = decrypt_array(c_in,1,SECRET_KEY);
//    int ptxt_a = decrypt_array(&a[i],1,SECRET_KEY);
//    int ptxt_b = decrypt_array(&b[i],1,SECRET_KEY);
    full_adder(&s[i], c_out, &a[i], &b[i], c_in, bk);
//    int ptxt_s = decrypt_array(&s[i],1,SECRET_KEY);
//    int ptxt_cout = decrypt_array(c_out,1,SECRET_KEY);
    bootsCOPY(c_in, c_out, bk);

  }
}

// Computes
void simple_multiplier(LweSample *result,
                       const LweSample *a,
                       const LweSample *b,
                       const int nb_bits, /* input length */
                       const TFheGateBootstrappingCloudKeySet *bk) {
  // Build a large array for all the intermediate results
  LweSample *intermediates[nb_bits];
  for (int i = 0; i < nb_bits; ++i) {
    intermediates[i] = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
    for (int j = 0; j < 2*nb_bits; ++j) {
      bootsCONSTANT(&intermediates[i][j], 0, bk);
    }
  }

  // shift copies of b and AND with a at the same time
  for (int i = 0; i < nb_bits; ++i) {
    // take b, shift it by i, i.e. save to ..[j+i] and AND each bit with a[i] and write into i-th intermediate result
    for (int j = 0; j < nb_bits; ++j) {
      bootsAND(&intermediates[i][j + i], &a[i], &b[j], bk);
    }
  }

//  // DEBUG: output all the intermediates
//  printf("Intermediates:\n");
//  for(int i = 0; i < nb_bits; ++i) {
//    int ptxt = decrypt_array(intermediates[i],2*nb_bits,SECRET_KEY);
//    printf("%d: %d\n", i, ptxt);
//  }


  //TODO: Use 3-for-2 compressor to make this faster
  // go through and add all the intermediates
  LweSample *carry = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *temp = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
  for (int i = 0; i < nb_bits; ++i) {
    bootsCONSTANT(carry, 0, bk);
    for (int j = 0; j < 2*nb_bits; ++j) {
      bootsCOPY(&temp[j], &result[j], bk);
      bootsCONSTANT(&result[j], 0, bk);
    }
//    int result_ptxt_before = decrypt_array(temp, 2*nb_bits, SECRET_KEY);
//    int intermediate_ptxt = decrypt_array(intermediates[i], 2*nb_bits, SECRET_KEY);
    ripple_carry_adder(result, carry, temp, intermediates[i], 2*nb_bits, bk);
//    int result_ptxt_after = decrypt_array(result, 2*nb_bits, SECRET_KEY);
//    printf("Adding %d to %d resulted in %d\n", intermediate_ptxt, result_ptxt_before, result_ptxt_after);
  }
  delete_gate_bootstrapping_ciphertext(carry);
  delete_gate_bootstrapping_ciphertext_array(2*nb_bits, temp);

  for (int i = 0; i < nb_bits; ++i) {
    delete_gate_bootstrapping_ciphertext_array(2*nb_bits, intermediates[i]);
  }
}

void cloud() {
  auto t4 = Time::now();

  //reads the cloud key from file
  FILE *cloud_key = fopen("cloud.key", "rb");
  TFheGateBootstrappingCloudKeySet *bk = new_tfheGateBootstrappingCloudKeySet_fromFile(cloud_key);
  fclose(cloud_key);

  //if necessary, the params are inside the key
  const TFheGateBootstrappingParameterSet *params = bk->params;

  //create the ciphertexts
  LweSample *n0 = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
  LweSample *n1 = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
  LweSample *n2 = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);


  //reads the ciphertexts from the cloud file
  FILE *cloud_data = fopen("cloud.data", "rb");

  for (int j = 0; j < BIT_SIZE; j++)
    import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &n0[j], params);

  for (int j = 0; j < BIT_SIZE; j++)
    import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &n1[j], params);

  for (int j = 0; j < BIT_SIZE; j++)
    import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &n2[j], params);

  // DEBUG: DECRYPT ALL THE CIPHERTEXTS
  int n0_ptxt = decrypt_array(n0, BIT_SIZE, SECRET_KEY);
  printf("n0: %d\n", n0_ptxt);
  int n1_ptxt = decrypt_array(n1, BIT_SIZE, SECRET_KEY);
  printf("n1: %d\n", n1_ptxt);
  int n2_ptxt = decrypt_array(n2, BIT_SIZE, SECRET_KEY);
  printf("n2: %d\n", n2_ptxt);

  /// alpha = (4(n0*n2) - n1*n1)^2
  LweSample *alpha = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&alpha[i], 0, bk);
  }
  /// beta1 = 2*(2n0 + n1)^2
  LweSample *beta1 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&beta1[i], 0, bk);
  }
  /// beta2 = (2n0+n1) * (2n2 + n1)
  LweSample *beta2 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&beta2[i], 0, bk);
  }
  /// beta3 = 2*(2n2 + n1)^2
  LweSample *beta3 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&beta3[i], 0, bk);
  }


  /// term1 = (2n0 + n1) // 2*10 + 20 = 40
  LweSample *term1 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&term1[i], 0, bk);
  }
  // start by copying n0, but right-shifting it (multiplies by two)
  LweSample *n0_twice = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&n0_twice[i], 0, bk);
  }
  for (int i = 0; i < BIT_SIZE; ++i) {
    bootsCOPY(&n0_twice[i + 1], &n0[i], bk);
  }
  // Now add n1
  ripple_carry_adder(term1, &term1[BIT_SIZE + 1], n0_twice, n1, BIT_SIZE, bk);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, n0_twice);

  /// term2 = (2n2 + n1) // 2*30 + 20 = 80
  LweSample *term2 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&term2[i], 0, bk);
  }
  // start by copying n2, but right-shifting it (multiplies by two)
  LweSample *n2_twice = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&n2_twice[i], 0, bk);
  }
  for (int i = 0; i < BIT_SIZE; ++i) {
    bootsCOPY(&n2_twice[i + 1], &n2[i], bk);
  }
  // Now add n1
  ripple_carry_adder(term2, &term2[BIT_SIZE + 1], n2_twice, n1, BIT_SIZE, bk);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, n2_twice);

  // DEBUG: VERIFY TERM RESULTS
  auto term1_ptxt = decrypt_array(term1, 4*BIT_SIZE, SECRET_KEY);
  printf("term1: %d\n", term1_ptxt);
  auto term2_ptxt = decrypt_array(term2, 4*BIT_SIZE, SECRET_KEY);
  printf("term2: %d\n", term2_ptxt);

  // Multiply n0 and n2
  LweSample *n0_n2 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&n0_n2[i], 0, bk);
  }
  simple_multiplier(n0_n2, n0, n2, BIT_SIZE, bk);

  auto n02_n2_ptxt = decrypt_array(n0_n2, 4*BIT_SIZE, SECRET_KEY);
  printf("n0*n2: %d\n", n02_n2_ptxt);

  // shift result by 2
  LweSample *four_n0_n2 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&four_n0_n2[i], 0, bk);
  }
  for (int i = 0; i < 2*BIT_SIZE; ++i) {
    bootsCOPY(&four_n0_n2[i + 2], &n0_n2[i], bk);
  }
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, n0_n2);

  auto four_n02_n2_ptxt = decrypt_array(four_n0_n2, 4*BIT_SIZE, SECRET_KEY);
  printf("4*n0*n2: %d\n", four_n02_n2_ptxt);

  // square n1
  LweSample *n1_squared = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&n1_squared[i], 0, bk);
  }
  simple_multiplier(n1_squared, n1, n1, BIT_SIZE, bk);

  auto n1_squared_ptxt = decrypt_array(n1_squared, 4*BIT_SIZE, SECRET_KEY);
  printf("n1^2: %d\n", n1_squared_ptxt);

  // Alpha:
  // first add (yes, original formula is minus, but runtime is pretty much the same and it's already implemented)
  LweSample *sqrt_alpha = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&sqrt_alpha[i], 0, bk);
  }
  ripple_carry_adder(sqrt_alpha, &sqrt_alpha[2*BIT_SIZE + 1], four_n0_n2, n1_squared, 2*BIT_SIZE, bk);

  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, four_n0_n2);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, n1_squared);

  auto sqrt_alpha_ptxt = decrypt_array(sqrt_alpha, 4*BIT_SIZE, SECRET_KEY);
  printf("sqrt_alpha: %d\n", sqrt_alpha_ptxt);

  // now square
  simple_multiplier(alpha, sqrt_alpha, sqrt_alpha, 2*BIT_SIZE, bk);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, sqrt_alpha);


  // Square term 1
  LweSample *term1_squared = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&term1_squared[i], 0, bk);
  }
  simple_multiplier(term1_squared, term1, term1, BIT_SIZE, bk);

  auto term1_squared_ptxt = decrypt_array(term1_squared, 4*BIT_SIZE, SECRET_KEY);
  printf("sqrt_alpha: %d\n", term1_squared_ptxt);

  // Square term 2
  LweSample *term2_squared = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&term2_squared[i], 0, bk);
  }
  simple_multiplier(term2_squared, term2, term2, BIT_SIZE, bk);

  auto term2_squared_ptxt = decrypt_array(term2_squared, 4*BIT_SIZE, SECRET_KEY);
  printf("sqrt_alpha: %d\n", term2_squared_ptxt);

  // beta 1 is  2*(term1)^2 so we shift by one
  for (int i = 0; i < 2*BIT_SIZE; ++i) {
    bootsCOPY(&beta1[i + 1], &term1_squared[i], bk);
  }

  // beta 2 is term1 * term2
  simple_multiplier(beta2, term1, term2, BIT_SIZE, bk);


  // beta 3 is  2*(term2)^2 so we shift by one
  for (int i = 0; i < 2*BIT_SIZE; ++i) {
    bootsCOPY(&beta3[i + 1], &term2_squared[i], bk);
  }


  //export the resulting ciphertexts to a file (for the cloud)
  FILE *answer_data = fopen("answer.data", "wb");
  for (int i = 0; i < BIT_SIZE; i++) export_gate_bootstrapping_ciphertext_toFile(answer_data, &alpha[i], params);
  for (int i = 0; i < BIT_SIZE; i++) export_gate_bootstrapping_ciphertext_toFile(answer_data, &beta1[i], params);
  for (int i = 0; i < BIT_SIZE; i++) export_gate_bootstrapping_ciphertext_toFile(answer_data, &beta2[i], params);
  for (int i = 0; i < BIT_SIZE; i++) export_gate_bootstrapping_ciphertext_toFile(answer_data, &beta3[i], params);
  fclose(answer_data);

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, term1);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, term2);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, term1_squared);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, term2_squared);
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, n0);
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, n1);
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, n2);

  delete_gate_bootstrapping_cloud_keyset(bk);

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

  //create the ciphertext for the results
  LweSample *alpha = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  LweSample *beta1 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  LweSample *beta2 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  LweSample *beta3 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);

  //import the  ciphertexts from the answer file
  FILE *answer_data = fopen("answer.data", "rb");
  for (int i = 0; i < BIT_SIZE; i++)
    import_gate_bootstrapping_ciphertext_fromFile(answer_data, &alpha[i], params);
  for (int i = 0; i < BIT_SIZE; i++)
    import_gate_bootstrapping_ciphertext_fromFile(answer_data, &beta1[i], params);
  for (int i = 0; i < BIT_SIZE; i++)
    import_gate_bootstrapping_ciphertext_fromFile(answer_data, &beta2[i], params);
  for (int i = 0; i < BIT_SIZE; i++)
    import_gate_bootstrapping_ciphertext_fromFile(answer_data, &beta3[i], params);
  fclose(answer_data);

  //decrypt and rebuild the plaintext answer
  uint32_t int_alpha = decrypt_array(alpha, 4*BIT_SIZE, key);
  uint32_t int_beta1 = decrypt_array(beta1, 4*BIT_SIZE, key);
  uint32_t int_beta2 = decrypt_array(beta2, 4*BIT_SIZE, key);
  uint32_t int_beta3 = decrypt_array(beta3, 4*BIT_SIZE, key);

  printf("And the results are:\nalpha: %d\nbeta1: %d\nbeta2: %d\nbeta3: %d\n",
         int_alpha,
         int_beta1,
         int_beta2,
         int_beta3);
  printf("I hope you remember what was the question!\n");

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, alpha);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, beta1);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, beta2);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, beta3);
  delete_gate_bootstrapping_secret_keyset(key);

  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);
}
