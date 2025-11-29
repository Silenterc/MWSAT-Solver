//
// Created by Lukáš Zima on 28.11.2025.
//
#include "solver.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <cstdlib>

void Solver::clearAll() {
    numVars = 0;
    numClauses = 0;
    weights.clear();
    clauses.clear();
    steps = 0;
}

double Solver::cool(const double T) const {
    return T * params.alpha;
}

bool Solver::frozen(const double T) const {
    return T <= params.tempMin;
}

bool Solver::equilibrium(int iterAtTemp) const {
    return iterAtTemp >= params.itersPerTemp;
}

// Penalty for each unsatisfied clause (Its very big)
double Solver::computePenalty() const {
    int sumW = 0;
    for (const int w : weights) {
        sumW += w;
    }
    return sumW + 1; // any unsat assignment worse than any sat
}

// E = unsat * penalty - sum(weights of vars set to 1)
double Solver::energy(const vector<bool>& assign,
                         double penalty,
                         int& unsatOut,
                         int& weightOut) const
{
    int wSum = 0;
    for (int i = 0; i < numVars; ++i) {
        if (assign[i]) {
            wSum += weights[i];
        }
    }

    int unsat = 0;
    for (const auto& clause : clauses) {
        bool sat = false;
        for (int lit : clause) {
            int varIdx = abs(lit) - 1; // 0-based index
            bool val = assign[varIdx];
            if ((lit > 0 && val) || (lit < 0 && !val)) {
                // If any literal is satisfied, its enough (OR)
                sat = true;
                break;
            }
        }
        if (!sat) {
            ++unsat;
        }
    }

    unsatOut = unsat;
    weightOut = wSum;

    return unsat * penalty - wSum;
}

bool Solver::load(const string& filename) {
    clearAll();
    ifstream in(filename);
    if (!in) {
        cerr << "Failed to open file: " << filename << "\n";
        return false;
    }
    instanceName = std::filesystem::path(filename).stem().string();

    string line;

    // 1) Read header line: p mwcnf <numVars> <numClauses>
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == 'c')
            continue;  // skip comments and empty lines

        if (line[0] == 'p') {
            std::istringstream iss(line);
            char p;
            std::string format;
            iss >> p >> format >> numVars >> numClauses;
            break;
        }
    }

    if (numVars <= 0) {
        cerr << "No valid 'p mwcnf n m' line in file: " << filename << "\n";
        return false;
    }

    // prepare weights (0-based: weights[i] is var (i+1))
    weights.assign(numVars, 0);

    // 2) Read weights line: w w1 w2 ... wn 0
    bool haveWeights = false;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == 'c')
            continue;

        if (line[0] == 'w') {
            std::istringstream iss(line);
            char wchar;
            iss >> wchar; // consume 'w'

            int w;
            int idx = 0;
            while (iss >> w && w != 0 && idx < numVars) {
                weights[idx++] = w;
            }

            haveWeights = true;
            break;
        }
    }

    if (!haveWeights) {
        cerr << "No 'w' line with weights in file: " << filename << "\n";
        return false;
    }

    // 3) Remaining (non-comment) lines are clauses, each terminated by 0
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == 'c')
            continue;  // skip comments

        std::istringstream iss(line);
        int lit;
        std::vector<int> clause;

        while (iss >> lit && lit != 0) {
            clause.emplace_back(lit);  // literals are in DIMACS style: ±1..±numVars
        }

        if (!clause.empty()) {
            clauses.emplace_back(std::move(clause));
        }
    }

    return true;
}

vector<bool> Solver::getInitialAssignment() const {
    vector<bool> current(numVars);
    // Initial random assignment, indexed from 0
    for (int i = 0; i < numVars; ++i) {
        current[i] = rand() % 2; // 0 or 1
    }
    return current;
}

vector<bool> Solver::getNeighbourAssignment(vector<bool> assignment) const {
    int index = rand() % numVars;
    // Flip random bit
    assignment[index] = !assignment[index];
    return assignment;
}



void Solver::solve(const SAParams& inParams) {
    if (numVars == 0 || weights.empty() || clauses.empty()) {
        std::cerr << "Solver::solve: instance not loaded.\n";
        return;
    }
    params = inParams;

    vector<bool> current = getInitialAssignment();

    // Initial energy
    const double penalty = computePenalty();
    int unsatCur;
    int weightCur;
    double Ecur = energy(current, penalty, unsatCur, weightCur);

    // Best so far
    bestAssignment = current;
    bestEnergy = Ecur;
    bestWeight = weightCur;

    // Simulated annealing loop
    for (double T = params.tempStart; !frozen(T); T = cool(T)) {
        int iter = 0;

        while (!equilibrium(iter)) {
            steps++;

            auto neighbour = getNeighbourAssignment(current);

            int unsatNew;
            int weightNew;
            double Enew = energy(neighbour, penalty, unsatNew, weightNew);
            double deltaE = Enew - Ecur;

            bool accept = false;
            if (deltaE <= 0.0) {
                // It is simply better
                accept = true;
            } else {
                // We accept it with probability
                const double u = static_cast<double>(rand()) / RAND_MAX; // random(0,1)
                const double prob = exp(-deltaE / T);
                if (u < prob) {
                    accept = true;
                }
            }

            if (accept) {
                // We accept the new state
                current = std::move(neighbour);
                Ecur = Enew;
                unsatCur = unsatNew;
                weightCur = weightNew;

                // We possibly update the best state
                if (Ecur < bestEnergy) {
                    bestEnergy = Ecur;
                    bestAssignment = current;
                    bestWeight = weightCur;
                }
            }
            iter++;
        }
    }
}

// Corresponds to the .dat format
void Solver::printBestSolution(std::ostream& os) const {
    // todo: probably print the debug info, not just the .dat results
    if (bestAssignment.empty()) {
        cerr << "Solver::printBestSolution: no solution available.\n";
        return;
    }

    // Instance name
    cout << instanceName << ' ';

    // Best weight (objective value)
    cout << bestWeight << ' ';

    // Assignment as ±var index, variables are 1..numVars
    for (int i = 0; i < numVars; ++i) {
        int lit = bestAssignment[i] ? (i + 1) : -(i + 1);
        cout << lit << ' ';
    }

    // Terminating 0 and newline
    cout << 0 << '\n';
}

void Solver::printBestSolution() const {
    printBestSolution(std::cout);
}

void Solver::printCompleteSolution() const {
    if (bestAssignment.empty()) {
        std::cerr << "Solver::printCompleteSolution: no solution available.\n";
        return;
    }

    // Recompute unsatisfied / weight for the best assignment
    const double penalty = computePenalty();
    int unsat = 0;
    int weight = 0;
    // We ignore the returned energy - we only care about unsat and weightOut
    (void) energy(bestAssignment, penalty, unsat, weight);


    // Should ideally == totalClauses and unsat == 0
    int satisfied = numClauses - unsat;

    std::cout << "Instance: " << instanceName << '\n';
    std::cout << "Best weight: " << bestWeight << '\n';
    std::cout << "Satisfied clauses: " << satisfied << '\n';
    std::cout << "Unsatisfied clauses: " << unsat << '\n';
    std::cout << "Total steps: " << steps << '\n';
}

