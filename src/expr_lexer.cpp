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

#include <cctype>
#include <charconv>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "expr_tok.hpp"

namespace hf {

std::expected<std::vector<Tok>, std::string> lex(std::string_view src) {
  std::vector<Tok> out;
  size_t i = 0;
  const auto ident_start = [](char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
  };
  const auto ident_char = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  };
  while (i < src.size()) {
    const char c = src[i];
    if (c == ' ') {
      ++i;
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
      size_t j = i;
      while (j < src.size() &&
             ((std::isdigit(static_cast<unsigned char>(src[j])) != 0) || src[j] == '.')) {
        ++j;
      }
      double d = 0;
      const auto [ptr, ec] = std::from_chars(src.data() + i, src.data() + j, d);
      if (ec != std::errc{} || ptr != src.data() + j) {
        return std::unexpected(std::format("表达式数值无效: {}", src.substr(i, j - i)));
      }
      out.push_back(Tok{.k = Tok::K::Num, .text = std::string(src.substr(i, j - i)), .num = d});
      i = j;
      continue;
    }
    if (c == '\'' || c == '"') {
      const size_t close = src.find(c, i + 1);
      if (close == std::string_view::npos) {
        return std::unexpected(std::string("表达式引号未闭合"));
      }
      out.push_back(
          Tok{.k = Tok::K::Str, .text = std::string(src.substr(i + 1, close - i - 1)), .num = 0});
      i = close + 1;
      continue;
    }
    if (ident_start(c)) {
      size_t j = i;
      while (j < src.size() && ident_char(src[j])) {
        ++j;
      }
      out.push_back(Tok{.k = Tok::K::Ident, .text = std::string(src.substr(i, j - i)), .num = 0});
      i = j;
      continue;
    }
    if (c == '(') {
      out.push_back(Tok{.k = Tok::K::LParen, .text = "(", .num = 0});
      ++i;
      continue;
    }
    if (c == ')') {
      out.push_back(Tok{.k = Tok::K::RParen, .text = ")", .num = 0});
      ++i;
      continue;
    }
    if (c == ',') {
      out.push_back(Tok{.k = Tok::K::Comma, .text = ",", .num = 0});
      ++i;
      continue;
    }
    if (i + 1 < src.size()) {
      const std::string_view two = src.substr(i, 2);
      if (two == "&&" || two == "||" || two == "==" || two == "!=" || two == "<=" || two == ">=") {
        out.push_back(Tok{.k = Tok::K::Op, .text = std::string(two), .num = 0});
        i += 2;
        continue;
      }
    }
    if (std::string_view("+-*/<>!.").contains(c)) {
      out.push_back(Tok{.k = Tok::K::Op, .text = std::string(1, c), .num = 0});
      ++i;
      continue;
    }
    return std::unexpected(std::format("表达式非法字符: '{}'", c));
  }
  out.push_back(Tok{.k = Tok::K::End, .text = "", .num = 0});
  return out;
}

}  // namespace hf
