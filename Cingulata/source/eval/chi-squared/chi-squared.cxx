/*
    (C) Copyright 2017 CEA LIST. All Rights Reserved.
    Contributor(s): Cingulata team (formerly Armadillo team)

    This software is governed by the CeCILL-C license under French law and
    abiding by the rules of distribution of free software.  You can  use,
    modify and/ or redistribute the software under the terms of the CeCILL-C
    license as circulated by CEA, CNRS and INRIA at the following URL
    "http://www.cecill.info".

    As a counterpart to the access to the source code and  rights to copy,
    modify and redistribute granted by the license, users are provided only
    with a limited warranty  and the software's author,  the holder of the
    economic rights,  and the successive licensors  have only  limited
    liability.

    The fact that you are presently reading this means that you have had
    knowledge of the CeCILL-C license and that you accept its terms.
*/

// === NOTE =======
// This file is not used during the benchmarking run but serves for
// documentation purposes to see how the bfv-chi-squared.blif circuit was
// originally created.
// ================

#define blif_name "bfv-chi-squared.blif"

#include <vector>

/* local includes */
#include <bit_exec/tracker.hxx>
#include <ci_context.hxx>
#include <ci_fncs.hxx>
#include <ci_int.hxx>
#include <int_op_gen/mult_depth.hxx>
#include <int_op_gen/size.hxx>


/* namespaces */
using namespace std;
using namespace cingulata;

int main() {
  /* Set context to bit tracker and multiplicative depth minimized integer
   * operations */
  CiContext::set_config(make_shared<BitTracker>(),
                        make_shared<IntOpGenSize>());

  // Creates a 5-bit flags variable initialized with 0.
  // CiInt flags{0, 5};  // 5 flags
  // flags.read("flags");

  // Declares variables for conditions with each 8 bit.
  CiInt n0{CiInt::u8};
  CiInt n1{CiInt::u8};
  CiInt n2{CiInt::u8};

  // Fills conditions with values from file.
  n0.read("n0");
  n1.read("n1");
  n2.read("n2");

  CiInt alpha{CiInt::u16};
  alpha = ((n0<<2)*n2)-(n1*n1);
  alpha = alpha * alpha;

  CiInt beta_1{CiInt::u16};
  CiInt orange{CiInt::u16};
  orange = ((n0<<1)+n1);
  beta_1 = (orange*orange)<<1;

  CiInt beta_2{CiInt::u16};
  CiInt green{CiInt::u16};
  green = ((n2<<1)+n1);
  beta_2 = orange*green;

  CiInt beta_3{CiInt::u16};
  beta_3 = (green * green)<<1;

  alpha.write("alpha");
  beta_1.write("beta1");
  beta_2.write("beta2");
  beta_3.write("beta3");

  /* Export to file the "tracked" circuit */
  CiContext::get_bit_exec_t<BitTracker>()->export_blif(blif_name, "chi_squared");
}
