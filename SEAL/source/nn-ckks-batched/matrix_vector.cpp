#include "matrix_vector.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

matrix random_square_matrix(size_t dim) {
  matrix M(dim);
  for (size_t i = 0; i < M.size(); i++) {
    M[i].resize(dim);
    for (size_t j = 0; j < dim; j++) {
      M[i][j] = (static_cast<double>(rand())/RAND_MAX) - 0.5;
    }
  }
  return M;
}

matrix identity_matrix(size_t dim) {
  matrix M(dim);
  for (size_t i = 0; i < M.size(); i++) {
    M[i].resize(dim);
    M[i][i] = 1;
  }
  return M;
}

vec random_vector(size_t dim) {
  vec v(dim);
  for (size_t j = 0; j < dim; j++) {
    v[j] = (static_cast<double>(rand())/RAND_MAX) - 0.5;
  }
  return v;
}

vec mvp(matrix M, vec v) {
  if (M.size()==0) {
    throw invalid_argument("Matrix must be well formed and non-zero-dimensional");
  }

  vec Mv(M.size(), 0);
  for (size_t i = 0; i < M.size(); i++) {
    if (v.size()!=M[0].size()) {
      throw invalid_argument("Vector and Matrix dimension not compatible.");
    } else {
      for (size_t j = 0; j < M[0].size(); j++) {
        Mv[i] += M[i][j]*v[j];
      }
    }
  }
  return Mv;
}

matrix add(matrix A, matrix B) {
  if (A.size()!=B.size() || (A.size() > 0 && A[0].size()!=B[0].size())) {
    throw invalid_argument("Matrices must have the same dimensions.");
  } else {
    matrix C(A.size());
    for (size_t i = 0; i < A.size(); i++) {
      C[i].resize(A[0].size());
      for (size_t j = 0; j < A[0].size(); j++) {
        C[i][j] = A[i][j] + B[i][j];
      }
    }
    return C;
  }
}

vec add(vec a, vec b) {
  if (a.size()!=b.size()) {
    throw invalid_argument("Vectors must have the same dimensions.");
  } else {
    vec c(a.size());
    for (size_t i = 0; i < a.size(); i++) {
      c[i] = a[i] + b[i];
    }
    return c;
  }
}

vec mult(vec a, vec b) {
  if (a.size()!=b.size()) {
    throw invalid_argument("Vectors must have the same dimensions.");
  } else {
    vec c(a.size());
    for (size_t i = 0; i < a.size(); i++) {
      c[i] = a[i]*b[i];
    }
    return c;
  }
}

vec diag(matrix M, size_t d) {
  const size_t dim = M.size();
  if (dim==0 || M[0].size()!=dim || d >= dim) {
    throw invalid_argument("Matrix must be square and d must be smaller than matrix dimension.");
  }
  vec diag(dim);
  for (size_t i = 0; i < dim; i++) {
    diag[i] = M[i][(i + d)%dim];
  }
  return diag;
}

vector<vec> diagonals(const matrix M) {
  if (M.size()==0) {
    throw invalid_argument("Matrix must be square and have non-zero dimension.");
  }
  vector<vec> diagonals(M.size());
  for (size_t i = 0; i < M.size(); ++i) {
    diagonals[i] = diag(M, i);
  }
  return diagonals;
}

vec duplicate(const vec v) {
  size_t dim = v.size();
  vec r;
  r.reserve(2*dim);
  r.insert(r.begin(), v.begin(), v.end());
  r.insert(r.end(), v.begin(), v.end());
  return r;
}

vec mvp_from_diagonals(std::vector<vec> diagonals, vec v) {
  const size_t dim = diagonals.size();
  if (dim==0 || diagonals[0].size()!=dim || v.size()!=dim) {
    throw invalid_argument("Matrix must be square, Matrix and vector must have matching non-zero dimension.");
  }
  vec r(dim);
  for (size_t i = 0; i < dim; ++i) {
    // t = diagonals[i] * v, component wise
    vec t = mult(diagonals[i], v);

    // Accumulate result
    r = add(r, t);

    // Rotate v to next position (at the end, because it needs to be un-rotated for first iteration)
    rotate(v.begin(), v.begin() + 1, v.end());
  }
  return r;
}

vec mvp_from_diagonals_bsgs(std::vector<vec> diagonals, vec v) {
  const size_t dim = diagonals.size();
  if (dim==0 || diagonals[0].size()!=dim || v.size()!=dim || !perfect_square(dim)) {
    throw invalid_argument(
        "Matrix must be square, Matrix and vector must have matching non-zero dimension, Dimension must be a square number!");
  }

  // Since dim is a power-of-two, this should be accurate even with the conversion to double and back
  const size_t sqrt_dim = sqrt(dim);

  // Baby step-giant step algorithm based on "Techniques in privacy-preserving machine learning" by Hao Chen, Microsoft Research
  // Talk presented at the Microsoft Research Private AI Bootcamp on 2019-12-02.
  // Available at https://youtu.be/d2bIhv9ExTs (Recording) or https://github.com/WeiDaiWD/Private-AI-Bootcamp-Materials (Slides)
  // Note that here, n1 = n2 = sqrt(n)

  vec r(dim, 0);

  // Precompute the inner rotations (space-runtime tradeoff of BSGS) at the cost of n2 rotations
  vector<vec> rotated_vs(sqrt_dim, v);
  for (size_t j = 0; j < sqrt_dim; ++j) {
    rotate(rotated_vs[j].begin(), rotated_vs[j].begin() + j, rotated_vs[j].end());
  }

  for (size_t k = 0; k < sqrt_dim; ++k) {
    vec inner_sum(dim, 0);
    for (size_t j = 0; j < sqrt_dim; ++j) {
      // Take the current_diagonal and rotate it by -k*sqrt_dim to match the not-yet-enough-rotated vector v
      vec current_diagonal = diagonals[(k*sqrt_dim + j)%dim];
      rotate(current_diagonal.begin(), current_diagonal.begin() + current_diagonal.size() - k*sqrt_dim,
             current_diagonal.end());

      // inner_sum += rot(current_diagonal) * current_rot_v
      inner_sum = add(inner_sum, mult(current_diagonal, rotated_vs[j]));
    }
    rotate(inner_sum.begin(), inner_sum.begin() + (k*sqrt_dim), inner_sum.end());
    r = add(r, inner_sum);
  }
  return r;
}

/// Split n int n1 and n2 s.t. n1 * n2 = n and n1 is close to sqrt(n)
/// \param n number to be factored, must not be prime
/// \return n1
/// \throw std::invalid_argument if n cannot be factored
size_t find_factor(size_t n) {
  size_t sqrt_n = sqrt(n); //approximate, because size_t->double->size_t
  size_t n1 = sqrt_n;
  size_t n2 = n/sqrt_n;
  while (n1 < n && n1*n2!=n) {
    n1++;
    n2 = n/n1;
  }
  if (n1*n2!=n) {
    throw std::invalid_argument("Cannot factor n.");
  }
  return n1;

}
vec general_mvp_from_diagonals_bsgs(std::vector<vec> diagonals, vec v) {
  const size_t dim_k = diagonals.size();
  if (dim_k==0) {
    throw invalid_argument(
        "Matrix must not be empty!");
  }
  const size_t dim_n = diagonals[0].size();
  if (dim_n!=v.size() || dim_n==0) {
    throw invalid_argument(
        "Matrix and vector must have matching non-zero dimension");
  }

  const size_t n1 = find_factor(dim_n);
  const size_t n2 = dim_n/n1;
  std::cout << "Split " << dim_n << " into n1=" << n1 << " and n2=" << n2 << std::endl;

  // Baby step-giant step algorithm based on "Techniques in privacy-preserving machine learning" by Hao Chen, Microsoft Research
  // Talk presented at the Microsoft Research Private AI Bootcamp on 2019-12-02.
  // Available at https://youtu.be/d2bIhv9ExTs (Recording) or https://github.com/WeiDaiWD/Private-AI-Bootcamp-Materials (Slides)

  vec r(dim_k, 0);

  // Precompute the inner rotations (space-runtime tradeoff of BSGS) at the cost of n1 rotations
  vector<vec> rotated_vs(n1, v);
  for (size_t j = 0; j < n1; ++j) {
    rotate(rotated_vs[j].begin(), rotated_vs[j].begin() + j, rotated_vs[j].end());
  }

  for (size_t k = 0; k < n1; ++k) {
    vec inner_sum(dim_k, 0);
    for (size_t j = 0; j < n1; ++j) {
      // Take the current_diagonal and rotate it by -k*sqrt_dim to match the not-yet-enough-rotated vector v
      vec current_diagonal = diagonals[(k*n1 + j)%dim_k];
      rotate(current_diagonal.begin(), current_diagonal.begin() + current_diagonal.size() - k*n1,
             current_diagonal.end());

      // inner_sum += rot(current_diagonal) * current_rot_v
      inner_sum = add(inner_sum, mult(current_diagonal, rotated_vs[j]));
    }
    rotate(inner_sum.begin(), inner_sum.begin() + (k*n1), inner_sum.end());
    r = add(r, inner_sum);
  }
  return r;
}

bool perfect_square(unsigned long long x) {
  auto sqrt_x = static_cast<unsigned long long>(sqrt(x));
  return (sqrt_x*sqrt_x==x);
}

vec rnn_with_relu(vec x, vec h, matrix W_x, matrix W_h, vec b) {
  const size_t dim = x.size();
  if (dim==0 || h.size()!=dim || W_x.size()!=dim || W_h.size()!=dim || b.size()!=dim) {
    throw invalid_argument("All dimensions must be non-zero and matching");
  }

  // Compute W_x * x + W_h * h + b
  vec r = add(mvp(W_x, x), mvp(W_h, h));
  r = add(r, b);

  // ReLU(x) = max(0,x)
  for (auto &t : r) {
    t = max(0., t);
  }

  return r;
}

vec rnn_with_squaring(vec x, vec h, matrix W_x, matrix W_h, vec b) {
  const size_t dim = x.size();
  if (dim==0 || h.size()!=dim || W_x.size()!=dim || W_h.size()!=dim || b.size()!=dim) {
    throw invalid_argument("All dimensions must be non-zero and matching");
  }

  // Compute W_x * x + W_h * h + b
  vec r = add(mvp(W_x, x), mvp(W_h, h));
  r = add(r, b);

  // squaring as activation function
  for (auto &t : r) {
    t = t*t;
  }
  return r;
}

bool equal(vec r, vec expected, float tolerance) {
  bool equal = true;
  for (size_t i = 0; i < r.size(); ++i) {
    // Test if value is within tolerance of the actual value or 10 sig figs
    const auto difference = abs(r[i] - expected[i]);
    if (difference > max(0.000000001, tolerance*abs(expected[i]))) {
      equal = false;
      std::cout << "\tERROR: difference of " << difference << " detected, where r[" << i << "]: " << r[i] <<
                " and expected[" << i << "]: " << expected[i] << std::endl;
      //throw runtime_error("Comparison to expected failed");
    }
  }
  return equal;
}