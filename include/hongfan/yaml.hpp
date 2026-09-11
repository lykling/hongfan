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
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hf::yaml {

// 受控 YAML 子集解析器（M1 DSL 专用，替代 ryml——分发资产不可得，子集受控故自研）：
//   - 块映射（空格缩进）/ 块序列（"- "）/ 单行 flow map "{ k: v, k: { ... } }"
//   - 标量：数字 / true / false / 裸词 / 引号字符串（'...' 或 "..."；外层引号字符不得在串内出现）
//   - 仅支持整行注释（行首 #）；不支持行内注释、锚点、多行块标量、Tab 缩进
struct Node {
  enum class T : uint8_t { Scalar, Map, Seq };
  T t = T::Scalar;
  std::string scalar;
  bool quoted = false;
  std::string key;        // Map 子项的键
  std::vector<Node> map;  // 自引用仅经 vector 持有（标准允许不完整元素类型），跨编译器可移植
  std::vector<Node> seq;

  [[nodiscard]] const Node* get(std::string_view k) const;  // 仅 Map；未命中返回 nullptr
};

[[nodiscard]] std::expected<Node, std::string> parse(std::string_view text);

// 标量强制转换（边界一次性完成，IMPL.md §5.4）：引号 → 字符串；否则依次数值 / 布尔 / 字符串
struct Coerced {
  double num{};
  bool boolean{};
  bool is_num{};
  bool is_bool{};
  bool is_str{};
  std::string str;
};
[[nodiscard]] Coerced coerce(const Node& n);

}  // namespace hf::yaml
