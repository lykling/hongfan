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
#include "hongfan/expr.hpp"

#include <doctest.h>

using namespace hf;

namespace {
World mini_world() {
  World w;
  w.entities.push_back(
      Entity{"Traveler", {{"stamina", Value{5.0}}, {"name", Value{std::string("旅人")}}}});
  w.named["p"] = EntityId{0};
  return w;
}
}  // namespace

TEST_CASE("运算优先级与比较") {
  World w = mini_world();
  EvalCtx ctx{&w, nullptr, nullptr};
  auto e = parse_expr("1 + 2 * 3 == 7");
  REQUIRE(e);
  auto v = eval_bool(*e, ctx);
  REQUIRE(v);
  CHECK(*v);
  auto e2 = parse_expr("!(1 > 2) && (3 <= 3 || false)");
  REQUIRE(e2);
  auto v2 = eval_bool(*e2, ctx);
  REQUIRE(v2);
  CHECK(*v2);
}

TEST_CASE("字符串相等与不等") {
  World w = mini_world();
  EvalCtx ctx{&w, nullptr, nullptr};
  auto e = parse_expr("'rainy' == 'rainy' && 'rainy' != 'sunny'");
  REQUIRE(e);
  auto v = eval_bool(*e, ctx);
  REQUIRE(v);
  CHECK(*v);
}

TEST_CASE("短路求值：除零不触发") {
  World w = mini_world();
  EvalCtx ctx{&w, nullptr, nullptr};
  auto e = parse_expr("false && 1 / 0 > 0");
  REQUIRE(e);
  auto v = eval_bool(*e, ctx);
  REQUIRE(v);
  CHECK_FALSE(*v);
}

TEST_CASE("除零报错") {
  World w = mini_world();
  EvalCtx ctx{&w, nullptr, nullptr};
  auto e = parse_expr("1 / 0 > 0");
  REQUIRE(e);
  CHECK_FALSE(eval_bool(*e, ctx).has_value());
}

TEST_CASE("rand_int 确定性、范围与禁用") {
  World w = mini_world();
  auto e = parse_expr("rand_int(1, 6)");
  REQUIRE(e);
  Rng a(42), b(42);
  EvalCtx ca{&w, nullptr, &a};
  EvalCtx cb{&w, nullptr, &b};
  auto va = eval(*e, ca);
  auto vb = eval(*e, cb);
  REQUIRE(va);
  REQUIRE(vb);
  CHECK(*va == *vb);
  for (int i = 0; i < 100; ++i) {
    EvalCtx c{&w, nullptr, &a};
    auto v = eval(*e, c);
    REQUIRE(v);
    const auto n = as_num(*v).value();
    CHECK(n >= 1);
    CHECK(n <= 6);
  }
  EvalCtx no_rng{&w, nullptr, nullptr};
  CHECK_FALSE(eval(*e, no_rng).has_value());
}

TEST_CASE("路径求值：命名实体与绑定") {
  World w = mini_world();
  auto e = parse_expr("p.stamina >= 4 && p.name == '旅人'");
  REQUIRE(e);
  EvalCtx named{&w, nullptr, nullptr};
  auto v = eval_bool(*e, named);
  REQUIRE(v);
  CHECK(*v);
  Bindings b{{"x", Binding{Binding::Kind::Entity, 0}}};
  auto e2 = parse_expr("x.stamina == 5 && x.id == p.id");
  REQUIRE(e2);
  EvalCtx bound{&w, &b, nullptr};
  auto v2 = eval_bool(*e2, bound);
  REQUIRE(v2);
  CHECK(*v2);
}

TEST_CASE("contains_rand 与 path_roots") {
  auto e1 = parse_expr("rand_int(1, 2) + 1");
  REQUIRE(e1);
  CHECK(contains_rand(*e1));
  auto e2 = parse_expr("1 + a.b");
  REQUIRE(e2);
  CHECK_FALSE(contains_rand(*e2));
  const auto roots = path_roots(*e2);
  CHECK(roots == std::vector<std::string>{"a"});
}

TEST_CASE("解析错误") {
  CHECK_FALSE(parse_expr("1 +"));
  CHECK_FALSE(parse_expr(""));
  CHECK_FALSE(parse_expr("(1 + 2"));
}
