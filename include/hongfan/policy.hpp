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
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include "hongfan/expr.hpp"
#include "hongfan/rng.hpp"

namespace hf {

struct Candidate {
  size_t rule_idx;
  Bindings bindings;
  double cost;
  int priority;
};

// 三德：在线策略——每段一次咨询（IMPL.md §6.3）
class OnlinePolicy {
 public:
  virtual ~OnlinePolicy() = default;
  virtual std::optional<size_t> pick(std::span<const Candidate> cands, Rng& rng) = 0;
};

// first | random | min-cost；未知名字返回 nullptr
[[nodiscard]] std::unique_ptr<OnlinePolicy> make_online(std::string_view name);

}  // namespace hf
