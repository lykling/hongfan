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
// 规则集加载内部共享（不进公共 include/）
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "hongfan/rule.hpp"
#include "hongfan/yaml.hpp"

namespace hf {

[[nodiscard]] inline const yaml::Node* scalar_field(const yaml::Node& m, std::string_view key) {
  const auto* n = m.get(key);
  if (n == nullptr || n->t != yaml::Node::T::Scalar)
    return nullptr;
  return n;
}
[[nodiscard]] inline const yaml::Node* seq_field(const yaml::Node& m, std::string_view key) {
  const auto* n = m.get(key);
  if (n == nullptr || n->t != yaml::Node::T::Seq)
    return nullptr;
  return n;
}
[[nodiscard]] inline const yaml::Node* map_field(const yaml::Node& m, std::string_view key) {
  const auto* n = m.get(key);
  if (n == nullptr || n->t != yaml::Node::T::Map)
    return nullptr;
  return n;
}

[[nodiscard]] inline std::optional<Kind> kind_from(std::string_view s) {
  if (s == "definition")
    return Kind::Definition;
  if (s == "invariant")
    return Kind::Invariant;
  if (s == "transformation")
    return Kind::Transformation;
  if (s == "behavior")
    return Kind::Behavior;
  if (s == "evolution")
    return Kind::Evolution;
  if (s == "relation")
    return Kind::Relation;
  return std::nullopt;
}

[[nodiscard]] std::string canonical_ruleset(const RuleSet& rs);
void validate_rules(const std::vector<Rule>& parsed, const TypeSchema& schema,
                    std::vector<std::string>& problems);

}  // namespace hf
