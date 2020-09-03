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
  // flags
  SecureMint man = _0_Ea;
  SecureMint not_man = _1_Ea;

  SecureInt antecedent = _1_Ep;
  SecureInt smoking = _1_Ep;
  SecureInt diabetic = _1_Ep;
  SecureInt high_blood_pressure = _1_Ep;

  // values
  SecureInt age = _55_Ep;
  SecureInt hdl_cholesterol = _50_Ep;
  SecureInt height = _80_Ep;
  SecureInt weight = _80_Ep;
  SecureInt weight_plus_ninety = _170_Ep;
  SecureInt phy_activity = _45_Ep;
  SecureInt drinking_habits = _4_Ep;

  // = //END INPUTS ===

  SecureMint one = _1_Ea;
  SecureInt bool_false = _0_Ep;
  SecureInt bool_true = _1_Ep;

  // // +1  if man and age > 50 years
  SecureInt age_threshold = _50_Ep;
  SecureMint result0 = (age > age_threshold) * man;

  // +1  if cardiology disease in family history
  SecureMint result1 = (antecedent == bool_true)*one;

  // +1  if smoking
  SecureMint result2 = (smoking == bool_true)*one;

  // +1  if diabetic
  SecureMint result3 = (diabetic == bool_true)*one;

  // +1  if high blood pressure
  SecureMint result4 = (high_blood_pressure == bool_true)*one;

  // // +1  if HDL cholesterol less than 40
  SecureInt hdl_threshold = _40_Ep;
  SecureMint result5 = (hdl_cholesterol < hdl_threshold) * one;

  // // +1  if weight > height-90 <=> height < weight+90
  SecureMint result6 = (height < weight_plus_ninety) * one;

  // // +1  if daily physical activity less than 30 minutes
  SecureInt phy_activity_threshold = _30_Ep;
  SecureMint result7 = (phy_activity < phy_activity_threshold) * one;

  // // +1  if man and alcohol consumption more than 3 glasses/day
  SecureInt dh_threshold_man = _3_Ep;
  SecureMint result8 = (drinking_habits > dh_threshold_man) * man;
  
  // // // +1  if woman and alcohol consumption more than 2 glasses/day
  SecureInt dh_threshold_woman = _2_Ep;
  SecureMint result9 = (drinking_habits > dh_threshold_woman) * not_man;

  SecureMint resultA = result0 + result1;
  SecureMint resultB = result2 + result3;
  SecureMint resultC = result4 + result5;
  SecureMint resultD = result6 + result7;
  SecureMint resultE = result8 + result9;

  SecureMint resultAlpha = resultA + resultB;
  SecureMint resultBeta = resultC + resultD;
  SecureMint resultGamma = resultE;

  SecureMint resultI = resultAlpha + resultGamma;
  SecureMint resultII = resultBeta;

  SecureMint resultFinal = resultI + resultII;

  std::cout << e3::decrypt(resultFinal) << '\n';
}
