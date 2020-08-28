#include "matrix_vector_crypto.h"

using namespace std;
using namespace seal;

void ptxt_matrix_enc_vector_product(const GaloisKeys &galois_keys, Evaluator &evaluator,
                                    size_t dim, vector<Plaintext> ptxt_diagonals, const Ciphertext &ctv,
                                    Ciphertext &enc_result) {
  Ciphertext temp;
  for (size_t i = 0; i < dim; i++) {
    //  Rotate v
    evaluator.rotate_vector(ctv, i, galois_keys, temp);
    //TODO: Implement Halevi-Shoup "Hoisting", where you save the common parts of the rotations?
    // See Appendix of "GAZELLE: A Low Latency Framework for  Secure Neural Network Inference"

    // multiply
    evaluator.mod_switch_to_inplace(ptxt_diagonals[i], temp.parms_id());
    evaluator.multiply_plain_inplace(temp, ptxt_diagonals[i]);
    if (i==0) {
      enc_result = temp;
    } else {
      evaluator.add_inplace(enc_result, temp);
    }
  }
}

void ptxt_matrix_enc_vector_product_bsgs(const GaloisKeys &galois_keys, Evaluator &evaluator,
                                         CKKSEncoder &encoder, size_t dim, vector<vec> diagonals,
                                         const Ciphertext &ctv, Ciphertext &enc_result) {
  if (dim==0 || diagonals[0].size()!=dim || !perfect_square(dim)) {
    throw invalid_argument(
        "Matrix must be square, Matrix and vector must have matching non-zero dimension, Dimension must be a square number!");
  }
  if (ctv.poly_modulus_degree()/2!=dim && ctv.poly_modulus_degree()/2 < 2*dim) {
    throw invalid_argument(
        "The number of ciphertext slots must be either exactly dim, or at least 2*dim to allow for duplicate encoding for meaningful rotations.");
  }
  /// Whether or not we need to duplicate elements in the diagonals vectors during encoding to ensure meaningful rotations
  const bool duplicating = (ctv.poly_modulus_degree()/2)!=dim;


  // Since dim is a power-of-two, this should be accurate even with the conversion to double and back
  const size_t sqrt_dim = sqrt(dim);

  // Baby step-giant step algorithm based on "Techniques in privacy-preserving machine learning" by Hao Chen, Microsoft Research
  // Talk presented at the Microsoft Research Private AI Bootcamp on 2019-12-02.
  // Available at https://youtu.be/d2bIhv9ExTs (Recording) or https://github.com/WeiDaiWD/Private-AI-Bootcamp-Materials (Slides)
  // Note that here, n1 = n2 = sqrt(n)

  // Precompute the inner rotations (space-runtime tradeoff of BSGS) at the cost of n2 rotations and some memory
  vector<Ciphertext> rotated_vs(sqrt_dim, ctv);
  for (size_t j = 0; j < sqrt_dim; ++j) {
    // TODO: Implement Halevi-Shoup "Hoisting", where you save the common parts of the rotations?
    // See Appendix of "GAZELLE: A Low Latency Framework for  Secure Neural Network Inference"
    evaluator.rotate_vector(ctv, j, galois_keys, rotated_vs[j]);
  }

  for (size_t k = 0; k < sqrt_dim; ++k) {
    Ciphertext inner_sum;
    for (size_t j = 0; j < sqrt_dim; ++j) {
      // Take the current_diagonal and rotate it by -k*sqrt_dim to match the not-yet-enough-rotated vector v
      vec current_diagonal = diagonals[(k*sqrt_dim + j)%dim];
      rotate(current_diagonal.begin(), current_diagonal.begin() + current_diagonal.size() - k*sqrt_dim,
             current_diagonal.end());
      Plaintext ptxt_current_diagonal;
      current_diagonal = duplicating ? duplicate(current_diagonal) : current_diagonal;
      // Duplicate only if necessary
      encoder.encode(current_diagonal, rotated_vs[j].parms_id(), rotated_vs[j].scale(), ptxt_current_diagonal);

      // inner_sum += rot(current_diagonal) * current_rot_v
      // multiply
      Ciphertext temp;
      evaluator.multiply_plain(rotated_vs[j], ptxt_current_diagonal, temp);
      // add
      if (j==0) {
        inner_sum = temp;
      } else {
        evaluator.add_inplace(inner_sum, temp);
      }
    }

    // Apply "missing bit" of rotation
    evaluator.rotate_vector_inplace(inner_sum, k*sqrt_dim, galois_keys);

    if (k==0) {
      enc_result = inner_sum;
    } else {
      evaluator.add_inplace(enc_result, inner_sum);
    }
  }
}

void ptxt_general_matrix_enc_vector_product(const seal::GaloisKeys &galois_keys, seal::Evaluator &evaluator,
                                            seal::CKKSEncoder &encoder, size_t m, size_t n,
                                            std::vector<vec> diagonals,
                                            const seal::Ciphertext &ctv, seal::Ciphertext &enc_result) {
  if (m==0 || m!=diagonals.size()) {
    throw invalid_argument(
        "Matrix must not be empty, and diagonals vector must have size m!");
  }
  if (n!=diagonals[0].size() || n==0) {
    throw invalid_argument(
        "Diagonals must have non-zero dimension that matches n");
  }
  size_t n_div_m = n/m;
  size_t log2_n_div_m = ceil(log2(n_div_m));
  if (m*n_div_m!=n || (2ULL << (log2_n_div_m - 1)!=n_div_m && n_div_m!=1)) {
    throw invalid_argument(
        "Matrix dimension m must divide n and the result must be power of two");
  }

  // Hybrid algorithm based on "GAZELLE: A Low Latency Framework for Secure Neural Network Inference" by Juvekar et al.
  // Available at https://www.usenix.org/conference/usenixsecurity18/presentation/juvekar
  // Actual Implementation based on the description in
  // "DArL: Dynamic Parameter Adjustment for LWE-based Secure Inference" by Bian et al. 2019.
  // Available at https://ieeexplore.ieee.org/document/8715110/ (paywall)

  //  vec t(n, 0);
  Ciphertext ctxt_t;

  for (size_t i = 0; i < m; ++i) {

    // rotated_v = rot(v,i)
    Ciphertext ctxt_rotated_v = ctv;
    if ( i != 0)  evaluator.rotate_vector_inplace(ctxt_rotated_v, i, galois_keys);

    // auto tmp = mult(diagonals[i], rotated_v);
    Plaintext ptxt_current_diagonal;
    encoder.encode(diagonals[i], ctxt_rotated_v.parms_id(), ctxt_rotated_v.scale(), ptxt_current_diagonal);
    Ciphertext ctxt_tmp;
    evaluator.multiply_plain(ctxt_rotated_v, ptxt_current_diagonal, ctxt_tmp);

    // t = add(t, tmp);
    if (i==0) {
      ctxt_t = ctxt_tmp;
    } else {
      evaluator.add_inplace(ctxt_t, ctxt_tmp);
    }
  }

  // vec r = t;
  Ciphertext ctxt_r = std::move(ctxt_t);

  //TODO: if n/m isn't a power of two, we need to masking/padding here
  for (int i = 0; i < log2_n_div_m; ++i) {
    // vec rotated_r = r;
    Ciphertext ctxt_rotated_r = ctxt_r;

    // Calculate offset
    size_t offset = n/(2ULL << i);

    // rotated_r = rot(rotated_r, offset)
    evaluator.rotate_vector_inplace(ctxt_rotated_r, offset, galois_keys);

    // r = add(r, rotated_r);
    evaluator.add_inplace(ctxt_r, ctxt_rotated_r);
  }
  //  r.resize(m); <- has to be done by the client
  // for efficiency we do not mask away the other entries
  enc_result = std::move(ctxt_r);
}

void ptxt_weights_enc_input_rnn(const seal::GaloisKeys &galois_keys,
                                seal::Evaluator &evaluator,
                                seal::CKKSEncoder &encoder,
                                size_t dim,
                                std::vector<vec> diagonals_W_x,
                                std::vector<vec> diagonals_W_h,
                                vec b,
                                const seal::Ciphertext &ctxt_x,
                                seal::Ciphertext &ctxt_h) {
  if (dim==0 || diagonals_W_x.size()!=dim || diagonals_W_h.size()!=dim || !perfect_square(dim)) {
    throw invalid_argument(
        "Matrix must be square, Matrix and vector must have matching non-zero dimension, Dimension must be a square number!");
  }
  if (ctxt_x.poly_modulus_degree()/2!=dim && ctxt_x.poly_modulus_degree()/2 < 2*dim) {
    throw invalid_argument(
        "The number of ciphertext slots must be either exactly dim, or at least 2*dim to allow for duplicate encoding for meaningful rotations.");
  }

  /// Whether or not we need to duplicate elements in the diagonals vectors during encoding to ensure meaningful rotations
  const bool duplicating = (ctxt_x.poly_modulus_degree()/2)!=dim;

  // W_h * h
  ptxt_matrix_enc_vector_product_bsgs(galois_keys, evaluator, encoder, dim, diagonals_W_h, ctxt_h, ctxt_h);

  // W_x * x
  Ciphertext ctxt_t;
  ptxt_matrix_enc_vector_product_bsgs(galois_keys, evaluator, encoder, dim, diagonals_W_x, ctxt_x, ctxt_t);

  // h = W_h * h + W_x * x
  evaluator.add_inplace(ctxt_h, ctxt_t);

  // h = (W_h * h + W_x * x) + b
  Plaintext ptxt_b;
  b = duplicating ? duplicate(b) : b;
  encoder.encode(b, ctxt_h.parms_id(), ctxt_h.scale(), ptxt_b);
  evaluator.add_plain_inplace(ctxt_h, ptxt_b);

  // Rescale before multiplication, to prevent blow-up
  evaluator.rescale_to_next_inplace(ctxt_h);

  // Squaring
  evaluator.square_inplace(ctxt_h);
}

bool decrypt_and_compare(const seal::Ciphertext &ctxt_r,
                         vec expected,
                         seal::Decryptor &decryptor,
                         seal::CKKSEncoder &encoder,
                         float tolerance) {
  Plaintext ptxt_t;
  decryptor.decrypt(ctxt_r, ptxt_t);
  vec r;
  encoder.decode(ptxt_t, r);
  r.resize(expected.size());
  return equal(r, expected, tolerance);
}