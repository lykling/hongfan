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
// YAML 子集解析器内部共享（不进公共 include/）
#include <string>
#include <string_view>
#include <utility>

#include "hongfan/yaml.hpp"

namespace hf::yaml {

[[nodiscard]] inline bool is_space(char c) { return c == ' '; }

[[nodiscard]] inline std::string_view trim(std::string_view s) {
  while (!s.empty() && is_space(s.front()))
    s.remove_prefix(1);
  while (!s.empty() && (is_space(s.back()) || s.back() == '\r'))
    s.remove_suffix(1);
  return s;
}

[[nodiscard]] inline bool try_unquote(std::string_view s, std::string& out) {
  const bool dq = s.size() >= 2 && s.front() == '"' && s.back() == '"';
  const bool sq = s.size() >= 2 && s.front() == '\'' && s.back() == '\'';
  if (dq || sq) {
    out.assign(s.substr(1, s.size() - 2));
    return true;
  }
  return false;
}

[[nodiscard]] inline Node scalar_node(std::string_view s) {
  Node n;
  std::string unq;
  if (try_unquote(s, unq)) {
    n.scalar = std::move(unq);
    n.quoted = true;
  } else {
    n.scalar = std::string(s);
  }
  return n;
}

// 单行 flow map 游标（yaml_flow.cpp 实现）
struct FlowCur {
  std::string_view s;
  size_t pos = 0;
  size_t line_no = 0;
};

[[nodiscard]] std::expected<Node, std::string> parse_flow_map(FlowCur& c);
[[nodiscard]] std::expected<void, std::string> flow_tail_clean(const FlowCur& c);

}  // namespace hf::yaml
