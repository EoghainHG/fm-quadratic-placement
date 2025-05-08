#ifndef PLACEMENT_COMMON_H
#define PLACEMENT_COMMON_H

#include <cmath>

// Cell position structure
struct CellPosition {
    int cellId;
    double x, y;
    double width, height;
    bool isFixed;
    
    CellPosition(int id, double x, double y, double w = 0.01, double h = 0.01, bool fixed = false)
        : cellId(id), x(x), y(y), width(w), height(h), isFixed(fixed) {}
    CellPosition() : cellId(-1), x(0.0), y(0.0), width(0.01), height(0.01), isFixed(false) {}


    bool overlaps(const CellPosition& other) const {
        // Check if two cells overlap
        double dx = std::abs(x - other.x);
        double dy = std::abs(y - other.y);
        
        double minDistX = (width + other.width) / 2.0;
        double minDistY = (height + other.height) / 2.0;
        
        return (dx < minDistX && dy < minDistY);
    }
};

// Region structure
struct Region {
    double x1, y1, x2, y2;
    
    // Add a default constructor
    Region() : x1(0.0), y1(0.0), x2(0.0), y2(0.0) {}
    
    // Existing constructor
    Region(double x1, double y1, double x2, double y2)
        : x1(x1), y1(y1), x2(x2), y2(y2) {}
    
    std::pair<Region, Region> splitHorizontal() const {
        double midX = (x1 + x2) / 2.0;
        return {
            Region(x1, y1, midX, y2),
            Region(midX, y1, x2, y2)
        };
    }
    
    std::pair<Region, Region> splitVertical() const {
        double midY = (y1 + y2) / 2.0;
        return {
            Region(x1, y1, x2, midY),
            Region(x1, midY, x2, y2)
        };
    }
};

#endif // PLACEMENT_COMMON_H