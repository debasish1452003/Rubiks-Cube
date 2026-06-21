// tests.cpp — lightweight self-contained test suite for the Rubik's Cube engine.
//
// These tests encode the *physical invariants* of a real cube. They are the
// same invariants that exposed a geometry bug in the r() move during
// development (corner reads produced impossible pieces like W-R-O, where Red
// and Orange are opposite faces). Keeping them as regression tests means that
// class of bug can never silently return.
//
// Build:  g++ -std=c++20 -O2 tests.cpp Model/RubiksCube.cpp \
//             PatternDatabases/CornerPatternDatabase.cpp PatternDatabases/PatternDatabase.cpp \
//             PatternDatabases/NibbleArray.cpp PatternDatabases/math.cpp -o tests
// Run:    ./tests

#include <bits/stdc++.h>

#include "Model/RubiksCube.h"
#include "Model/RubiksCube3DArray.cpp"
#include "Model/RubiksCubeBitBoard.cpp"

#include "Solver/BFSSolver.h"
#include "Solver/IDDFSolver.h"

using namespace std;

// ----- tiny test framework ---------------------------------------------------
static int g_passed = 0, g_failed = 0;
#define CHECK(cond, msg)                                              \
    do {                                                             \
        if (cond) { ++g_passed; }                                    \
        else { ++g_failed; cout << "  [FAIL] " << msg << "\n"; }     \
    } while (0)

static void section(const string &name) { cout << "== " << name << " ==\n"; }

// The 8 physically valid corner pieces (sorted color letters). On a real cube,
// opposite-face colors (W/Y, R/O, G/B) never share a corner.
static const set<string> VALID_CORNERS = {
    "BOW", "BOY", "BRW", "BRY", "GOW", "GOY", "GRW", "GRY"};

static string sortedCorner(const string &s) {
    string t = s; sort(t.begin(), t.end()); return t;
}

template <typename Cube>
static set<string> cornerSet(Cube &c) {
    set<string> s;
    for (int k = 0; k < 8; k++) s.insert(sortedCorner(c.getCornerColorString(k)));
    return s;
}

// ----- tests -----------------------------------------------------------------

// A solved cube is solved; a single move un-solves it.
template <typename Cube>
static void test_solved_state(const string &model) {
    Cube c;
    CHECK(c.isSolved(), model + ": fresh cube is solved");
    c.move(RubiksCube::MOVE::R);
    CHECK(!c.isSolved(), model + ": one move un-solves");
}

// Any quarter turn applied 4x (or half turn 2x) returns to solved.
template <typename Cube>
static void test_move_identities(const string &model) {
    for (int i = 0; i < 18; i++) {
        Cube c;
        int reps = (i % 3 == 2) ? 2 : 4;  // L2/R2/... are at index%3==2
        for (int r = 0; r < reps; r++) c.move(RubiksCube::MOVE(i));
        CHECK(c.isSolved(), model + ": move " + RubiksCube::getMove(RubiksCube::MOVE(i)) +
                                " x" + to_string(reps) + " == identity");
    }
}

// move(m) and invert(m) are inverses.
template <typename Cube>
static void test_move_inverse(const string &model) {
    for (int i = 0; i < 18; i++) {
        Cube c;
        c.move(RubiksCube::MOVE(i));
        c.invert(RubiksCube::MOVE(i));
        CHECK(c.isSolved(), model + ": invert undoes move " +
                                RubiksCube::getMove(RubiksCube::MOVE(i)));
    }
}

// A random scramble followed by its reversed inverse returns to solved.
// Proves the move set forms a consistent permutation group.
template <typename Cube>
static void test_scramble_then_inverse(const string &model) {
    srand(20240611);
    int fails = 0;
    for (int t = 0; t < 500; t++) {
        Cube c;
        vector<int> seq;
        for (int s = 0; s < 25; s++) { int m = rand() % 18; seq.push_back(m); c.move(RubiksCube::MOVE(m)); }
        for (int s = (int)seq.size() - 1; s >= 0; s--) c.invert(RubiksCube::MOVE(seq[s]));
        if (!c.isSolved()) fails++;
    }
    CHECK(fails == 0, model + ": 500 scramble+inverse round-trips (fails=" + to_string(fails) + ")");
}

// THE regression test for the r()-geometry bug: every reachable state must read
// 8 valid, distinct corner pieces. A geometry bug makes fixed facelet reads
// produce impossible corners.
template <typename Cube>
static void test_corner_invariants(const string &model) {
    srand(424242);
    int bad = 0;
    for (int t = 0; t < 3000; t++) {
        Cube c;
        for (int s = 0; s < 20; s++) c.move(RubiksCube::MOVE(rand() % 18));
        if (cornerSet(c) != VALID_CORNERS) bad++;
    }
    CHECK(bad == 0, model + ": 3000 scrambles all read 8 valid distinct corners (bad=" +
                        to_string(bad) + ")");
}

// Solvers must return a move list that actually solves the scramble.
static void test_solver_correctness() {
    srand(7);
    for (int trial = 0; trial < 20; trial++) {
        RubiksCubeBitboard cube;
        cube.randomShuffleCube(5);

        RubiksCubeBitboard forIddfs = cube;
        IDDFSSolver<RubiksCubeBitboard, HashBitBoard> solver(forIddfs, 8);
        auto moves = solver.solve();

        RubiksCubeBitboard check = cube;
        for (auto m : moves) check.move(m);
        CHECK(check.isSolved(), "IDDFS solution actually solves scramble (trial " + to_string(trial) + ")");
    }
}

// BFS / IDDFS find optimal-length solutions for short scrambles.
static void test_optimal_length() {
    RubiksCubeBitboard cube;
    // A single move scramble must be solvable in exactly 1 move.
    cube.move(RubiksCube::MOVE::R);
    BFSSolver<RubiksCubeBitboard, HashBitBoard> bfs(cube);
    auto moves = bfs.solve();
    CHECK(moves.size() == 1, "BFS solves a 1-move scramble in 1 move (got " +
                                 to_string(moves.size()) + ")");
}

int main() {
    cout << "Running Rubik's Cube engine tests...\n\n";

    section("Bitboard model");
    test_solved_state<RubiksCubeBitboard>("bitboard");
    test_move_identities<RubiksCubeBitboard>("bitboard");
    test_move_inverse<RubiksCubeBitboard>("bitboard");
    test_scramble_then_inverse<RubiksCubeBitboard>("bitboard");
    test_corner_invariants<RubiksCubeBitboard>("bitboard");

    section("3D-array model");
    test_solved_state<RubiksCube3dArray>("3darray");
    test_move_identities<RubiksCube3dArray>("3darray");
    test_move_inverse<RubiksCube3dArray>("3darray");
    test_scramble_then_inverse<RubiksCube3dArray>("3darray");
    test_corner_invariants<RubiksCube3dArray>("3darray");

    section("Solvers");
    test_solver_correctness();
    test_optimal_length();

    cout << "\n----------------------------------------\n";
    cout << "PASSED: " << g_passed << "   FAILED: " << g_failed << "\n";
    return g_failed == 0 ? 0 : 1;
}
