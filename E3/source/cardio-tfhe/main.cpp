#include <iostream>

#include "../../src/e3key.h"
#include "../../tutorials/bench/spec/circuits.inc"
#include "e3int.h"

using SecureInt = TypeUint<8>;
using SecureBool = TypeBool;

template <class T>
inline std::string de(T x) {
  return e3::decrypt<T>(x);
}

int main() {
  // flags
  SecureInt man = _0_Ep;
  SecureInt antecedent = _1_Ep;
  SecureInt smoking = _1_Ep;
  SecureInt diabetic = _1_Ep;
  SecureInt high_blood_pressure = _1_Ep;

  // values
  SecureInt age = _55_Ep;
  SecureInt hdl_cholesterol = _50_Ep;
  SecureInt height = _80_Ep;
  SecureInt weight = _80_Ep;
  SecureInt phy_activity = _45_Ep;
  SecureInt drinking_habits = _4_Ep;

  SecureInt result = _0_Ep;
  SecureInt one = _1_Ep;
  SecureInt zero = _0_Ep;

  // +1  if man and age > 50 years
  SecureInt age_threshold = _50_Ep;
  result += (man == one && age > age_threshold) * one;

  // +1  if cardiology disease in family history
  result += (antecedent)*one;

  // +1  if smoking
  result += (smoking)*one;

  // +1  if diabetic
  result += (diabetic)*one;

  // +1  if high blood pressure
  result += (high_blood_pressure)*one;

  // +1  if HDL cholesterol less than 40
  SecureInt hdl_threshold = _40_Ep;
  result += (hdl_cholesterol < hdl_threshold) * one;

  // +1  if weight > height-90
  SecureInt height_minus_operand = _90_Ep;
  result += (height < weight + height_minus_operand) * one;

  // +1  if daily physical activity less than 30 minutes
  SecureInt phy_activity_threshold = _30_Ep;
  result += (phy_activity < phy_activity_threshold) * one;

  // +1  if man and alcohol consumption more than 3 glasses/day
  SecureInt dh_threshold_man = _3_Ep;
  result += (man == one && drinking_habits > dh_threshold_man) * one;

  // +1  if woman and alcohol consumption more than 2 glasses/day
  SecureInt dh_threshold_woman = _2_Ep;
  result += (man == zero && drinking_habits > dh_threshold_woman) * one;

  std::cout << e3::decrypt(result) << '\n';
}
