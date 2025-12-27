//
// Created by Lukáš Zima on 28.11.2025.
//

#ifndef KOP2_SOLVER_H
#define KOP2_SOLVER_H
#include <string>
#include <vector>
#include <random>
using namespace std;

// Simulated annealing parameters
struct SAParams {
    // Starting temperature, 1 <= tempStart < ...
    double tempStart;
    // Cooling factor <= 1, is usually > 0.9
    double alpha;
    // Minimal temperature, 0 < tempMin < tempStart
    double tempMin;
    // How many configurations the algorithm tries before changing temperature
    int itersPerTemp;
};

class Solver {
    int numVars = 0;
    int numClauses = 0;
    // weights[i] corresponds to variable (i+1)
    vector<int> weights;
    vector<vector<int>> clauses;
    string instanceName;
    SAParams params;
    std::mt19937 rng;

    vector<bool> bestAssignment;
    double bestEnergy = 0;
    int bestWeight = 0;
    int steps = 0;

    void clearAll();
    double cool(double T) const;
    bool frozen(double T) const;
    bool equilibrium(int iterAtTemp) const;

    double computeBasePenalty() const;

    double energy(const vector<bool>& assign,
                     double penalty,
                     int& unsatOut,
                     int& weightOut) const;
    int countUnsatisfiedClauses(const vector<bool>& assign) const;
    vector<bool> getInitialAssignment();
    vector<bool> getNeighbourAssignment(vector<bool> assignment);
    void setInitialParams();

    public:
        Solver();
        void setSeed(std::mt19937::result_type seed);
        bool load(const string& filename);
        void solve(const SAParams& params, ostream* trace = nullptr);
        void printBestSolution() const;
        void printBestSolution(std::ostream& os) const;
        void printCompleteSolution() const;
        void printCompleteFormattedSolution(std::ostream& os) const;
};

#endif //KOP2_SOLVER_H
