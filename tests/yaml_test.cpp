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
#include "hongfan/yaml.hpp"

#include <doctest.h>

using namespace hf::yaml;

TEST_CASE("规则集结构解析") {
  const std::string text = R"YAML(ruleset: demo
time:
  model: linear-tick
rules:
  - rule: def-food
    kind: definition
    entity: Food
    properties: { amount: num }
  - rule: eat
    kind: behavior
    when:
      - "f: Food { amount >= 1 }"
    effects:
      - "update f.amount - 1"
    narration: "吃了一口。"
)YAML";
  auto doc = parse(text);
  REQUIRE(doc);
  REQUIRE(doc->t == Node::T::Map);
  CHECK(doc->get("ruleset")->scalar == "demo");
  const auto* time = doc->get("time");
  REQUIRE(time);
  CHECK(time->get("model")->scalar == "linear-tick");
  const auto* rules = doc->get("rules");
  REQUIRE(rules);
  REQUIRE(rules->t == Node::T::Seq);
  REQUIRE(rules->seq.size() == 2);
  CHECK(rules->seq[0].get("kind")->scalar == "definition");
  const auto* when = rules->seq[1].get("when");
  REQUIRE(when);
  REQUIRE(when->seq.size() == 1);
  CHECK(when->seq[0].quoted);
  CHECK(when->seq[0].scalar == "f: Food { amount >= 1 }");
  CHECK(rules->seq[1].get("narration")->scalar == "吃了一口。");
}

TEST_CASE("嵌套 flow map 与标量强制转换") {
  auto doc = parse(
      "entities:\n  - { id: p, type: Person, props: { name: 阿岩, ok: true, n: 3, q: '3' } }\n");
  REQUIRE(doc);
  const auto* ents = doc->get("entities");
  REQUIRE(ents);
  REQUIRE(ents->seq.size() == 1);
  const auto& e = ents->seq[0];
  CHECK(e.get("id")->scalar == "p");
  const auto* props = e.get("props");
  REQUIRE(props);
  CHECK(coerce(*props->get("name")).is_str);
  CHECK(coerce(*props->get("name")).str == "阿岩");
  CHECK(coerce(*props->get("ok")).is_bool);
  CHECK(coerce(*props->get("ok")).boolean);
  CHECK(coerce(*props->get("n")).is_num);
  CHECK(coerce(*props->get("q")).is_str);
  CHECK(coerce(*props->get("q")).str == "3");
}

TEST_CASE("错误：Tab 缩进与空文档") {
  CHECK_FALSE(parse("\t- a"));
  CHECK_FALSE(parse(""));
}
