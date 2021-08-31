#ifndef COMMON_H_
#define COMMON_H_
#endif

#include <seal/seal.h>

#include <fstream>
#include <iostream>

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
