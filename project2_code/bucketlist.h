#ifndef BUCKETLIST_H
#define BUCKETLIST_H

#include "cell.h"
#include <vector>
#include <unordered_map>

class BucketList {
public:
    BucketList();
    void insertCell(Cell* cell); // Inserts a cell into the bucket list
    Cell* getBestCell(); // Returns the best cell to move
    void updateCell(Cell* cell, int newGain); // Updates a cell's gain
    bool isEmpty(); // Checks if the bucket list is empty

private:
    std::unordered_map<int, std::vector<Cell*>> gainBuckets;
    int maxGain;
};

#endif // BUCKETLIST_H
