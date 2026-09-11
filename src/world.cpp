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

#include "hongfan/world.hpp"

#include <cstdint>
#include <optional>
#include <string>

#include "hongfan/fnv.hpp"
#include "hongfan/id.hpp"
#include "hongfan/value.hpp"

namespace hf {

std::optional<EntityId> World::find_named(const std::string& name) const {
  if (const auto it = named.find(name); it != named.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<std::string> World::name_of(EntityId id) const {
  for (const auto& [name, slot] : named) {
    if (slot == id) {
      return name;
    }
  }
  return std::nullopt;
}

namespace {

void append_entity(std::string& out, const std::string& name, const Entity& e) {
  out += name;
  out += ':';
  out += e.type;
  for (const auto& [prop, val] : e.props) {
    out.push_back(';');
    out += prop;
    out.push_back('=');
    out += canonical(val);
  }
  out.push_back('\n');
}

}  // namespace

std::string World::canonical_state() const {
  std::string out;
  for (const auto& [name, slot] : named) {
    append_entity(out, name, entities.at(static_cast<uint32_t>(slot)));
  }
  for (const auto& rel : relations) {
    const auto src =
        name_of(rel.src).value_or("#" + std::to_string(static_cast<uint32_t>(rel.src)));
    const auto dst =
        name_of(rel.dst).value_or("#" + std::to_string(static_cast<uint32_t>(rel.dst)));
    out += "R:";
    out += rel.type;
    out += ':';
    out += src;
    out += '>';
    out += dst;
    for (const auto& [prop, val] : rel.props) {
      out.push_back(';');
      out += prop;
      out.push_back('=');
      out += canonical(val);
    }
    out.push_back('\n');
  }
  return out;
}

uint64_t World::state_hash() const { return fnv1a64(canonical_state()); }

}  // namespace hf
