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
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "hongfan/id.hpp"
#include "hongfan/value.hpp"

namespace hf {

struct Entity {
  std::string type;
  PropMap props;
};

struct Relation {
  std::string type;
  EntityId src, dst;
  PropMap props;
};

// 世界状态：时态属性图的 M1 形态（实体槽位稳定、关系按声明序；IMPL.md §5.2）
struct World {
  std::vector<Entity> entities;
  std::vector<Relation> relations;
  std::map<std::string, EntityId> named;  // 初始态命名实体 → 槽位
  Tick now{0};

  [[nodiscard]] std::optional<EntityId> find_named(const std::string& name) const;
  [[nodiscard]] std::optional<std::string> name_of(EntityId id) const;
  // A.2 规范序列化：命名实体按字典序，属性按键序，数值 %.6f；关系按声明序
  [[nodiscard]] std::string canonical_state() const;
  [[nodiscard]] uint64_t state_hash() const;
};

}  // namespace hf
