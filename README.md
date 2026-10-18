# VLSI Standard-Cell Placement Engine

A C++ implementation of a standard-cell placement flow for VLSI physical design. It reads industry-standard **Bookshelf-format** benchmark circuits (`.nodes` / `.nets`, e.g. the ISPD `superblue12` benchmark), partitions the netlist, places every cell on the die, legalizes the layout to remove overlaps, and visualizes the result as a scatter plot and a density heatmap.

Two independent placement strategies are implemented, each in its own directory:

| Directory | Approach |
|---|---|
| [`project2_code/`](project2_code) | **Min-cut recursive bipartitioning placement** — repeatedly partitions the chip region and the netlist in lockstep |
| [`Quadrature/`](Quadrature) | **Quadratic (analytic) placement** — formulates placement as a sparse linear system solved with Eigen, followed by cell spreading |

## How it works

1. **Parsing** (`Parser.cpp`, `nets_parse.cpp`, `NodeNet.cpp`) — reads `.nodes` and `.nets` benchmark files into a netlist graph, distinguishing movable cells from fixed terminals (`terminal` / `terminal_NI`) and recording each cell's area and pin connectivity.

2. **Min-cut partitioning** (`algorithm.cpp`, `bucketlist.cpp`) — implements the **Fiduccia–Mattheyses (FM)** algorithm: cells are assigned to one of two partitions, gains are computed and maintained in a bucket-list priority structure, and cells are repeatedly moved and locked to minimize the net cut size. This partitioning drives both placement strategies below. A checkpointing mechanism (`fm_checkpoint*.dat`) allows FM results to be saved/reloaded instead of recomputed.

3a. **Recursive bipartitioning placement** (`project2_code/placement.cpp`) — recursively splits the chip's rectangular region in half (alternating horizontal/vertical cuts), using FM partitioning to decide which cells belong in each half at every level of recursion, until each region holds a small group of cells. A legalization pass then removes any remaining overlaps while respecting fixed terminal positions.

3b. **Quadratic placement** (`Quadrature/quadratic_placement.cpp`) — builds a sparse connectivity matrix from the netlist (using clique and star net models) and solves for cell positions that minimize squared wirelength via Eigen's sparse solver. Cells are then spread out to relieve density and legalized to eliminate overlaps.

4. **Output & visualization** — both flows write final cell positions to a CSV (`placement_result*.csv`: cell ID, X, Y, width, height, fixed flag). `project2_code/plotting.py` reads this CSV and renders:
   - a scatter plot of all cell positions (fixed cells in red, movable cells in blue), and
   - a density heatmap of area utilization across the die.

   Rendered outputs for both strategies are collected in [`Placement Images/`](Placement%20Images) (`placementN.png` for min-cut placement, `quadplacementN.png` for quadratic placement).

## Building & running

Each variant is self-contained with its own `Makefile`.

### Min-cut bipartitioning placement
```bash
cd project2_code
make        # builds `main` and runs it
```

### Quadratic placement
Requires the [Eigen3](https://eigen.tuxfamily.org/) header-only linear algebra library.
```bash
cd Quadrature
# update the Eigen include path in the Makefile (CXXFLAGS) for your system
make
```

Both executables expect the benchmark's `.nets` and `.nodes` files (e.g. `superblue12.nets` / `superblue12.nodes`) in the working directory, and write a `placement_result*.csv` on completion.

### Visualizing results
```bash
cd project2_code
pip install pandas matplotlib numpy seaborn
python plotting.py   # reads placement_result1.csv, writes cell_placement_analysis.png
```

## Repository layout

```
project2_code/    Min-cut recursive bipartitioning placer + FM partitioner + parser + plotting script
Quadrature/        Quadratic (Eigen-based) placer, sharing the same FM partitioner and parser
Placement Images/  Rendered scatter/heatmap outputs for both placement strategies
```

## Background

This implements the classic two-stage physical design flow: (1) partitioning via FM min-cut, and (2) global placement via recursive bisection or analytic (quadratic) methods — core techniques used in real EDA placement tools.
