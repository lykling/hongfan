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

#include "hongfan/chronicle.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "hongfan/expr.hpp"
#include "hongfan/id.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/value.hpp"
#include "hongfan/world.hpp"

namespace hf {

std::string_view kind_tag(Kind k) {
  switch (k) {
    case Kind::Definition:
      return "定义";
    case Kind::Invariant:
      return "不变量";
    case Kind::Transformation:
      return "五行";
    case Kind::Behavior:
      return "八政";
    case Kind::Evolution:
      return "庶征";
    case Kind::Relation:
      return "联系";
  }
  return "?";
}

std::map<std::string, std::string> render_bindings(const Bindings& b, const World& w) {
  std::map<std::string, std::string> out;
  for (const auto& [var, bind] : b) {
    if (bind.kind == Binding::Kind::Entity) {
      out[var] = w.name_of(EntityId{bind.ref}).value_or("#" + std::to_string(bind.ref));
    } else {
      const Relation& rel = w.relations.at(bind.ref);
      out[var] = std::format("{}:{}->{}", rel.type, w.name_of(rel.src).value_or("?"),
                             w.name_of(rel.dst).value_or("?"));
    }
  }
  return out;
}

std::string render_narration(const std::string& tpl, const Bindings& b, const World& w) {
  std::string out;
  size_t pos = 0;
  while (true) {
    const size_t open = tpl.find("{{", pos);
    if (open == std::string::npos) {
      out += tpl.substr(pos);
      break;
    }
    const size_t close = tpl.find("}}", open + 2);
    if (close == std::string::npos) {
      out += tpl.substr(pos);
      break;
    }
    out += tpl.substr(pos, open - pos);
    const std::string token = tpl.substr(open + 2, close - open - 2);
    const size_t dot = token.find('.');
    const std::string var = dot == std::string::npos ? token : token.substr(0, dot);
    const std::optional<std::string> prop =
        dot == std::string::npos ? std::nullopt : std::optional<std::string>(token.substr(dot + 1));
    std::string val = "[?]";
    const auto it = b.find(var);
    if (it != b.end()) {
      if (it->second.kind == Binding::Kind::Entity) {
        const EntityId id{it->second.ref};
        if (!prop || *prop == "id") {
          val = w.name_of(id).value_or("[?]");
        } else {
          const auto& ent = w.entities.at(it->second.ref);
          if (const auto pit = ent.props.find(*prop); pit != ent.props.end()) {
            val = display(pit->second);
          }
        }
      } else {
        const Relation& rel = w.relations.at(it->second.ref);
        if (!prop) {
          val = std::format("{}:{}->{}", rel.type, w.name_of(rel.src).value_or("?"),
                            w.name_of(rel.dst).value_or("?"));
        } else if (*prop == "type") {
          val = rel.type;
        } else if (*prop == "src") {
          val = w.name_of(rel.src).value_or("[?]");
        } else if (*prop == "dst") {
          val = w.name_of(rel.dst).value_or("[?]");
        } else if (const auto pit = rel.props.find(*prop); pit != rel.props.end()) {
          val = display(pit->second);
        }
      }
    }
    out += val;
    pos = close + 2;
  }
  return out;
}

std::string chronicle_text(const Trajectory& t) {
  std::string out;
  Tick cur{};
  bool first = true;
  const auto step_line = [](const Step& s) {
    return std::format("{}[{}] {}  ({}){}\n", s.rejected ? "[否]" : "", kind_tag(s.kind),
                       s.narration, s.rule_id, s.rejected ? "  " + s.reject_reason : "");
  };
  for (const Step& s : t.steps) {
    if (s.tick != Tick{}) {
      if (first || s.tick != cur) {
        if (!first) {
          out += "\n";
        }
        out += std::format("—— 第 {} 刻 ——\n", static_cast<uint32_t>(s.tick));
        cur = s.tick;
        first = false;
      }
      out += step_line(s);
    } else {
      out += std::format("第{}步 {}", s.seq, step_line(s));
    }
  }
  return out;
}

std::string snapshot_text(const World& w) {
  std::string out;
  for (const auto& [name, slot] : w.named) {
    const Entity& e = w.entities.at(static_cast<uint32_t>(slot));
    out += std::format("{} {} {{", name, e.type);
    bool first_prop = true;
    for (const auto& [prop, val] : e.props) {
      if (!first_prop) {
        out += ", ";
      }
      out += prop + "=" + display(val);
      first_prop = false;
    }
    out += "}\n";
  }
  for (const auto& rel : w.relations) {
    out += std::format("R {} {} -> {}\n", rel.type, w.name_of(rel.src).value_or("?"),
                       w.name_of(rel.dst).value_or("?"));
  }
  return out;
}

}  // namespace hf
