#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "cell.h"
#include "net.h"
#include "bucketlist.h"
#include <vector>

class FM_Partitioner {
public:
    FM_Partitioner(std::vector<Cell>& cells, std::vector<Net>& nets);
    void runPartitioning();  // Runs the FM partitioning algorithm
    void computeInitialGain(); // Computes initial gain for all cells
    void updateGainAfterMove(Cell& movedCell); // Updates gain when a cell moves
    bool moveCell(Cell& cell); // Moves a cell to the other partition
    int calculateCutSize(); // Calculates the current cut size
    void printResults(); // Prints final partitioning results
    bool allCellsLocked(const std::vector<bool>& locked);

private:
    std::vector<Cell>& cells;
    std::vector<Net>& nets;
    BucketList bucketList;
    int partitionSize[2]; // Keeps track of partition sizes
};

#endif // ALGORITHM_H