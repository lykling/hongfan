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

#include <charconv>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <system_error>
#include <utility>

#include "hongfan/yaml.hpp"

#include "yaml_util.hpp"

namespace hf::yaml {
namespace {

std::expected<Node, std::string> parse_flow_value(FlowCur& c);

}  // namespace

std::expected<Node, std::string> parse_flow_map(FlowCur& c) {
  c.pos++;  // 消费 '{'
  Node m;
  m.t = Node::T::Map;
  for (;;) {
    while (c.pos < c.s.size() && is_space(c.s[c.pos])) {
      c.pos++;
    }
    if (c.pos >= c.s.size()) {
      return std::unexpected(std::format("第{}行: flow map 未闭合", c.line_no));
    }
    if (c.s[c.pos] == '}') {
      c.pos++;
      return m;
    }
    const size_t kbegin = c.pos;
    while (c.pos < c.s.size() && c.s[c.pos] != ':' && c.s[c.pos] != '}' && c.s[c.pos] != ',') {
      c.pos++;
    }
    if (c.pos >= c.s.size() || c.s[c.pos] != ':') {
      return std::unexpected(std::format("第{}行: flow map 键缺少 ':'", c.line_no));
    }
    const std::string key{trim(c.s.substr(kbegin, c.pos - kbegin))};
    c.pos++;  // 消费 ':'
    while (c.pos < c.s.size() && is_space(c.s[c.pos])) {
      c.pos++;
    }
    auto v = parse_flow_value(c);
    if (!v) {
      return std::unexpected(v.error());
    }
    v->key = key;
    m.map.push_back(std::move(*v));
    while (c.pos < c.s.size() && is_space(c.s[c.pos])) {
      c.pos++;
    }
    if (c.pos < c.s.size() && c.s[c.pos] == ',') {
      c.pos++;
      continue;
    }
    if (c.pos < c.s.size() && c.s[c.pos] == '}') {
      c.pos++;
      return m;
    }
    return std::unexpected(std::format("第{}行: flow map 期望 ',' 或 '}}'", c.line_no));
  }
}

namespace {

std::expected<Node, std::string> parse_flow_value(FlowCur& c) {
  if (c.pos < c.s.size() && c.s[c.pos] == '{') {
    return parse_flow_map(c);
  }
  const size_t begin = c.pos;
  if (c.pos < c.s.size() && (c.s[c.pos] == '"' || c.s[c.pos] == '\'')) {
    const char q = c.s[c.pos];
    c.pos++;
    while (c.pos < c.s.size() && c.s[c.pos] != q) {
      c.pos++;
    }
    if (c.pos >= c.s.size()) {
      return std::unexpected(std::format("第{}行: 引号未闭合", c.line_no));
    }
    c.pos++;
    return scalar_node(trim(c.s.substr(begin, c.pos - begin)));
  }
  while (c.pos < c.s.size() && c.s[c.pos] != ',' && c.s[c.pos] != '}') {
    c.pos++;
  }
  return scalar_node(trim(c.s.substr(begin, c.pos - begin)));
}

}  // namespace

std::expected<void, std::string> flow_tail_clean(const FlowCur& c) {
  auto pos = c.pos;
  while (pos < c.s.size() && is_space(c.s[pos])) {
    pos++;
  }
  if (pos != c.s.size()) {
    return std::unexpected(std::format("第{}行: flow map 后有多余内容", c.line_no));
  }
  return {};
}

Coerced coerce(const Node& n) {
  Coerced c;
  if (n.t != Node::T::Scalar || n.quoted) {
    c.is_str = true;
    c.str = n.scalar;
    return c;
  }
  double d = 0;
  const char* begin = n.scalar.data();
  const char* end = begin + n.scalar.size();
  const auto [ptr, ec] = std::from_chars(begin, end, d);
  if (ec == std::errc{} && ptr == end) {
    c.is_num = true;
    c.num = d;
    return c;
  }
  if (n.scalar == "true") {
    c.is_bool = true;
    c.boolean = true;
    return c;
  }
  if (n.scalar == "false") {
    c.is_bool = true;
    c.boolean = false;
    return c;
  }
  c.is_str = true;
  c.str = n.scalar;
  return c;
}

}  // namespace hf::yaml
