#include "kernel_batched.h"

#define IMAGE_SIZE 32

void Evaluation::run_kernel(VecInt2D img) {
  seal::EncryptionParameters params(seal::scheme_type::BFV);
  // in BFV, this degree is also the number of slots.
  params.set_poly_modulus_degree(DEFAULT_NUM_SLOTS);
  // Let SEAL select a "suitable" coefficient modulus (not necessarily maximal)
  params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(params.poly_modulus_degree()));
  // Let SEAL select a plaintext modulus that actually supports batching
  params.set_plain_modulus(
      seal::PlainModulus::Batching(params.poly_modulus_degree(), 20));
  context = seal::SEALContext::Create(params);

  /// Create keys
  seal::KeyGenerator keygen(context);
  secret_key = std::make_unique<seal::SecretKey>(keygen.secret_key());
  public_key = std::make_unique<seal::PublicKey>(keygen.public_key());
  galois_keys = std::make_unique<seal::GaloisKeys>(keygen.galois_keys_local());
  relin_keys = std::make_unique<seal::RelinKeys>(keygen.relin_keys_local());

  // Create helper objects
  encoder = std::make_unique<seal::BatchEncoder>(context);
  // secret key encryptor is more efficient
  encryptor = std::make_unique<seal::Encryptor>(context, *public_key, *secret_key);

  // Encrypt input image
  std::vector<int64_t> img_as_vec;
  img_as_vec.reserve(img.size() * img.size());
  for (auto row : img) {
    for (auto elem : row) {
      img_as_vec.push_back(elem);
    }
  }

  seal::Plaintext img_ptxt;
  encoder->encode(img_as_vec, img_ptxt);
  seal::Ciphertext img_ctxt(context);
  encryptor->encrypt_symmetric(img_ptxt, img_ctxt);

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
    evaluator.rotate_rows_inplace(img_ctxts[i], (int)rotations[i], *galois_keys);
    //    seal::Ciphertext dst(context);
    //    evaluator.rotate_vector(img_ctxts[i], (int) rotations[i], *galois_keys,
    //    dst);
    encoder->encode(std::vector<int64_t>(img_as_vec.size(), weights[i]), w_ptxt);
    evaluator.multiply_plain_inplace(img_ctxts[i], w_ptxt);
  }

  // Sum up all the ctxts
  seal::Ciphertext res_ctxt(context);
  evaluator.add_many(img_ctxts, res_ctxt);
}

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'kernel-bfv-batched'..." << std::endl;

  // std::vector<int> image_sizes = { 8, 16, 32, 64, 96, 128 };
  std::vector<int> image_sizes = {8};

  for (auto img_size : image_sizes) {
    // generate input image with dummy data
    std::vector<int> vec(img_size);
    std::iota(vec.begin(), vec.end(), 0);

    // run kernel using FHE
    std::vector<std::vector<int>> img(img_size, vec);
    Evaluation eval;
    eval.run_kernel(img);
  }
}
