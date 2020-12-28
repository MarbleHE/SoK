// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "eva/ir/program.h"
#include "eva/ir/term_map.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <set>

namespace eva {

class RotationKeysSelector {
public:
  RotationKeysSelector(Program &g, const TermMap<Type> &type)
      : program_(g), type(type) {}

  void operator()(const Term::Ptr &term) {
    auto op = term->op;

    // Nothing to do if this is not a rotation
    if (!isLeftRotationOp(op) && !isRightRotationOp(op)) return;

    // No rotation keys needed for raw computation
    if (type[term] == Type::Raw) return;

    // Add the rotation count
    auto rotation = term->get<RotationAttribute>();
    keys_.insert(isRightRotationOp(op) ? -rotation : rotation);
  }

  void free(const Term::Ptr &term) {
    // No-op
  }

  auto getRotationKeys() {
    // Return the set of rotations needed
    int max = 0;
    for (int i : keys_) {
      max = std::max(max, abs(i));
    }
    std::set<int> steps{};
    for (int i = 1; i <= max; i <<= 1) {
      steps.insert(i);
      steps.insert(-i);
    }
    if (steps.size() < keys_.size()) {
      return steps;
    } else {
      return keys_;
    }
    // For small_mnist_hw, this creates 24 keys, vs standard 31 via return keys_
    // This is actually optimal for the required keys:
    //  -2240    # **-2408** -256 +64
    //  -1120    # **-1024** -128 + 32
    //  -560     # **-512** -64 +16
    //  -320     # **-256** -64
    //  -160     # **-128** -32
    //  -80      # -64 -16
    //  -40      # -32 **-8**
    //  -20      # -16 -4
    //  -1       # **-1**
    //   1       # **1**
    //   2       # **2**
    //   4       # **4**
    //   8       # **8**
    //   16      # **16**
    //   20      # 16 + 4
    //   28      # 16 + 8
    //   29      # **32** **-4**
    //   30      # 32 **-2**
    //   40      # 32 + 8
    //   56      # **64** + 8
    //   57      # 64 + 8 + 1
    //   58      # 64 + 8 + 2
    //   60      # 64 -4
    //   112     # **128** **-16**
    //   114     # 128 - 16 + 2
    //   116     # 128 - 16 + 4
    //   224     # **256** **-32**
    //   448     # **512**
    //   560     # 512 **-64**
    //   1120    # **1024** + 128 -32
    //   2240    # **2408** + 256 -64
  }

private:
  Program &program_;
  const TermMap<Type> &type;
  std::set<int> keys_;

  bool isLeftRotationOp(const Op &op_code) {
    return (op_code == Op::RotateLeftConst);
  }

  bool isRightRotationOp(const Op &op_code) {
    return (op_code == Op::RotateRightConst);
  }
};

} // namespace eva
