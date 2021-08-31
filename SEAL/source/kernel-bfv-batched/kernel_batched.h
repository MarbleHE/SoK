#ifndef LAPLACIAN_SHARPENING_BATCHED_H_
#define LAPLACIAN_SHARPENING_BATCHED_H_
#endif

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <seal/seal.h>

// This method is taken from SEAL's examples helper methods, see
// https://github.com/microsoft/SEAL/blob/master/native/examples/examples.h.
inline void write_parameters_to_file(std::shared_ptr<seal::SEALContext> context,
                                     std::string filename) {
  std::ofstream outfile(filename);

  // Verify parameters
  if (!context) {
    throw std::invalid_argument("context is not set");
  }
  auto &context_data = *context->key_context_data();

  /*
  Which scheme are we using?
  */
  std::string scheme_name;
  switch (context_data.parms().scheme()) {
    case seal::scheme_type::bfv:
      scheme_name = "BFV";
      break;
    case seal::scheme_type::ckks:
      scheme_name = "CKKS";
      break;
    default:
      throw std::invalid_argument("unsupported scheme");
  }
  outfile << "/" << std::endl;
  outfile << "| Encryption parameters :" << std::endl;
  outfile << "|   scheme: " << scheme_name << std::endl;
  outfile << "|   poly_modulus_degree: "
          << context_data.parms().poly_modulus_degree() << std::endl;

  /*
  Print the size of the true (product) coefficient modulus.
  */
  outfile << "|   coeff_modulus size: ";
  outfile << context_data.total_coeff_modulus_bit_count() << " (";
  auto coeff_modulus = context_data.parms().coeff_modulus();
  std::size_t coeff_modulus_size = coeff_modulus.size();
  for (std::size_t i = 0; i < coeff_modulus_size - 1; i++) {
    outfile << coeff_modulus[i].bit_count() << " + ";
  }
  outfile << coeff_modulus.back().bit_count();
  outfile << ") bits" << std::endl;

  /*
  For the BFV scheme print the plain_modulus parameter.
  */
  if (context_data.parms().scheme() == seal::scheme_type::bfv) {
    outfile << "|   plain_modulus: "
            << context_data.parms().plain_modulus().value() << std::endl;
  }

  outfile << "\\" << std::endl;

  outfile.close();
}

typedef std::vector<std::vector<int>> VecInt2D;

typedef std::vector<std::vector<int>> VecInt2D;
typedef std::chrono::high_resolution_clock Time;
typedef decltype(std::chrono::high_resolution_clock::now()) Timepoint;
typedef long long Duration;
typedef std::chrono::milliseconds ms;

#define DEFAULT_NUM_SLOTS 16'384
#define PRINT_LIMIT 70

class Evaluation {
 private:
  // e.g., image_size=8 corresponds to a 8x8 pixels image
  int image_size;

  const std::vector<int> weight_matrix = {1, 1, 1, 1, -8, 1, 1, 1, 1};

  std::shared_ptr<seal::SEALContext> context;

  std::unique_ptr<seal::SecretKey> secret_key;
  std::unique_ptr<seal::PublicKey> public_key = std::make_unique<seal::PublicKey>();
  std::unique_ptr<seal::GaloisKeys> galois_keys = std::make_unique<seal::GaloisKeys>();
  std::unique_ptr<seal::RelinKeys> relin_keys = std::make_unique<seal::RelinKeys>();

  std::unique_ptr<seal::BatchEncoder> encoder;
  std::unique_ptr<seal::Encryptor> encryptor;
  std::unique_ptr<seal::Decryptor> decryptor;
  std::unique_ptr<seal::Evaluator> evaluator;

  std::vector<int64_t> decrypt_and_decode(seal::Ciphertext &ctxt);

  std::vector<int64_t> decode(seal::Plaintext &ptxt);

  std::vector<int64_t> generate_border_mask(bool invert);

 public:
  Evaluation(int image_size);

  std::vector<int64_t> run_kernel(VecInt2D img);

  void check_results(VecInt2D img, std::vector<int64_t> computed_values);

  int main(int argc, char *argv[]);
};
