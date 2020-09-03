#include <iostream>
#include "e3int.h"
#include "../../src/e3key.h"

int main()
{
  // == inputs ====
  Secure n0 = _2_Ep;
  Secure n1 = _7_Ep;
  Secure n2 = _9_Ep;
  // == //END inputs ====

  Secure alpha_half = ((n0 * n2) << 2) - (n1 * n1);
  Secure alpha = (alpha_half*alpha_half);
  Secure orange = (n0 << 1) + n1;
  Secure beta_1 = (orange * orange) << 1;
  Secure green = (n2 << 1) + n1;
  Secure beta_2 = orange * green;
  Secure beta_3 = (green * green) << 1;

  std::cout << e3::decrypt(alpha) << '\n';
  std::cout << e3::decrypt(beta_1) << '\n';
  std::cout << e3::decrypt(beta_2) << '\n';
  std::cout << e3::decrypt(beta_3) << '\n';
}
