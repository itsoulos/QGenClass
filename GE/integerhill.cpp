#include "integerhill.h"

IntegerHill::IntegerHill(Program *pr)
{
    myProblem = pr;
}

vector<int> IntegerHill::hill_climbing(vector<int> &current)
{

    const int MAX_ITER = 1000;
    int n = current.size();
    double current_value = evaluate(current);

    for (int iter = 0; iter < MAX_ITER; iter++) {
        vector<vector<int>> neighbors = generate_neighbors(current);

        vector<int> best_neighbor = current;
        double best_value = current_value;
        for ( auto& neighbor : neighbors) {
            double value = evaluate(neighbor);
            printf("nn = %lf \n",value);
            if (value < best_value) {
                best_value = value;
                best_neighbor = neighbor;
            }
        }
        printf("HILL ITER=%d VALUE=%lf \n",iter,best_value);
      //  if (best_value <= current_value)
      //      break; // Τοπικό μέγιστο

        current = best_neighbor;
        current_value = best_value;
    }

    return current;
}

vector<vector<int>> IntegerHill::generate_neighbors(const vector<int>& x)
{


    vector<vector<int>> neighbors;

    for (int i = 0; i < x.size(); i++) {
        if (x[i] < HIGH) {
            vector<int> up = x;
            up[i]++;
            neighbors.push_back(up);
        }
        if (x[i] > LOW) {
            vector<int> down = x;
            down[i]--;
            neighbors.push_back(down);
        }
    }
    return neighbors;
}

vector<int> IntegerHill::random_solution(int n)
{
    vector<int> x(n);
    for (int i = 0; i < n; i++)
        x[i] = LOW + rand() % (HIGH - LOW + 1);
    return x;
}

double IntegerHill::evaluate( vector<int>& x)
{
    return myProblem->fitness(x);
}

IntegerHill::~IntegerHill()
{

}
