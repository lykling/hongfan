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
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "hongfan/chronicle.hpp"
#include "hongfan/expr.hpp"
#include "hongfan/rng.hpp"
#include "hongfan/rule.hpp"

namespace hf {

struct CompleteOptions {
  uint32_t depth = 6;
  size_t max_solutions = 1000;
};

struct SelectSpec {
  enum class Kind : uint8_t { Random, MinCost, Prefer };
  Kind kind = Kind::MinCost;
  std::string rule_id;  // Prefer：解须包含此规则
};

// 稽疑（M1）：界间补全 = 前向 DFS 枚举 + 目标测试（枚举即正向执行即验证）
[[nodiscard]] std::expected<std::vector<Trajectory>, std::string> complete_between(
    const RuleSet& rs, const World& start, const std::vector<Expr>& goal,
    const CompleteOptions& opt);

[[nodiscard]] std::optional<Trajectory> select_solution(std::vector<Trajectory>&& sols,
                                                        const SelectSpec& spec, Rng& rng);
[[nodiscard]] std::string solution_brief(const Trajectory& t);

}  // namespace hf
