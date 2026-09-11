# Changelog

All notable changes to HONGFAN are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).
Versioning follows [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added — M1 completion engine (first runnable milestone)

- Deterministic online completion kernel: temporal fact graph, six rule kinds,
  three-phase ticks (evolution → transformation → behavior), transactional
  application with invariant rollback, seeded SplitMix64 RNG — a Run tuple
  (ruleset hash, state hash, strategy, seed) replays byte-identically
- Slice interpolation (jiyi): forward DFS enumeration + goal test, where
  enumerating is executing forward, hence verifying; solution policies
  random / min-cost / prefer:<rule>
- YAML DSL subset with load-time validation (binding closure, type and
  property existence, rand placement) — in-tree controlled-subset parser,
  zero third-party runtime dependencies
- Chronicle rendering: Chinese narration templates, per-tick grouping,
  rejected-step markers; golden-file determinism harness (dual-run `cmp` +
  regression diffs)
- Two example worlds: village (online derivation, 8 ticks) and traveler
  (interpolation, 160 solutions at depth 6)
- CLI `hongfan derive|complete` with Run-tuple trace, chronicle and final
  snapshot output

### Added — design documentation

- Concept design v0.3: unified completion primitive, slices with confidence,
  three-state solution space, scopes with partitioned determinism, instance
  rules as data, attention expand/condense duality
- Implementation design: C++ language ADR, LLM boundary ADR, M1 data
  structures, normative determinism contract, task breakdown
- 17 architecture decision records under docs/adr/

### Added — project infrastructure

- Apache-2.0 license, NOTICE, VERSION, dependency allowlist
- CMake presets (dev / release / asan / ubsan), format and static-analysis
  configuration, CI workflow

### Earlier (already on main before this changelog)

- docs: v0.1 concept design of the world rule engine
- docs: unify derivation/reconstruction into trajectory completion (v0.2)
