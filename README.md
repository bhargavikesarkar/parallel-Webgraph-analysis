# Parallel Web Graph Analysis

A performance analysis of Breadth-First Search (BFS) on a large-scale web graph using sequential and parallel implementations.

## Project Overview

This project analyzes the performance of graph traversal on a large-scale real-world web graph. The system uses Breadth-First Search (BFS) to explore relationships between interconnected web resources and investigates how execution performance changes as the size of the graph increases.

The project compares:

* Sequential BFS
* Parallel BFS using OpenMP
* Distributed BFS using MPI

The implementations will be evaluated using execution time, speedup, parallel efficiency, and the effect of increasing graph size and number of parallel workers.

## Objectives

* Implement a sequential BFS algorithm for web graph traversal.
* Develop a parallel BFS implementation using OpenMP.
* Develop a distributed BFS implementation using MPI.
* Verify the correctness of the parallel implementations against the sequential implementation.
* Evaluate performance as the web graph size increases.
* Analyze scalability, speedup, parallel efficiency, and performance bottlenecks.

## Dataset

The project uses the **web-Google** dataset from the Stanford Network Analysis Project (SNAP).

The dataset represents a directed web graph in which:

* Nodes represent webpages.
* Directed edges represent hyperlinks between webpages.

Dataset:

* Nodes: 875,713
* Edges: 5,105,039

Source: Stanford SNAP
Dataset: web-Google

The dataset is not stored in this repository due to its size. Instructions for downloading and preparing the dataset will be provided here.

## Technologies

* C
* OpenMP
* MPI
* Git & GitHub

Python may be used for performance analysis, data processing, and visualization where required.

## Project Structure

```text
web-graph-analysis/
│
├── README.md
├── data/
│
├── src/
│   ├── sequential/
│   ├── openmp/
│   └── mpi/
│
├── tests/
│
├── results/
│
└── docs/
```

## Methodology

The project will follow the following workflow:

1. Obtain and preprocess the web-Google dataset.
2. Represent the web graph using an appropriate graph data structure.
3. Implement and test sequential BFS.
4. Implement parallel BFS using OpenMP.
5. Implement distributed BFS using MPI.
6. Verify parallel results against the sequential implementation.
7. Run experiments with increasing graph sizes.
8. Measure execution time and calculate speedup and parallel efficiency.
9. Analyze scalability and identify performance bottlenecks.
10. Present results using tables and graphs.

## Performance Evaluation

The following metrics will be investigated:

* Execution Time
* Speedup
* Parallel Efficiency
* Effect of increasing graph size
* Effect of increasing number of OpenMP threads
* Effect of increasing number of MPI processes
* Parallelization and communication overhead
* Performance bottlenecks

Performance results and graphs will be added as the project progresses.

## Team

| Member   | Contribution |
| -------- | ------------ |
| Bhargavi | TBD          |
| Madhura  | TBD          |
| Shamita  | TBD          |
| Priyanka | TBD          |

Individual contributions will be documented as the project develops.

## Project Documentation

Additional documentation, experimental results, test cases, and project reports will be maintained in the `docs/` and `results/` directories.

## AI-Assisted Development

AI tools may be used during development for algorithm understanding, implementation guidance, debugging, optimization, test-case generation, and documentation.

All AI-assisted outputs will be reviewed, tested, modified where necessary, and verified by the project team.

An LLM usage log will be maintained separately as required by the project guidelines.
