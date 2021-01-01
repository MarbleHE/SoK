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

#include <iostream>

/* local includes */
#include <tfhe_bit_exec.hxx>
#include <ci_context.hxx>
#include <ci_int.hxx>


/* namespaces */
using namespace std;
using namespace cingulata;

int main(int argc, char *argv[]) {

  std::vector<int> KS = {241, 210, 225, 219, 92, 43, 197};

  /* get input values */
  CiInt flags{15 ^ KS[0], 5, false};
  CiInt age{55 ^ KS[1], 8, false};
  CiInt hdl{50 ^ KS[2], 8, false};
  CiInt height{80 ^ KS[3], 8, false};
  CiInt weight{80 ^ KS[4], 8, false};
  CiInt physical_act{45 ^ KS[5], 8, false};
  CiInt drinking{4 ^ KS[6], 8, false};

  std::vector<CiInt> ks;
  for (auto K : KS) {
    ks.emplace_back(K, 8, false);
  }

  /* Only tfhe bit executor is needed for encryption/decryption and IO operations  */
  CiContext::set_bit_exec(make_shared<TfheBitExec>("tfhe.sk", TfheBitExec::Secret));

  flags.encrypt().write("flags");
  age.encrypt().write("age");
  hdl.encrypt().write("hdl");
  height.encrypt().write("height");
  weight.encrypt().write("weight");
  physical_act.encrypt().write("physical_act");
  drinking.encrypt().write("drinking");
  for (int i = 0; i < ks.size(); ++i) {
    ks[i].encrypt().write("ks_" + to_string(i));
  }
}
