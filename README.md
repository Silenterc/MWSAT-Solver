# MWSAT Solver

A C++ implementation of a solver for **Maximum Weighted SAT (MWSAT)**.

## Problem
In Maximum Weighted SAT, each clause has an associated weight. The goal is to find a Boolean assignment that **satisfies all clauses** (if possible) and, among satisfying assignments, **maximizes the total satisfied weight** (equivalently: minimize the loss w.r.t. the optimal weight).

## Solver Description
The solver is based on **Simulated Annealing** and has following features:

- **Energy function:** prioritizes satisfiability first and then maximizes satisfied weight. It is normalized by the average clause weight.
- **Neighbor generation (WalkSAT-style):** pick a random unsatisfied clause - with probability \(p=0.5\) flip a random literal (diversification), otherwise flip the literal that minimizes the number of unsatisfied clauses (intensification). If the assignment is already satisfying, perform a random flip.
- **Experimentally tuned / calibrated parameters:** the values below were chosen using a mix of mathematical reasoning and empirical testing:
  - **Initial temperature** (`T_start`)
  - **Cooling rate / factor** (`alpha`)
  - **Equilibrium length** - number of steps per temperature level (`N`)
  - **Stopping temperature / termination threshold** (`T_min`, derived from a target final acceptance probability

## Results
The solver was executed on all versions (M, N, Q, R) of the provided datasets with reference solutions (`.dat` files):  
`wuf20-91`, `wuf36-157`, `wuf50-218`, `wuf75-325`.

Each instance was run **100 times**. Reported values were aggregated in two steps:
1. For each instance: average metrics across its runs.
2. For each dataset version: arithmetic mean across all instances in that version.

**Metrics:**
- **Success rate (%):**
  - **Satisfiable:** share of runs that found a valid assignment satisfying all clauses.
  - **Optimal:** share of runs that matched the optimal solution from the provided `.dat` file.
- **Weight loss (%):** distance from optimum (computed only for satisfiable runs).
  - **Average:** mean percentage deviation from the optimal weight.
  - **Maximum:** worst recorded deviation (lowest-quality satisfiable solution).
- **Steps:** number of heuristic iterations before termination (depends only on instance size in this solver).

| Instance set | Success rate - Satisfiable (%) | Success rate - Optimal (%) | Weight loss - Average (%) | Weight loss - Maximum (%) | Steps |
|---|---:|---:|---:|---:|---:|
| wuf20-91-M | 99.99 | 99.99 | 0.00 | 40.00 | 26,700 |
| wuf20-91-N | 100.00 | 99.99 | 0.00 | 28.21 | 26,700 |
| wuf20-91-Q | 99.99 | 99.86 | 0.05 | 91.96 | 26,700 |
| wuf20-91-R | 99.99 | 99.89 | 0.05 | 57.98 | 26,700 |
| wuf36-157-M | 99.64 | 99.15 | 0.03 | 64.86 | 48,060 |
| wuf36-157-N | 99.63 | 99.15 | 0.03 | 64.86 | 48,060 |
| wuf36-157-Q | 99.54 | 96.51 | 0.77 | 98.96 | 48,060 |
| wuf36-157-R | 99.57 | 96.58 | 0.73 | 99.81 | 48,060 |
| wuf50-218-M | 98.41 | 93.15 | 0.25 | 61.03 | 66,750 |
| wuf50-218-N | 98.43 | 93.12 | 0.26 | 61.09 | 66,750 |
| wuf50-218-Q | 98.25 | 85.52 | 2.40 | 99.29 | 66,750 |
| wuf50-218-R | 98.27 | 85.52 | 2.39 | 99.92 | 66,750 |
| wuf75-325-M | 94.11 | 60.56 | 1.34 | 41.78 | 100,125 |
| wuf75-325-N | 94.04 | 60.98 | 1.28 | 41.81 | 100,125 |
| wuf75-325-Q | 93.76 | 56.00 | 6.99 | 81.54 | 100,125 |
| wuf75-325-R | 93.54 | 55.46 | 7.23 | 77.53 | 100,125 |
