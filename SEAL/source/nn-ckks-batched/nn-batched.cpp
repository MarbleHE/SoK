#include "nn-batched.h"
#include "../common.h"
#include "matrix_vector_crypto.h"

/// Create only the required power-of-two rotations
/// This can save quite a bit, for example for poly_modulus_degree = 16384
/// The default galois keys (with zlib compression) are 247 MB large
/// Whereas with dimension = 256, they are only 152 MB
/// For poly_modulus_degree = 32768, the default keys are 532 MB large
/// while with dimension = 256, they are only 304 MB
std::vector<int> custom_steps(size_t dimension) {
  if (dimension==256) {
    // Slight further optimization: No -128, no -256
    return {1, -1, 2, -2, 4, -4, 8, -8, 16, -16, 32, -32, 64, -64, 128, 256};
  } else {
    std::vector<int> steps{};
    for (int i = 1; i <= dimension; i <<= 1) {
      steps.push_back(i);
      steps.push_back(-i);
    }
    return steps;
  }
}

/*
 * Batched CKKS implementation for nn benchmark.
 */

void NNBatched::setup_context_ckks(std::size_t poly_modulus_degree) {
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
  std::vector<int> steps = custom_steps(784);
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

void NNBatched::internal_print_info(std::string variable_name,
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

seal::Ciphertext NNBatched::encode_and_encrypt(
    std::vector<double> number) {
  seal::Ciphertext encrypted_numbers(context);
  encryptor->encrypt(encode(number), encrypted_numbers);
  return encrypted_numbers;
}

seal::Plaintext NNBatched::encode(std::vector<double> numbers,
                                  seal::parms_id_type parms_id) {

  // encode bit sequence into a seal::Plaintext
  seal::Plaintext encoded_numbers;
  encoder->encode(numbers, parms_id, initial_scale, encoded_numbers);
  return encoded_numbers;
}

seal::Plaintext NNBatched::encode(std::vector<double> numbers) {
  return encode(std::move(numbers), context->first_parms_id());
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

void NNBatched::run_nn() {
  std::stringstream ss_time;

  auto t0 = Time::now();
  // poly_modulus_degree:
  // - must be a power of two
  // - determines the number of ciphertext slots
  // - determines the max. of the sum of coeff_moduli bits
  setup_context_ckks(32768);

  auto t1 = Time::now();
  log_time(ss_time, t0, t1, false);

  // === client-side computation ====================================

  /// vectorized MNIST image (28x28px, 1 channel)
  std::vector<double> image(784, 0);


  // encode and encrypt the input
  auto t2 = Time::now();
  seal::Ciphertext image_ctxt = encode_and_encrypt(image);

  auto t3 = Time::now();
  log_time(ss_time, t2, t3, false);

  // // transmit data to server...

  // // === server-side computation ====================================

  auto t4 = Time::now();

  // Create the Weights and Biases for the first dense layer
  DenseLayer d1(30, 784);



  // First, compute the MVP between d1_weights and the input
  seal::Ciphertext result;
  ptxt_matrix_enc_vector_product_bsgs(*galoisKeys,
                                      *evaluator,
                                      *encoder,
                                      784,
                                      d1.weights_as_diags(),
                                      image_ctxt,
                                      result);

  // Now add the bias
  seal::Plaintext b1 = encode(d1.bias());
  evaluator->add_plain_inplace(result, b1);

  // Activation, x -> x^2
  evaluator->square_inplace(result);

  // Create the Weights and Biases for the second  dense layer
  DenseLayer d2(10, 30);

  // Weights
  ptxt_matrix_enc_vector_product_bsgs(*galoisKeys, *evaluator, *encoder, 30, d2.weights_as_diags(), result, result);

  // Bias
  seal::Plaintext b2 = encode(d2.bias());
  evaluator->add_plain_inplace(result, b2);

  // Activation, x -> x^2
  evaluator->square_inplace(result);

  auto t5 = Time::now();
  log_time(ss_time, t4, t5, false);

  // // === retrieve final result ====================================
  auto t6 = Time::now();
  seal::Plaintext p;
  decryptor->decrypt(result, p);
  std::vector<double> dec;
  encoder->decode(p, dec);

  std::cout << "Result:" << std::endl;
  for (int i = 0; i < 10; ++i) {
    std::cout << (double) dec[i] << std::endl;
  }
  auto t7 = Time::now();
  log_time(ss_time, t6, t7, true);

  // write ss_time into file
  std::ofstream myfile;
  myfile.open("seal_batched_ckks_nn.csv", std::ios::out | std::ios::app);
  if (myfile.fail()) throw std::ios_base::failure(std::strerror(errno));
  // make sure write fails with exception if something is wrong
  myfile.exceptions(myfile.exceptions() | std::ios::failbit |
      std::ifstream::badbit);
  myfile << ss_time.str() << std::endl;

  // write FHE parameters into file
  write_parameters_to_file(context, "fhe_parameters_nn.txt");
}

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'nn-batched-ckks'..." << std::endl;
  NNBatched().run_nn();
  return 0;
}

DenseLayer::DenseLayer(size_t units, size_t input_size) {
  if (units!=input_size) {
    throw std::invalid_argument("Only square matrices currently supported.");
    //TODO: Extend logic to general rectangular matrices
  }
  bias_vec = random_vector(units);
  diags = std::vector<vec>();
  for (int i = 0; i < units; ++i) {
    diags.push_back(random_vector((units)));
  }
}

const std::vector<vec> &DenseLayer::weights_as_diags() {
  return diags;
}
const vec &DenseLayer::bias() {
  return bias_vec;
}
