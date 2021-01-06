/*
    (C) Copyright 2019 CEA LIST. All Rights Reserved.
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

#include <vector>

/* local includes */
#include <bit_exec/decorator/attach.hxx>
#include <bit_exec/decorator/stat.hxx>
#include <ci_context.hxx>
#include <ci_fncs.hxx>
#include <ci_int.hxx>
#include <int_op_gen/size.hxx>
#include <tfhe_bit_exec.hxx>
#include <int_op_gen/mult_depth.hxx>

/* namespaces */
using namespace std;
using namespace cingulata;

#define SEX_FIELD 0
#define ANTECEDENT_FIELD 1
#define SMOKER_FIELD 2
#define DIABETES_FIELD 3
#define PRESSURE_FIELD 4

int main() {
  /* Set context to tfhe bit executor and size minimized integer
   * operations */
  CiContext::set_config(
      make_shared<decorator::Attach<TfheBitExec, decorator::Stat<IBitExecFHE>>>(
          "tfhe.pk", TfheBitExec::Public),
      make_shared<IntOpGenSize>());

  // Since the inputs are actually encrypted under a symmetric KS,
  // and not under FHE, they are provided here as plaintexts
  std::vector<int> KS = {241, 210, 225, 219, 92, 43, 197};
  CiInt flags{15 ^ KS[0], 5, false};
  CiInt age{55 ^ KS[1], 8, false};
  CiInt hdl{50 ^ KS[2], 8, false};
  CiInt height{80 ^ KS[3], 8, false};
  CiInt weight{80 ^ KS[4], 8, false};
  CiInt physical_act{45 ^ KS[5], 8, false};
  CiInt drinking{4 ^ KS[6], 8, false};

  vector<CiInt> keystream(7, CiInt::u8);
  // Read the pre-calculated and encrypted keystream.
  for (int i = 0; i < 7; i++)
    keystream[i].read("ks_" + to_string(i));

  // Homomorphically decrypt the KS-encrypted inputs
  // to give an FHE ctxt that encrypts the (KS-free) ptxt message
  for (int i = 0; i < 5; i++)
    flags[i] ^= keystream[0][i];
  age ^= keystream[1];
  hdl ^= keystream[2];
  height ^= keystream[3];
  weight ^= keystream[4];
  physical_act ^= keystream[5];
  drinking ^= keystream[6];

  vector<CiInt> risk_factors;

  risk_factors.emplace_back(flags[SEX_FIELD] && (age > 50)); // true
  risk_factors.emplace_back(!flags[SEX_FIELD] && (age > 60)); // false

  risk_factors.emplace_back(flags[ANTECEDENT_FIELD]); //true
  risk_factors.emplace_back(flags[SMOKER_FIELD]);  //true
  risk_factors.emplace_back(flags[DIABETES_FIELD]); //true
  risk_factors.emplace_back(flags[PRESSURE_FIELD]); //false

  risk_factors.emplace_back(hdl < 40); //false

  risk_factors.emplace_back(weight - CiInt{10, 8, false} > height); //false
  //WARNING: Without the explicit cast to CiInt with 8 bits, this does something funny/wrong!

  risk_factors.emplace_back(physical_act < 30); //false

  risk_factors.emplace_back(flags[SEX_FIELD] && (drinking > 3)); //true
  risk_factors.emplace_back(!flags[SEX_FIELD] && (drinking > 2)); //false

  CiInt risk = sum(risk_factors);

  risk.write("risk");

  CiContext::get_bit_exec_t<decorator::Stat<IBitExecFHE>>()->print();
}
