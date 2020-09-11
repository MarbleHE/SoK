#include <iostream>
#include <vector>

#include "../../src/e3key.h"
#include "e3int.h"

using namespace std;

using SecureMint = TypeMint;

int main() {
  // ==== INPUTS ===============================================================
  // Note: Last element (0) is just added to have zero in the remaining slots.

  std::vector<SecureMint> rotated_ctxts;

  // As computation of "rotated input image" * "i-th weight" does not place the
  // result value at the expected position and we cannot rotate it as it is not
  // supported by E3 yet, we instead take an already rotated input image so that
  // rotating the result of "rotated input image" * "i-th weight" is not
  // required anymore.
  SecureMint img_rotated =
      _0_0_0_0_0_0_0_0_0_0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea;

  // manually rotated rotated_ctxts[0] copies as E3 does not support rotations

  // rotated by 0 (original):  A 8x8 pixels image with image_size=8.
  rotated_ctxts.push_back(
      _0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);

  // rotated by -1
  rotated_ctxts.push_back(
      _0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);
  
  // rotated by -2
  rotated_ctxts.push_back(
      _0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);

  // rotated by -(image_size)
  rotated_ctxts.push_back(
      _0_0_0_0_0_0_0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);

  // rotated by -(image_size+1)
  rotated_ctxts.push_back(
      _0_0_0_0_0_0_0_0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);

  // rotated by -(image_size+2)
  rotated_ctxts.push_back(
      _0_0_0_0_0_0_0_0_0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);

  // rotated by -(2*image_size)
  rotated_ctxts.push_back(
      _0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);
  
  // rotated by -(2*image_size+1)
  rotated_ctxts.push_back(
      _0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);
    
  // rotated by -(2*image_size+2)
  rotated_ctxts.push_back(
      _0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_1_2_3_4_5_6_7_0_Ea);

  // ==== END: INPUTS ==========================================================

  // the matrix of the kernel weights
  std::vector<SecureMint> wm = {_1_Ea, _1_Ea, _1_Ea, 
                                _1_Ea, -SecureMint(_8_Ea), _1_Ea, 
                                _1_Ea, _1_Ea, _1_Ea};

  // sum up the products between the i-th rotated ciphertexts and the i-th
  // weight vector
  SecureMint value = _0_Ea;
  for (size_t i = 0; i < rotated_ctxts.size(); i++) {
    value += wm[i] * rotated_ctxts[i];
  }

  // set all slots in the result vector 'value' to 0 that are not of our
  // interest but were computed as byproduct
  SecureMint extract_values_mask =
      _0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_Ea;
  SecureMint masked_values = value * extract_values_mask;

  SecureMint img_center_mask =
      _0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_1_1_1_1_1_1_0_0_0_0_0_0_0_0_0_0_0_Ea;

  SecureMint img_border_mask =
      _0_0_0_0_0_0_0_0_0_0_0_1_1_1_1_1_1_1_1_0_0_0_0_0_0_0_1_1_0_0_0_0_0_0_1_1_0_0_0_0_0_0_1_1_0_0_0_0_0_0_1_1_0_0_0_0_0_0_1_1_0_0_0_0_0_0_1_1_1_1_1_1_1_1_1_0_Ea;

  // extract only the inner value of the rotated input image (as the kernel does
  // not modify the image's border)
  SecureMint img_center_only = img_rotated * img_center_mask;

  // compute the value img2[x][y], i.e., the output pixel at (x,y)
  // originaL img2[x][y] = img[x][y] - (value / 2);
  // modified variant as we cannot divide in FHE:
  SecureMint final = (2 * img_center_only) - masked_values;

  // extract the border from the original image that must be preserved
  SecureMint img_border_only = img_rotated * img_border_mask;

  // merge the input image's border with the computed kernel values
  SecureMint output_img = img_border_only + final;

  std::cout << e3::decrypt(output_img) << '\n';
  return 0;
}
