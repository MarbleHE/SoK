#pragma once
#include <vector>

/// Matrix in row-major order
typedef std::vector<std::vector<double>> matrix;

/// Vector
/// Defined to allow clear semantic difference in the code between std::vectors and vectors in the mathematical sense)
typedef std::vector<double> vec;


/// \name Plaintext Matrix-Vector Helpers
///@{
/**
 * \brief Generates a square matrix with random values from [-1/2,1/2]
 * \param dim Dimension of the matrix
 * \return A square matrix of size dim x dim with values in [-1/2,1/2]
 */
matrix random_square_matrix(size_t dim);

/**
 * \brief Generates the identity matrix of size dim (I.e. a matrix where the diagonal elements are 1 and all other elements are 0)
 * \param dim Dimension of the matrix
 * \return A square matrix of size dim x dim with diagonal values = 1 and all other values = 0
 */
matrix identity_matrix(size_t dim);

/**
 * \brief Generates a vector with random values from [-1/2,1/2]
 * \param dim Length of the vector
 * \return A vector of length dim with values in [-1/2,1/2]
 */
vec random_vector(size_t dim);

/**
 * \brief Computes the matrix-vector-product between a matrix M and a vector v. The length of v must be the same as the second dimension of M.
 * \param M Matrix of any size d1xd2
 * \param v Vector of length d2
 * \return The matrix-vector product between M and v, a vector of length d1
 * \throw std::invalid_argument if the dimensions mismatch
 */
vec mvp(matrix M, vec v);

/**
 * \brief Addition between two matrices (component-wise). Both matrices must have the same dimensions
 * \param A Matrix of any size d1xd2
 * \param B Matrix of same size d1xd2
 * \return The sum between A and B, a matrix of the same size d1xd2 as the inputs
 * \throw std::invalid_argument if the dimensions mismatch
 */
matrix add(matrix A, matrix B);

/**
 * \brief Addition between two vectors (component-wise). Both vectors must have the same length
 * \param a Vector of any length d
 * \param b Vector of same length d
 * \return The sum between a and b, a vector of the same length d as the inputs
 * \throw std::invalid_argument if the dimensions mismatch
 */
vec add(vec a, vec b);

/**
 * \brief Multiplication between two vectors (component-wise). Both vectors must have the same length
 * \param a Vector of any length d
 * \param b Vector of same length d
 * \return The component-wise product between a and b, a vector of the same length d as the inputs
 * \throw std::invalid_argument if the dimensions mismatch
 */
vec mult(vec a, vec b);


/**
 * \brief The d-th diagonal of a matrix. The matrix M must be square.
 * \param M A *square* matrix of size dim x dim
 * \param d Index of the diagonal, where d = 0 is the main diagonal. Wraps around, i.e. d = dim is the last diagonal (the one below main diagonal)
 * \return d-th diagonal  of M, a vector of length dim
 * \throw std::invalid_argument if M is non-square or d is geq than matrix dimension
 */
vec diag(matrix M, size_t d);


/**
 * \brief Returns a list of all the diagonals of a matrix. The matrix must be square. Numbering starts with the main diagonal and moves up with wrap-around, i.e. the last element is the diagonal one below the main diagonal).
 * \param M A *square* matrix of size dim x dim.
 * \return The list of length dim of all the diagonals of M, each a vector of length dim
 * \throw std::invalid_argument if M is non-square
 */
std::vector<vec> diagonals(const matrix M);


/**
 * \brief Returns a vector of twice the length, with the elements repeated in the same sequence
 * \param v Vector of length d
 * \return Vector of length 2*d that contains two concatenated copies of the input vector
 */
vec duplicate(const vec v);

/**
 * \brief Computes the matrix-vector-product between a *square* matrix M, represented by its diagonals, and a vector.
 *  Plaintext implementation of the FHE-optimized approach due to Smart et al. (diagonal-representation) 
 * \param diagonals Matrix of size dxd represented by the its diagonals (numbering starts with the main diagonal and moves up with wrap-around, i.e. the last element is the diagonal one below the main diagonal)
 * \param v Vector of length d
 * \return The matrix-vector product between M and v, a vector of length d
 * \throw std::invalid_argument if the dimensions mismatch
 */
vec mvp_from_diagonals(std::vector<vec> diagonals, vec v);

/**
 * \brief Computes the matrix-vector-product between a *square* matrix M, represented by its diagonals, and a vector. **Matrix dimension must be a square number**
 *  Plaintext implementation of the FHE-optimized approach due to Smart et al. (diagonal-representation) and the baby-step giant-step algorithm
 * \param diagonals Matrix of size dxd represented by the its diagonals (numbering starts with the main diagonal and moves up with wrap-around, i.e. the last element is the diagonal one below the main diagonal)
 * \param v Vector of length d
 * \return The matrix-vector product between M and v, a vector of length d
 * \throw std::invalid_argument if the dimensions mismatch or the dimension is not a square number
 */
vec mvp_from_diagonals_bsgs(std::vector<vec> diagonals, vec v);


/**
 * \brief Test if x is a perfect square, i.e. x = y^2 for an integer y?
 * \param x Integer to test
 * \return True if x = y^2 for integer y, false otherwise
 */
bool perfect_square(unsigned long long x);

/**
 * \brief Computes a single step of a simple recurrent neural network (RNN) using ReLU rather than tanh() as the activation function
 * \param x Vector of length d containing the current input
 * \param h Vector length d containing the hidden input, usually from a previous layer
 * \param W_x Weight matrix of size d x d (weights for current input)
 * \param W_h Weight matrix of size d x d (weights for hidden input)
 * \param b  Bias vector of length d
 * \return Vector of length d, containing ReLU(W_x * x + W_h * h + b)
 * \throw std::invalid_argument if the dimensions mismatch
 */
vec rnn_with_relu(vec x, vec h, matrix W_x, matrix W_h, vec b);

/**
 * \brief Computes a single step of a simple recurrent neural network (RNN) using x^2 rather than tanh() or ReLU as the activation function
 * \param x Vector of length d containing the current input
 * \param h Vector length d containing the hidden input, usually from a previous layer
 * \param W_x Weight matrix of size d x d (weights for current input)
 * \param W_h Weight matrix of size d x d (weights for hidden input)
 * \param b  Bias vector of length d
 * \return Vector of length d, containing (W_x * x + W_h * h + b)^2
 * \throw std::invalid_argument if the dimensions mismatch
 */
vec rnn_with_squaring(vec x, vec h, matrix W_x, matrix W_h, vec b);

///@} // End of Plaintext Matrix-Vector Helpers