#ifndef LAPLACIAN_SHARPENING_BATCHED_H_
#define LAPLACIAN_SHARPENING_BATCHED_H_
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

class Evaluation {
 public:
  static void encryptedLaplacianSharpeningAlgorithmBatched(VecInt2D img);
  int main(int argc, char *argv[]);
};
