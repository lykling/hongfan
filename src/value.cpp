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

#include "hongfan/value.hpp"

#include <cmath>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <variant>

#include "hongfan/id.hpp"

namespace hf {

std::optional<double> as_num(const Value& v) {
  if (const auto* n = std::get_if<double>(&v.v)) {
    return *n;
  }
  return std::nullopt;
}

bool operator==(const Value& a, const Value& b) { return a.v == b.v; }

std::string canonical(const Value& v) {
  if (const auto* n = std::get_if<double>(&v.v)) {
    return std::format("{:.6f}", *n);
  }
  if (const auto* b = std::get_if<bool>(&v.v)) {
    return *b ? "true" : "false";
  }
  if (const auto* s = std::get_if<std::string>(&v.v)) {
    return *s;
  }
  return std::format("#{}", static_cast<uint32_t>(std::get<EntityId>(v.v)));
}

std::string display(const Value& v) {
  if (const auto* n = std::get_if<double>(&v.v)) {
    if (std::abs(*n) < 1e15 && std::abs(*n - std::round(*n)) < 1e-9) {
      return std::format("{}", static_cast<long long>(std::round(*n)));
    }
    return std::format("{:.2}", *n);
  }
  if (const auto* b = std::get_if<bool>(&v.v)) {
    return *b ? "true" : "false";
  }
  if (const auto* s = std::get_if<std::string>(&v.v)) {
    return *s;
  }
  return std::format("#{}", static_cast<uint32_t>(std::get<EntityId>(v.v)));
}

}  // namespace hf
