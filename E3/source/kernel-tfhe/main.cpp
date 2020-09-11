#include <iostream>

#include "../../src/e3key.h"
#include "../../tutorials/bench/spec/circuits.inc"
#include "e3int.h"

using SecureInt = TypeUint<8>;
using SecureBool = TypeBool;

int main() {
  // = INPUTS ===
  std::vector<std::vector<SecureInt>> img = {
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep},
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep},
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep},
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep},
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep},
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep},
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep},
      {_0_Ep, _1_Ep, _2_Ep, _3_Ep, _4_Ep, _5_Ep, _6_Ep, _7_Ep}
  };

  std::vector<std::vector<SecureInt>> img2 = img;

  // NOTE: We use different weights here compared to the other Kernel
  // implementations because E3 has issues working with negative numbers.
  std::vector<std::vector<SecureInt>> wm = {
      {_0_Ep, _0_Ep, _0_Ep}, {_0_Ep, _0_Ep, _1_Ep}, {_0_Ep, _0_Ep, _0_Ep}};

  for (size_t x = 1; x < img.size() - 1; x++) {
    for (size_t y = 1; y < img.at(x).size() - 1; y++) {
      SecureInt value = _0_Ep;
      for (size_t j = 0; j < 3; j++) {
        for (size_t i = 0; i < 3; i++) {
          value = value + (wm.at(i).at(j) * img.at(x + i - 1).at(y + j - 1));
        }
      }
      // original:  img2[x][y] = img[x][y] - (value / 2);
      // modified variant as we cannot divide in FHE:
      img2[x][y] = (img[x][y] << 1) - value;
    }
  }

  for (size_t i = 0; i < img2.size(); i++) {
    for (size_t j = 0; j < img2.at(i).size(); j++) {
      std::cout << "(" << i << "," << j << "): " << e3::decrypt(img2[i][j])
                << '\n';
    }
  }
}
