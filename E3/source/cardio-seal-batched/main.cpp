#include <iostream>
#include "e3int.h"
#include "../../src/e3key.h"

using namespace std;

#ifndef SZ
#define SZ 8
#endif

using SecureInt  = TypeUint<SZ>;
using SecureBool = TypeBool;
using SecureMint = TypeMint;


int main()
{
  // = INPUTS ===
  // man, antecedent, smoking, diabetic, high_blood_pressure, man, !man, 1, 1, 1
  SecureMint lhs = _0_1_1_1_1_1_0_1_1_1_0_Ea;

  // 50, 0, 0, 0, 0, 3, 2, HDL, height, phy_act
  SecureInt mid = _50_0_0_0_0_3_2_50_80_45_Ep;

  // age, 1, 1, 1, 1, alc_consumption, alc_consumption, 40, weight+90, 30 
  SecureInt rhs = _55_1_1_1_1_4_4_40_170_30_Ep;

  // = //END INPUTS ===

  std::cout << e3::decrypt(lhs * (mid < rhs)) << '\n';
}
