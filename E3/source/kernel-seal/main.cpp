#include <iostream>
#include <vector>

#include "../../src/e3key.h"
#include "e3int.h"

using namespace std;

using SecureMint = TypeMint;

void run_kernel(std::vector<std::vector<SecureMint>> img) {
  const size_t image_size = img.size();

  // create a copy of the input image to preserve image's border
  std::vector<std::vector<SecureMint>> output = img;

  // NOTE: We use different weights here compared to the other Kernel
  // implementations because E3 has issues working with negative numbers (i.e.,
  // the weight -8).
  std::vector<std::vector<SecureMint>> wm = {
      {_0_Ea, _0_Ea, _0_Ea}, {_0_Ea, _0_Ea, _1_Ea}, {_0_Ea, _0_Ea, _0_Ea}};

  for (size_t x = 1; x < image_size - 1; x++) {
    for (size_t y = 1; y < img.at(x).size() - 1; y++) {
      SecureMint value = _0_Ea;
      for (size_t j = 0; j < 3; j++) {
        for (size_t i = 0; i < 3; i++) {
          value = value + (wm.at(i ).at(j ) * img.at(x + i-1).at(y + j-1));
        }
      }
      // output[x][y] = img[x][y] - (value / 2);
      // modified variant as we cannot divide in FHE:
      output[x][y] = (2 * img[x][y]) - value;
    }
  }

  for (size_t i = 0; i < output.size(); i++) {
    for (size_t j = 0; j < output.at(i).size(); j++) {
      std::cout << "(" << i << "," << j << "): " << e3::decrypt(output[i][j])
                << '\n';
    }
  }
}

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
      {_0_Ea, _1_Ea, _2_Ea, _3_Ea, _4_Ea, _5_Ea, _6_Ea, _7_Ea}};

  run_kernel(img);
}
