# DancingLinks
Dancing Links X Implementation in C++

A few years ago Prof. Donald Knuth described a new algorithm to solve exact covering problems he called Algorithm X or Dancing Links. Typically, these problems are solved with some variety of backtracking. Those solutions are more or less straightforward but in some cases can still take a lot of computations. Most of that is due to a large number of checks needed when trying a new element - backtracking is an Depth First type algorithm, and the tree it needs to search may be quite big. Algorith X first appeared in a separate article but now it is available in the Volume 4 Fascicle 5 of The Art of Computing Programming or in the Volume 4B of the final book (that volume just got published and I am still waiting for my copy). At the first glance, the algorithm is very confusing and looks a bit magical. It takes some time to understand, it may take even more time to implement it. The thing is, it is well worth both understanding and implementing. What it does is essentially eliminating a very large share of those checks in the backtracking effectively pruning the search tree, in many cases by orders of magnitude. In its heart, it is still backtracking, but the computation cost on most problems is much lower.

Here are two of my experiments with the algorithm that I've performed a couple years ago. One is a fully fledged implementation that allows to find one or all solutions to a problem. That one comes with a set of tests - rather typical Eight Queens, Pentominoes, and Sudoku puzzles. Oddly enough, when I searched for implementations online, I did not find any in C++. The most complete one I've found is written in C and can be found here on GitHub: https://github.com/blynn/dlx. My implementation is not based on that one. The implementation is completely generic, and the API is very simple. Here are the use steps (you may need to read the algorithm description to understand the used terms):
0) Include the header DancingLinks.h
1) Instantiate the algorith:
	SparseMatrix* dlx = SparseMatrix::Create();
2) Define the problem in terms of "conditions":
	dlx->SetCondition(row, column);
3) If needed, set some conditions as optional:
	dlx->SetConditionOptional(column);
4) If needed, preselect some elements:
	dlx->PreselectRow(row);
5a) Old C-style solver with callbacks, these will be called every time an element is placed, removed, or when the condition has been reached:
	dlx->Solve(tryRow, undoRow, complete);
5b) New C++-style solver yielding solutions, this one is much neater:
	for(auto solution: dlx->Solve()) { ... }
6) Cleanup:
	SparseMatrix::Destroy(dlx);
Check the included tests for examples. I've used this implemenation in the same exact form for other problems since, planning to upload some of those projects to GitHub in the future.

The second experiment was prompted by this page: https://www.cs.mcgill.ca/~aassaf9/python/algorithm_x.html, which claimed the Algorithm X implementation in 30 lines of Python. Took me some time to believe that it is indeed the same algorithm. Indeed it is. In fact, this concise implementation is a great tool to understand what the algorithm is doing. So, I've tried to repeat the same in C++ to prove that C++ can be as concise as Python. Got very close - not counting the curly braces, this implementation is 33 lines long, and those three lines are only lost due to a peculiartiy of STL (pop_back() and push_back() require an extra call to access the data while Python allows to access the data and modify the container in one call). I did not play much with this implementation yet, the first one is more useful for practical purposes. Probably, faster as well. So, this implementation is here for illustration purposes for now. I should get to it one day and polish it to the point to be easier to use, for now it is "as is."

The code can be built with Visual C++. It was developed with VS2019 and tested with VS2022. There is no real point including projects or solutions, instead you can issue the following commands in VS Developer Prompt:
	cl Test.cpp DancingLinks.cpp /std:c++latest /EHsc /O2 
	cl TinyDLX.cpp /std:c++latest /EHsc /O2 
