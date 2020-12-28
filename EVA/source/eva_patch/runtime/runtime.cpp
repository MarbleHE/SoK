#include "eva/eva.h"
#include <chrono>
#include <iostream>

using namespace eva;
using namespace std;

typedef chrono::high_resolution_clock Time;
typedef chrono::milliseconds ms;

void log_time(stringstream &ss,
              chrono::time_point<chrono::high_resolution_clock> start,
              chrono::time_point<chrono::high_resolution_clock> end,
              bool last = false) {
  ss << chrono::duration_cast<ms>(end - start).count();
  if (!last) ss << ",";
}

int main(int argc, char *argv[]) {
  string path = "program";
  if (argc) {
    path = argv[1];
  }

  stringstream ss_time;

  cout << "Loading Program" << endl;
  auto kt_program = loadFromFile(path + ".eva");
  auto kt_params = loadFromFile(path + ".evaparams");
  auto kt_signature = loadFromFile(path + ".evasignature");

  auto program = move(get<unique_ptr<Program>>(kt_program));
  auto params = move(get<unique_ptr<CKKSParameters>>(kt_params));
  auto signature = move(get<unique_ptr<CKKSSignature>>(kt_signature));

  cout << "Generating Keys" << endl;
  auto t0 = Time::now();
  auto [public_key, secret_key] = generateKeys(*params);
  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  cout << "Encrypting Inputs" << endl;
  // TODO: Proper inputs from args/cin
  auto t2 = Time::now();
  Valuation inputs = {{"input_0", vector<double>(signature->vecSize, 0.5)}};
  auto encrypted_inputs = public_key->encrypt(inputs, *signature);
  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

  cout << "Executing Program" << endl;
  auto t4 = Time::now();
  auto encrypted_result = public_key->execute(*program, encrypted_inputs);
  auto t5 = Time::now();
  log_time(ss_time, t4, t5, false);

  cout << "Decrypting Result" << endl;
  auto t6 = Time::now();
  auto result = secret_key->decrypt(encrypted_result, *signature);
  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);

  for (double d : result["output"]) {
    cout << "[" << d << "]";
  }
  cout << endl;

  cout << "Verifying Result" << endl;
  auto expected_result = evaluate(*program, inputs);

  for (double d : expected_result["output"]) {
    cout << "[" << d << "]";
  }
  cout << endl;

  double mse = 0;
  for (int i = 0; i < signature->vecSize; ++i) {
    mse += pow(result["output"][i] - expected_result["output"][i], 2);
  }
  mse = mse / signature->vecSize;

  cout << "MSE: " << mse << endl;

  // write ss_time into file
  ofstream myfile;
  string out_filename;
  if (getenv("OUTPUT_FILENAME")) {
    out_filename = getenv("OUTPUT_FILENAME");
  } else {
    out_filename = "local_results.csv";
  }
  myfile.open(out_filename, std::ios_base::app);
  myfile << ss_time.str() << std::endl;
  myfile.close();
}