// === NOTE =======
// This file is not used during the benchmarking run but serves for
// documentation purposes to see how the bfv-kernel.blif circuit was
// originally created.
// ================

#define blif_name "bfv-kernel.blif"

#include <vector>
// === NOTE =======
// This file is not used during the benchmarking run but serves for
// documentation purposes to see how the bfv-kernel.blif circuit was
// originally created.
// ================

// local includes
#include <bit_exec/tracker.hxx>
#include <ci_context.hxx>
#include <ci_fncs.hxx>
#include <ci_int.hxx>
#include <int_op_gen/mult_depth.hxx>
#include <integer.hxx>

// namespaces
using namespace std;
using namespace cingulata;

int main() {
  // Set context (bit tracker) and mult. depth-minimized integer operations
  CiContext::set_config(make_shared<BitTracker>(),
                        make_shared<IntOpGenDepth>());

  SlicedInteger<int8_t> img[image_size][image_size];
  SlicedInteger<int8_t> out[image_size][image_size];
  SlicedInteger<int8_t> weight[3][3] = {{1, 1, 1}, {1, -8, 1}, {1, 1, 1}};

  // read input data from input/ directory
  for (int i = 0; i < image_size; i++) {
    for (int j = 0; j < image_size; j++) {
      std::cin >> img[i][j];
      out[i][j] = img[i][j];
    }
  }

  // perform computation (kernel)
  for (int x = 1; x < image_size - 1; x++) {
    for (int y = 1; y < image_size - 1; y++) {
      SlicedInteger<int8_t> value = 0;
      for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
          value += weight[i + 1][j + 1] * img[x + i][y + j];
        }
      }
      out[x][y] = (img[x][y] << 1) - value;
    }
  }

  // print result
  for (int i = 0; i < image_size; ++i) {
    for (int j = 0; j < image_size; j++) {
      std::cout << out[i][j];
    }
  }

  // export to file "bfv-kernel.blif"
  CiContext::get_bit_exec_t<BitTracker>()->export_blif(blif_name, "kernel");
}
