# Rubik's Cube Solver (C++)

A 3×3×3 Rubik's Cube engine and solver written in modern C++. It models the
cube three different ways, implements four search algorithms, and uses a
**corner pattern database** as an admissible heuristic to drive an
**IDA\*** solver that solves scrambled cubes in milliseconds.

> Originally built while following a data-structures course, then extended into
> a tested, benchmarked, and debugged system. See
> [What I added on top of the tutorial](#what-i-added-on-top-of-the-tutorial).

---

## Highlights

- **Three cube representations** behind one abstract interface (`RubiksCube`):
  a human-readable 3D array, a flat 1D array, and a bit-packed **bitboard**
  (each face stored in a single `uint64_t`, moves done with bit shifts/masks).
- **Four solvers**, all templated on the cube model and its hash:
  - `BFSSolver` — optimal, but memory-bound.
  - `DFSSolver` — depth-limited; finds *a* solution, not the shortest.
  - `IDDFSSolver` — iterative-deepening DFS; optimal with DFS-like memory.
  - `IDAstarSolver` — IDA\* guided by a corner pattern-database heuristic.
- **Corner pattern database**: all 8-corner configurations
  (8! × 3⁷ ≈ 88.2M states) are BFS-enumerated and their solve-depths stored
  **4 bits per entry** in a `NibbleArray` (≈42 MB on disk), indexed by a
  **Lehmer-code permutation rank** (`PermutationIndexer`).
- **101-assertion test suite** and a **benchmark harness** comparing all solvers.

### Benchmark (avg over 5 random scrambles per depth, GCC -O2)

| scramble depth | BFS | IDDFS | IDA\* |
|---:|---:|---:|---:|
| 3  | 17 ms | 0.07 ms | **0.06 ms** |
| 5  | 5,558 ms | 15 ms | **0.35 ms** |
| 7  | — (capped) | 2,825 ms | **1.55 ms** |
| 9  | — | — | **3.1 ms** |
| 11 | — | — | **52 ms** |

All three find the same optimal solution length where they run. The uninformed
searches (BFS/IDDFS) explode combinatorially and become impractical past ~7
moves; IDA\* with the pattern-database heuristic stays in the millisecond range.
At depth 5 IDA\* is roughly **16,000× faster than BFS**. (Run `make benchmark`
to reproduce on your machine.)

---

## Build & run

Requires a C++20 compiler (developed with MinGW-w64 g++ 15.2 / GCC). The code
uses `<bits/stdc++.h>`, so a GCC-family toolchain is expected.

```sh
make              # builds ./solver
make tests        # builds and runs the test suite
make benchmark    # builds ./benchmark
make db           # generates the corner pattern database (needed for IDA*)
```

Or compile directly:

```sh
g++ -std=c++20 -O2 main.cpp Model/RubiksCube.cpp \
    PatternDatabases/CornerDBMaker.cpp PatternDatabases/CornerPatternDatabase.cpp \
    PatternDatabases/PatternDatabase.cpp PatternDatabases/NibbleArray.cpp \
    PatternDatabases/math.cpp -o solver
```

### Using the CLI

```sh
./solver <algorithm> [scrambleLength]
#   algorithm = bfs | dfs | iddfs | ida | makedb
#   scrambleLength defaults to 6

./solver makedb        # generate Databases/cornerDepth.bin first (one-time, ~90s)
./solver ida 9         # scramble 9 moves, solve with IDA*
./solver iddfs 6       # solve a 6-move scramble with iterative deepening
```

The driver scrambles a cube, prints it, solves it, then prints the solution,
the solve time, and a `Verified solved: YES/NO` check that re-applies the
solution to confirm correctness.

Example:

```
Scramble (6 moves): D U F F D2 U'
Solver: ida
Solution (5 moves): D2 U F2 U' D'
Time: 61.56 ms
Verified solved: YES
```

---

## How it works

### Cube model
Each model implements 18 moves (`U, U', U2, …` for all six faces) behind the
`RubiksCube` abstract base class, so solvers are written once and run against
any representation. The **bitboard** is the performance model: each face is 8
facelet-bytes packed into a `uint64_t`, a face turn is a couple of shifts, and
side-effects on neighbouring faces are masked copies.

### Pattern-database heuristic
IDA\* needs an *admissible* heuristic (never overestimates). The corner pattern
database precomputes, for every possible arrangement of the 8 corner cubies,
the minimum number of moves to solve **just the corners** — a guaranteed
lower bound on the full solve. Because the real solve can only be longer, this
heuristic is admissible, so IDA\* still returns optimal solutions while
pruning the search dramatically.

The database is built once by BFS from the solved state and serialized to
`Databases/cornerDepth.bin`.

---

## Tests

```sh
make tests
```

The suite (`tests.cpp`) checks physical invariants that any correct cube engine
must satisfy, for **both** the bitboard and 3D-array models:

- a fresh cube is solved; one move un-solves it
- every quarter turn ×4 (and half turn ×2) is the identity
- `invert(m)` undoes `move(m)`
- 500 random scramble + reversed-inverse round-trips return to solved
  (proves the moves form a consistent permutation group)
- **3000 random scrambles each read 8 valid, distinct corner pieces**
  (the regression test for the bug described below)
- solver solutions actually solve their scramble; BFS returns optimal length

---

## What I added on top of the tutorial

The tutorial leaves you with model and solver classes but no runnable program.
Turning it into a working, trustworthy project meant:

1. **Made it build and run.** Fixed several compile/link blockers (a duplicated
   constructor, a missing class semicolon, a misspelled member, an unimplemented
   `PatternDatabase::isFull()`, and a missing `CornerPatternDatabase`
   implementation), and wrote the `main.cpp` driver that ties scramble → solve
   → verify together.

2. **Found and fixed a real geometry bug.** IDA\* crashed with an out-of-range
   index into the pattern database. I traced it down with invariants: the cube
   was a *valid permutation group* (scramble+inverse always solved, every
   move⁴ = identity) — yet ~16% of corner reads were physically impossible
   (e.g. a corner reading White-Red-**Orange**, but Red and Orange are opposite
   faces and can never meet). Isolating per-move showed only the **`R` move**
   was at fault; a brute-force over the back-face sticker cycle pinned the fix
   to a two-element swap in the ring order. After the fix: 0 invalid reads over
   thousands of scrambles, and IDA\* solves 6–9 move scrambles in ~60 ms.

3. **Added a regression test suite** that encodes those same invariants, so the
   bug class can't silently return.

4. **Added a benchmark harness** comparing all solvers on identical scrambles
   (time and solution length vs. scramble depth).

5. **Added build tooling** (Makefile) and this documentation.

---

## Project layout

```
Model/                  cube representations + abstract interface
  RubiksCube.{h,cpp}      base class, move dispatch, printing, corner reads
  RubiksCube3DArray.cpp   readable [6][3][3] model
  RubiksCube1dArray.cpp   flat-array model
  RubiksCubeBitBoard.cpp  bit-packed uint64_t-per-face model (used by solvers)
Solver/                 header-only, templated on <CubeModel, Hash>
  BFSSolver.h  DFSSolver.h  IDDFSolver.h  IDAstarSolver.h
PatternDatabases/       corner DB: storage, indexing, generation
  NibbleArray.*           4-bits-per-entry packed array
  PermutationIndexer.h    Lehmer-code permutation ranking
  CornerPatternDatabase.* corner-state index + DB
  CornerDBMaker.*         BFS generator
  math.*                  factorial / permutation helpers
main.cpp                CLI driver
tests.cpp               invariant + solver test suite
benchmark.cpp           solver comparison harness
Makefile
```
