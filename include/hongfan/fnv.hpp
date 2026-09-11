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
#include <string_view>

namespace hf {

// FNV-1a 64：规范序列化的内容寻址哈希（IMPL.md 附录 A.2）
constexpr uint64_t fnv1a64(std::string_view s) {
  uint64_t h = 14695981039346656037ULL;
  for (char c : s) {
    h ^= static_cast<uint8_t>(c);
    h *= 1099511628211ULL;
  }
  return h;
}

}  // namespace hf
