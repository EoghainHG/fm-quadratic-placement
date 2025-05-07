#include "algorithm.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <cmath>
#include <chrono>

using namespace std::chrono;

FM_Partitioner::FM_Partitioner(std::vector<Cell>& cells, std::vector<Net>& nets)
    : cells(cells), nets(nets) {
    // Initialize partition sizes
    partitionSize[0] = 0;
    partitionSize[1] = 0;
    
    // Count cells in each partition
    for (const auto& cell : cells) {
        partitionSize[cell.partition]++;
    }
}

void FM_Partitioner::runPartitioning() {
    // Step 1: Compute initial gain for all cells
    computeInitialGain();
    
    // Step 2: Initialize data structures for FM algorithm
    std::cout << "Initializing data structures..." << std::endl;
    std::vector<int> moveHistory;
    std::vector<int> gainHistory;
    int pass = 1;
    int totalGain = 0;
    bool improved = true;
    
    std::cout << "Initial cutsize: " << calculateCutSize() << std::endl;
    
    while (improved && pass <= 1) { // Limit to 1 pass
        std::cout << "Starting FM pass " << pass << std::endl;
        auto start = high_resolution_clock::now();
        // Reset locked status for all cells
        for (auto& cell : cells) {
            cell.locked = false;
        }
        
        // Initialize bucketList for this pass
        std::cout << "Initializing bucket list..." << std::endl;
        bucketList = BucketList();
        
        // Insert all cells into bucket list
        std::cout << "Inserting cells into bucket list..." << std::endl;
        for (size_t i = 0; i < cells.size(); i++) {
            bucketList.insertCell(&cells[i]);
        }
        
        int bestGainIndex = -1;
        int currentPartialGain = 0;
        int bestPartialGain = 0;
        moveHistory.clear();
        gainHistory.clear();
        
        // Keep moving cells until all are locked or no cell available
        int moveCount = 0;
        while (true) {
            // Find best cell to move (highest gain)
            Cell* bestCell = bucketList.getBestCell();
            if (bestCell == nullptr) break;
            
            int cellIndex = bestCell->id;
            
            // Skip if cell is already locked (should not happen with proper bucket management)
            if (bestCell->locked) {
                //std::cout << "Warning: Locked cell selected from bucket list" << std::endl;
                continue;
            }
            
            // Mark cell as locked
            bestCell->locked = true;
            
            // Save original partition for rollback if needed
            int originalPartition = bestCell->partition;
            
            // Move the cell if balance constraint is satisfied
            bool moved = moveCell(*bestCell);
            if (!moved) {
                // If move was rejected, still mark as locked but don't update gain
                continue;
            }
            
            // Record the move
            moveHistory.push_back(cellIndex);
            
            // Update gain of affected cells
            updateGainAfterMove(*bestCell);
            
            // Update partial gain and track history
            currentPartialGain += bestCell->gain;
            gainHistory.push_back(currentPartialGain);
            
            // Keep track of best solution
            if (currentPartialGain > bestPartialGain) {
                bestPartialGain = currentPartialGain;
                bestGainIndex = moveHistory.size() - 1;
            }
            
            moveCount++;
            if (moveCount % 10000 == 0) {
                std::cout << "  Completed " << moveCount << " moves" << std::endl;
            }
        }
        
        // If no improvement, exit
        if (bestPartialGain <= 0) {
            improved = false;
            std::cout << "No improvement in pass " << pass << ", terminating." << std::endl;
            break;
        }
        
        // Roll back to best solution
        if (bestGainIndex >= 0) {
            std::cout << "Rolling back to best solution with gain " << bestPartialGain << std::endl;
            
            // Roll back moves after the best gain index
            for (int i = moveHistory.size() - 1; i > bestGainIndex; i--) {
                int cellToRollback = moveHistory[i];
                
                // Revert the move
                int currentPartition = cells[cellToRollback].partition;
                cells[cellToRollback].partition = 1 - currentPartition;
                
                // Update partition sizes
                partitionSize[cells[cellToRollback].partition]++;
                partitionSize[1 - cells[cellToRollback].partition]--;
            }
        }
        
        totalGain += bestPartialGain;
        std::cout << "Pass " << pass << " complete. Pass gain: " << bestPartialGain << std::endl;
        std::cout << "Current cutsize: " << calculateCutSize() << std::endl;
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);
        std::cout << "Pass took: " << duration.count() << "seconds" << std::endl;
        pass++;
    }
    
    std::cout << "FM partitioning complete. Total gain: " << totalGain << std::endl;
}

void FM_Partitioner::computeInitialGain() {
    // Reset all gains to zero first
    for (auto& cell : cells) {
        cell.gain = 0;
    }
    
    // For each cell, compute its gain
    for (auto& cell : cells) {
        int gain = 0;
        int currentPartition = cell.partition;
        int otherPartition = 1 - currentPartition;
        
        // For each net connected to the cell
        for (int netId : cell.connectedNets) {
            if (netId < 0 || netId >= static_cast<int>(nets.size())) {
                std::cerr << "Warning: Invalid net ID " << netId << " referenced by cell " << cell.id << std::endl;
                continue;
            }
            
            Net& net = nets[netId];
            
            // Count cells in each partition for this net
            int partCounts[2] = {0, 0};
            for (Cell* netCell : net.cells) {
                if (netCell == nullptr) {
                    std::cerr << "Warning: Null cell pointer in net " << netId << std::endl;
                    continue;
                }
                partCounts[netCell->partition]++;
            }
            
            // If this is the only cell in its partition for this net: gain++
            if (partCounts[currentPartition] == 1) {
                gain++;
            }
            
            // If no cells in the other partition for this net: gain--
            if (partCounts[otherPartition] == 0) {
                gain--;
            }
        }
        
        cell.gain = gain;
    }
}

void FM_Partitioner::updateGainAfterMove(Cell& movedCell) {
    int fromPartition = 1 - movedCell.partition; // The partition the cell was moved from
    int toPartition = movedCell.partition;       // The partition the cell was moved to
    
    // For each net connected to the moved cell
    for (int netId : movedCell.connectedNets) {
        if (netId < 0 || netId >= static_cast<int>(nets.size())) {
            std::cerr << "Warning: Invalid net ID " << netId << " during gain update" << std::endl;
            continue;
        }
        
        Net& net = nets[netId];
        
        // Count cells in each partition for this net after the move
        int partCounts[2] = {0, 0};
        for (Cell* cell : net.cells) {
            if (cell == nullptr) {
                std::cerr << "Warning: Null cell pointer in net " << netId << " during gain update" << std::endl;
                continue;
            }
            partCounts[cell->partition]++;
        }
        
        // Update gains of unlocked cells connected to this net
        for (Cell* cell : net.cells) {
            if (cell == nullptr || cell->id == movedCell.id || cell->locked) {
                continue;
            }
            
            int cellPart = cell->partition;
            int otherPart = 1 - cellPart;
            
            // Case 1: If this cell is the last one in its partition for this net
            if (partCounts[cellPart] == 1) {
                cell->updateGain(1);
                bucketList.updateCell(cell, cell->gain);
            }
            
            // Case 2: If there are no cells in the other partition for this net
            if (partCounts[otherPart] == 0) {
                cell->updateGain(-1);
                bucketList.updateCell(cell, cell->gain);
            }
            
            // Case 3: If the moved cell was the only one in its original partition
            if (partCounts[fromPartition] == 0 && cell->partition == toPartition) {
                cell->updateGain(-1);
                bucketList.updateCell(cell, cell->gain);
            }
            
            // Case 4: If the moved cell is the first one in its new partition
            if (partCounts[toPartition] == 1 && cell->partition == fromPartition) {
                cell->updateGain(1);
                bucketList.updateCell(cell, cell->gain);
            }
        }
    }
}

bool FM_Partitioner::moveCell(Cell& cell) {
    // First check if cell is movable
    if (!cell.movable) {
        return false;  // Can't move terminals
    }
    // Change partition
    int oldPartition = cell.partition;
    int newPartition = 1 - oldPartition;
    
    // Check if move would violate balance constraint
    double balanceTolerance = 0.1; // Allow 10% imbalance
    int totalCells = cells.size();
    int targetSize = totalCells / 2;
    double maxSizeDeviation = targetSize * balanceTolerance;
    
    if (std::abs((partitionSize[newPartition] + 1) - targetSize) > maxSizeDeviation) {
        // Move rejected due to balance constraint
        return false;
    }
    
    // Update partition
    cell.partition = newPartition;
    
    // Update partition sizes
    partitionSize[oldPartition]--;
    partitionSize[newPartition]++;
    
    return true;
}


int FM_Partitioner::calculateCutSize() {
    int cutSize = 0;
    
    // Calculate cut size by checking each net
    for (size_t i = 0; i < nets.size(); i++) {
        const auto& net = nets[i];
        
        // Add proper logging without any unsafe operations
        //std::cout << "Checking net " << net.id << " with " << net.cells.size() << " cells" << std::endl;
        
        bool hasPartition[2] = {false, false};
        
        for (size_t j = 0; j < net.cells.size(); j++) {
            const Cell* cell = net.cells[j];
            if (cell == nullptr) {
                std::cerr << "Warning: Null cell pointer at index " << j << " in net " << net.id << std::endl;
                continue;
            }
            
            if (cell->partition != 0 && cell->partition != 1) {
                std::cerr << "Error: Invalid partition value " << cell->partition 
                          << " for cell " << cell->id << " in net " << net.id << std::endl;
                continue;
            }
            
            hasPartition[cell->partition] = true;
        }
        
        // If net has cells in both partitions, it's cut
        if (hasPartition[0] && hasPartition[1]) {
            cutSize++;
        }
    }
    
    return cutSize;
}

void FM_Partitioner::printResults() {
    int cutSize = calculateCutSize();
    
    std::cout << "\nPartitioning Results:" << std::endl;
    std::cout << "Partition 0 size: " << partitionSize[0] << std::endl;
    std::cout << "Partition 1 size: " << partitionSize[1] << std::endl;
    std::cout << "Cut size: " << cutSize << std::endl;
    
    // Print partition assignments (limit to first 20 cells to avoid excessive output)
    std::cout << "Sample Cell Assignments:" << std::endl;
    int printLimit = std::min(20, static_cast<int>(cells.size()));
    for (int i = 0; i < printLimit; i++) {
        std::cout << "Cell " << cells[i].id << " -> Partition " << cells[i].partition << std::endl;
    }
    if (cells.size() > 20) {
        std::cout << "... and " << (cells.size() - 20) << " more cells" << std::endl;
    }
}

bool FM_Partitioner::allCellsLocked(const std::vector<bool>& locked) {
    for (bool isLocked : locked) {
        if (!isLocked) return false;
    }
    return true;
}