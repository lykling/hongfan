#!/usr/bin/env bash
# M1 E2E：确定性双跑比对 + 黄金文件（IMPL.md §9）
set -euo pipefail
BIN="$1"
SRC="$2"
cd "$SRC"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# 确定性：同 Run 四元组两次运行逐字节一致
"$BIN" derive --rules examples/village/rules.yaml --state examples/village/initial.yaml \
  --ticks 8 --strategy random --seed 42 > "$TMP/v1.txt"
"$BIN" derive --rules examples/village/rules.yaml --state examples/village/initial.yaml \
  --ticks 8 --strategy random --seed 42 > "$TMP/v2.txt"
cmp "$TMP/v1.txt" "$TMP/v2.txt"

diff -u "$TMP/v1.txt" tests/golden/village_random.txt

"$BIN" derive --rules examples/village/rules.yaml --state examples/village/initial.yaml \
  --ticks 8 --strategy min-cost --seed 7 | diff -u - tests/golden/village_mincost.txt

"$BIN" complete --rules examples/traveler/rules.yaml --state examples/traveler/initial.yaml \
  --goal examples/traveler/goal.yaml --depth 6 --strategy min-cost --seed 7 \
  | diff -u - tests/golden/traveler_mincost.txt

"$BIN" complete --rules examples/traveler/rules.yaml --state examples/traveler/initial.yaml \
  --goal examples/traveler/goal.yaml --depth 6 --strategy random --seed 11 \
  | diff -u - tests/golden/traveler_random.txt

echo "e2e ok"
