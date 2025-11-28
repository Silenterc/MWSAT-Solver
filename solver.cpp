//
// Created by Lukáš Zima on 28.11.2025.
//
#include "Solver.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

bool Solver::load(const string& filename) {
    ifstream in(filename);
    if (!in) {
        cerr << "Failed to open file: " << filename << "\n";
        return false;
    }
    instanceName = std::filesystem::path(filename).stem().string();

    numVars = 0;
    numClauses = 0;
    weights.clear();
    clauses.clear();

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