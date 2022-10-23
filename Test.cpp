// Test.cpp
// Dancing Links algorithm test and examples of use

#include "DancingLinks.h"

#include <stdio.h>

using namespace DancingLinks;

#define QUEENS 1
#define SUDOKU 1
#define PENTOMINO 1

int main()
{
#if QUEENS
    // N Queens problem
    {
        // Using 11 queens, should generate 2680 solutions
        constexpr int NUMBER_OF_QUEENS = 11;

        SparseMatrix* dlx = SparseMatrix::Create();

        // Note that the data can be optimized slightly further: there is no need to create conditions for the shortest
        // diagonals (those of just one cell). The net effect of that optimization would be rather small.
        for (int row = 0; row < NUMBER_OF_QUEENS; ++row)
            for (int col = 0; col < NUMBER_OF_QUEENS; ++col)
            {
                int r = col * NUMBER_OF_QUEENS + row;
                dlx->SetCondition(row, r);                              // one queen per row
                dlx->SetCondition(col + NUMBER_OF_QUEENS, r);           // one queen per col

                dlx->SetCondition(col + row + 2 * NUMBER_OF_QUEENS, r); // one queen per slash diagonal (see below)
                dlx->SetCondition(col - row + 5 * NUMBER_OF_QUEENS, r); // one queen per backslash diagonal (see below)
            }

        for (int i = 0; i < 2 * NUMBER_OF_QUEENS - 1; ++i)
            dlx->SetConditionOptional(i + 2 * NUMBER_OF_QUEENS);         // queens on diagonals are not required = no more than one queen per diagonal

        for (int i = -NUMBER_OF_QUEENS + 1; i < NUMBER_OF_QUEENS; ++i)
            dlx->SetConditionOptional(i + 5 * NUMBER_OF_QUEENS);

        // The actual algorithm product cannot be used easily for visual output
        std::vector<int> sol;
        sol.resize(NUMBER_OF_QUEENS);

        // Everything is setup, go through the solutions
        int counter = 0;
        for (auto s : dlx->Solve())
        {
            for (int x : s)
                sol[x % NUMBER_OF_QUEENS] = x / NUMBER_OF_QUEENS;

            printf("Solution %d:\n\r", ++counter);
            for (int x : sol)
            {
                for (int i = 0; i < NUMBER_OF_QUEENS; ++i)
                    putchar(i == x ? 'X' : '.');
                puts("");
            }

        }

        SparseMatrix::Destroy(dlx);
    }
#endif

#if SUDOKU
    // Sudoku
    {
        constexpr int CELL_START = 0;
        constexpr int ROW_START = 81;
        constexpr int COL_START = 162;
        constexpr int SQUARE_START = 243;

        SparseMatrix* dlx = SparseMatrix::Create();
        // Just run through every cell and every possible number on that cell. Each row, column,
        // and square has to have every number. Each cell has to have exactly one number
        // So, a digit N at row R and column C will form DLX row R*81+C*9+N-1 (R and C are 0-base).
        // This is actually a naive implementation and can be significantly optimized. See:
        // https://www.kth.se/social/files/58861771f276547fe1dbf8d1/HLaestanderMHarrysson_dkand14.pdf
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c)
                for (int n = 0; n < 9; ++n)
                {
                    int element = r * 81 + c * 9 + n;
                    int sq = (r / 3) * 3 + (c / 3);
                    dlx->SetCondition(CELL_START + 9 * r + c, element);
                    dlx->SetCondition(ROW_START + 9 * r + n, element);
                    dlx->SetCondition(COL_START + 9 * c + n, element);
                    dlx->SetCondition(SQUARE_START + 9 * sq + n, element);
                }

        // No optional conditions in this case

        // Now preset the puzzle. Normally, these should be read from file or standard input, hardcoded here
        // Actual puzzle source: https://en.wikipedia.org/wiki/Sudoku
#define SET(r, c, n) dlx->PreselectRow(r*81 + c*9 + n - 1);
        SET(0, 0, 5); SET(0, 1, 3); SET(0, 4, 7);
        SET(1, 0, 6); SET(1, 3, 1); SET(1, 4, 9); SET(1, 5, 5);
        SET(2, 1, 9); SET(2, 2, 8); SET(2, 7, 6);
        SET(3, 0, 8); SET(3, 4, 6); SET(3, 8, 3);
        SET(4, 0, 4); SET(4, 3, 8); SET(4, 5, 3); SET(4, 8, 1);
        SET(5, 0, 7); SET(5, 4, 2); SET(5, 8, 6);
        SET(6, 1, 6); SET(6, 6, 2); SET(6, 7, 8);
        SET(7, 3, 4); SET(7, 4, 1); SET(7, 5, 9); SET(7, 8, 5);
        SET(8, 4, 8); SET(8, 7, 7); SET(8, 8, 9);
#undef SET

        int sol[9][9];

        int counter = 0;
        for (auto s : dlx->Solve())
        {
            for (int x : s)
                sol[x / 81][(x/9) % 9] = x % 9 + 1;

            printf("Solution %d:\n\r\n\r", ++counter);
            for (int i = 0; i < 9; ++i)
            {
                for (int j = 0; j < 9; ++j)
                {
                    printf("%d%c", sol[i][j], j == 2 || j == 5 ? '|' : ' ');
                }
                puts("");
                if (i == 2 || i == 5)
                    puts("-----+-----+-----");
            }
            puts("");
        }

        SparseMatrix::Destroy(dlx);
    }
#endif

#if PENTOMINO
    // Pentominoes
    {
        // The most tedious part here - to preset all pieces. With all rotations and symmetries
        // there are 63 possible configurations. It may be more fun to generate all those programmatically
        // (and it would be the only feasible way with higher order puzzles) but this is good enough for this
        // test. The table below is modified from another project of mine, the data presentation is not ideal for this
        // one but I did not want to reenter everything from the scratch.
        struct PieceInfo
        {
            char type;
            int coverage[5]; // bitfield for five rows started with current one
        };
        const PieceInfo pieceInfo[] =
        {
            { 'F', 0x18, 0x30, 0x10, 0x00, 0x00 },
            { 'F', 0x08, 0x0e, 0x04, 0x00, 0x00 },
            { 'F', 0x08, 0x0c, 0x18, 0x00, 0x00 },
            { 'F', 0x08, 0x1c, 0x04, 0x00, 0x00 },
            { 'F', 0x18, 0x0c, 0x08, 0x00, 0x00 },
            { 'F', 0x08, 0x38, 0x10, 0x00, 0x00 },
            { 'F', 0x08, 0x18, 0x0c, 0x00, 0x00 },
            { 'F', 0x08, 0x1c, 0x10, 0x00, 0x00 },
            { 'I', 0xf8, 0x00, 0x00, 0x00, 0x00 },
            { 'I', 0x08, 0x08, 0x08, 0x08, 0x08 },
            { 'L', 0x78, 0x40, 0x00, 0x00, 0x00 },
            { 'L', 0x78, 0x08, 0x00, 0x00, 0x00 },
            { 'L', 0x08, 0x08, 0x08, 0x18, 0x00 },
            { 'L', 0x08, 0x08, 0x08, 0x0c, 0x00 },
            { 'L', 0x08, 0x78, 0x00, 0x00, 0x00 },
            { 'L', 0x08, 0x0f, 0x00, 0x00, 0x00 },
            { 'L', 0x18, 0x10, 0x10, 0x10, 0x00 },
            { 'L', 0x18, 0x08, 0x08, 0x08, 0x00 },
            { 'P', 0x18, 0x18, 0x08, 0x00, 0x00 },
            { 'P', 0x18, 0x18, 0x10, 0x00, 0x00 },
            { 'P', 0x08, 0x18, 0x18, 0x00, 0x00 },
            { 'P', 0x08, 0x0c, 0x0c, 0x00, 0x00 },
            { 'P', 0x38, 0x18, 0x00, 0x00, 0x00 },
            { 'P', 0x38, 0x30, 0x00, 0x00, 0x00 },
            { 'P', 0x18, 0x1c, 0x00, 0x00, 0x00 },
            { 'P', 0x18, 0x38, 0x00, 0x00, 0x00 },
            { 'N', 0x18, 0x70, 0x00, 0x00, 0x00 },
            { 'N', 0x18, 0x0e, 0x00, 0x00, 0x00 },
            { 'N', 0x38, 0x60, 0x00, 0x00, 0x00 },
            { 'N', 0x38, 0x0c, 0x00, 0x00, 0x00 },
            { 'N', 0x08, 0x18, 0x10, 0x10, 0x00 },
            { 'N', 0x08, 0x0c, 0x04, 0x04, 0x00 },
            { 'N', 0x08, 0x08, 0x18, 0x10, 0x00 },
            { 'N', 0x08, 0x08, 0x0c, 0x04, 0x00 },
            { 'T', 0x38, 0x10, 0x10, 0x00, 0x00 },
            { 'T', 0x08, 0x08, 0x1c, 0x00, 0x00 },
            { 'T', 0x08, 0x0e, 0x08, 0x00, 0x00 },
            { 'T', 0x08, 0x38, 0x08, 0x00, 0x00 },
            { 'U', 0x28, 0x38, 0x00, 0x00, 0x00 },
            { 'U', 0x38, 0x28, 0x00, 0x00, 0x00 },
            { 'U', 0x18, 0x08, 0x18, 0x00, 0x00 },
            { 'U', 0x18, 0x10, 0x18, 0x00, 0x00 },
            { 'V', 0x38, 0x08, 0x08, 0x00, 0x00 },
            { 'V', 0x38, 0x20, 0x20, 0x00, 0x00 },
            { 'V', 0x08, 0x08, 0x0e, 0x00, 0x00 },
            { 'V', 0x08, 0x08, 0x38, 0x00, 0x00 },
            { 'W', 0x18, 0x30, 0x20, 0x00, 0x00 },
            { 'W', 0x18, 0x0c, 0x04, 0x00, 0x00 },
            { 'W', 0x08, 0x18, 0x30, 0x00, 0x00 },
            { 'W', 0x08, 0x0c, 0x06, 0x00, 0x00 },
            { 'X', 0x08, 0x1c, 0x08, 0x00, 0x00 },
            { 'Y', 0x78, 0x10, 0x00, 0x00, 0x00 },
            { 'Y', 0x78, 0x20, 0x00, 0x00, 0x00 },
            { 'Y', 0x08, 0x3c, 0x00, 0x00, 0x00 },
            { 'Y', 0x08, 0x1e, 0x00, 0x00, 0x00 },
            { 'Y', 0x08, 0x18, 0x08, 0x08, 0x00 },
            { 'Y', 0x08, 0x0c, 0x08, 0x08, 0x00 },
            { 'Y', 0x08, 0x08, 0x18, 0x08, 0x00 },
            { 'Y', 0x08, 0x08, 0x0c, 0x08, 0x00 },
            { 'Z', 0x18, 0x10, 0x30, 0x00, 0x00 },
            { 'Z', 0x18, 0x08, 0x0c, 0x00, 0x00 },
            { 'Z', 0x08, 0x38, 0x20, 0x00, 0x00 },
            { 'Z', 0x08, 0x0e, 0x02, 0x00, 0x00 },
        };

        SparseMatrix* dlx = SparseMatrix::Create();

        // The puzzle is traditional - 6x10 rectangle
        // The code below can be fairly easily modified to solve any puzzle: just use field larger than 6x10, large
        // enough to cover the entire puzzle, and check each piece against the puzzle cells instead of the field
        // boundaries like below.
        for (int piece = 0; piece < 63; ++piece)
        {
            // Convert 'coverage' fields into offsets. x can actually be negative but y is always positive.
            struct point { int x, y; };
            point offset[5];
            int count = 0;
            for(int r=0; r<5; ++r)
                for(int off=0; off<8; ++off)
                    if (pieceInfo[piece].coverage[r] & (1 << off))
                    {
                        offset[count].y = r;
                        offset[count].x = off - 3; // 0x08 is considered (0,0)
                        ++count;
                    }

            // Go through all 60 cells
            for(int x = 0; x < 10; ++x)
                for (int y = 0; y < 6; ++y)
                {
                    // Does the piece fit when placed on this cell?
                    bool good = true;
                    for(int off=0; off<5; ++off)
                        if (x + offset[off].x >= 10 || x + offset[off].x < 0 || y + offset[off].y >= 6)
                        {
                            good = false;
                            break;
                        }

                    // If it does, add constraints for five cells that this piece covers and the piece type so
                    // each type will only be used once
                    int pieceHere = piece * 60 + y * 10 + x;
                    if (good)
                    {
                        for (int off = 0; off < 5; ++off)
                            dlx->SetCondition((x + offset[off].x) * 10 + y + offset[off].y, pieceHere);

                        dlx->SetCondition(4000 + pieceInfo[piece].type, pieceHere);
                    }
                }
        }

        // No optional conditions in this case and no preselected pieces

        // Run the solver
        // Note that there are no provisions against symmetry here so each solution will appear four times (expect
        // to see 9356 solutions). The easiest trick to eliminate redundant solutions is to remove some transformations
        // for one of the pieces (like reducing symmetries for L from 8 to 2).
        char sol[6][10];

        int counter = 0;
        for (auto s : dlx->Solve())
        {
            for (int x : s)
            {
                for (int r = 0; r < 5; ++r)
                    for (int off = 0; off < 8; ++off)
                        if (pieceInfo[x/60].coverage[r] & (1 << off))
                        {
                            sol[(x / 10) % 6 + r][x % 10 + off - 3] = pieceInfo[x / 60].type;
                        }

                sol[(x / 10) % 6][x % 10] = pieceInfo[x / 60].type;
            }
 
            printf("Solution %d:\n\r\n\r", ++counter);
            for (int i = 0; i < 6; ++i)
            {
                printf("%.10s", sol[i]);
                puts("");
            }
            puts("");
        }



        SparseMatrix::Destroy(dlx);
    }
#endif

    return 0;
}

