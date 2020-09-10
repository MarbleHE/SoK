// === NOTE =======
// This file is not used during the benchmarking run but serves for
// documentation purposes to see how the bfv-kernel.blif circuit was
// originally created.
// ================

#define blif_name "bfv-kernel.blif"

#include <vector>

/* local includes */
#include <bit_exec/tracker.hxx>
#include <ci_context.hxx>
#include <ci_fncs.hxx>
#include <ci_int.hxx>
#include <int_op_gen/mult_depth.hxx>
#include <integer.hxx>

/* namespaces */
using namespace std;
using namespace cingulata;

// NOTE: This must correspond to IMAGE_SIZE in run.sh
const static int image_size = 8;

int main() {
  /* Set context to bit tracker and multiplicative depth-minimized integer
   * operations */
  CiContext::set_config(make_shared<BitTracker>(),
                        make_shared<IntOpGenDepth>());

  SlicedInteger<int8_t> img[image_size][image_size];
  SlicedInteger<int8_t> out[image_size][image_size];
  SlicedInteger<int8_t> weight[3][3] = {{1, 1, 1}, {1, -8, 1}, {1, 1, 1}};

  for (int i = 0; i < image_size; i++) {
    for (int j = 0; j < image_size; j++) {
      std::cin >> img[i][j];
      out[i][j] = img[i][j];
    }
  }

  for (int x = 0; x < image_size - 2; x++) {
    for (int y = 0; y < image_size - 2; y++) {
      SlicedInteger<int8_t> value = 0;
      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          value += weight[i + 1][j + 1] * img[x + i][y + i];
        }
      }
      out[x][y] = (img[x][y] << 1) - value;
    }
  }

  for (int i = 0; i < image_size; ++i) {
    for (int j = 0; j < image_size; j++) {
      std::cout << out[i][j];
    }
  }

  /* Export to file the "tracked" circuit */
  CiContext::get_bit_exec_t<BitTracker>()->export_blif(blif_name, "kernel");
}
