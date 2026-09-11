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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "hongfan/world.hpp"

#include <doctest.h>

using namespace hf;

TEST_CASE("规范序列化与哈希稳定") {
  World a;
  a.entities.push_back(Entity{"Food", {{"amount", Value{3.0}}}});
  a.named["pantry"] = EntityId{0};
  a.relations.push_back(Relation{"neighbor", EntityId{0}, EntityId{0}, {}});
  const World b = a;
  CHECK(a.state_hash() == b.state_hash());
  World c = a;
  c.entities[0].props["amount"] = Value{4.0};
  CHECK(a.state_hash() != c.state_hash());
  CHECK(a.canonical_state().find("pantry:Food") != std::string::npos);
  CHECK(a.canonical_state().find("R:neighbor") != std::string::npos);
  CHECK(a.canonical_state().find("amount=3.000000") != std::string::npos);
}

TEST_CASE("命名查找") {
  World w;
  w.entities.push_back(Entity{"Food", {{"amount", Value{1.0}}}});
  w.named["pantry"] = EntityId{0};
  CHECK(w.find_named("pantry").has_value());
  CHECK_FALSE(w.find_named("nope").has_value());
  CHECK(w.name_of(EntityId{0}).value() == "pantry");
}
