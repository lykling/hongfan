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
#include <optional>
#include <string>
#include <vector>

#include "hongfan/chronicle.hpp"
#include "hongfan/policy.hpp"
#include "hongfan/rng.hpp"

namespace hf {

struct DeriveResult {
  Trajectory traj;
  uint64_t ruleset_hash = 0;
  uint64_t init_state_hash = 0;
};

// 单候选应用：克隆→按序求值效果（基于应用前快照）→ 调用方检查不变量
struct ApplyOutcome {
  World next;
  std::vector<EffectRecord> records;
  std::string reject;  // 非空 = 拒绝原因（世界不动）
};

[[nodiscard]] std::vector<Candidate> match_rule(const RuleSet& rs, size_t rule_idx, const World& w);
[[nodiscard]] ApplyOutcome apply_rule(const Rule& r, const Bindings& b, const World& w, Rng* rng);
[[nodiscard]] std::optional<std::string> invariant_violation(const RuleSet& rs, const World& w);

// 皇极：在线补全——每刻三段（庶征→五行→八政），每段至多一次应用（M1 语义）
[[nodiscard]] DeriveResult derive(const RuleSet& rs, World world, uint32_t ticks, OnlinePolicy& pol,
                                  Rng& rng);

}  // namespace hf
