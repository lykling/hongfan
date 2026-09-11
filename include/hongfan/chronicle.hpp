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
#include <map>
#include <string>
#include <vector>

#include "hongfan/expr.hpp"
#include "hongfan/id.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/world.hpp"

namespace hf {

struct EffectRecord {
  std::string text;
};

// 一步 = 一次规则应用的原子记录（tick==0 表示界间补全的无时刻步）
struct Step {
  Tick tick{};
  uint32_t seq{};
  std::string rule_id;
  Kind kind{};
  std::map<std::string, std::string> bindings;
  std::vector<EffectRecord> effects;
  std::string narration;
  bool rejected = false;
  std::string reject_reason;
};

struct Trajectory {
  World start, final;
  std::vector<Step> steps;
  uint64_t final_hash = 0;
  double total_cost = 0;
};

// Chinese display label for a rule kind (presentation-layer output, per the
// language policy only chronicle.cpp/main.cpp may hold Chinese literals)
[[nodiscard]] std::string_view kind_tag(Kind k);

[[nodiscard]] std::map<std::string, std::string> render_bindings(const Bindings& b, const World& w);
// 模板语法：{{var}} / {{var.prop}} / {{var.id}}；未知占位渲染为 [?]
[[nodiscard]] std::string render_narration(const std::string& tpl, const Bindings& b,
                                           const World& w);
[[nodiscard]] std::string chronicle_text(const Trajectory& t);
[[nodiscard]] std::string snapshot_text(const World& w);

}  // namespace hf
