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
#include <optional>
#include <string>
#include <variant>

#include "hongfan/id.hpp"

namespace hf {

// 属性值：数值/布尔/字符串/实体引用（关系 src/dst 表达式产出）
struct Value {
  std::variant<double, bool, std::string, EntityId> v;
};

using PropMap = std::map<std::string, Value>;  // 有序 → 序列化规范序（A.2）

// 展示格式：整数化数值省小数，其余保留两位并去尾零
[[nodiscard]] std::string display(const Value& v);

// 规范格式：数值 %.6f 定点，禁科学计数/NaN/inf（A.2）
[[nodiscard]] std::string canonical(const Value& v);

[[nodiscard]] std::optional<double> as_num(const Value& v);

// 值相等：同型且值相等（variant 语义；跨型不相等）
[[nodiscard]] bool operator==(const Value& a, const Value& b);

}  // namespace hf
