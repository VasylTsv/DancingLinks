// DancingLinks.cpp
// Dancing Links algorithm implementation by Vasyl Tsvirkunov
// See:
//        https://arxiv.org/pdf/cs/0011047.pdf
//        https://en.wikipedia.org/wiki/Dancing_Links

#include <assert.h>
#include <vector>
#include <limits>
#include <experimental/generator>

#include "DancingLinks.h"

using namespace std;

namespace DancingLinks
{
    // Single cell in the sparse matrix. This class is used for regular cells but also for column headers and the root.
    class SetCell
    {
    public:
        enum { up = 0, down = 1, left = 2, right = 3 };

    private:
        SetCell* link[4];

    public:
        // Each cell knows its row (has to be maxint for the header)
        int row = numeric_limits<int>::max();

        union
        {
            int col;       // Each non-header cell knows its column
            int counter;   // Column header does not need column but it stores the count of ones in that column (maxint for root)
        };

        SetCell()
        {
            link[0] = link[1] = link[2] = link[3] = this;
        }

        SetCell(const SetCell&) = delete;
        SetCell(SetCell&&) = delete;

        void InsertAbove(SetCell* target)
        {
            link[down] = target;
            link[up] = target->link[up];
            target->link[up]->link[down] = this;
            target->link[up] = this;
        }

        void InsertBefore(SetCell* target)
        {
            link[right] = target;
            link[left] = target->link[left];
            target->link[left]->link[right] = this;
            target->link[left] = this;
        }

        void ColumnDetach()
        {
            link[up]->link[down] = link[down];
            link[down]->link[up] = link[up];
        }

        void RowDetach()
        {
            link[left]->link[right] = link[right];
            link[right]->link[left] = link[left];
        }

        void ColumnRestore()
        {
            link[up]->link[down] = link[down]->link[up] = this;
        }

        void RowRestore()
        {
            link[left]->link[right] = link[right]->link[left] = this;
        }

        void Orphan()
        {
            link[left] = link[right] = this;
        }

        template<int dir> SetCell* Move()
        {
            return link[dir];
        }

        template<int dir> experimental::generator<SetCell*> Traverse()
        {
            for (auto ptr = link[dir]; ptr != this; ptr = ptr->link[dir])
                co_yield ptr;
        }
    };

    // Concrete sparse matrix class
    class SparseMatrixImp final : public SparseMatrix
    {
    private:
        // Root for the linked list of column headers (left-right). Always exists.
        SetCell* mRoot;

        // Column headers. These are not real cells as they don't correspond any constraints by themselves,
        // rather carrying column information. Note that not all headers have to be linked in the list
        // off the root (e.g., optional constraints are not linked) and hiding columns temporarily unlinks
        // from that list as well.
        // Note that due to the implementation details cells in each column are sorted by row but columns
        // themselves are not sorted.
        std::vector<SetCell*> mColumns;
        // Row headers. Point to actual cells in the matrix (no extra per-row information needed). Rows are
        // linked in the list but the cells are not sorted.
        std::vector<SetCell*> mRows;

        // All preselected rows are recorded in this vector so they are prepended to every solution.
        std::vector<int> mSolutionPrefix;

        // Algorithm state for call flow validation.
        enum State { init, setup, options, solving, done };
        State state;
        void ValidateState(State s);

        // Ensure the column header exists and find an element just below the insertion point (or the element
        // with the same row and column which allows to check for duplicates).
        SetCell* GetByColumn(int c, int r);
        // Ensure the spot in the row header
        SetCell*& GetRow(int r);

        void HideColumn(SetCell* ptr);
        void UnhideColumn(SetCell* ptr);

        void SolveImp(std::function<void(int)> tryRow, std::function<void(int)> undoRow, std::function<void()> complete);

        SetCell* MostConstrainedColumn();
        void Cover(std::function<void(int)> tryRow, SetCell* cell);
        void Uncover(std::function<void(int)> undoRow, SetCell* cell);

    public:
        SparseMatrixImp();
        SparseMatrixImp(const SparseMatrixImp&) = delete;
        SparseMatrixImp(SparseMatrixImp&&) = delete;
        ~SparseMatrixImp();

        // SparseMatrix implementation
        virtual void SetCondition(int c, int r) override;
        virtual void SetConditionOptional(int c) override;
        virtual void PreselectRow(int r) override;

        virtual void Solve(std::function<void(int)> tryRow, std::function<void(int)> undoRow, std::function<void()> complete) override;
        virtual std::experimental::generator<const std::vector<int>> Solve() override;
    };

    // Class factory calls
    SparseMatrix* SparseMatrix::Create()
    {
        return new SparseMatrixImp;
    }

    void SparseMatrix::Destroy(SparseMatrix* ptr)
    {
        delete ptr;
    }

    void SparseMatrixImp::ValidateState(State s)
    {
        assert(state <= s);
        state = s;
    }


    SetCell* SparseMatrixImp::GetByColumn(int c, int r)
    {
    // Ensure that the column header has a space for the new column
        if((int)mColumns.size() <= c)
            mColumns.resize(c + 1, nullptr);

        SetCell* ptr = mColumns[c];

    // Create a brand new column and return the header element as the insertion point
        if (ptr == nullptr)
        {
            ptr = mColumns[c] = new SetCell;
            ptr->counter = 0;
            ptr->InsertBefore(mRoot);
            return ptr;
        }

    // Lookup for the best insertion point in the existing column - return the closest
    // element by row #. This will allow to test for duplicates. It also makes the columns
    // sorted, not that it is required for the algorithm to work.
        auto cell = ptr;
        for (auto test : ptr->Traverse<SetCell::down>())
            if (test->row >= r && test->row < cell->row)
                cell = test;

        return cell;
    }

    SetCell*& SparseMatrixImp::GetRow(int r)
    {
    // Only need to ensure that the header has space for this new row
        if ((int)mRows.size() <= r)
            mRows.resize(r + 1, nullptr);

        return mRows[r];
    }


    SparseMatrixImp::SparseMatrixImp()
    {
        mRoot = new SetCell;
        mRoot->counter = numeric_limits<int>::max();
        state = init;
    }

    SparseMatrixImp::~SparseMatrixImp()
    {
        // The row links stay intact all the time making it very easy to do a complete sweep
        for(auto rowStart : mRows)
            if (rowStart)
            {
                auto condemned = rowStart;
                for (auto x : rowStart->Traverse<SetCell::right>())
                {
                    delete condemned;
                    condemned = x;
                }
                delete condemned;
            }
        mRows.clear();

        // Column headers
        for (auto col : mColumns)
            if (col)
                delete col;
        mColumns.clear();

        delete mRoot;
        mRoot = nullptr;
    }

    void SparseMatrixImp::SetCondition(int c, int r)
    {
        ValidateState(setup);
        assert(c >= 0 && r >= 0);

        SetCell* ptrByCol = GetByColumn(c, r);
        if (ptrByCol->row != r) // duplicates are silently ignored
        {
            SetCell* newCell = new SetCell;
            newCell->col = c;
            newCell->row = r;

            newCell->InsertAbove(ptrByCol); // this will make the column sorted (not actually required for the algorithm to work)

            auto& rowPtr = GetRow(r);
            if (rowPtr == nullptr)
                rowPtr = newCell;
            else
                newCell->InsertBefore(rowPtr);

            mColumns[c]->counter++;
        }
    }

    void SparseMatrixImp::SetConditionOptional(int c)
    {
        ValidateState(options);
        assert(c >= 0 && c < (int)mColumns.size());

        auto ptr = mColumns[c];
        if (ptr != nullptr)
        {
            ptr->RowDetach();
            ptr->Orphan();
        }
    }

    void SparseMatrixImp::PreselectRow(int r)
    {
        ValidateState(options);
        assert(r >= 0 && r < (int)mRows.size());

        if (std::find(mSolutionPrefix.begin(), mSolutionPrefix.end(), r) == mSolutionPrefix.end())
        {
            SetCell* rowHeader = mRows[r];
            if (rowHeader)
            {
                HideColumn(mColumns[rowHeader->col]);
                for (auto c : rowHeader->Traverse<SetCell::right>())
                {
                    HideColumn(mColumns[c->col]);
                }
            }
            mSolutionPrefix.push_back(r);
        }
    }

    void SparseMatrixImp::HideColumn(SetCell* ptr)
    {
        if (ptr != nullptr)
        {
            ptr->RowDetach();

            for (auto i : ptr->Traverse<SetCell::down>())
                for (auto j : i->Traverse<SetCell::right>())
                {
                    j->ColumnDetach();
                    --mColumns[j->col]->counter;
                }
        }
    }

    void SparseMatrixImp::UnhideColumn(SetCell* ptr)
    {
        if (ptr != nullptr)
        {
            for (auto i : ptr->Traverse<SetCell::up>())
                for (auto j : i->Traverse<SetCell::left>())
                {
                    j->ColumnRestore();
                    ++mColumns[j->col]->counter;
                }

            ptr->RowRestore();
        }
    }

    SetCell* SparseMatrixImp::MostConstrainedColumn()
    {
        auto col = mRoot;
        for (auto test : mRoot->Traverse<SetCell::right>())
        {
            if (test->counter == 0)
            {
                // No solution in this direction (there is column
                // that cannot be covered)
                return nullptr;
            }
            if (test->counter < col->counter)
                col = test;
        }
        return col;
    }

    void SparseMatrixImp::Cover(std::function<void(int)> tryRow, SetCell* cell)
    {
        tryRow(cell->row);

        for (auto test : cell->Traverse<SetCell::right>())
            HideColumn(mColumns[test->col]);
    }

    void SparseMatrixImp::Uncover(std::function<void(int)> undoRow, SetCell* cell)
    {
        for (auto test : cell->Traverse<SetCell::left>())
            UnhideColumn(mColumns[test->col]);

        undoRow(cell->row);
    }

// The recursive implementation is very simple and straightforward.
    void SparseMatrixImp::SolveImp(function<void(int)> tryRow, function<void(int)> undoRow, function<void()> complete)
    {
        // Find the most constrained column if any
        auto col = MostConstrainedColumn();
        if (col == mRoot)
        {
            complete();
            return;
        }
        else if (col == nullptr)
        {
            return;
        }

        HideColumn(col);

        // For each row on that column try covering that row and solve the remaining matrix using recursive call
        for(auto cell : col->Traverse<SetCell::down>())
        {
            Cover(tryRow, cell);

            SolveImp(tryRow, undoRow, complete);

            Uncover(undoRow, cell);
        }

        UnhideColumn(col);
    }

    void SparseMatrixImp::Solve(std::function<void(int)> tryRow, std::function<void(int)> undoRow, std::function<void()> complete)
    {
        ValidateState(solving);
        for (int p : mSolutionPrefix)
            tryRow(p);
        SolveImp(tryRow, undoRow, complete);
        state = done;
    }

    // The generator uses an iterative implementation of the algorith. The issue with the recursive implmentation is that it would
    // yield a solution from some deeper recursion level meaning that the recursive function itself should be a generator. Calling a
    // generator is not free and that implementation would incur a significant performance cost.
    experimental::generator<const vector<int>> SparseMatrixImp::Solve()
    {
        ValidateState(solving);

        vector<int> solution;

        auto tryRow = [&solution](int r) { solution.push_back(r); };
        auto undoRow = [&solution](int r) { solution.pop_back(); };

        for (int p : mSolutionPrefix)
            tryRow(p);

        auto col = MostConstrainedColumn();

        if (col == mRoot)
        {
            co_yield solution;
        }

        if (col != mRoot && col != nullptr)
        {
            // This is the backtracking step. The first element of the pair is the current column header,
            // the second is the row being tested.
            vector<pair<SetCell*, SetCell*>> stack;

            stack.push_back(make_pair(col, col));
            HideColumn(stack.back().first);

            while (true)
            {
                // Undo the last step unless we just started with this column
                if (stack.back().second->row != numeric_limits<int>::max())
                    Uncover(undoRow, stack.back().second);

                // Move to the next row
                auto cell = stack.back().second->Move<SetCell::down>();

                // Check if we are done with this column
                if (cell != stack.back().first)
                {
                    // Adjust the top of the stack
                    auto t = make_pair(stack.back().first, cell);
                    swap(stack.back(), t);

                    Cover(tryRow, stack.back().second);

                    // And see if there are any more columns left
                    col = MostConstrainedColumn();
                    if (col == mRoot)
                    {
                        co_yield solution;
                    }
                    else if (col != nullptr)
                    {
                        stack.push_back(make_pair(col, col));
                        HideColumn(stack.back().first);
                    }
                }
                else
                {
                    // Done with the column, pop the stack and continue unless the stack is empty
                    UnhideColumn(stack.back().first);
                    stack.pop_back();
                    if (stack.empty())
                        break;
                }
            }
        }
        state = done;
    }
}
