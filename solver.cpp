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
#include <random>
#include <algorithm>

Solver::Solver() : rng(random_device{}()) {}

void Solver::setSeed(mt19937::result_type seed) {
    rng.seed(seed);
}

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

long long Solver::computeMaxStagnation() const {
    // Compute the cooling steps
    // It is: K = (ln(T_min) - ln(T_start)) / ln(alpha)
    double numerator = log(params.tempMin) - log(params.tempStart);
    double denominator = log(params.alpha);

    long long coolingSteps = static_cast<long long>(ceil(numerator / denominator));

    long long totalIterations = coolingSteps * params.itersPerTemp;

    // Set to 50%, TODO experiment
    long long calculatedStagnation = totalIterations / 2;

    // If there are not enough steps
    long long minimumSteps = static_cast<long long>(numVars) * 100;

    return max(calculatedStagnation, minimumSteps);
}

// Base penalty for clause - max weight
double Solver::computeBasePenalty() const {
    int maxW = 0;
    int sumW = 0;
    for (const int w : weights) {
        maxW  = max(maxW, w);
        sumW += w;
    }
    return sumW;
}

void Solver::setInitialParams() {
    // Initial Temperature
    int sumW = 0;
    for (const int w : weights) {
        sumW += w;
    }
    const int avgW = sumW / numVars;
    const double initialTemp = -1 * (computeBasePenalty() / avgW + 1.0) / log(0.9);
    params.tempStart = initialTemp;

    // Final/Minimal Temperature
    params.tempMin = -1 * (computeBasePenalty() / avgW + 1.0) / log(0.0001);

    // Equilibrium size
    params.itersPerTemp = numVars * 3;

    // Alpha
    params.alpha = 0.99;
}


// E = NORMALIZE[unsat * penalty + (idealSum - sum(weights of vars set to 1))] -> minimize to 0
double Solver::energy(const vector<bool>& assign,
                         double penalty,
                         int& unsatOut,
                         int& weightOut) const
{
    int wSum = 0;
    int idealSum = 0;
    for (int i = 0; i < numVars; ++i) {
        if (assign[i]) {
            wSum += weights[i];
        }
        idealSum += weights[i];
    }

    const int unsat = countUnsatisfiedClauses(assign);

    unsatOut = unsat;
    weightOut = wSum;
    auto preEnergy = unsat * penalty + (idealSum - wSum);
    auto avgWeight = idealSum / numVars;

    // Normalize the weights
    return preEnergy / avgWeight;
}

bool Solver::load(const string& filename) {
    clearAll();
    ifstream in(filename);
    if (!in) {
        cerr << "Failed to open file: " << filename << "\n";
        return false;
    }
    instanceName = filesystem::path(filename).stem().string();

    string line;

    // 1) Read header line: p mwcnf <numVars> <numClauses>
    while (getline(in, line)) {
        if (line.empty() || line[0] == 'c')
            continue;  // skip comments and empty lines

        if (line[0] == 'p') {
            istringstream iss(line);
            char p;
            string format;
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
    while (getline(in, line)) {
        if (line.empty() || line[0] == 'c')
            continue;

        if (line[0] == 'w') {
            istringstream iss(line);
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
    while (getline(in, line)) {
        if (line.empty() || line[0] == 'c')
            continue;  // skip comments

        istringstream iss(line);
        int lit;
        vector<int> clause;

        while (iss >> lit && lit != 0) {
            clause.emplace_back(lit);  // literals are in DIMACS style: ±1..±numVars
        }

        if (!clause.empty()) {
            clauses.emplace_back(move(clause));
        }
    }

    return true;
}

int Solver::countUnsatisfiedClauses(const vector<bool>& assign) const {
    int unsat = 0;
    for (const auto& clause : clauses) {
        bool satisfied = false;
        for (int lit : clause) {
            int varIdx = abs(lit) - 1;
            bool val = assign[varIdx];
            if ((lit > 0 && val) || (lit < 0 && !val)) {
                satisfied = true;
                break;
            }
        }
        if (!satisfied) {
            ++unsat;
        }
    }
    return unsat;
}

vector<bool> Solver::getInitialAssignment() {
    vector<bool> current(numVars);
    uniform_int_distribution<int> bitDist(0, 1);
    // Initial random assignment, indexed from 0
    for (int i = 0; i < numVars; ++i) {
        current[i] = static_cast<bool>(bitDist(rng));
    }
    return current;
}

vector<bool> Solver::getNeighbourAssignment(vector<bool> assignment) {
    // Identify unsatisfied clauses for the current assignment
    vector<int> unsatisfiedIdx;
    for (int ci = 0; ci < clauses.size(); ci++) {
        const auto& clause = clauses[ci];
        bool satisfied = false;
        for (int lit : clause) {
            int varIdx = abs(lit) - 1;
            bool val = assignment[varIdx];
            if ((lit > 0 && val) || (lit < 0 && !val)) {
                satisfied = true;
                break;
            }
        }
        if (!satisfied) {
            unsatisfiedIdx.push_back(ci);
        }
    }

    if (unsatisfiedIdx.empty()) {
        uniform_int_distribution indexDist(0, numVars - 1);
        int index = indexDist(rng);
        assignment[index] = !assignment[index];
        return assignment;
    }

    // 1) pick a random unsatisfied clause
    uniform_int_distribution<size_t> unsatDist(0, unsatisfiedIdx.size() - 1);
    const auto& clause = clauses[unsatisfiedIdx[unsatDist(rng)]];

    bernoulli_distribution doNoise(0.5);

    int bestVar = -1;

    if (doNoise(rng)) {
        // Random
        // I get a random literal
        uniform_int_distribution<size_t> litPosDist(0, clause.size() - 1);
        bestVar = abs(clause[litPosDist(rng)]) - 1;
    }
    else {
        // Greedy
        // I find the literal that helps the most
        int bestNetGain = -9999999;

        for (int lit : clause) {
            int varIdx = abs(lit) - 1;

            assignment[varIdx] = !assignment[varIdx];
            int currentUnsat = countUnsatisfiedClauses(assignment);
            int currentGain = -currentUnsat;

            assignment[varIdx] = !assignment[varIdx];

            if (currentGain > bestNetGain) {
                bestNetGain = currentGain;
                bestVar = varIdx;
            }
        }
    }

    assignment[bestVar] = !assignment[bestVar];
    return assignment;
}



void Solver::solve(const SAParams& inParams, ostream* trace) {
    if (numVars == 0 || weights.empty() || clauses.empty()) {
        cerr << "Solver::solve: instance not loaded.\n";
        return;
    }
    params = inParams;
    // TESTED AND DOESNT HELP, SO ITS 1
    penaltyCoefficient = 1.0;
    long long maxStagnation = computeMaxStagnation();
    long long stagnationCounter = 0;
    setInitialParams();
    vector<bool> current = getInitialAssignment();

    // Initial energy
    double penalty = computeBasePenalty();
    int unsatCur;
    int weightCur;
    double Ecur = energy(current, penalty, unsatCur, weightCur);

    if (trace) {
        (*trace) << "step,energy,satisfied,unsatisfied,weight\n";
        int satisfied = numClauses - unsatCur;
        (*trace) << steps << "," << Ecur << "," << satisfied << "," << unsatCur << "," << weightCur << "\n";
    }

    // Best so far
    bestAssignment = current;
    bestEnergy = Ecur;
    bestWeight = weightCur;
    int bestUnsat = numClauses;

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
                // We accept it with a probability,
                // which decreases as time goes on - diversification -> intensification
                uniform_real_distribution<> dist(0, 1);
                const double u = dist(rng);
                const double prob = exp(-deltaE / T);
                if (u < prob) {
                    accept = true;
                }
            }

            if (accept) {
                // We accept the new state
                current = move(neighbour);
                Ecur = Enew;
                unsatCur = unsatNew;
                weightCur = weightNew;
                stagnationCounter = 0;

                // We possibly update the best state
                if (unsatCur == 0) {
                    if (bestUnsat > 0 || weightCur > bestWeight) {
                        bestUnsat = 0;
                        bestWeight = weightCur;
                        bestAssignment = current;
                    }
                } else if (bestUnsat > 0 && unsatCur < bestUnsat) {
                    bestUnsat = unsatCur;
                    bestWeight = weightCur;
                    bestAssignment = current;
                }
            } else {
                stagnationCounter++;
            }

            // if (stagnationCounter >= maxStagnation) {
            //     // RESET THE WHOLE RUN
            //     stagnationCounter = 0;
            //     current = getInitialAssignment();
            //     Ecur = energy(current, penalty, unsatCur, weightCur);
            // }
            if (trace) {
                int satisfied = numClauses - unsatCur;
                (*trace) << steps << "," << Ecur << "," << satisfied << "," << unsatCur << "," << weightCur << "\n";
            }
            iter++;
        }
       // penalty *= penaltyCoefficient;
    }
}

// Corresponds to the .dat format
void Solver::printBestSolution(ostream& os) const {
    // todo: probably print the debug info, not just the .dat results
    if (bestAssignment.empty()) {
        cerr << "Solver::printBestSolution: no solution available.\n";
        return;
    }
    cout << instanceName << ' ';
    cout << bestWeight << ' ';
    // Assignment as ±var index, variables are 1..numVars
    for (int i = 0; i < numVars; ++i) {
        int lit = bestAssignment[i] ? (i + 1) : -(i + 1);
        cout << lit << ' ';
    }
    cout << 0 << '\n';
}

void Solver::printBestSolution() const {
    printBestSolution(cout);
}

void Solver::printCompleteSolution() const {
    if (bestAssignment.empty()) {
        cerr << "Solver::printCompleteSolution: no solution available.\n";
        return;
    }

    // Recompute unsatisfied / weight for the best assignment
    const double penalty = computeBasePenalty();
    int unsat = 0;
    int weight = 0;
    // We ignore the returned energy - we only care about unsat and weightOut
    (void) energy(bestAssignment, penalty, unsat, weight);


    // Should ideally == totalClauses and unsat == 0
    int satisfied = numClauses - unsat;

    cout << "Instance: " << instanceName << '\n';
    cout << "Best weight: " << bestWeight << '\n';
    cout << "Satisfied clauses: " << satisfied << '\n';
    cout << "Unsatisfied clauses: " << unsat << '\n';
    cout << "Total steps: " << steps << '\n';
}

void Solver::printCompleteFormattedSolution(ostream& os) const {
    if (bestAssignment.empty()) {
        cerr << "Solver::printCompleteFormattedSolution: no solution available.\n";
        return;
    }

    const double penalty = computeBasePenalty();
    int unsat = 0;
    int weight = 0;

    (void) energy(bestAssignment, penalty, unsat, weight);

    int satisfied = numClauses - unsat;

    // CSV: instanceName,bestWeight,satisfied,unsatisfied,steps
    os << instanceName << ','
       << bestWeight   << ','
       << satisfied    << ','
       << unsat        << ','
       << steps        << '\n';
}
