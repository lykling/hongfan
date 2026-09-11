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

#include "hongfan/policy.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include "hongfan/rng.hpp"

namespace hf {
namespace {

class FirstPolicy final : public OnlinePolicy {
 public:
  std::optional<size_t> pick(std::span<const Candidate> cands, Rng& /*rng*/) override {
    if (cands.empty()) {
      return std::nullopt;
    }
    return size_t{0};
  }
};

class RandomPolicy final : public OnlinePolicy {
 public:
  std::optional<size_t> pick(std::span<const Candidate> cands, Rng& rng) override {
    if (cands.empty()) {
      return std::nullopt;
    }
    return static_cast<size_t>(rng.rand_int(0, cands.size() - 1));
  }
};

class MinCostPolicy final : public OnlinePolicy {
 public:
  std::optional<size_t> pick(std::span<const Candidate> cands, Rng& /*rng*/) override {
    if (cands.empty()) {
      return std::nullopt;
    }
    size_t best = 0;
    for (size_t i = 1; i < cands.size(); ++i) {
      if (cands[i].cost < cands[best].cost) {
        best = i;  // 平手取规范序先者
      }
    }
    return best;
  }
};

}  // namespace

std::unique_ptr<OnlinePolicy> make_online(std::string_view name) {
  if (name == "first") {
    return std::make_unique<FirstPolicy>();
  }
  if (name == "random") {
    return std::make_unique<RandomPolicy>();
  }
  if (name == "min-cost") {
    return std::make_unique<MinCostPolicy>();
  }
  return nullptr;
}

}  // namespace hf
