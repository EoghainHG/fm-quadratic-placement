#ifndef PLACEMENT_H
#define PLACEMENT_H

#include "algorithm.h"
#include "cell.h"
#include "net.h"
#include <vector>
#include <utility>
#include <unordered_map>

// Struct to represent a rectangular region
struct Region {
    double x1, y1;  // Bottom-left corner
    double x2, y2;  // Top-right corner
    
    Region(double x1 = 0.0, double y1 = 0.0, double x2 = 1.0, double y2 = 1.0) 
        : x1(x1), y1(y1), x2(x2), y2(y2) {}
    
    double width() const { return x2 - x1; }
    double height() const { return y2 - y1; }
    double area() const { return width() * height(); }
    
    // Split region horizontally (returns left and right regions)
    std::pair<Region, Region> splitHorizontal() const {
        double midX = (x1 + x2) / 2.0;
        return std::make_pair(
            Region(x1, y1, midX, y2),
            Region(midX, y1, x2, y2)
        );
    }
    
    // Split region vertically (returns bottom and top regions)
    std::pair<Region, Region> splitVertical() const {
        double midY = (y1 + y2) / 2.0;
        return std::make_pair(
            Region(x1, y1, x2, midY),
            Region(x1, midY, x2, y2)
        );
    }
    
    // Check if point is inside region
    bool contains(double x, double y) const {
        return x >= x1 && x <= x2 && y >= y1 && y <= y2;
    }
};

// Structure to store cell position
struct CellPosition {
    int cellId;
    double x, y;  // Center position
    double width, height;
    bool isFixed;
    
    // Default constructor needed for std::unordered_map
    CellPosition() : cellId(-1), x(0), y(0), width(0), height(0), isFixed(false) {}
    
    CellPosition(int id, double x, double y, double w, double h, bool fixed = false)
        : cellId(id), x(x), y(y), width(w), height(h), isFixed(fixed) {}
    
    // Check if this cell overlaps with another
    bool overlaps(const CellPosition& other) const {
        return (x - width/2 < other.x + other.width/2) &&
               (x + width/2 > other.x - other.width/2) &&
               (y - height/2 < other.y + other.height/2) &&
               (y + height/2 > other.y - other.height/2);
    }
};

class Placement {
public:
    Placement(std::vector<Cell>& cells, std::vector<Net>& nets);
    
    // Add a fixed cell with given position
    void addFixedCell(int cellId, double x, double y, double width, double height);
    
    // Run the placement algorithm
    void runPlacement();
    
    // Output placement results to a file
    void outputPlacement(const std::string& filename);
    
    // Get the final positions
    const std::vector<CellPosition>& getPositions() const { return cellPositions; }

private:
    std::vector<Cell>& cells;
    std::vector<Net>& nets;
    std::vector<CellPosition> cellPositions;
    std::unordered_map<int, CellPosition> fixedCells;
    
    // Pre-place fixed cells
    void preprocessFixedCells();
    
    // Recursive bipartitioning (modified to handle fixed cells)
    void recursiveBipartition(const Region& region, const std::vector<int>& cellIndices, int depth);
    
    // Legalization to remove overlaps (respecting fixed cells)
    void legalizePlacement();
    
    // Calculate the wirelength of current placement
    double calculateWirelength() const;
    
    // Get cell dimensions
    std::pair<double, double> getCellDimensions(int cellId) const;
};

#endif // PLACEMENT_H