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

#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "hongfan/expr.hpp"
#include "hongfan/id.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/value.hpp"
#include "hongfan/world.hpp"
#include "hongfan/yaml.hpp"

#include "ruleset_internal.hpp"

namespace hf {

std::expected<World, std::vector<std::string>> load_world(const yaml::Node& root,
                                                          const TypeSchema& schema) {
  std::vector<std::string> problems;
  World w;
  if (root.t != yaml::Node::T::Map) {
    return std::unexpected(std::vector<std::string>{"初始状态根节点必须是映射"});
  }
  const auto* ents = root.get("entities");
  if (ents == nullptr || ents->t != yaml::Node::T::Seq) {
    return std::unexpected(std::vector<std::string>{"缺少 entities 序列"});
  }
  for (const auto& item : ents->seq) {
    if (item.t != yaml::Node::T::Map) {
      problems.emplace_back("entities 项必须是映射");
      continue;
    }
    const auto* id = scalar_field(item, "id");
    const auto* type = scalar_field(item, "type");
    if (id == nullptr || id->scalar.empty() || type == nullptr) {
      problems.emplace_back("entity 需要 id/type");
      continue;
    }
    if (w.find_named(id->scalar)) {
      problems.push_back(std::format("实体 id 重复: {}", id->scalar));
      continue;
    }
    const auto sit = schema.find(type->scalar);
    if (sit == schema.end()) {
      problems.push_back(std::format("实体 {}: 未知类型 {}", id->scalar, type->scalar));
      continue;
    }
    Entity e;
    e.type = type->scalar;
    if (const auto* props = map_field(item, "props")) {
      for (const auto& pv : props->map) {
        const auto pit = sit->second.find(pv.key);
        if (pit == sit->second.end()) {
          problems.push_back(std::format("实体 {}: 类型 {} 无属性 {}", id->scalar, e.type, pv.key));
          continue;
        }
        const auto c = yaml::coerce(pv);
        if (pit->second == "num") {
          if (!c.is_num) {
            problems.push_back(std::format("实体 {}: 属性 {} 需为数值", id->scalar, pv.key));
            continue;
          }
          e.props[pv.key] = Value{c.num};
        } else if (pit->second == "bool") {
          if (!c.is_bool) {
            problems.push_back(std::format("实体 {}: 属性 {} 需为布尔", id->scalar, pv.key));
            continue;
          }
          e.props[pv.key] = Value{c.boolean};
        } else if (!c.is_str) {
          problems.push_back(
              std::format("实体 {}: 属性 {} 需为字符串（数值请加引号）", id->scalar, pv.key));
          continue;
        } else {
          e.props[pv.key] = Value{c.str};
        }
      }
    }
    for (const auto& [key, typ] : sit->second) {
      if (!e.props.contains(key)) {
        problems.push_back(std::format("实体 {}: 缺少属性 {} ({})", id->scalar, key, typ));
      }
    }
    w.named[id->scalar] = EntityId{static_cast<uint32_t>(w.entities.size())};
    w.entities.push_back(std::move(e));
  }
  if (const auto* rels = root.get("relations")) {
    if (rels->t != yaml::Node::T::Seq) {
      problems.emplace_back("relations 必须是序列");
    } else {
      for (const auto& item : rels->seq) {
        if (item.t != yaml::Node::T::Map) {
          problems.emplace_back("relation 项必须是映射");
          continue;
        }
        const auto* type = scalar_field(item, "type");
        const auto* src = scalar_field(item, "src");
        const auto* dst = scalar_field(item, "dst");
        if (type == nullptr || src == nullptr || dst == nullptr) {
          problems.emplace_back("relation 需要 type/src/dst");
          continue;
        }
        const auto s = w.find_named(src->scalar);
        const auto d = w.find_named(dst->scalar);
        if (!s || !d) {
          problems.push_back(
              std::format("relation 引用未知实体: {} -> {}", src->scalar, dst->scalar));
          continue;
        }
        w.relations.push_back(Relation{.type = type->scalar, .src = *s, .dst = *d, .props = {}});
      }
    }
  }
  if (!problems.empty()) {
    return std::unexpected(std::move(problems));
  }
  return w;
}

std::expected<std::vector<Expr>, std::vector<std::string>> load_goal(const yaml::Node& root,
                                                                     const World& w) {
  std::vector<std::string> problems;
  std::vector<Expr> out;
  if (root.t != yaml::Node::T::Map) {
    return std::unexpected(std::vector<std::string>{"目标根节点必须是映射"});
  }
  const auto* goal = root.get("goal");
  if (goal == nullptr || goal->t != yaml::Node::T::Seq) {
    return std::unexpected(std::vector<std::string>{"缺少 goal 序列"});
  }
  for (const auto& item : goal->seq) {
    if (item.t != yaml::Node::T::Scalar) {
      problems.emplace_back("goal 项必须是字符串");
      continue;
    }
    auto e = parse_expr(item.scalar);
    if (!e) {
      problems.push_back(std::format("目标 {}: {}", item.scalar, e.error()));
      continue;
    }
    if (contains_rand(*e)) {
      problems.push_back(std::format("目标禁止 rand_int: {}", item.scalar));
      continue;
    }
    for (const auto& root_name : path_roots(*e)) {
      if (!w.find_named(root_name)) {
        problems.push_back(std::format("目标引用未知实体: {}", root_name));
      }
    }
    out.push_back(std::move(*e));
  }
  if (!problems.empty()) {
    return std::unexpected(std::move(problems));
  }
  return out;
}

}  // namespace hf
