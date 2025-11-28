//
// Created by Lukáš Zima on 28.11.2025.
//

#ifndef KOP2_SOLVER_H
#define KOP2_SOLVER_H
#include <string>
#include <vector>
using namespace std;

class Solver {
    int numVars = 0;
    int numClauses = 0;
    // weights[i] corresponds to variable (i+1)
    vector<int> weights;
    vector<vector<int>> clauses;
    string instanceName;

    public:
        Solver();
        bool load(const string& filename);

};







#endif //KOP2_SOLVER_H