#pragma once
#include "matrix_vector.h"
#include "seal/seal.h"

/**
 * \brief Compute the matrix-vector-product between a *square* plaintext matrix, represented by its diagonals, and an encrypted vector.
 *  Uses the optimizations due to Smart et al. (diagonal-representation)
 *  *ATTENTION*: Batching must be done in a way so that if the matrix has dimension d, rotating the vector left d times results in a correct cyclic rotation of the first d elements, same for diagonals!
 *  This is usually done by simply duplicating the vector, e.g. using function duplicate(vec x), if the number of slots in the ciphertexts and the dimension of the vector are not the same
 * \param[in] galois_keys Rotation keys, should allow arbitrary rotations (reality is slightly more complicated)
 * \param[in] evaluator Evaluation object from SEAL
 * \param[in] ptxt_diagonals The plaintext matrix, represented by the its diagonals (numbering starts with the main diagonal and moves up with wrap-around, i.e. the last element is the diagonal one below the main diagonal)
 * \param[in] ctv The encrypted vector, batched into a single ciphertext. The length must match the matrix dimension
 * \param[out] enc_result  Encrypted vector, batched into a single ciphertext
 * \param[in] dim Length of the vector and dimension of the (square) Matrix, which must match
 */
void ptxt_matrix_enc_vector_product(const seal::GaloisKeys& galois_keys, seal::Evaluator& evaluator,
    size_t dim, std::vector<seal::Plaintext> ptxt_diagonals,
    const seal::Ciphertext& ctv, seal::Ciphertext& enc_result);


/**
 * \brief Compute the matrix-vector-product between a *square* plaintext matrix, represented by its diagonals, and an encrypted vector.
 *  Uses the optimizations due to Smart et al. (diagonal-representation) and the baby-step giant-step algorithm
 *  *ATTENTION*: Batching must be done in a way so that if the matrix has dimension d, rotating the vector left d times results in a correct cyclic rotation of the first d elements!
 *  This is usually done by simply duplicating the vector, e.g. using function duplicate(vec x), if the number of slots in the ciphertexts and the dimension of the vector are not the same
 *  Since this is also done internally for the diagonals, ** the number of slots in the ciphertext must be either >= 2*dim or must be equal to dim **
 * \param[in] galois_keys Rotation keys, should allow arbitrary rotations (reality is slightly more complicated due to baby-step--giant-step algorithm)
 * \param[in] evaluator Evaluation object from SEAL
 * \param[in] encoder Encoder object from SEAL
 * \param[in] dim Length of the vector and dimension of the (square) Matrix, which must match and **dim must be a square number**
 * \param[in] diagonals The plaintext matrix, represented by the its diagonals (numbering starts with the main diagonal and moves up with wrap-around, i.e. the last element is the diagonal one below the main diagonal)
 * \param[in] ctv The encrypted vector, batched into a single ciphertext. The length must match the matrix dimension
 * \param[out] enc_result  Encrypted vector, batched into a single ciphertext
 * \throw std::invalid_argument if the dimensions mismatch or the dimension is not a square number
 */
void ptxt_matrix_enc_vector_product_bsgs(const seal::GaloisKeys& galois_keys, seal::Evaluator& evaluator,
    seal::CKKSEncoder& encoder, size_t dim,
    std::vector<vec> diagonals,
    const seal::Ciphertext& ctv, seal::Ciphertext& enc_result);

/**
 * \brief Computes a single step of a simple RNN, where the non-linearity/activation function is approximated by x^2, i.e. it returns (W_x * x + W_h * h + b)^2
 * *ATTENTION*: Batching must be done in a way so that if the matrix has dimension d, rotating the vector left d times results in a correct cyclic rotation of the first d elements!
 *  This is usually done by simply duplicating the vector, e.g. using function duplicate(vec x), if the number of slots in the ciphertexts and the dimension of the vector are not the same
 *  Since this is also done internally for the diagonals, ** the number of slots in the ciphertext must be either >= 2*dim or must be equal to dim **
 * \param[in] galois_keys Rotation keys, should allow arbitrary rotations (reality is slightly more complicated due to baby-step--giant-step algorithm)
 * \param[in] evaluator Evaluation object from SEAL
 * \param[in] encoder Encoder object from SEAL
 * \param[in] dim Length of the vector and dimension of the (square) Matrix, which must match and **dim must be a square number**
 * \param[in] diagonals_W_x Plaintext weights matrix, represented by the its diagonals (numbering starts with the main diagonal and moves up with wrap-around, i.e. the last element is the diagonal one below the main diagonal)
 * \param[in] diagonals_W_h Plaintext weights matrix, represented by the its diagonals (numbering starts with the main diagonal and moves up with wrap-around, i.e. the last element is the diagonal one below the main diagonal)
 * \param[in] b Plaintext bias vector of length dim
 * \param[in] ctxt_x The encrypted input vector, batched into a single ciphertext. The length must match the matrix dimension
 * \param[in,out] ctxt_h The encrypted hidden vector, batched into a single ciphertext. The length must match the matrix dimension. Will be overwritten with new output
 * \throw std::invalid_argument if the dimensions mismatch or the dimension is not a square number
 */
void ptxt_weights_enc_input_rnn(const seal::GaloisKeys& galois_keys, seal::Evaluator& evaluator,
                                seal::CKKSEncoder& encoder, size_t dim, std::vector<vec> diagonals_W_x, std::vector<vec> diagonals_W_h, vec b,
                                const seal::Ciphertext& ctxt_x, seal::Ciphertext& ctxt_h);