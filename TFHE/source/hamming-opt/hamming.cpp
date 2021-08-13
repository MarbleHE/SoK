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

// Hamming takes two 8-digit hexadecimal numbers a and b as its arguments,
// and finds the Hamming distance between them.
// i.e., we need to compare 8 pairs of 4-bit numbers and then compute the sum of the comparisons
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

const int NB_VALUES = 4;

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

  /// Vector of 2x8 "hex digits"
  std::vector<int> a = {0, 0, 0, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};

  // encrypt a
  LweSample *as[16];
  for (int i = 0; i < 16; ++i) {
    as[i] = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
    for (int j = 0; j < NB_VALUES; ++j) {
      bootsSymEncrypt(&as[i][j], (a[i] >> j) & 1, key);
    }
  }

  //export the ciphertexts to a file (for the cloud)
  FILE *cloud_data = fopen("cloud.data", "wb");
  for (auto &k : as) {
    for (int j = 0; j < NB_VALUES; ++j) {
      export_gate_bootstrapping_ciphertext_toFile(cloud_data, &k[j], params);
    }
  }
  fclose(cloud_data);

  //clean up all pointers
  for (auto &k : as) {
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

  // Circuit as described in Cingulata's lower.cxx (LowerCompSize::oper)
  LweSample *n1 = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *n2 = new_gate_bootstrapping_ciphertext(bk->params);
  LweSample *n1_AND_n2 = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCONSTANT(result, 0, bk);
  for (int i = 0; i < nb_bits; ++i) {
    bootsXOR(n1, result, &a[i], bk);
    bootsXOR(n2, result, &b[i], bk);
    bootsAND(n1_AND_n2, n1, n2, bk);
    bootsXOR(result, n1_AND_n2, &b[i], bk);
  }

  delete_gate_bootstrapping_ciphertext(n1);
  delete_gate_bootstrapping_ciphertext(n2);
  delete_gate_bootstrapping_ciphertext(n1_AND_n2);

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

  LweSample *as[16];
  for (auto &a : as) {
    a = new_gate_bootstrapping_ciphertext_array(NB_VALUES, params);
  }

  //reads the encrypted KS from the cloud file
  FILE *cloud_data = fopen("cloud.data", "rb");
  for (auto &a : as) {
    for (int i = 0; i < NB_VALUES; ++i) {
      import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &a[i], params);
    }
  }
  fclose(cloud_data);

#ifdef DEBUG
  std::cout << "0: " << decrypt_array(as[0], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[8], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "1: " << decrypt_array(as[1], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[9], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "2: " << decrypt_array(as[2], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[10], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "3: " << decrypt_array(as[3], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[11], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "4: " << decrypt_array(as[4], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[12], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "5: " << decrypt_array(as[5], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[13], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "6: " << decrypt_array(as[6], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[14], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "7: " << decrypt_array(as[7], NB_VALUES, SECRET_KEY) << " =?= "
            << decrypt_array(as[15], NB_VALUES, SECRET_KEY) << std::endl;
#endif

  // Create the 8 comparison results
  LweSample *equals[8];
  for (auto &q : equals) {
    // technically, we only need one bit here, but this makes adding later easier
    q = encode_n(1, bk);
  }

  for (int i = 0; i < 8; ++i) {
    // compare as[i] to as[i+8]
    for (int j = 0; j < NB_VALUES; ++j) {
      auto xor_res = new_gate_bootstrapping_ciphertext(params);
      // x == y <==> !x^y
      bootsXOR(xor_res, &as[i][j], &as[i + 8][j], bk);
//#ifdef DEBUG
//      std::cout << "comparing " << i << " at bit " << j << "(" << bootsSymDecrypt(&as[i][j], SECRET_KEY) << ") with "
//                << i+8 << " at bit " << j << "(" << bootsSymDecrypt(&as[i+8][j], SECRET_KEY) << ") :"
//                << bootsSymDecrypt(xor_res, SECRET_KEY) << std::endl;
//#endif
      bootsNOT(xor_res, xor_res, bk);
      bootsAND(&equals[i][0], &equals[i][0], xor_res, bk);
//#ifdef DEBUG
//      std::cout << "inverted: " << bootsSymDecrypt(xor_res, SECRET_KEY) << std::endl;
//      std::cout << "total result: " << decrypt_array(equals[i], NB_VALUES, SECRET_KEY) << std::endl;
//#endif

    }
    bootsNOT(equals[i], equals[i], bk);
//#ifdef DEBUG
//    std::cout << "inverted total result: " << decrypt_array(equals[i], NB_VALUES, SECRET_KEY) << std::endl;
//#endif

  }

#ifdef DEBUG
  std::cout << "0: " << decrypt_array(equals[0], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "1: " << decrypt_array(equals[1], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "2: " << decrypt_array(equals[2], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "3: " << decrypt_array(equals[3], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "4: " << decrypt_array(equals[4], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "5: " << decrypt_array(equals[5], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "6: " << decrypt_array(equals[6], NB_VALUES, SECRET_KEY) << std::endl;
  std::cout << "7: " << decrypt_array(equals[7], NB_VALUES, SECRET_KEY) << std::endl;
#endif


  // Add up the eight results
  auto carry = new_gate_bootstrapping_ciphertext(params);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(equals[0], carry, equals[0], equals[1], 2, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[1]);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(equals[2], carry, equals[2], equals[3], 2, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[3]);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(equals[4], carry, equals[4], equals[5], 2, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[5]);

  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(equals[6], carry, equals[6], equals[7], 2, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[7]);




  // 1-4
  // Adding 4 bits will never result in a number larger than 3 bits
  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(equals[0], carry, equals[0], equals[2], 3, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[2]);


  // 5-8
  // Adding 4 bits will never result in a number larger than 3 bits
  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(equals[4], carry, equals[4], equals[6], 3, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[6]);

  // 1-8
  // Adding 8 bits will never result in a number larger than 4 bits
  bootsCONSTANT(carry, 0, bk);
  ripple_carry_adder(equals[0], carry, equals[0], equals[4], 4, bk);
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[4]);

#ifdef DEBUG
  std::cout << "Final Result: " << decrypt_array(equals[0], NB_VALUES, SECRET_KEY) << std::endl;
#endif

  //export the resulting ciphertext to a file (for the cloud)
  FILE *answer_data = fopen("answer.data", "wb");
  for (int i = 0; i < NB_VALUES; i++) {
    export_gate_bootstrapping_ciphertext_toFile(answer_data, equals[0], params);
  }
  fclose(answer_data);

  //clean up all pointers
  delete_gate_bootstrapping_ciphertext_array(NB_VALUES, equals[0]);

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
  for (int i = NB_VALUES - 1; i >= 0; i--)
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
