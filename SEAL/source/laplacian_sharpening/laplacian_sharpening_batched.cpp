#include "laplacian_sharpening_batched.h"

#define NUM_CTXT_SLOTS 16384
#define IMAGE_SIZE 32

void Evaluation::encryptedLaplacianSharpeningAlgorithmBatched(VecInt2D img) {
  /// the encrypted data in this Ciphertext
  seal::Ciphertext ciphertext;

  /// the seal context, i.e. object that holds params/etc
  static std::shared_ptr<seal::SEALContext> context;

  /// secret key, also used for (more efficient) encryption (ptr for
  /// consistency)
  static std::unique_ptr<seal::SecretKey> secretKey;

  /// public key (ptr because PublicKey() segfaults)
  static std::unique_ptr<seal::PublicKey> publicKey;

  /// keys required to rotate (ptr because GaloisKeys() segfaults)
  static std::unique_ptr<seal::GaloisKeys> galoisKeys;

  /// keys required to relinearize after multipliction (segfaulting not tested,
  /// but ptr for consistency)
  static std::unique_ptr<seal::RelinKeys> relinKeys;

  /// Wrapper for parameters
  seal::EncryptionParameters params(seal::scheme_type::BFV);

  // in BFV, this degree is also the number of slots.
  params.set_poly_modulus_degree(NUM_CTXT_SLOTS);

  // Let SEAL select a "suitable" coefficient modulus (not necessarily maximal)
  params.set_coeff_modulus(
      seal::CoeffModulus::BFVDefault(params.poly_modulus_degree()));

  // Let SEAL select a plaintext modulus that actually supports batching
  params.set_plain_modulus(
      seal::PlainModulus::Batching(params.poly_modulus_degree(), 20));

  // Instantiate context
  context = seal::SEALContext::Create(params);

  /// Helper object to create keys
  seal::KeyGenerator keyGenerator(context);

  secretKey = std::make_unique<seal::SecretKey>(keyGenerator.secret_key());
  publicKey = std::make_unique<seal::PublicKey>(keyGenerator.public_key());
  galoisKeys =
      std::make_unique<seal::GaloisKeys>(keyGenerator.galois_keys_local());
  relinKeys =
      std::make_unique<seal::RelinKeys>(keyGenerator.relin_keys_local());

  auto encoder = seal::BatchEncoder(context);
  auto encryptor =
      seal::Encryptor(context, *publicKey,
                      *secretKey);  // secret Key encryptor is more efficient

  // Encrypt input
  std::vector<int64_t> img_as_vec;
  img_as_vec.reserve(img.size() * img.size());
  for (int x = 1; x < img.size() - 1; ++x) {
    for (int y = 1; y < img.at(x).size() - 1; ++y) {
      img_as_vec.push_back(img[x][y]);
    }
  }
  seal::Plaintext img_ptxt;
  encoder.encode(img_as_vec, img_ptxt);
  seal::Ciphertext img_ctxt(context);
  encryptor.encrypt_symmetric(img_ptxt, img_ctxt);

  // Compute sharpening filter
  auto evaluator = seal::Evaluator(context);

  // Take copies of the image, rotate and mult with weights
  // TODO: Not 100% sure if weight and rotation order match up correctly. But
  // doesn't matter for benchmarking
  std::vector<int> weights = {1, 1, 1, 1, -8, 1, 1, 1, 1};
  std::vector<size_t> rotations = {0,
                                   1,
                                   2,
                                   img.size(),
                                   img.size() + 1,
                                   img.size() + 2,
                                   2 * img.size(),
                                   2 * img.size() + 1,
                                   2 * img.size() + 2};
  seal::Plaintext w_ptxt;
  std::vector<seal::Ciphertext> img_ctxts(img.size(),
                                          seal::Ciphertext(context));
  img_ctxts.reserve(weights.size());
  for (size_t i = 0; i < weights.size(); ++i) {
    img_ctxts[i] = (i == 0)
                       ? std::move(img_ctxt)
                       : img_ctxts[0];  // move for i == 0 saves one ctxt copy
    evaluator.rotate_rows_inplace(img_ctxts[i], (int)rotations[i], *galoisKeys);
    //    seal::Ciphertext dst(context);
    //    evaluator.rotate_vector(img_ctxts[i], (int) rotations[i], *galoisKeys,
    //    dst);
    encoder.encode(std::vector<int64_t>(img_as_vec.size(), weights[i]), w_ptxt);
    evaluator.multiply_plain_inplace(img_ctxts[i], w_ptxt);
  }

  // Sum up all the ctxts
  seal::Ciphertext res_ctxt(context);
  evaluator.add_many(img_ctxts, res_ctxt);
}

int main(int argc, char *argv[]) {
  std::cout << "image_size,time_ms" << std::endl;

  std::vector<int> image_sizes = { 8, 16, 32, 64, 96, 128 };

  for (auto img_size : image_sizes) {
    std::vector<int> vec(img_size);
    std::iota(vec.begin(), vec.end(), 0);
    std::vector<std::vector<int>> img(img_size, vec);
    decltype(std::chrono::high_resolution_clock::now()) t_start;
    decltype(std::chrono::high_resolution_clock::now()) t_end;
    decltype(std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start)) duration_ms;
    t_start = std::chrono::high_resolution_clock::now();
    Evaluation::encryptedLaplacianSharpeningAlgorithmBatched(img);
    t_end = std::chrono::high_resolution_clock::now();
    duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
    std::cout << img_size << "," << duration_ms.count() << std::endl;
  }
}