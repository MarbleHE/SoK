#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>
#include <stdio.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <functional>
#include <queue>

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

// Counters for gates
int and_gates = 0;
int xor_gates = 0;

std::stringstream ss_time;

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
  const char *out_filename = std::getenv("OUTPUT_FILENAME");
  if (!out_filename) out_filename = "tfhe_chi-squared.csv";
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();
  return 0;
}

/// Number of bits in numerical parameters
const int BIT_SIZE = 8;

//#define DEBUG

//#define DEBUG_WALLACE

#ifdef DEBUG
// DEBUG SECRET_KEY
TFheGateBootstrappingSecretKeySet *SECRET_KEY;
#endif

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

  printf("Hi there! Today we will calculate a chi-squared-naive test !\n");

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

#ifdef DEBUG
  SECRET_KEY = key;
#else
  delete_gate_bootstrapping_secret_keyset(key);
  delete_gate_bootstrapping_parameters(params);
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
  std::cout << "addition result: " << decrypt_array(s, nb_bits, SECRET_KEY)
            << " carry out: " << bootsSymDecrypt(carry, SECRET_KEY) << std::endl;
#endif

  // Clean up  temp ctxt's
  delete_gate_bootstrapping_ciphertext(n1);
  delete_gate_bootstrapping_ciphertext(n2);
  delete_gate_bootstrapping_ciphertext(n1_AND_n2);
}

/// Wallace multiplier, implementation based on Cingulata's multiplier.cxx
/// \param result
/// \param lhs
/// \param rhs
/// \param nb_bits
/// \param bk
void wallace_multiplier(LweSample *result,
                        const LweSample *lhs,
                        const LweSample *rhs,
                        const int nb_bits, /* input length */
                        const TFheGateBootstrappingCloudKeySet *bk) {
#ifdef DEBUG
  std::cout << "multiplying " << decrypt_array(lhs, nb_bits, SECRET_KEY) << " * "
            << decrypt_array(rhs, nb_bits, SECRET_KEY)
            << std::endl;
#endif
  if (nb_bits==1) {
    bootsAND(&result[0], &lhs[0], &rhs[0], bk);
    ++and_gates;
  } else {
    using T = std::tuple<int, LweSample *>;
    std::priority_queue<T, std::vector<T>, std::function<bool(const T &, const T &)>>
        elems_sorted_by_depth(
        [](const T &a, const T &b) -> bool { return std::get<0>(a) > std::get<0>(b); });

    // shift copies of rhs and AND with lhs at the same time
    for (int i = 0; i < nb_bits; ++i) {
      // take rhs, shift it by i, i.e. save to ..[j+i] and AND each bit with lhs[i]
      // then write into i-th intermediate result
      LweSample *temp = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
      for (int k = 0; k < 2*nb_bits; ++k) {
        bootsCONSTANT(&temp[k], 0, bk); //initialize all the other positions
      }
      for (int j = 0; j < nb_bits; ++j) {
        bootsAND(&temp[j + i], &lhs[i], &rhs[j], bk);
#ifdef  DEBUG_WALLACE
        std::cout << "i: " << i
                  << ", lhs[i]=" << bootsSymDecrypt(&lhs[i], SECRET_KEY)
                  << ", rhs[j]=" << bootsSymDecrypt(&rhs[j], SECRET_KEY)
                  << ", AND=" << bootsSymDecrypt(&temp[j + i], SECRET_KEY)
                  << std::endl;
#endif
      }
#ifdef  DEBUG_WALLACE
      std::cout << "Adding to queue: " << decrypt_array(temp, 2*nb_bits, SECRET_KEY) << std::endl;
#endif
      elems_sorted_by_depth.push(std::forward_as_tuple(1, temp));

    }

    while (elems_sorted_by_depth.size() > 2) {
      int da, db, dc;
      LweSample *a, *b, *c;

      std::tie(da, a) = elems_sorted_by_depth.top();
      elems_sorted_by_depth.pop();
      std::tie(db, b) = elems_sorted_by_depth.top();
      elems_sorted_by_depth.pop();
      std::tie(dc, c) = elems_sorted_by_depth.top();
      elems_sorted_by_depth.pop();

#ifdef  DEBUG_WALLACE
      std::cout << "3-for-2: " << std::endl
                << "a: " << decrypt_array(a, 2*nb_bits, SECRET_KEY) << std::endl
                << "b: " << decrypt_array(b, 2*nb_bits, SECRET_KEY) << std::endl
                << "c: " << decrypt_array(c, 2*nb_bits, SECRET_KEY) << std::endl;
#endif

      // tmp1 = lhs ^ rhs ^ c;
      LweSample *tmp1 = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
      for (int i = 0; i < 2*nb_bits; ++i) {
        bootsXOR(&tmp1[i], &a[i], &b[i], bk);
        ++xor_gates;
        bootsXOR(&tmp1[i], &tmp1[i], &c[i], bk);
        ++xor_gates;
      }
#ifdef  DEBUG_WALLACE
      std::cout << "tmp1: " << decrypt_array(tmp1, 2*nb_bits, SECRET_KEY) << std::endl;
#endif
      // Shift a, b and c 1 to the right
      // Actually, we instead later shift tmp2!
      //
      //      a >>= 1;
      //      b >>= 1;
      //      c >>= 1;

      // tmp2 = ((a ^ c) & (b ^ c)) ^c;
      LweSample *tmp2 = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
      LweSample *a_XOR_c = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
      LweSample *b_XOR_c = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
      LweSample *a_x_c_AND_b_x_c = new_gate_bootstrapping_ciphertext_array(2*nb_bits, bk->params);
      bootsCONSTANT(&tmp2[0], 0, bk); //because we do the shift during the bootsXOR
      for (int i = 0; i < 2*nb_bits; ++i) {
        bootsXOR(&a_XOR_c[i], &a[i], &c[i], bk);
        ++xor_gates;
        bootsXOR(&b_XOR_c[i], &b[i], &c[i], bk);
        ++xor_gates;
        bootsAND(&a_x_c_AND_b_x_c[i], &a_XOR_c[i], &b_XOR_c[i], bk);
        ++and_gates;
        if (i < 2*nb_bits - 1) {
          bootsXOR(&tmp2[i + 1], &a_x_c_AND_b_x_c[i], &c[i], bk);
        }
      }

#ifdef  DEBUG_WALLACE
      std::cout << "tmp2: " << decrypt_array(tmp2, 2*nb_bits, SECRET_KEY) << std::endl << std::endl;
#endif

      delete_gate_bootstrapping_ciphertext_array(2*nb_bits, a_XOR_c);
      delete_gate_bootstrapping_ciphertext_array(2*nb_bits, b_XOR_c);
      delete_gate_bootstrapping_ciphertext_array(2*nb_bits, a_x_c_AND_b_x_c);

      elems_sorted_by_depth.push(std::forward_as_tuple(dc, tmp1));
      elems_sorted_by_depth.push(std::forward_as_tuple(dc + 1, tmp2));
    }

    int da, db;
    LweSample *a, *b;

    std::tie(da, a) = elems_sorted_by_depth.top();
    elems_sorted_by_depth.pop();
    std::tie(db, b) = elems_sorted_by_depth.top();
    elems_sorted_by_depth.pop();

    /// add final two numbers
    LweSample *carry = new_gate_bootstrapping_ciphertext(bk->params);
    bootsCONSTANT(carry, 0, bk);
    ripple_carry_adder(result, carry, a, b, 2*nb_bits, bk);
  }
#ifdef DEBUG
  std::cout << "multiplication result: " << decrypt_array(result, 2*nb_bits, SECRET_KEY) << std::endl;
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

#ifdef DEBUG
  // DECRYPT ALL THE CIPHERTEXTS
  int n0_ptxt = decrypt_array(n0, BIT_SIZE, SECRET_KEY);
  printf("n0: %u\n", n0_ptxt);
  int n1_ptxt = decrypt_array(n1, BIT_SIZE, SECRET_KEY);
  printf("n1: %u\n", n1_ptxt);
  int n2_ptxt = decrypt_array(n2, BIT_SIZE, SECRET_KEY);
  printf("n2: %u\n", n2_ptxt);
#endif

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

#ifdef DEBUG
  // VERIFY TERM RESULTS
  auto term1_ptxt = decrypt_array(term1, 4*BIT_SIZE, SECRET_KEY);
  printf("term1: %u\n", term1_ptxt);
  auto term2_ptxt = decrypt_array(term2, 4*BIT_SIZE, SECRET_KEY);
  printf("term2: %u\n", term2_ptxt);
#endif

  // Multiply n0 and n2
  LweSample *n0_n2 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&n0_n2[i], 0, bk);
  }
  wallace_multiplier(n0_n2, n0, n2, BIT_SIZE, bk);

#ifdef DEBUG
  auto n02_n2_ptxt = decrypt_array(n0_n2, 4*BIT_SIZE, SECRET_KEY);
  printf("n0*n2: %u\n", n02_n2_ptxt);
#endif

  // shift result by 2
  LweSample *four_n0_n2 = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&four_n0_n2[i], 0, bk);
  }
  for (int i = 0; i < 2*BIT_SIZE; ++i) {
    bootsCOPY(&four_n0_n2[i + 2], &n0_n2[i], bk);
  }
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, n0_n2);

#ifdef DEBUG
  auto four_n02_n2_ptxt = decrypt_array(four_n0_n2, 4*BIT_SIZE, SECRET_KEY);
  printf("4*n0*n2: %u\n", four_n02_n2_ptxt);
#endif
  // square n1
  LweSample *n1_squared = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&n1_squared[i], 0, bk);
  }
  wallace_multiplier(n1_squared, n1, n1, BIT_SIZE, bk);

#ifdef DEBUG
  auto n1_squared_ptxt = decrypt_array(n1_squared, 4*BIT_SIZE, SECRET_KEY);
  printf("n1^2: %u\n", n1_squared_ptxt);
#endif

  // Alpha:
  // first add (yes, original formula is minus, but runtime is pretty much the same and it's already implemented)
  LweSample *sqrt_alpha = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&sqrt_alpha[i], 0, bk);
  }
  ripple_carry_adder(sqrt_alpha, &sqrt_alpha[2*BIT_SIZE + 1], four_n0_n2, n1_squared, 2*BIT_SIZE, bk);

  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, four_n0_n2);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, n1_squared);

#ifdef DEBUG
  auto sqrt_alpha_ptxt = decrypt_array(sqrt_alpha, 4*BIT_SIZE, SECRET_KEY);
  printf("sqrt_alpha: %u\n", sqrt_alpha_ptxt);
#endif

  // now square
  wallace_multiplier(alpha, sqrt_alpha, sqrt_alpha, 2*BIT_SIZE, bk);
  delete_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, sqrt_alpha);


  // Square term 1
  LweSample *term1_squared = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&term1_squared[i], 0, bk);
  }
  wallace_multiplier(term1_squared, term1, term1, BIT_SIZE, bk);

#ifdef DEBUG
  auto term1_squared_ptxt = decrypt_array(term1_squared, 4*BIT_SIZE, SECRET_KEY);
  printf("term1_squared: %u\n", term1_squared_ptxt);
#endif

  // Square term 2
  LweSample *term2_squared = new_gate_bootstrapping_ciphertext_array(4*BIT_SIZE, params);
  for (int i = 0; i < 4*BIT_SIZE; ++i) {
    bootsCONSTANT(&term2_squared[i], 0, bk);
  }
  wallace_multiplier(term2_squared, term2, term2, BIT_SIZE, bk);

#ifdef DEBUG
  auto term2_squared_ptxt = decrypt_array(term2_squared, 4*BIT_SIZE, SECRET_KEY);
  printf("term2_squared: %u\n", term2_squared_ptxt);
#endif

  // beta 1 is  2*(term1)^2 so we shift by one
  for (int i = 0; i < 2*BIT_SIZE; ++i) {
    bootsCOPY(&beta1[i + 1], &term1_squared[i], bk);
  }

  // beta 2 is term1 * term2
  wallace_multiplier(beta2, term1, term2, BIT_SIZE, bk);


  // beta 3 is  2*(term2)^2 so we shift by one
  for (int i = 0; i < 2*BIT_SIZE; ++i) {
    bootsCOPY(&beta3[i + 1], &term2_squared[i], bk);
  }


  //export the resulting ciphertexts to lhs file (for the cloud)
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

  printf("And the results are:\nalpha: %u\nbeta1: %u\nbeta2: %u\nbeta3: %u\n",
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
