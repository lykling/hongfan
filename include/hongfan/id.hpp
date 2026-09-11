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

namespace hf {

// 强类型 Id（IMPL.md §5.1）：语义单元不与裸整数混用
enum class EntityId : uint32_t {};  // 世界槽位下标（M1 实体不删除，槽位稳定）
enum class RuleId : uint32_t {};    // 规则集内下标
enum class Tick : uint32_t {};      // 离散刻

}  // namespace hf
