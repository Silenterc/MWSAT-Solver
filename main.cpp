//
// Created by Lukáš Zima on 28.11.2025.
//

#include "solver.h"

#include <iostream>
#include <fstream>
#include <random>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " instance.mwcnf tempStart alpha tempMin itersPerTemp [output.dat] [trace.csv]\n";
        return 1;
    }

    const string instancePath = argv[1];

    const double tempStart = atof(argv[2]);
    const double alpha = atof(argv[3]);
    const double tempMin = atof(argv[4]);
    const int itersPerTemp = atoi(argv[5]);

    SAParams params;
    params.tempStart = tempStart;
    params.alpha = alpha;
    params.tempMin = tempMin;
    params.itersPerTemp = itersPerTemp;

    std::random_device rd;

    Solver solver;
    solver.setSeed(rd());
    if (!solver.load(instancePath)) {
        return 1;
    }

    ofstream outFile;
    ofstream traceFile;
    ostream* tracePtr = nullptr;

    // Optional .dat output (append)
    if (argc >= 7) {
        const string outPath = argv[6];
        outFile.open(outPath, ios::app);
        if (!outFile) {
            cerr << "Failed to open output file for writing: " << outPath << "\n";
            return 1;
        }
    }

    // Optional trace CSV (overwrite)
    if (argc >= 8) {
        const string tracePath = argv[7];
        traceFile.open(tracePath, ios::trunc);
        if (!traceFile) {
            cerr << "Failed to open trace file for writing: " << tracePath << "\n";
            return 1;
        }
        tracePtr = &traceFile;
    }

    solver.solve(params, tracePtr);

    if (outFile.is_open()) {
        solver.printCompleteFormattedSolution(outFile);
    }

    return 0;
}
