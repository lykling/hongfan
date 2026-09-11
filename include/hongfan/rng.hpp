// Copyright 2026 Pride Leong.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <cstdint>
#include <limits>

namespace hf {

// SplitMix64：确定性内核的唯一随机源（IMPL.md 附录 A.3）
class Rng {
 public:
  explicit Rng(uint64_t seed) : state_(seed) {}

  uint64_t next_u64() {
    state_ += 0x9E3779B97F4A7C15ULL;
    uint64_t z = state_;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }

  // [0, 1)，53 位精度
  double next_unit() { return static_cast<double>(next_u64() >> 11) * (1.0 / 9007199254740992.0); }

  // 闭区间 [lo, hi]，拒绝采样消模偏
  uint64_t rand_int(uint64_t lo, uint64_t hi) {
    if (hi <= lo)
      return lo;
    const uint64_t range = hi - lo + 1;
    const uint64_t threshold =
        std::numeric_limits<uint64_t>::max() - (std::numeric_limits<uint64_t>::max() % range);
    uint64_t x = next_u64();
    while (x >= threshold)
      x = next_u64();
    return lo + (x % range);
  }

  bool bernoulli(double p) { return next_unit() < p; }

 private:
  uint64_t state_;
};

}  // namespace hf
