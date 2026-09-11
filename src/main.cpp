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

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <expected>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <map>
#include <optional>
#include <print>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "hongfan/chronicle.hpp"
#include "hongfan/engine.hpp"
#include "hongfan/interpolate.hpp"
#include "hongfan/policy.hpp"
#include "hongfan/rng.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/value.hpp"
#include "hongfan/world.hpp"
#include "hongfan/yaml.hpp"

namespace {

using hf::Rng;
using hf::RuleSet;

[[nodiscard]] std::expected<std::string, std::string> read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::unexpected("无法读取文件: " + path);
  }
  std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return data;
}

using Args = std::map<std::string, std::string>;

[[nodiscard]] Args parse_args(char** argv, int begin, int argc) {
  Args out;
  for (int i = begin; i + 1 < argc; i += 2) {
    std::string const key = argv[i];
    if (!key.starts_with("--")) {
      continue;
    }
    out[key.substr(2)] = argv[i + 1];
  }
  return out;
}

[[nodiscard]] std::optional<std::string> arg(const Args& a, const char* k) {
  const auto it = a.find(k);
  if (it == a.end()) {
    return std::nullopt;
  }
  return it->second;
}

template <class T>
[[nodiscard]] std::expected<T, std::string> parse_num(const std::string& s) {
  T v{};
  const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
  if (ec != std::errc{} || ptr != s.data() + s.size()) {
    return std::unexpected("数值无效: " + s);
  }
  return v;
}

struct Loaded {
  RuleSet rs;
  hf::World world;
};

[[nodiscard]] std::optional<Loaded> load_world_from(const std::string& rules_path,
                                                    const std::string& state_path) {
  const auto rules_text = read_file(rules_path);
  if (!rules_text) {
    std::println(stderr, "error: {}", rules_text.error());
    return std::nullopt;
  }
  const auto state_text = read_file(state_path);
  if (!state_text) {
    std::println(stderr, "error: {}", state_text.error());
    return std::nullopt;
  }
  const auto rules_doc = hf::yaml::parse(*rules_text);
  if (!rules_doc) {
    std::println(stderr, "error: 规则集 {}: {}", rules_path, rules_doc.error());
    return std::nullopt;
  }
  const auto state_doc = hf::yaml::parse(*state_text);
  if (!state_doc) {
    std::println(stderr, "error: 初始状态 {}: {}", state_path, state_doc.error());
    return std::nullopt;
  }
  auto loaded = hf::load_ruleset(*rules_doc);
  if (!loaded.problems.empty()) {
    for (const auto& p : loaded.problems) {
      std::println(stderr, "error: {}", p);
    }
    return std::nullopt;
  }
  auto world = hf::load_world(*state_doc, loaded.rs.schema);
  if (!world) {
    for (const auto& p : world.error()) {
      std::println(stderr, "error: {}", p);
    }
    return std::nullopt;
  }
  return Loaded{.rs = std::move(loaded.rs), .world = std::move(*world)};
}

[[nodiscard]] int cmd_derive(const Args& args) {
  const auto rules_path = arg(args, "rules");
  const auto state_path = arg(args, "state");
  const auto ticks_s = arg(args, "ticks");
  const auto strat = arg(args, "strategy");
  const auto seed_s = arg(args, "seed");
  if (!rules_path || !state_path || !ticks_s || !strat || !seed_s) {
    std::println(stderr,
                 "usage: hongfan derive --rules R --state S --ticks N --strategy "
                 "first|random|min-cost --seed U64");
    return 1;
  }
  auto loaded = load_world_from(*rules_path, *state_path);
  if (!loaded) {
    return 1;
  }
  const auto ticks = parse_num<uint32_t>(*ticks_s);
  const auto seed = parse_num<uint64_t>(*seed_s);
  if (!ticks || *ticks == 0 || *ticks > 100000) {
    std::println(stderr, "error: ticks 需在 1..100000");
    return 1;
  }
  if (!seed) {
    std::println(stderr, "error: {}", seed.error());
    return 1;
  }
  auto pol = hf::make_online(*strat);
  if (!pol) {
    std::println(stderr, "error: 未知策略 {}（可选 first|random|min-cost）", *strat);
    return 1;
  }
  Rng rng(*seed);
  auto res = hf::derive(loaded->rs, std::move(loaded->world), *ticks, *pol, rng);
  std::println("run: ruleset={:016x} state={:016x} strategy={} seed={}", loaded->rs.hash,
               res.init_state_hash, *strat, *seed);
  std::println("");
  std::println("—— 编年史 ——");
  std::print("{}", hf::chronicle_text(res.traj));
  std::println("");
  std::println("—— 终态 ——");
  std::print("{}", hf::snapshot_text(res.traj.final));
  std::println("final_hash={:016x}", res.traj.final_hash);
  return 0;
}

[[nodiscard]] int cmd_complete(const Args& args) {
  const auto rules_path = arg(args, "rules");
  const auto state_path = arg(args, "state");
  const auto goal_path = arg(args, "goal");
  const auto strat = arg(args, "strategy");
  const auto seed_s = arg(args, "seed");
  if (!rules_path || !state_path || !goal_path || !strat || !seed_s) {
    std::println(stderr,
                 "usage: hongfan complete --rules R --state S --goal G --depth N --strategy "
                 "random|min-cost|prefer:<rule> --seed U64 [--list K]");
    return 1;
  }
  auto loaded = load_world_from(*rules_path, *state_path);
  if (!loaded) {
    return 1;
  }
  const auto goal_text = read_file(*goal_path);
  if (!goal_text) {
    std::println(stderr, "error: {}", goal_text.error());
    return 1;
  }
  const auto goal_doc = hf::yaml::parse(*goal_text);
  if (!goal_doc) {
    std::println(stderr, "error: 目标 {}: {}", *goal_path, goal_doc.error());
    return 1;
  }
  auto goal = hf::load_goal(*goal_doc, loaded->world);
  if (!goal) {
    for (const auto& p : goal.error()) {
      std::println(stderr, "error: {}", p);
    }
    return 1;
  }
  uint32_t depth = 6;
  if (const auto d = arg(args, "depth")) {
    const auto n = parse_num<uint32_t>(*d);
    if (!n) {
      std::println(stderr, "error: {}", n.error());
      return 1;
    }
    depth = *n;
  }
  size_t max_sol = 1000;
  if (const auto m = arg(args, "max-solutions")) {
    const auto n = parse_num<size_t>(*m);
    if (!n || *n == 0) {
      std::println(stderr, "error: max-solutions 需为正整数");
      return 1;
    }
    max_sol = *n;
  }
  size_t list = 0;
  if (const auto l = arg(args, "list")) {
    const auto n = parse_num<size_t>(*l);
    if (!n) {
      std::println(stderr, "error: {}", n.error());
      return 1;
    }
    list = *n;
  }
  hf::SelectSpec spec;
  if (*strat == "random") {
    spec.kind = hf::SelectSpec::Kind::Random;
  } else if (*strat == "min-cost") {
    spec.kind = hf::SelectSpec::Kind::MinCost;
  } else if (strat->starts_with("prefer:")) {
    spec.kind = hf::SelectSpec::Kind::Prefer;
    spec.rule_id = strat->substr(7);
  } else {
    std::println(stderr, "error: 未知策略 {}（可选 random|min-cost|prefer:<rule>）", *strat);
    return 1;
  }
  const auto seed = parse_num<uint64_t>(*seed_s);
  if (!seed) {
    std::println(stderr, "error: {}", seed.error());
    return 1;
  }
  Rng rng(*seed);
  auto sols = hf::complete_between(loaded->rs, loaded->world, *goal,
                                   hf::CompleteOptions{.depth = depth, .max_solutions = max_sol});
  if (!sols) {
    std::println(stderr, "error: {}", sols.error());
    return 1;
  }
  if (sols->empty()) {
    std::println(stderr, "error: 无解（深度 {} 内无满足目标的合法轨迹）", depth);
    return 1;
  }
  std::println("run: ruleset={:016x} state={:016x} strategy={} seed={}", loaded->rs.hash,
               loaded->world.state_hash(), *strat, *seed);
  std::println("");
  std::println("解数: {}（深度 {}，上限 {}）", sols->size(), depth, max_sol);
  for (size_t i = 0; i < list && i < sols->size(); ++i) {
    std::println("解[{}]: {}", i, hf::solution_brief((*sols)[i]));
  }
  auto selected = hf::select_solution(std::move(*sols), spec, rng);
  if (!selected) {
    std::println(stderr, "error: 无满足偏好策略的解（prefer:{}）", spec.rule_id);
    return 1;
  }
  std::println("—— 选中解的编年史 ——");
  std::print("{}", hf::chronicle_text(*selected));
  std::println("total_cost={}", hf::display(hf::Value{selected->total_cost}));
  std::println("final_hash={:016x}", selected->final_hash);
  std::println("（界间补全 M1：前向枚举即验证——选中解由正向逐步执行产生）");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      std::println(stderr, "用法: hongfan derive|complete ...");
      std::println(stderr, "  derive  在线补全（正向推衍）");
      std::println(stderr, "  complete 界间补全（稽疑还原）");
      return 1;
    }
    const std::string cmd = argv[1];
    const Args args = parse_args(argv, 2, argc);
    if (cmd == "derive") {
      return cmd_derive(args);
    }
    if (cmd == "complete") {
      return cmd_complete(args);
    }
    std::println(stderr, "error: 未知子命令: {}", cmd);
    return 1;
  } catch (const std::exception& ex) {
    // catch 处理器内不可再抛（bugprone-exception-escape）：fputs 无异常；
    // 返回值在此最终错误路径上无可处理者，定点弃置（cert-err33-c）
    std::fputs("error: ", stderr);  // NOLINT(cert-err33-c)
    std::fputs(ex.what(), stderr);  // NOLINT(cert-err33-c)
    std::fputs("\n", stderr);       // NOLINT(cert-err33-c)
    return 1;
  }
}
