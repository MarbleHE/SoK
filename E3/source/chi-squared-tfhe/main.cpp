#include <iostream>
#include "e3int.h"
#include "../../src/e3key.h"

using SecureInt = TypeUint<16>;

int main()
{
  // == inputs ====
  SecureInt n0 = _2_Ep;
  SecureInt n1 = _7_Ep;
  SecureInt n2 = _9_Ep;
  // == //END inputs ====

  // SecureInt alpha = ((n0 * n2) << 2) - (n1 * n1);

  // SecureInt orange = (2*n0)+n1;
  SecureInt orange = (n0 << 1) + n1;
  // SecureInt beta_1 = 2*(orange*orange);
  SecureInt beta_1 = (orange * orange) << 1;

  // // SecureInt green = (2*n2)+n1;
  // SecureInt green = (n2 << 1) + n1;
  // SecureInt beta_2 = orange * green;

  // // SecureInt beta_3 = 2*(green*green);
  // SecureInt beta_3 = (green * green) << 1;

  // std::cout << e3::decrypt(alpha) << '\n';
  std::cout << e3::decrypt(beta_1) << '\n';
  // std::cout << e3::decrypt(beta_2) << '\n';
  // std::cout << e3::decrypt(beta_3) << '\n';
}
