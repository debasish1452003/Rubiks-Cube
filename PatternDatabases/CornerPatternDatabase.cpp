#include "CornerPatternDatabase.h"
using namespace std;

// 8! permutations of corners (40320) * 3^7 orientations (2187) = 88,179,840 states.
// The 8th corner's orientation is fixed by the other 7 (orientations sum to 0 mod 3),
// so we only encode 7 of them.
const uint32_t CORNER_DB_SIZE = 88179840;

CornerPatternDatabase::CornerPatternDatabase() : PatternDatabase(CORNER_DB_SIZE)
{
}

CornerPatternDatabase::CornerPatternDatabase(uint8_t init_val) : PatternDatabase(CORNER_DB_SIZE, init_val)
{
}

uint32_t CornerPatternDatabase::getDatabaseIndex(const RubiksCube &cube) const
{
    array<uint8_t, 8> perm{};
    uint32_t orientation = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        perm[i] = cube.getCornerIndex(i);
        // base-3 number built from the first 7 corner orientations
        if (i < 7)
        {
            orientation = orientation * 3 + cube.getCornerOrientation(i);
        }
    }

    uint32_t permRank = this->permIndexer.rank(perm);

    // 3^7 = 2187 orientation states per permutation
    return permRank * 2187 + orientation;
}
