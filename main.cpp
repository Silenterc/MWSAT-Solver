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
             << " instance.mwcnf tempStart alpha tempMin itersPerTemp [output.dat]\n";
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

    solver.solve(params);

    //solver.printCompleteSolution();

    // Always print .dat line to stdout
    //solver.printBestSolution();

    // If 6th argument is given, append .dat line to that file
    if (argc >= 7) {
        const string outPath = argv[6];
        ofstream out(outPath, ios::app);
        if (!out) {
            cerr << "Failed to open output file for writing: " << outPath << "\n";
            return 1;
        }
        solver.printCompleteFormattedSolution(out);
    }

    return 0;
}
