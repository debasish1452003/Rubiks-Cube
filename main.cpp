#include <bits/stdc++.h>

#include "Model/RubiksCube.h"
#include "Model/RubiksCube3DArray.cpp"
#include "Model/RubiksCubeBitBoard.cpp"

#include "Solver/DFSSolver.h"
#include "Solver/BFSSolver.h"
#include "Solver/IDDFSolver.h"
#include "Solver/IDAstarSolver.h"

#include "PatternDatabases/CornerDBMaker.h"

using namespace std;

static string movesToString(const vector<RubiksCube::MOVE> &moves)
{
    string out;
    for (auto m : moves)
        out += RubiksCube::getMove(m) + " ";
    return out;
}

// Apply a list of moves to a fresh solved cube and confirm it ends solved.
template <typename CubeT>
static bool verifySolution(CubeT scrambled, const vector<RubiksCube::MOVE> &solution)
{
    for (auto m : solution)
        scrambled.move(m);
    return scrambled.isSolved();
}

int main(int argc, char **argv)
{
    string algorithm = (argc > 1) ? argv[1] : "ida";
    int scrambleLen = (argc > 2) ? stoi(argv[2]) : 6;

    RubiksCubeBitboard cube;
    auto scramble = cube.randomShuffleCube(scrambleLen);

    cout << "Scramble (" << scrambleLen << " moves): " << movesToString(scramble) << "\n";
    cout << "Scrambled cube:\n";
    cube.print();

    auto start = chrono::high_resolution_clock::now();
    vector<RubiksCube::MOVE> solution;

    if (algorithm == "bfs")
    {
        BFSSolver<RubiksCubeBitboard, HashBitBoard> solver(cube);
        solution = solver.solve();
        cube = solver.rubiksCube;
    }
    else if (algorithm == "dfs")
    {
        DFSSolver<RubiksCubeBitboard, HashBitBoard> solver(cube, 8);
        solution = solver.solve();
        cube = solver.rubiksCube;
    }
    else if (algorithm == "iddfs")
    {
        IDDFSSolver<RubiksCubeBitboard, HashBitBoard> solver(cube, 8);
        solution = solver.solve();
        cube = solver.rubiksCube;
    }
    else if (algorithm == "ida")
    {
        string dbPath = "Databases/cornerDepth.bin";
        IDAstartSolver<RubiksCubeBitboard, HashBitBoard> solver(cube, dbPath);
        solution = solver.solve();
        cube = solver.rubiksCube;
    }
    else if (algorithm == "makedb")
    {
        CornerDBMaker maker("Databases/cornerDepth.bin", 0xFF);
        maker.bfsAndStore();
        cout << "Corner pattern database written to Databases/cornerDepth.bin\n";
        return 0;
    }
    else
    {
        cerr << "Unknown algorithm: " << algorithm << "\n";
        cerr << "Usage: " << argv[0] << " [bfs|dfs|iddfs|ida|makedb] [scrambleLen]\n";
        return 1;
    }

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Solver: " << algorithm << "\n";
    cout << "Solution (" << solution.size() << " moves): " << movesToString(solution) << "\n";
    cout << "Solved cube:\n";
    cube.print();
    cout << fixed << setprecision(2);
    cout << "Time: " << ms << " ms\n";
    cout << "Verified solved: " << (cube.isSolved() ? "YES" : "NO") << "\n";

    return 0;
}
