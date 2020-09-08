#ifndef LAPLACIAN_SHARPENING_H_
#define LAPLACIAN_SHARPENING_H_
#endif

#include <seal/seal.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

typedef std::vector<std::vector<int>> VecInt2D;
#define DEFAULT_NUM_SLOTS 16'384

class Evaluation {
 public:
  static void encryptedLaplacianSharpeningAlgorithmNaive(VecInt2D img);
  int main(int argc, char *argv[]);
};
