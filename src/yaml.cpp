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

#include "hongfan/yaml.hpp"

#include <cctype>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "yaml_util.hpp"

namespace hf::yaml {
namespace {

struct Line {
  size_t no;
  int indent;
  std::string_view content;
};

std::expected<std::vector<Line>, std::string> split_lines(std::string_view text) {
  std::vector<Line> out;
  size_t pos = 0;
  size_t no = 0;
  while (pos <= text.size()) {
    const size_t eol = text.find('\n', pos);
    const auto len = eol == std::string_view::npos ? std::string_view::npos : eol - pos;
    const std::string_view raw = text.substr(pos, len);
    ++no;
    int ind = 0;
    size_t i = 0;
    for (; i < raw.size() && raw[i] == ' '; ++i) {
      ++ind;
    }
    if (i < raw.size() && raw[i] == '\t') {
      return std::unexpected(std::format("第{}行: 禁止 Tab 缩进", no));
    }
    const std::string_view c = trim(raw.substr(i));
    if (!c.empty() && c.front() != '#') {
      out.push_back(Line{.no = no, .indent = ind, .content = c});
    }
    if (eol == std::string_view::npos) {
      break;
    }
    pos = eol + 1;
  }
  return out;
}

struct Cur {
  std::vector<Line>* ls;
  size_t i = 0;
  [[nodiscard]] bool done() const { return i >= ls->size(); }
  [[nodiscard]] const Line& cur() const { return (*ls)[i]; }
};

std::expected<Node, std::string> parse_map(Cur& c);
std::expected<Node, std::string> parse_node(Cur& c);

// "ident:" 形态（后跟空格或行尾）→ 块序列内的映射项起点
[[nodiscard]] bool looks_like_map_entry(std::string_view s) {
  if (s.empty() || (!(std::isalpha(static_cast<unsigned char>(s[0])) != 0) && s[0] != '_')) {
    return false;
  }
  for (size_t i = 1; i < s.size(); ++i) {
    const char ch = s[i];
    if ((std::isalnum(static_cast<unsigned char>(ch)) != 0) || ch == '_' || ch == '-') {
      continue;
    }
    if (ch == ':') {
      return i + 1 >= s.size() || is_space(s[i + 1]);
    }
    return false;
  }
  return false;
}

std::expected<Node, std::string> parse_seq(Cur& c) {
  const int ind = c.cur().indent;
  Node s;
  s.t = Node::T::Seq;
  while (!c.done() && c.cur().indent == ind && c.cur().content.starts_with("- ")) {
    const size_t no = c.cur().no;
    const std::string_view rest = trim(c.cur().content.substr(2));
    if (rest.empty()) {
      ++c.i;
      if (c.done() || c.cur().indent <= ind) {
        return std::unexpected(std::format("第{}行: 序列项缺少值", no));
      }
      auto item = parse_node(c);
      if (!item) {
        return std::unexpected(item.error());
      }
      s.seq.push_back(std::move(*item));
    } else if (rest.front() == '{') {
      FlowCur fc{.s = rest, .pos = 0, .line_no = no};
      auto item = parse_flow_map(fc);
      if (!item) {
        return std::unexpected(item.error());
      }
      if (auto tail = flow_tail_clean(fc); !tail) {
        return std::unexpected(tail.error());
      }
      ++c.i;
      s.seq.push_back(std::move(*item));
    } else if (looks_like_map_entry(rest)) {
      // 序列项是映射：重写本行为首键行（缩进 = 序列缩进 + 2），parse_map 连续消费
      (*c.ls)[c.i].indent = ind + 2;
      (*c.ls)[c.i].content = rest;
      auto item = parse_map(c);
      if (!item) {
        return std::unexpected(item.error());
      }
      s.seq.push_back(std::move(*item));
    } else {
      s.seq.push_back(scalar_node(rest));
      ++c.i;
    }
  }
  if (!c.done() && c.cur().indent > ind) {
    return std::unexpected(std::format("第{}行: 意外缩进", c.cur().no));
  }
  return s;
}

std::expected<Node, std::string> parse_map(Cur& c) {
  const int ind = c.cur().indent;
  Node m;
  m.t = Node::T::Map;
  while (!c.done() && c.cur().indent == ind && !c.cur().content.starts_with("- ")) {
    const size_t no = c.cur().no;
    const std::string_view content = c.cur().content;
    const size_t colon = content.find(':');
    if (colon == std::string_view::npos) {
      return std::unexpected(std::format("第{}行: 期望 '键: 值'", no));
    }
    const std::string key{trim(content.substr(0, colon))};
    const std::string_view rest = trim(content.substr(colon + 1));
    if (key.empty()) {
      return std::unexpected(std::format("第{}行: 键为空", no));
    }
    if (rest.empty()) {
      ++c.i;
      if (c.done() || c.cur().indent <= ind) {
        return std::unexpected(std::format("第{}行: '{}' 后缺少值", no, key));
      }
      auto v = parse_node(c);
      if (!v) {
        return std::unexpected(v.error());
      }
      v->key = key;
      m.map.push_back(std::move(*v));
    } else if (rest.front() == '{') {
      FlowCur fc{.s = rest, .pos = 0, .line_no = no};
      auto v = parse_flow_map(fc);
      if (!v) {
        return std::unexpected(v.error());
      }
      if (auto tail = flow_tail_clean(fc); !tail) {
        return std::unexpected(tail.error());
      }
      ++c.i;
      v->key = key;
      m.map.push_back(std::move(*v));
    } else {
      Node v = scalar_node(rest);
      v.key = key;
      m.map.push_back(std::move(v));
      ++c.i;
    }
  }
  if (!c.done() && c.cur().indent > ind) {
    return std::unexpected(std::format("第{}行: 意外缩进", c.cur().no));
  }
  return m;
}

std::expected<Node, std::string> parse_node(Cur& c) {
  if (c.cur().content.starts_with("- ")) {
    return parse_seq(c);
  }
  return parse_map(c);
}

}  // namespace

std::expected<Node, std::string> parse(std::string_view text) {
  auto lines = split_lines(text);
  if (!lines) {
    return std::unexpected(lines.error());
  }
  if (lines->empty()) {
    return std::unexpected(std::string("YAML 文档为空"));
  }
  Cur c{.ls = &*lines, .i = 0};
  auto root = parse_node(c);
  if (!root) {
    return std::unexpected(root.error());
  }
  if (!c.done()) {
    return std::unexpected(std::format("第{}行: 缩进不一致", c.cur().no));
  }
  return root;
}

const Node* Node::get(std::string_view k) const {
  for (const auto& e : map) {
    if (e.key == k) {
      return &e;
    }
  }
  return nullptr;
}

}  // namespace hf::yaml
