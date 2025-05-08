#ifndef CELL_H
#define CELL_H

#include <vector>

class Cell {
public:
    int id;
    int gain;
    bool locked;
    bool movable;
    int partition;
    std::vector<int> connectedNets; // List of connected nets

    Cell(int id);
    void updateGain(int delta);
};

#endif // CELL_H