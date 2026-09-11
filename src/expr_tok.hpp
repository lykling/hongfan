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
#include <vector>

namespace hf {

// 表达式词法内部共享（不进公共 include/）
struct Tok {
  enum class K : uint8_t { Num, Str, Ident, Op, LParen, RParen, Comma, End };
  K k;
  std::string text;
  double num{};
};

[[nodiscard]] std::expected<std::vector<Tok>, std::string> lex(std::string_view src);

}  // namespace hf
