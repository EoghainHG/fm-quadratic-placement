#ifndef PLACEMENT_H
#define PLACEMENT_H

#include "cell.h"
#include "net.h"
#include "algorithm.h"
#include "placement_common.h"  // Include the common header
#include <vector>
#include <unordered_map>
#include <set>

class Placement {
public:
    Placement(std::vector<Cell>& cells, std::vector<Net>& nets);
    
    void addFixedCell(int cellId, double x, double y, double width = 0.03, double height = 0.03);
    void runPlacement();
    double calculateWirelength() const;
    void outputPlacement(const std::string& filename);
    
private:
    // References to input data
    std::vector<Cell>& cells;
    std::vector<Net>& nets;
    
    // Cell positions
    std::vector<CellPosition> cellPositions;
    std::unordered_map<int, CellPosition> fixedCells;
    
    // Private methods
    void preprocessFixedCells();
    void recursiveBipartition(const Region& region, const std::vector<int>& cellIndices, int depth);
    void legalizePlacement();
    std::pair<double, double> getCellDimensions(int cellId) const;
};

#endif // PLACEMENT_H