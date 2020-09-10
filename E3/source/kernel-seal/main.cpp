#include <iostream>
#include <vector>

#include "../../src/e3key.h"
#include "e3int.h"

using namespace std;

#ifndef SZ
#define SZ 8
#endif

// using SecureInt = TypeUint<SZ>;
// using SecureBool = TypeBool;
using SecureMint = TypeMint;

int main() {
  // = INPUTS ===
  std::vector<std::vector<SecureMint>> img = {
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea},
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea}, 
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea}, 
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea}, 
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea},
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea}, 
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea}, 
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea}
  };

  std::vector<std::vector<SecureMint>> img2 = img;

  std::vector<std::vector<SecureMint>> weights = {
      {_1_Ea, _1_Ea, _1_Ea}, 
      {_1_Ea, _8_Ean, _1_Ea}, 
      {_1_Ea, _1_Ea, _1_Ea}
  };

  for (size_t x = 1; x < img.size() - 1; x++) {
    for (size_t y = 1; y < img.at(x).size() - 1; y++) {
      SecureMint value = _0_Ea;
      for (size_t i = -1; i < 2; i++) {
        for (size_t j = -1; j < 2; j++) {
          value += weights.at(i+1).at(j+1) * img.at(x + i).at(y + j);
        }
      }
      // img2[x][y] = img[x][y] - (value / 2);
      // modified variant as we cannot divide in FHE:
      img2[x][y] = (img[x][y] << 1) - value;
    }
  }

  for (size_t i = 0; i < img2.size(); i++) {
    for (size_t j = 0; j < img2.at(i).size(); j++) {
     std::cout << "(" << i << "," << j << "): " << e3::decrypt(img2[i][j]) << '\n';
    }
  }
}
