#include "gtest/gtest.h"
#include "../matrix_vector_crypto.h"

using namespace std;
using namespace seal;

namespace MVCryptoTests {

	/**
	 * \brief Helper function to test plaintext-matrix-encrypted-vector products.
	 * \param n Length of vector and second dimension of matrix
	 * \param bsgs Whether or not to use the baby-step giant-step algorithm
	 * \param m Second dimension of matrix. If m != 0, we use general MVP
	 * \throws std::invalid_argument if both bsgs and m != 0
	 */
	void MatrixVectorProductTest(size_t n, bool bsgs = false, size_t m = 0)
	{
        if (bsgs && m) {
          throw std::invalid_argument("Cannot enable BSGS for general setting");
        }
        matrix M;
        bool general = false;
        if (m) {
          M = random_matrix(m,n);
          general = true;
        } else {
          M = random_square_matrix(n);
          m = n;
        }
		const auto v = random_vector(n);
		const auto expected = mvp(M, v);



		// Setup SEAL Parameters
		EncryptionParameters params(scheme_type::CKKS);
		const double scale = pow(2.0, 40);
		params.set_poly_modulus_degree(8192);
		params.set_coeff_modulus(CoeffModulus::Create(8192, { 50, 40, 50 }));
		auto context = SEALContext::Create(params);


		// Generate required keys
		KeyGenerator keygen(context);
		auto public_key = keygen.public_key();
		auto secret_key = keygen.secret_key();
		auto relin_keys = keygen.relin_keys_local();
		auto galois_keys = keygen.galois_keys_local();

		Encryptor encryptor(context, public_key);
		encryptor.set_secret_key(secret_key);
		Decryptor decryptor(context, secret_key);
		CKKSEncoder encoder(context);
		Evaluator evaluator(context);

		// Encode matrix
		vector<Plaintext> ptxt_diagonals(m);
		for (size_t i = 0; i < m; ++i)
		{
			encoder.encode(diag(M, i), scale, ptxt_diagonals[i]);
		}

		// Decode and compare
		for (size_t i = 0; i < m; ++i)
		{
			vec t;
			encoder.decode(ptxt_diagonals[i], t);
			t.resize(n);
			for (size_t j = 0; j < n; ++j)
			{
				// Test if value is within 0.1% of the actual value or 10 sig figs
				EXPECT_NEAR(t[j], diag(M, i)[j], max(0.00000001, 0.001 * diag(M, i)[j]));
			}
		}

		// Encrypt vector
		Plaintext ptxt_v;

		// Do we need to duplicate elements in the vector during encoding to ensure meaningful rotations?
		if ((params.poly_modulus_degree() / 2) != n) {
			encoder.encode(duplicate(v), pow(2.0, 40), ptxt_v);
		}
		else
		{
			encoder.encode(v, pow(2.0, 40), ptxt_v);
		}
		Ciphertext ctxt_v;
		encryptor.encrypt_symmetric(ptxt_v, ctxt_v);

		// Decrypt and compare
		// Compute MVP
		Ciphertext ctxt_r;
		if (general) {
            ptxt_general_matrix_enc_vector_product(galois_keys,evaluator,encoder,m,n,diagonals(M),ctxt_v,ctxt_r);
		}
		else if (bsgs)
		{
			ptxt_matrix_enc_vector_product_bsgs(galois_keys, evaluator, encoder, n, diagonals(M), ctxt_v, ctxt_r);
		}
		else
		{
			ptxt_matrix_enc_vector_product(galois_keys, evaluator, n, ptxt_diagonals, ctxt_v, ctxt_r);
		}


		// Decrypt and decode result
		Plaintext ptxt_r;
		decryptor.decrypt(ctxt_r, ptxt_r);
		vec r;
		encoder.decode(ptxt_r, r);
		r.resize(m);

		for (size_t i = 0; i < m; ++i)
		{
			// Test if value is within 0.1% of the actual value or 5 sig figs
			EXPECT_NEAR(r[i], expected[i], max(0.0001, abs(0.001 * expected[i])));
		}
	}

	TEST(EncryptedMVP, MatrixVectorProduct_15)
	{
		MatrixVectorProductTest(15);
	}

	TEST(EncryptedMVP, MatrixVectorProduct_256)
	{
		MatrixVectorProductTest(256);
	}

	TEST(EncryptedMVP, MatrixVectorProductBSGS_15)
	{
		// BSGS currently only supports square-number  dimensions
		EXPECT_THROW(MatrixVectorProductTest(15, true), invalid_argument);
	}

	TEST(EncryptedMVP, MatrixVectorProductBSGS_4)
	{
		MatrixVectorProductTest(4, true);
	}

	TEST(EncryptedMVP, MatrixVectorProductBSGS_16)
	{
		MatrixVectorProductTest(16, true);
	}

	TEST(EncryptedMVP, MatrixVectorProductBSGS_49)
	{
		MatrixVectorProductTest(49, true);
	}
	TEST(EncryptedMVP, MatrixVectorProductBSGS_256)
	{
		MatrixVectorProductTest(256, true);
	}

	// This test would be nice to have, but takes an unreasonably long time to complete on a desktop PC
	// TEST(EncryptedMVP, MatrixVectorProductBSGS_exact_slots)
	// {
	// 	MatrixVectorProductTest(4096, true);
	// }

	TEST(EncryptedMVP, MatrixVectorProductBSGS_5000)
	{
		// Since this is neither the number of slots, nor does it fit if duplicated, this should fail
		EXPECT_THROW(MatrixVectorProductTest(5000, true), invalid_argument);
	}


    TEST(EncryptedGeneralMVP, MatrixVectorProduct_4)
    {
      MatrixVectorProductTest(4, false, 4);
    }
    
    TEST(EncryptedGeneralMVP, MatrixVectorProduct_16)
    {
      MatrixVectorProductTest(16, false, 16);
    }
    
    TEST(EncryptedGeneralMVP, MatrixVectorProduct_49)
    {
      MatrixVectorProductTest(49, false, 49);
    }
//    TEST(EncryptedGeneralMVP, MatrixVectorProduct_256)
//    {
//      MatrixVectorProductTest(256, false, 256);
//    }

    TEST(EncryptedGeneralMVP, MatrixVectorProduct_Simple)
    {
      MatrixVectorProductTest(4, false, 2);
      MatrixVectorProductTest(8, false, 4);
      MatrixVectorProductTest(8, false, 2);
    }

    TEST(EncryptedGeneralMVP, MatrixVectorProduct_32_1024)
    {
      MatrixVectorProductTest(1024, false, 32);
    }

    TEST(EncryptedGeneralMVP, MatrixVectorProduct_16_32)
    {
      MatrixVectorProductTest(32, false, 16);
    }


	/**
	 * \brief Helper function to test RNN cell.
	 * \param dimension Length of vector and dimension of matrix
	 */
	void RNNTest(size_t dimension)
	{
		const auto W_h = random_square_matrix(dimension);
		const auto W_x = random_square_matrix(dimension);
		const auto b = random_vector(dimension);
		const auto h = random_vector(dimension);
		const auto x = random_vector(dimension);
		const auto expected = rnn_with_squaring(x, h, W_x, W_h, b);

		// Setup SEAL Parameters
		EncryptionParameters params(scheme_type::CKKS);
		const double scale = pow(2.0, 40);
		params.set_poly_modulus_degree(8192);
		params.set_coeff_modulus(CoeffModulus::Create(8192, { 60, 40, 40, 60 }));
		auto context = SEALContext::Create(params);


		// Generate required keys
		KeyGenerator keygen(context);
		auto public_key = keygen.public_key();
		auto secret_key = keygen.secret_key();
		auto relin_keys = keygen.relin_keys_local();
		auto galois_keys = keygen.galois_keys_local();

		Encryptor encryptor(context, public_key);
		encryptor.set_secret_key(secret_key);
		Decryptor decryptor(context, secret_key);
		CKKSEncoder encoder(context);
		Evaluator evaluator(context);
		
		// Encrypt vectors
		Plaintext ptxt_x;
		Plaintext ptxt_h;

		// Do we need to duplicate elements in the diagonals vectors during encoding to ensure meaningful rotations?
		if ((params.poly_modulus_degree() / 2) != dimension) {
			encoder.encode(duplicate(x), pow(2.0, 40), ptxt_x);
			encoder.encode(duplicate(h), pow(2.0, 40), ptxt_h);
		}
		else
		{
			encoder.encode(x, pow(2.0, 40), ptxt_x);
			encoder.encode(h, pow(2.0, 40), ptxt_h);
		}
		Ciphertext ctxt_x;
		Ciphertext ctxt_h;
		encryptor.encrypt_symmetric(ptxt_x, ctxt_x);
		encryptor.encrypt_symmetric(ptxt_h, ctxt_h);

		// Decrypt and compare
		// TODO: Decrypt and check vector comes out alright.

		// Compute RNN cell	
		ptxt_weights_enc_input_rnn(galois_keys, evaluator, encoder, dimension, diagonals(W_x), diagonals(W_h), b, ctxt_x, ctxt_h);
		
		// Decrypt and decode result
		Plaintext ptxt_r;
		decryptor.decrypt(ctxt_h, ptxt_r);
		vec r;
		encoder.decode(ptxt_r, r);
		r.resize(dimension);

		for (size_t i = 0; i < dimension; ++i)
		{
			// Test if value is within 0.1% of the actual value or 10 sig figs
			EXPECT_NEAR(r[i], expected[i], max(0.000000001, 0.001 * expected[i]));
		}

		//TODO: The EXPECT_FLOAT_EQ assertions might occasionally fail since the noise is somewhat random and we get less than 32 bits of guaranteed precision from these parameters
	}

	TEST(EncryptedRNN, RNN_4)
	{
		RNNTest(4);
	}

	TEST(EncryptedRNN, RNN_15)
	{
		EXPECT_THROW(RNNTest(15),invalid_argument);
	}

	TEST(EncryptedRNN, RNN_16)
	{
		RNNTest(16);
	}

	TEST(EncryptedRNN, RNN_49)
	{
		RNNTest(49);
	}

	TEST(EncryptedRNN, RNN_256)
	{
		RNNTest(256);
	}
}