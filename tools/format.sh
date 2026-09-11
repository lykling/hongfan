#!/usr/bin/env bash
# =============================================================================
# HONGFAN code format helper
# =============================================================================
#
# Runs clang-format over first-party C++ sources.
# Enforced by pre-commit (staged files) and CI (whole tree).
#
# Usage:
#   tools/format.sh            # format all files
#   tools/format.sh --check    # check only, no modification (CI mode)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

CHECK_ONLY=0
if [[ "${1:-}" == "--check" ]]; then
	CHECK_ONLY=1
fi

FILES=$(find src include tests \
	-type f \( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null || true)

if [[ -z "${FILES}" ]]; then
	echo "[format] no source files found"
	exit 0
fi

CLANG_FMT="${CLANG_FORMAT:-clang-format}"
if ! command -v "${CLANG_FMT}" >/dev/null 2>&1; then
	echo "[format] clang-format not installed; skipping"
	exit 0
fi

if [[ ${CHECK_ONLY} -eq 1 ]]; then
	echo "[format] checking (no modification)"
	FAILED=0
	for f in ${FILES}; do
		if ! "${CLANG_FMT}" --style=file --dry-run "${f}" >/dev/null 2>&1; then
			echo "[format] FAIL: ${f}"
			FAILED=1
		fi
	done
	if [[ ${FAILED} -eq 1 ]]; then
		echo "[format] run tools/format.sh to fix"
		exit 1
	fi
	echo "[format] all clean"
else
	# shellcheck disable=SC2086
	"${CLANG_FMT}" --style=file -i ${FILES}
	echo "[format] formatted $(echo "${FILES}" | wc -l) files"
fi
