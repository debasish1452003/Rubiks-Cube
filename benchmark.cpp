// benchmark.cpp — compares the four solvers on identical scrambles.
//
// For each scramble depth it runs every applicable solver on the SAME scramble
// and reports average solve time and average solution length over N trials.
// IDA* requires the corner pattern database (Databases/cornerDepth.bin).
//
// Build:  g++ -std=c++20 -O2 benchmark.cpp Model/RubiksCube.cpp \
//             PatternDatabases/CornerPatternDatabase.cpp PatternDatabases/PatternDatabase.cpp \
//             PatternDatabases/NibbleArray.cpp PatternDatabases/math.cpp -o benchmark
// Run:    ./benchmark

#include <bits/stdc++.h>

#include "Model/RubiksCube.h"
#include "Model/RubiksCubeBitBoard.cpp"

#include "Solver/BFSSolver.h"
#include "Solver/DFSSolver.h"
#include "Solver/IDDFSolver.h"
#include "Solver/IDAstarSolver.h"

using namespace std;

struct Result { double ms = 0; double len = 0; int runs = 0; bool ran = false; };

static double timeMs(function<void()> fn) {
    auto a = chrono::high_resolution_clock::now();
    fn();
    auto b = chrono::high_resolution_clock::now();
    return chrono::duration<double, milli>(b - a).count();
}

int main() {
    const string dbPath = "Databases/cornerDepth.bin";
    bool haveDb = ifstream(dbPath, ios::binary).good();

    cout << "Rubik's Cube solver benchmark\n";
    cout << "Pattern DB: " << (haveDb ? "found (IDA* enabled)" : "MISSING (run ./solver makedb)") << "\n\n";

    // Scramble depths to test, and how many random scrambles per depth.
    vector<int> depths = {3, 5, 7, 9, 11};
    int trials = 5;

    cout << left << setw(8) << "depth" << setw(10) << "solver"
         << right << setw(12) << "avg ms" << setw(12) << "avg moves"
         << setw(10) << "trials" << "\n";
    cout << string(64, '-') << "\n";

    srand(13);
    for (int depth : depths) {
        // Pre-generate the scrambles so every solver sees the same set.
        vector<RubiksCubeBitboard> scrambles;
        for (int t = 0; t < trials; t++) {
            RubiksCubeBitboard c;
            for (int s = 0; s < depth; s++) c.move(RubiksCube::MOVE(rand() % 18));
            scrambles.push_back(c);
        }

        Result bfs, iddfs, ida;

        // BFS and IDDFS are uninformed and blow up combinatorially, so cap them:
        // BFS holds the whole frontier in memory (worst), IDDFS is time-bound only.
        bool runBfs = depth <= 5;
        bool runIddfs = depth <= 7;

        for (auto &scr : scrambles) {
            if (runBfs) {
                RubiksCubeBitboard c = scr;
                BFSSolver<RubiksCubeBitboard, HashBitBoard> s(c);
                vector<RubiksCube::MOVE> mv;
                bfs.ms += timeMs([&] { mv = s.solve(); });
                bfs.len += mv.size(); bfs.runs++; bfs.ran = true;
            }
            if (runIddfs) {
                RubiksCubeBitboard c = scr;
                IDDFSSolver<RubiksCubeBitboard, HashBitBoard> s(c, depth + 1);
                vector<RubiksCube::MOVE> mv;
                iddfs.ms += timeMs([&] { mv = s.solve(); });
                iddfs.len += mv.size(); iddfs.runs++; iddfs.ran = true;
            }
            if (haveDb) {
                RubiksCubeBitboard c = scr;
                IDAstartSolver<RubiksCubeBitboard, HashBitBoard> s(c, dbPath);
                vector<RubiksCube::MOVE> mv;
                ida.ms += timeMs([&] { mv = s.solve(); });
                ida.len += mv.size(); ida.runs++; ida.ran = true;
            }
        }

        auto row = [&](const string &name, Result &r) {
            if (!r.ran) return;
            cout << left << setw(8) << depth << setw(10) << name << right << fixed
                 << setprecision(2) << setw(12) << (r.ms / r.runs)
                 << setprecision(1) << setw(12) << (r.len / r.runs)
                 << setw(10) << r.runs << "\n";
        };
        row("BFS", bfs);
        row("IDDFS", iddfs);
        row("IDA*", ida);
        cout << string(64, '-') << "\n";
        cout.flush();
    }

    cout << "\nNotes:\n";
    cout << " - BFS/IDDFS are uninformed; they explode in time/memory past ~7 moves\n";
    cout << "   and are capped here. IDA* with the pattern-DB heuristic scales much further.\n";
    cout << " - BFS and IDDFS return optimal (shortest) solutions; DFS does not.\n";
    return 0;
}
