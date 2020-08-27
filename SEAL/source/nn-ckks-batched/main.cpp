#include "nn-batched.h"

int main(int argc, char *argv[]) {
  std::cout << "Starting benchmark 'nn-batched-ckks'..." << std::endl;
  NNBatched().run_nn();
  return 0;
}