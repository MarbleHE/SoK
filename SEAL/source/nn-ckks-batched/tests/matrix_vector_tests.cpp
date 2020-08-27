#include "gtest/gtest.h"
#include "../matrix_vector.h"

using namespace std;

namespace MVPlaintextTests
{
	const size_t dim = 15;
	TEST(Generation, RandomMatrix)
	{
		const auto t = random_square_matrix(15);
		ASSERT_EQ(t.size(), dim);
		for (auto& x : t)
		{
			ASSERT_EQ(x.size(), dim);
			for (auto& y : x)
			{
				EXPECT_TRUE((-0.5 <= y) && (y <= 0.5));
			}
		}
	}

	TEST(Generation, IdentityMatrix)
	{
		const auto t = identity_matrix(15);
		ASSERT_EQ(t.size(), dim);
		for (size_t i = 0; i < t.size(); ++i)
		{
			ASSERT_EQ(t[i].size(), dim);
			for (size_t j = 0; j < t[i].size(); ++j)
			{
				EXPECT_EQ(t[i][j], (i == j));
			}
		}
	}

	TEST(Generation, RandomVector)
	{
		const auto t = random_vector(15);
		ASSERT_EQ(t.size(), dim);
		for (auto& x : t)
		{
			EXPECT_TRUE((-0.5 <= x) && (x <= 0.5));
		}
	}

	TEST(PlaintextOperations, MatrixVectorProduct)
	{
		const auto m = random_square_matrix(dim);
		const auto v = random_vector(dim);

		// Standard multiplication
		const auto r = mvp(m, v);
		ASSERT_EQ(r.size(), dim);
		for (size_t i = 0; i < dim; ++i)
		{
			double sum = 0;
			for (size_t j = 0; j < dim; ++j)
			{
				sum += v[j] * m[i][j];
			}
			EXPECT_EQ(r[i], sum);
		}

		// identity matrix should be doing identity things
		const auto id = identity_matrix(dim);
		EXPECT_EQ(v, mvp(id, v));

		// Mismatching sizes should throw exception
		EXPECT_THROW(mvp(m, {}), invalid_argument);
		EXPECT_THROW(mvp({}, v), invalid_argument);
	}

	TEST(PlaintextOperations, MatrixAdd)
	{
		const auto m1 = random_square_matrix(dim);
		const auto m2 = random_square_matrix(dim);

		// Standard addition
		const auto r = add(m1, m2);
		ASSERT_EQ(r.size(), dim);
		for (size_t i = 0; i < dim; ++i)
		{
			ASSERT_EQ(r[i].size(), dim);
			for (size_t j = 0; j < dim; ++j)
			{
				EXPECT_EQ(r[i][j], (m1[i][j] + m2[i][j]));
			}
		}

		// Mismatched sizes
		EXPECT_THROW(add({}, m2), invalid_argument);
		EXPECT_THROW(add(m1, {}), invalid_argument);
		EXPECT_THROW(add(matrix(dim), m2), invalid_argument);
		EXPECT_THROW(add(m1, matrix(dim)), invalid_argument);
	}

	TEST(PlaintextOperations, VectorAdd)
	{
		const auto v1 = random_vector(dim);
		const auto v2 = random_vector(dim);

		// Standard addition
		const auto r = add(v1, v2);
		ASSERT_EQ(r.size(), dim);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_EQ(r[i], (v1[i] + v2[i]));
		}

		// Mismatched sizes
		EXPECT_THROW(add({}, v2), invalid_argument);
		EXPECT_THROW(add(v1, {}), invalid_argument);
	}

	TEST(PlaintextOperations, VectorMult)
	{
		const auto v1 = random_vector(dim);
		const auto v2 = random_vector(dim);

		// Standard component-wise multiplication
		const auto r = mult(v1, v2);
		ASSERT_EQ(r.size(), dim);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_EQ(r[i], (v1[i] * v2[i]));
		}

		// Mismatched sizes
		EXPECT_THROW(mult({}, v2), invalid_argument);
		EXPECT_THROW(mult(v1, {}), invalid_argument);
	}

	TEST(PlaintextOperations, Diag)
	{
		const auto m = random_square_matrix(dim);
		for (size_t d = 0; d < dim; ++d)
		{
			const auto r = diag(m, d);
			ASSERT_EQ(r.size(), dim);
			for (size_t i = 0; i < dim; ++i)
			{
				EXPECT_EQ(r[i], m[i][(i + d) % dim]);
			}
		}

		// Non-square
		EXPECT_THROW(diag(matrix(dim), 0), invalid_argument);

		// Non-existent diagonal
		EXPECT_THROW(diag(m, dim + 1), invalid_argument);
	}

	TEST(PlaintextOperations, Diagonals)
	{
		const auto m = random_square_matrix(dim);
		const auto r = diagonals(m);
		ASSERT_EQ(r.size(), dim);
		for (size_t d = 0; d < dim; ++d)
		{
			ASSERT_EQ(r[d].size(), dim);
			for (size_t i = 0; i < dim; ++i)
			{
				EXPECT_EQ(r[d][i], m[i][(i + d) % dim]);
			}
		}

		// Non-square
		EXPECT_THROW(diag(matrix(dim), 0), invalid_argument);
	}

	TEST(PlaintextOperations, DuplicateVector)
	{
		const auto v = random_vector(dim);

		const auto r = duplicate(v);

		ASSERT_EQ(r.size(), 2 * dim);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_EQ(r[i], v[i]);
			EXPECT_EQ(r[dim + i], v[i]);
		}
	}

	TEST(PlaintextOperations, MatrixVectorFromDiagonals)
	{
		const auto m = random_square_matrix(dim);
		const auto v = random_vector(dim);
		const auto expected = mvp(m, v);

		const auto r = mvp_from_diagonals(diagonals(m), v);

		ASSERT_EQ(r.size(), dim);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_FLOAT_EQ(r[i], expected[i]);
		}


		// Mismatching sizes should throw exception
		EXPECT_THROW(mvp_from_diagonals(diagonals(m), {}), invalid_argument);
		EXPECT_THROW(mvp_from_diagonals({}, v), invalid_argument);
		EXPECT_THROW(mvp_from_diagonals(vector(dim, vec()), v), invalid_argument);
	}

	void MatrixVectorBSGS(size_t dimension)
	{
		const auto m = random_square_matrix(dimension);
		const auto v = random_vector(dimension);
		const auto expected = mvp(m, v);

		vec r = mvp_from_diagonals_bsgs(diagonals(m), v);

		ASSERT_EQ(r.size(), dimension);
		for (size_t i = 0; i < dimension; ++i)
		{
			EXPECT_FLOAT_EQ(r[i], expected[i]);
		}
	}

	TEST(PlaintextOperations, MatrixVectorFromDiagonalsBSGS_mismatch)
	{
		// Non-square-number sizes should give errors
		EXPECT_THROW(MatrixVectorBSGS(15), invalid_argument);
		EXPECT_THROW(MatrixVectorBSGS(205), invalid_argument);

		// Mismatching sizes should throw exception, even if some are square numbers
		EXPECT_THROW(mvp_from_diagonals_bsgs(diagonals(random_square_matrix(16)), {}), invalid_argument);
		EXPECT_THROW(mvp_from_diagonals_bsgs({}, random_vector(16)), invalid_argument);
		EXPECT_THROW(mvp_from_diagonals_bsgs(vector(16, vec()), random_vector(16)), invalid_argument);
	}

	TEST(PlaintextOperations, MatrixVectorFromDiagonalsBSGS_16)
	{
		MatrixVectorBSGS(16);
	}

	TEST(PlaintextOperations, MatrixVectorFromDiagonalsBSGS_49)
	{
		MatrixVectorBSGS(49);
	}

	TEST(PlaintextOperations, MatrixVectorFromDiagonalsBSGS_256)
	{
		MatrixVectorBSGS(256);
	}

	TEST(PlaintextOperations, PerfectSquare)
	{
		EXPECT_EQ(perfect_square(49), true);
		EXPECT_EQ(perfect_square(16123 * 16123), true);
		EXPECT_EQ(perfect_square(7 * 5), false);
		EXPECT_EQ(perfect_square(929 * 941), false);
	}

	TEST(PlaintextOperations, RNN_with_ReLU)
	{
		const auto W_h = random_square_matrix(dim);
		const auto W_x = random_square_matrix(dim);
		const auto b = random_vector(dim);
		const auto h = random_vector(dim);
		const auto x = random_vector(dim);

		vec r = rnn_with_relu(x, h, W_x, W_h, b);
		vec t = add(add(mvp(W_x, x), mvp(W_h, h)), b);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_DOUBLE_EQ(r[i], max(0., t[i]));
		}
	}

	TEST(PlaintextOperations, RNN_with_ReLU_identity_matrix)
	{
		const auto b = random_vector(dim);
		const auto h = random_vector(dim);
		const auto x = random_vector(dim);

		vec r = rnn_with_relu(x, h, identity_matrix(dim), identity_matrix(dim), b);
		vec t = add(add(x, h), b);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_DOUBLE_EQ(r[i], max(0., t[i]));
		}
	}

	TEST(PlaintextOperations, RNN_with_ReLU_mismatch)
	{
		const auto W_h = random_square_matrix(dim);
		const auto W_x = random_square_matrix(dim);
		const auto b = random_vector(dim);
		const auto h = random_vector(dim);
		const auto x = random_vector(dim);

		EXPECT_THROW(rnn_with_relu({}, h, W_x, W_h, b),invalid_argument);
		EXPECT_THROW(rnn_with_relu(x, {}, W_x, W_h, b), invalid_argument);
		EXPECT_THROW(rnn_with_relu(x, h, {}, W_h, b), invalid_argument);
		EXPECT_THROW(rnn_with_relu(x, h, W_x, {}, b), invalid_argument);
		EXPECT_THROW(rnn_with_relu(x, h, W_x, W_h, {}), invalid_argument);			
	}
	
	TEST(PlaintextOperations, RNN_with_Squaring)
	{
		const auto W_h = random_square_matrix(dim);
		const auto W_x = random_square_matrix(dim);
		const auto b = random_vector(dim);
		const auto h = random_vector(dim);
		const auto x = random_vector(dim);

		vec r = rnn_with_squaring(x, h, W_x, W_h, b);
		vec t = add(add(mvp(W_x, x), mvp(W_h, h)), b);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_DOUBLE_EQ(r[i], t[i]*t[i]);
		}
	}

	TEST(PlaintextOperations, RNN_with_Squaring_identity_matrix)
	{
		const auto b = random_vector(dim);
		const auto h = random_vector(dim);
		const auto x = random_vector(dim);

		vec r = rnn_with_squaring(x, h, identity_matrix(dim), identity_matrix(dim), b);
		vec t = add(add(x, h), b);
		for (size_t i = 0; i < dim; ++i)
		{
			EXPECT_DOUBLE_EQ(r[i], t[i] * t[i]);
		}
	}

	TEST(PlaintextOperations, RNN_with_Squaring_mismatch)
	{
		const auto W_h = random_square_matrix(dim);
		const auto W_x = random_square_matrix(dim);
		const auto b = random_vector(dim);
		const auto h = random_vector(dim);
		const auto x = random_vector(dim);

		EXPECT_THROW(rnn_with_squaring({}, h, W_x, W_h, b), invalid_argument);
		EXPECT_THROW(rnn_with_squaring(x, {}, W_x, W_h, b), invalid_argument);
		EXPECT_THROW(rnn_with_squaring(x, h, {}, W_h, b), invalid_argument);
		EXPECT_THROW(rnn_with_squaring(x, h, W_x, {}, b), invalid_argument);
		EXPECT_THROW(rnn_with_squaring(x, h, W_x, W_h, {}), invalid_argument);
	}
}
