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

void client();
void cloud();
void verify();

int main() {
  client();
  cloud();
  verify();

  // write ss_time into file
  std::ofstream myfile;
  myfile.open("tfhe_cardio.csv", std::ios_base::app);
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

std::string decrypt_array_bits(const LweSample *array, int nb_size, const TFheGateBootstrappingSecretKeySet *key) {
  std::string answer;
  for (int i = 0; i < nb_size; i++) {
    int ai = bootsSymDecrypt(&array[i], key);
    answer = std::to_string(ai) + answer;
  }
  return answer;
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
    uint8_t lhs_ptxt = 55;
    uint8_t rhs_ptxt = 65;
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

/// This function implements a Sklansky prefix adder.
/// For more information, see Section 1.2.5ff in "Design of Two Different 128-bit Adders" by Vladislav Muravin
/// Available at https://www.semanticscholar.org/paper/e9c6e041a8ae27e17251fedcb95a835ca0ba7bd0
/// \param[out] s a gate_bootstrapping_ciphertext_array of size nb_bits containing the sum of a and b
/// \param[out] c_out a gate_bootstrapping_ciphertext (ptr) containing the carry out of the sum of a and b
/// \param[in] a gate_bootstrapping_ciphertext_array of size nb_bits containing the first summand
/// \param[in] b gate_bootstrapping_ciphertext_array of size nb_bits containing the second summand
/// \param[in] nb_bits size (in bits) of the input and output
/// \param[in] bk (ptr to) the bootstrapping keys
void sklansky_adder(LweSample *s,
                    LweSample *c_out,
                    const LweSample *a,
                    const LweSample *b,
                    const int nb_bits,
                    const TFheGateBootstrappingCloudKeySet *bk) {

  // We define propagate (p_i) and generate (g_i) bits
  // p_i = a[i] XOR b[i] // indicates if this position will propagate a carry if one is generated at i-1
  // g_i = a[i] AND b[i] // indicates if this position generates a new carry

  // We also define P and G (for i>=k<=k)
  // G[i][j] represents a carry being generated somewhere between bits i and j
  // P[i][j] represents if a carry is propagated from bit j to bit i.

  // Create P and G arrays and initialize to zero
  LweSample *P[nb_bits];
  LweSample *G[nb_bits];
  for (int i = 0; i < nb_bits; ++i) {
    P[i] = new_gate_bootstrapping_ciphertext_array(nb_bits, bk->params);
    G[i] = new_gate_bootstrapping_ciphertext_array(nb_bits, bk->params);
    for (int j = 0; j < nb_bits; ++j) {
      bootsCONSTANT(&P[i][j], 0, bk);
      bootsCONSTANT(&G[i][j], 0, bk);
    }
  }

  // pre_computation of p_i and g_i bits
  // These are stored into P[i][i] and G[i][i], respectively
  for (int i = 0; i < nb_bits; ++i) {
    bootsXOR(&P[i][i], &a[i], &b[i], bk);
  }
  for (int i = 0; i < nb_bits - 1; ++i) {
    bootsAND(&G[i][i], &a[i], &b[i], bk);
  }

  // for each level
  int num_steps = (nb_bits > 1) ? (int) std::floor(std::log2((double) nb_bits - 1)) + 1 : 0;
  for (int step = 1; step <= num_steps; ++step) {
    int row = 0;
    int col = 0;
    int k = 0;
    // shift row
    row += (int) std::pow(2, step - 1);
    // do while the size of enter is not reach
    while (row < nb_bits - 1) {
      col = (int) std::floor(row/std::pow(2, step))*(int) std::pow(2, step);
      for (size_t i = 0; i < (int) std::pow(2, step - 1); ++i) {
        //evaluate_G(P, G, row, col, step);
        k = col + (int) std::pow(2, step - 1);
        LweSample *r = new_gate_bootstrapping_ciphertext(bk->params);
        bootsCONSTANT(r, 0, bk);
        bootsAND(r, &P[row][k], &G[k - 1][col], bk);
        bootsXOR(&G[row][col], &G[row][k], r, bk);
        delete_gate_bootstrapping_ciphertext(r);
        if (col!=0) {
          //evaluate_P(P, G, row, col, step);
          k = col + (int) std::pow(2, step - 1);
          bootsAND(&P[row][col], &P[row][k], &P[k - 1][col], bk);
        }
        row += 1;
        if (row==nb_bits - 1) break;
      }
      row += (int) std::pow(2, step - 1);
    }
  }

  // compute results:
  // s[0] = P[0][0]
  bootsCOPY(&s[0], &P[0][0], bk);
  for (int i = 1; i < nb_bits; ++i) {
    bootsXOR(&s[i], &P[i][i], &G[i - 1][0], bk);
  }

  // Compute c_out (carry out)
  bootsXOR(c_out, &G[nb_bits - 1][0], &P[nb_bits - 1][nb_bits - 1], bk);

  // Clean up
  for (int i = 0; i < nb_bits; ++i) {
    delete_gate_bootstrapping_ciphertext_array(nb_bits, G[i]);
    delete_gate_bootstrapping_ciphertext_array(nb_bits, P[i]);
  }

}

///  Compares two numbers of length nb_bits, and puts the bit (a==?b) into result
/// \param[out] result (ptr to) gate_bootstrapping_ciphertext containing (a=?=b)
/// \param[in] a gate_bootstrapping_ciphertext_array of size nb_bits containing the first value
/// \param[in] b gate_bootstrapping_ciphertext_array of size nb_bits containing the second value
/// \param[in] nb_bits size (in bits) of the input
/// \param[in] bk (ptr to) the bootstrapping keys
void equal(LweSample *result,
           const LweSample *a,
           const LweSample *b,
           const int nb_bits,
           const TFheGateBootstrappingCloudKeySet *bk) {

  LweSample *one = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCONSTANT(one, 1, bk);

  LweSample *comp = new_gate_bootstrapping_ciphertext_array(nb_bits, bk->params);
  for (int i = 0; i < nb_bits; ++i) {
    LweSample *tmp = new_gate_bootstrapping_ciphertext(bk->params);
    bootsXOR(tmp, &a[i], &b[i], bk);
    bootsXOR(&comp[i], tmp, one, bk);
    delete_gate_bootstrapping_ciphertext(tmp);
  }
  //"multvec" with log depth
  for (int k = 1; k < nb_bits; k *= 2) {
    for (std::size_t i = 0; i < nb_bits - k; i += 2*k) {
      bootsAND(&comp[i], &comp[i], &comp[i + k], bk);
    }
  }
  bootsCOPY(result, &comp[0], bk);
  delete_gate_bootstrapping_ciphertext_array(nb_bits, comp);
}

///  Compares two numbers of length nb_bits, and puts the bit (a<=?b) into result
/// \param[out] result (ptr to) gate_bootstrapping_ciphertext containing (a<=?b)
/// \param[in] a gate_bootstrapping_ciphertext_array of size nb_bits containing the first value
/// \param[in] b gate_bootstrapping_ciphertext_array of size nb_bits containing the second value
/// \param[in] nb_bits size (in bits) of the input
/// \param[in] bk (ptr to) the bootstrapping keys
void less(LweSample *result,
          const LweSample *a,
          const LweSample *b,
          const int nb_bits,
          const TFheGateBootstrappingCloudKeySet *bk) {
  //std::string indent(BIT_SIZE - nb_bits, '\t');
  //fprintf(stderr, "%s less called with %d bits\n", indent.c_str(), nb_bits);
  if (nb_bits==1) {
    LweSample *lhs_neg = new_gate_bootstrapping_ciphertext(bk->params);
    LweSample *one = new_gate_bootstrapping_ciphertext(bk->params);
    bootsCONSTANT(one, 1, bk);
    // andNY(lhs[0], rhs[0]) = !(lhs[0]) & rhs[0]
    bootsXOR(lhs_neg, &a[0], one, bk);
    bootsAND(result, lhs_neg, &b[0], bk);
    delete_gate_bootstrapping_ciphertext(lhs_neg);
    delete_gate_bootstrapping_ciphertext(one);
    //int result_ptxt = bootsSymDecrypt(result, SECRET_KEY);
    // fprintf(stderr, "%s recursion ended and result is %d\n", indent.c_str(), result_ptxt);
    return;
  }
  const int len2 = (nb_bits >> 1);

  //  lhs_l = a[0, len2) = a, nb_bits
  //  lhs_h = a[len2-1, end) = a+len2-1, nb_bits-len2
  //  rhs_l = b[0, len2) = b, nb_bits
  //  rhs_h = b[len2-1, end) = b+len2-1, nb_bits-len2

  int h_start = len2;
  int h_nb_bits = std::max(nb_bits - h_start,1);
  int l_start = 0;
  int l_nb_bits = nb_bits - h_start;
  LweSample *term1 = new_gate_bootstrapping_ciphertext(bk->params);
  //lower(term1, lhs_h, rhs_h, ..., bk);
  //fprintf(stderr, "%s calling less for h on %d, %d\n", indent.c_str(), h_start, h_nb_bits);
  less(term1, a + h_start, b + h_start, h_nb_bits, bk);
  //int term1_ptxt = bootsSymDecrypt(term1, SECRET_KEY);
  //fprintf(stderr, "%s less h on %d, %d  is %d\n", indent.c_str(), h_start, h_nb_bits, term1_ptxt);

  LweSample *eq = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *ls = new_gate_bootstrapping_ciphertext(bk->params);
  //equal(eq,lhs_h,rhs_h,l,bk);
  equal(eq, a + len2, b + len2, nb_bits - len2, bk);
  //int eq_ptxt = bootsSymDecrypt(eq, SECRET_KEY);
  //fprintf(stderr, "%s equal h on %d, %d  is %d\n", indent.c_str(), len2, nb_bits - len2, eq_ptxt);
  //less(ls,lhs_l,rhs_l,l,bk);
  //fprintf(stderr, "%s calling less for l on %d, %d\n", indent.c_str(), l_start,l_nb_bits);
  less(ls, a + l_start, b + l_start, l_nb_bits, bk);
  int less_ptxt = bootsSymDecrypt(ls, SECRET_KEY);
  //fprintf(stderr, "%s less l on %d, %d is %d\n", indent.c_str(), l_start, l_nb_bits, less_ptxt);

  LweSample *term2 = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(term2, eq, ls, bk);
  bootsXOR(result, term1, term2, bk);

  delete_gate_bootstrapping_ciphertext(term1);
  delete_gate_bootstrapping_ciphertext(term2);
  delete_gate_bootstrapping_ciphertext(eq);
  delete_gate_bootstrapping_ciphertext(ls);

  //int result_ptxt = bootsSymDecrypt(result, SECRET_KEY);
  //fprintf(stderr, "%s function call for %d ended and result is %d\n", indent.c_str(), nb_bits, result_ptxt);
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

  // DEBUG: DECRYPT ALL THE CIPHERTEXTS
  //  fprintf(stderr, "FLAGS:\n");
  //  for (int i = 0; i < NUM_COND; ++i) {
  //    int f = decrypt_array(flags[i], BIT_SIZE, SECRET_KEY);
  //    fprintf(stderr, "%d\n", f);
  //  }
  //  fprintf(stderr, "LHS:\n");
  //  for (int i = 0; i < NUM_COND; ++i) {
  //    int f = decrypt_array(lhs[i], BIT_SIZE, SECRET_KEY);
  //    std::string s = decrypt_array_bits(lhs[i], BIT_SIZE, SECRET_KEY);
  //    fprintf(stderr, "%d (%s)\n", f, s.c_str());
  //  }
  //  fprintf(stderr, "RHS:\n");
  //  for (int i = 0; i < NUM_COND; ++i) {
  //    int f = decrypt_array(rhs[i], BIT_SIZE, SECRET_KEY);
  //    std::string s = decrypt_array_bits(rhs[i], BIT_SIZE, SECRET_KEY);
  //    fprintf(stderr, "%d (%s)\n", f, s.c_str());
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
    sklansky_adder(result, carry, cond_result, result, BIT_SIZE, bk);
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