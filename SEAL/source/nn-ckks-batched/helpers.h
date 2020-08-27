// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cstddef>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <random>
#include <memory>
#include <algorithm>
#include "seal/seal.h"


/*
Helper function: Prints the name of the example in a fancy banner.
*/
inline void print_example_banner(std::string title)
{
	if (!title.empty())
	{
		std::size_t title_length = title.length();
		std::size_t banner_length = title_length + 2 * 10;
		std::string banner_top = "+" + std::string(banner_length - 2, '-') + "+";
		std::string banner_middle =
			"|" + std::string(9, ' ') + title + std::string(9, ' ') + "|";

		std::cout << std::endl
			<< banner_top << std::endl
			<< banner_middle << std::endl
			<< banner_top << std::endl;
	}
}

/*
Helper function: Prints the parameters in a SEALContext.
*/
inline void print_parameters(std::shared_ptr<seal::SEALContext> context)
{
	// Verify parameters
	if (!context)
	{
		throw std::invalid_argument("context is not set");
	}
	auto& context_data = *context->key_context_data();

	/*
	Which scheme are we using?
	*/
	std::string scheme_name;
	switch (context_data.parms().scheme())
	{
	case seal::scheme_type::BFV:
		scheme_name = "BFV";
		break;
	case seal::scheme_type::CKKS:
		scheme_name = "CKKS";
		break;
	default:
		throw std::invalid_argument("unsupported scheme");
	}
	std::cout << "/" << std::endl;
	std::cout << "| Encryption parameters :" << std::endl;
	std::cout << "|   scheme: " << scheme_name << std::endl;	
	std::cout << "|   poly_modulus_degree: " <<
		context_data.parms().poly_modulus_degree() << std::endl;
	std::cout << "|   Maximal allowed coeff_modulus bit-count for this poly_modulus_degree: "
		<< seal::CoeffModulus::MaxBitCount(context_data.parms().poly_modulus_degree()) << std::endl;

	/*
	Print the size of the true (product) coefficient modulus.
	*/
	std::cout << "|   actual coeff_modulus size: ";
	std::cout << context_data.total_coeff_modulus_bit_count() << " (";
	auto coeff_modulus = context_data.parms().coeff_modulus();
	std::size_t coeff_mod_count = coeff_modulus.size();
	for (std::size_t i = 0; i < coeff_mod_count - 1; i++)
	{
		std::cout << coeff_modulus[i].bit_count() << " + ";
	}
	std::cout << coeff_modulus.back().bit_count();
	std::cout << ") bits" << std::endl;

	/*
	For the BFV scheme print the plain_modulus parameter.
	*/
	if (context_data.parms().scheme() == seal::scheme_type::BFV)
	{
		std::cout << "|   plain_modulus: " << context_data.
			parms().plain_modulus().value() << std::endl;
	}

	std::cout << "\\" << std::endl;
}

/*
Helper function: Prints the `parms_id' to std::ostream.
*/
inline std::ostream& operator <<(std::ostream& stream, seal::parms_id_type parms_id)
{
	/*
	Save the formatting information for std::cout.
	*/
	std::ios old_fmt(nullptr);
	old_fmt.copyfmt(std::cout);

	stream << std::hex << std::setfill('0')
		<< std::setw(16) << parms_id[0] << " "
		<< std::setw(16) << parms_id[1] << " "
		<< std::setw(16) << parms_id[2] << " "
		<< std::setw(16) << parms_id[3] << " ";

	/*
	Restore the old std::cout formatting.
	*/
	std::cout.copyfmt(old_fmt);

	return stream;
}

/*
Helper function: Prints a vector of floating-point values.
*/
template<typename T>
inline void print_vector(std::vector<T> vec, std::string msg = "", std::size_t print_size = 4, int prec = 3)
{
	/*
	Save the formatting information for std::cout.
	*/
	std::ios old_fmt(nullptr);
	old_fmt.copyfmt(std::cout);

	std::size_t slot_count = vec.size();

	std::cout << std::fixed << std::setprecision(prec);
	std::cout << msg << std::endl;
	if (slot_count <= 2 * print_size)
	{
		std::cout << "    [";
		for (std::size_t i = 0; i < slot_count; i++)
		{
			std::cout << " " << vec[i] << ((i != slot_count - 1) ? "," : " ]\n");
		}
	}
	else
	{
		vec.resize(std::max(vec.size(), 2 * print_size));
		std::cout << "    [";
		for (std::size_t i = 0; i < print_size; i++)
		{
			std::cout << " " << vec[i] << ",";
		}
		if (vec.size() > 2 * print_size)
		{
			std::cout << " ...,";
		}
		for (std::size_t i = slot_count - print_size; i < slot_count; i++)
		{
			std::cout << " " << vec[i] << ((i != slot_count - 1) ? "," : " ]\n");
		}
	}
	std::cout << std::endl;

	/*
	Restore the old std::cout formatting.
	*/
	std::cout.copyfmt(old_fmt);
}


/*
Helper function: Prints a matrix of values.
*/
template<typename T>
inline void print_matrix(std::vector<std::vector<T>> matrix, std::string msg = "", std::size_t print_size = 4, int prec = 3)
{
	/*
	We're not going to print every entry, instead we
	print this many elements per row/column at the beginning and end.
	*/
	print_size = std::min(matrix.size(), print_size);

	std::cout << msg << std::endl;
	for (std::size_t i = 0; i < print_size; i++)
	{
		//TODO: This is ugly. Instead have the print functions return a stream?
		//std::cout << std::setw(3) << std::right;
		print_vector(matrix[i], "", print_size, prec);
		//std::cout << "," << std::endl;
	}
	if (matrix.size() - print_size > 0) {
		std::cout << std::setw(3) << "    ... " << std::endl;

		for (std::size_t i = matrix.size() - print_size; i < matrix.size(); i++)
		{
			//TODO: This is ugly. Instead have the print functions return a stream?
			//std::cout << std::setw(3) << std::right;
			print_vector(matrix[i], "", print_size, prec);
			//std::cout << "," << std::endl;
		}
	}
}

/*
Helper function: Print line number.
*/
inline void print_line(int line_number)
{
	std::cout << "Line " << std::setw(3) << line_number << " --> ";
}