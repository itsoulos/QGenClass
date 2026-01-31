#ifndef INTEGERHILL_H
#define INTEGERHILL_H
# include <CORE/problem.h>
# include <GE/program.h>
class IntegerHill
{
private:
    const int LOW=0;
    const int HIGH=255;
    Program *myProblem;
public:
    IntegerHill(Program *pr);
    vector<int> hill_climbing(vector<int> &current);
    vector<vector<int>> generate_neighbors(const vector<int>& x);
    vector<int> random_solution(int n);
    double evaluate( vector<int> &x);
    ~IntegerHill();
};

#endif // INTEGERHILL_H
