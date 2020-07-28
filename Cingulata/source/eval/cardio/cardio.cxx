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

#define blif_name "bfv-cardio.blif"

#include <vector>

/* local includes */
#include <bit_exec/tracker.hxx>
#include <ci_context.hxx>
#include <ci_fncs.hxx>
#include <ci_int.hxx>
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
  /* Set context to bit tracker and multiplicative depth minimized integer
   * operations */
  CiContext::set_config(make_shared<BitTracker>(),
                        make_shared<IntOpGenDepth>());

  // Creates a 5-bit flags variable initialized with 0.
  CiInt flags{0, 5};  // 5 flags
  flags.read("flags");

  // Declares variables for conditions with each 8 bit.
  CiInt age{CiInt::u8};
  CiInt hdl{CiInt::u8};
  CiInt height{CiInt::u8};
  CiInt weight{CiInt::u8};
  CiInt physical_act{CiInt::u8};
  CiInt drinking{CiInt::u8};

  // Fills conditions with values from file.
  age.read("age");
  hdl.read("hdl");
  height.read("height");
  weight.read("weight");
  physical_act.read("physical_act");
  drinking.read("drinking");

  vector<CiInt> keystream(7, CiInt::u8);
  // Read the pre-calculated keystream.
  for (int i = 0; i < 7; i++) {
    keystream[i].read("ks_" + to_string(i));
  }

  // keystream (see run.sh.in): KS=(241 210 225 219 92 43 197)
  //    keystream[0] = 241  = 1111 0001
  //    keystream[1] = 210  = 1101 0010
  //    keystream[2] = 225  = 1110 0001
  //    keystream[3] = 219  = 1101 1011
  //    keystream[4] = 92   = 0101 1100
  //    keystream[5] = 43   = 0010 1011
  //    keystream[6] = 197  = 1100 0101

  // symmetrically encrypt the input
  for (int i = 0; i < 5; i++) { flags[i] ^= keystream[0][i];}
  age ^= keystream[1];
  hdl ^= keystream[2];
  height ^= keystream[3];
  weight ^= keystream[4];
  physical_act ^= keystream[5];
  drinking ^= keystream[6];

  vector<CiInt> risk_factors;
 
  // man == true && age > 50
  risk_factors.push_back(select((flags[SEX_FIELD]) && (age > 50), 1, 0));
  // woman == true && age > 60
  risk_factors.push_back(select((!flags[SEX_FIELD]) && (age > 60), 1, 0));

  // cardiology disease in family history == true
  risk_factors.push_back(flags[ANTECEDENT_FIELD]);
  // smoking == true
  risk_factors.push_back(flags[SMOKER_FIELD]);
  // diabetic == true
  risk_factors.push_back(flags[DIABETES_FIELD]);
  // high blood pressure == true
  risk_factors.push_back(flags[PRESSURE_FIELD]);
  
  // HDL cholesterol < 40
  risk_factors.push_back(select(hdl < 40, 1, 0));

  // weight > height-90
  risk_factors.push_back(select(weight - 10 > height, 1, 0));

  // daily physical activity < 30 min
  risk_factors.push_back(select(physical_act < 30, 1, 0));

  // man && alcohol consumption > 3 glasses/day
  risk_factors.push_back(select((flags[SEX_FIELD]) && (drinking > 3), 1, 0));
  // woman && alcohol consumption > 2 glasses/day
  risk_factors.push_back(select((!flags[SEX_FIELD]) && (drinking > 2), 1, 0));

  CiInt risk = sum(risk_factors);

  risk.write("risk");

  /* Export to file the "tracked" circuit */
  CiContext::get_bit_exec_t<BitTracker>()->export_blif(blif_name, "cardio");
}
