// Port of the Python implementation of Dancing Links X in 30 lines to C++

#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <experimental/generator>

using namespace std;

// The implementation uses for-range quite a bit for brevity but in one occasion it needs to iterate the container in reverse.
// Using trick from https://stackoverflow.com/questions/8542591/c11-reverse-range-based-for-loop to perform reverse for-range loops.
// Note that this requires C++14.
template <typename T> struct reversion_wrapper { T& iterable; };
template <typename T> auto begin(reversion_wrapper<T> w) { return rbegin(w.iterable); }
template <typename T> auto end(reversion_wrapper<T> w) { return rend(w.iterable); }
template <typename T> reversion_wrapper<T> reverse(T&& iterable) { return { iterable }; }
//

// Following the original implementation line by line
typedef vector<int> tXin;
typedef map<char, vector<int>> tY;
typedef shared_ptr<set<char>> tRow;
typedef map<int, tRow> tX;

typedef vector<tRow> tCols;
typedef vector<char> tSolution;

tCols select(tX& X, tY& Y, char r)
{
    tCols cols;
    for (auto j : Y[r])
    {
        for (auto i : *X[j])
        {
            for (auto k : Y[i])
            {
                if (k != j)
                    X[k]->erase(i);
            }
        }
        cols.push_back(X[j]);
        X.erase(j);
    }
    return cols;
}

void deselect(tX& X, tY& Y, char r, tCols cols)
{
    for (auto j : reverse(Y[r]))
    {
        X[j] = cols.back();
        cols.pop_back();
        for (auto i : *X[j])
        {
            for (auto k : Y[i])
            {
                if (k != j)
                    X[k]->insert(i);
            }
        }
    }
}

experimental::generator<tSolution> solve(tX& X, tY& Y, tSolution& solution)
{
    if (X.empty())
    {
        co_yield solution;
    }
    else
    {
        auto c = min_element(X.begin(), X.end(), [](const auto& a, const auto& b) { return a.second->size() < b.second->size(); });
        set<char> Xc = *c->second; // Note that it has to be a copy as X may (and will) be modified in the recursive call
        for (auto r : Xc)
        {
            solution.push_back(r);
            auto cols = select(X, Y, r);
            for (auto s : solve(X, Y, solution))
            {
                co_yield s;
            }
            deselect(X, Y, r, cols);
            solution.pop_back();
        }
    }
}

int main()
{
    tXin Xin = { 1, 2, 3, 4, 5, 6, 7 };
    tY Y = { 
        { 'A', {1, 4, 7} },
        { 'B', {1, 4} },
        { 'C', {4, 5, 7} },
        { 'D', {3, 5, 6} },
        { 'E', {2, 3, 6, 7} },
        { 'F', {2, 7} }
    };

    tX X;
    for (auto j : Xin)
    {
        X[j] = make_shared<set<char>>();
        for (auto i : Y)
            if (find(i.second.begin(), i.second.end(), j) != i.second.end())
                X[j]->insert(i.first);
    }

    tSolution solution;
    for (auto s : solve(X, Y, solution))
    {
        for (auto c : s)
        {
            printf("'%c' ", c);
        }
        printf("\n");
    }
}