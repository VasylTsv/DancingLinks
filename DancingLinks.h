// DancingLinks.cpp
// Dancing Links algorithm X implementation by Vasyl Tsvirkunov
// See:
//        https://arxiv.org/pdf/cs/0011047.pdf
//        https://en.wikipedia.org/wiki/Dancing_Links

#include <vector>
#include <functional>

// Second version of Solve returns all solutions through coroutine
#include <experimental/generator>

namespace DancingLinks
{
    class SparseMatrix
    {
    public:
        // The solver has to be acquired through Create and disposed of using Destroy
        static SparseMatrix* Create();
        static void Destroy(SparseMatrix* ptr);

        // Set constraint condition. In the final solution all conditions must be satisfied by exactly one row
        // except for conditions marked as optional - those must be satisfied at most by one row.
        virtual void SetCondition(int c, int r) = 0;
        // Set condition to optional state so it is not required to be satisfied but still checks for conflicts.
        // Note that all conditions need to be set before marking any as optional.
        virtual void SetConditionOptional(int c) = 0;
        // Mark row as required part of the solution. All conditions need to be set before calling this.
        virtual void PreselectRow(int r) = 0;

        // Two different ways to generate solution - the first one will use callbacks when trying and undoing rows, and on complete. The second
        // one will just produce the sequence of solutions via coroutine.
        virtual void Solve(std::function<void(int)> tryRow, std::function<void(int)> undoRow, std::function<void()> complete) = 0;
        virtual std::experimental::generator<const std::vector<int>> Solve() = 0;
    };
}
