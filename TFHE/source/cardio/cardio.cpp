#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>
#include <stdio.h>
#include <chrono>
#include <sstream>
#include <fstream>

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
  auto out_filename = std::getenv("OUTPUT_FILENAME");
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();
  return 0;
}

/// Number of conditions to generate
const int NUM_COND = 10;

/// Number of bits in numerical parameters
const int BIT_SIZE = 8;

// DEBUG SECRET_KEY
TFheGateBootstrappingSecretKeySet *SECRET_KEY;

int decrypt_array(const LweSample *array, int nb_size, const TFheGateBootstrappingSecretKeySet *key) {
  uint8_t int_answer = 0;
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
  //generate and encrypt the random flags,lhs and rhs
  LweSample *flags[NUM_COND];
  LweSample *lhs[NUM_COND];
  LweSample *rhs[NUM_COND];
  for (int i = 0; i < NUM_COND; ++i) {
    uint8_t flag_ptxt = 1;
    uint8_t lhs_ptxt = 65;
    uint8_t rhs_ptxt = 55;
    flags[i] = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
    lhs[i] = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
    rhs[i] = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
    for (int j = 0; j < BIT_SIZE; j++) {
      bootsSymEncrypt(&flags[i][j], (flag_ptxt >> j) & 1, key);
      bootsSymEncrypt(&lhs[i][j], (lhs_ptxt >> j) & 1, key);
      bootsSymEncrypt(&rhs[i][j], (rhs_ptxt >> j) & 1, key);
    }
  }

  printf("Hi there! Today we will calculate some cardio risk conditions!\n");

//  // DEBUG: DECRYPT ALL THE CIPHERTEXTS
//  printf("FLAGS:\n");
//  for (int i = 0; i < NUM_COND; ++i) {
//    int f = decrypt_array(flags[i], BIT_SIZE, SECRET_KEY);
//    printf("%d\n", f);
//  }
//  printf("LHS:\n");
//  for (int i = 0; i < NUM_COND; ++i) {
//    int f = decrypt_array(lhs[i], BIT_SIZE, SECRET_KEY);
//    printf("%d\n", f);
//  }
//  printf("RHS:\n");
//  for (int i = 0; i < NUM_COND; ++i) {
//    int f = decrypt_array(rhs[i], BIT_SIZE, SECRET_KEY);
//    printf("%d\n", f);
//  }

  //export the ciphertexts to a file (for the cloud)
  FILE *cloud_data = fopen("cloud.data", "wb");
  for (int i = 0; i < NUM_COND; ++i) {
    for (int j = 0; j < BIT_SIZE; j++)
      export_gate_bootstrapping_ciphertext_toFile(cloud_data, &flags[i][j], params);
  }
  for (int i = 0; i < NUM_COND; ++i) {
    for (int j = 0; j < BIT_SIZE; j++)
      export_gate_bootstrapping_ciphertext_toFile(cloud_data, &lhs[i][j], params);
  }
  for (int i = 0; i < NUM_COND; ++i) {
    for (int j = 0; j < BIT_SIZE; j++)
      export_gate_bootstrapping_ciphertext_toFile(cloud_data, &rhs[i][j], params);
  }

  fclose(cloud_data);

  //clean up all pointers
  for (int i = 0; i < NUM_COND; ++i) {
    delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, flags[i]);
    delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, lhs[i]);
    delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, rhs[i]);
  }
  //delete_gate_bootstrapping_secret_keyset(key);
  //delete_gate_bootstrapping_parameters(params);

  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

}

// elementary full comparator gate that is used to compare the i-th bit:
//   input: ai and bi the i-th bit of a and b
//          lsb_carry: the result of the comparison on the lowest bits
//   algo: if (a==b) return lsb_carry else return b
void compare_bit(LweSample *result,
                 const LweSample *a,
                 const LweSample *b,
                 const LweSample *lsb_carry,
                 LweSample *tmp,
                 const TFheGateBootstrappingCloudKeySet *bk) {
  bootsXNOR(tmp, a, b, bk);
  bootsMUX(result, tmp, lsb_carry, a, bk);
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
  for (int i = 0; i < nb_bits; i++) {
    full_adder(&s[i], c_out, &a[i], &b[i], c_out, bk);
  }
}

// this function compares two multibit words, and puts (a<=b) into result
void less(LweSample *result,
          const LweSample *a,
          const LweSample *b,
          const int nb_bits,
          const TFheGateBootstrappingCloudKeySet *bk) {
  LweSample *tmp = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *carry_in = new_gate_bootstrapping_ciphertext(bk->params);
  //initialize the tmp to 0
  bootsCONSTANT(tmp, 0, bk);
  bootsCONSTANT(carry_in, 0, bk);
  //run the elementary comparator gate n times
  for (int i = 0; i < nb_bits; i++) {
    compare_bit(result, &a[i], &b[i], carry_in, tmp, bk);
    bootsCOPY(carry_in, result, bk);
  }

  delete_gate_bootstrapping_ciphertext(tmp);
  delete_gate_bootstrapping_ciphertext(carry_in);
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
  LweSample *flags[NUM_COND];
  LweSample *lhs[NUM_COND];
  LweSample *rhs[NUM_COND];
  for (int i = 0; i < NUM_COND; ++i) {
    flags[i] = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
    lhs[i] = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
    rhs[i] = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
  }

  //reads the ciphertexts from the cloud file
  FILE *cloud_data = fopen("cloud.data", "rb");
  for (auto &flag : flags)
    for (int j = 0; j < BIT_SIZE; j++)
      import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &flag[j], params);
  for (auto &lh : lhs)
    for (int j = 0; j < BIT_SIZE; j++)
      import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &lh[j], params);
  for (auto &rh : rhs)
    for (int j = 0; j < BIT_SIZE; j++)
      import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &rh[j], params);
  fclose(cloud_data);

//  // DEBUG: DECRYPT ALL THE CIPHERTEXTS
//  printf("FLAGS:\n");
//  for (int i = 0; i < NUM_COND; ++i) {
//    int f = decrypt_array(flags[i], BIT_SIZE, SECRET_KEY);
//    printf("%d\n", f);
//  }
//  printf("LHS:\n");
//  for (int i = 0; i < NUM_COND; ++i) {
//    int f = decrypt_array(lhs[i], BIT_SIZE, SECRET_KEY);
//    printf("%d\n", f);
//  }
//  printf("RHS:\n");
//  for (int i = 0; i < NUM_COND; ++i) {
//    int f = decrypt_array(rhs[i], BIT_SIZE, SECRET_KEY);
//    printf("%d\n", f);
//  }

  // Calculate each condition
  LweSample *cond_results[NUM_COND];
  for (int i = 0; i < NUM_COND; ++i) {
    cond_results[i] = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
    // Set cond_result to zero
    for (int j = 0; j < BIT_SIZE; ++j) {
      bootsCONSTANT(&cond_results[i][j], 0, bk);
    }
    LweSample *tmp = new_gate_bootstrapping_ciphertext(params);
    bootsCONSTANT(tmp, 0, bk);
    // Compute lhs < rhs
    less(tmp, lhs[i], rhs[i], BIT_SIZE, bk);
//    int less_ptxt = bootsSymDecrypt(tmp,SECRET_KEY);
//    printf("Comparison result is %d\n", less_ptxt);

    bootsAND(&cond_results[i][0], tmp, &flags[i][0], bk);

//    int and_ptxt = decrypt_array(cond_results[i], BIT_SIZE, SECRET_KEY);
//    printf("And result is %d\n", and_ptxt);
    // clean up tmp
    delete_gate_bootstrapping_ciphertext(tmp);
  }


//  // DEBUG: DECRYPT ALL THE CONDITIONS
//  printf("COND_RESULTS:\n");
//  for (int i = 0; i < NUM_COND; ++i) {
//    int f = decrypt_array(cond_results[i], BIT_SIZE, SECRET_KEY);
//    printf("%d\n", f);
//  }

  // Add all conditions up to the score
  LweSample *result = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);
  LweSample *carry = new_gate_bootstrapping_ciphertext(params);
  // initialize the sum to 0
  for (int i = 0; i < BIT_SIZE; ++i) {
    bootsCONSTANT(&result[i], 0, bk);
  }
  for (auto &cond_result : cond_results) {
    //initialize the carry to 0
    bootsCONSTANT(carry, 0, bk);
    ripple_carry_adder(result, carry, cond_result, result, BIT_SIZE, bk);
  }
  delete_gate_bootstrapping_ciphertext(carry);

  //export the resulting ciphertext to a file (for the cloud)
  FILE *answer_data = fopen("answer.data", "wb");
  for (int i = 0; i < BIT_SIZE; i++) export_gate_bootstrapping_ciphertext_toFile(answer_data, &result[i], params);
  fclose(answer_data);

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, result);
  for (int i = 0; i < NUM_COND; ++i) {
    delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, flags[i]);
    delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, lhs[i]);
    delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, rhs[i]);
  }
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

  //create the ciphertext for the result
  LweSample *answer = new_gate_bootstrapping_ciphertext_array(BIT_SIZE, params);

  //import the  ciphertexts from the answer file
  FILE *answer_data = fopen("answer.data", "rb");
  for (int i = 0; i < BIT_SIZE; i++)
    import_gate_bootstrapping_ciphertext_fromFile(answer_data, &answer[i], params);
  fclose(answer_data);

  //decrypt and rebuild the plaintext answer
  uint8_t int_answer = decrypt_array(answer, BIT_SIZE, key);

  printf("And the result is: %d\n", int_answer);
  printf("I hope you remember what was the question!\n");

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(BIT_SIZE, answer);
  delete_gate_bootstrapping_secret_keyset(key);

  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);
}
