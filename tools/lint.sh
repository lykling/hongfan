#!/usr/bin/env bash
# =============================================================================
# HONGFAN lint helper
# =============================================================================
#
# Checks:
#   1. Apache-2.0 license header on every first-party C++ file
#   2. cppcheck (optional, when installed)
#
# Usage:
#   tools/lint.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

FAILED=0

# -----------------------------------------------------------------------------
# License headers
# -----------------------------------------------------------------------------
echo "[lint] checking license headers..."
FILES=$(find src include tests \
	-type f \( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null || true)

for f in ${FILES}; do
	if ! head -3 "${f}" | grep -q "Copyright 2026"; then
		echo "[lint] FAIL: missing license header: ${f}"
		FAILED=1
	fi
done

# -----------------------------------------------------------------------------
# cppcheck (optional)
# -----------------------------------------------------------------------------
if command -v cppcheck >/dev/null 2>&1; then
	echo "[lint] running cppcheck..."
	if ! cppcheck --enable=warning,style --suppress=missingIncludeSystem \
		--error-exitcode=1 \
		src include 2>&1; then
		echo "[lint] FAIL: cppcheck reported issues"
		FAILED=1
	fi
else
	echo "[lint] cppcheck not installed; skipping"
fi

if [[ ${FAILED} -eq 1 ]]; then
	echo "[lint] FAILED"
	exit 1
fi
echo "[lint] all clean"
