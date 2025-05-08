#include "bucketlist.h"
#include <iostream>

BucketList::BucketList() : maxGain(0) {}

void BucketList::insertCell(Cell* cell) {
    if (cell == nullptr) {
        std::cerr << "Warning: Attempted to insert null cell into bucket list" << std::endl;
        return;
    }
    
    if (cell->locked) {
        // Don't insert locked cells
        return;
    }
    
    gainBuckets[cell->gain].push_back(cell);
    if (cell->gain > maxGain) {
        maxGain = cell->gain;
    }
}

Cell* BucketList::getBestCell() {
    // Find the highest non-empty gain bucket
    while (maxGain >= -1000) { // Limit how far down we go to avoid infinite loops
        auto bucketIter = gainBuckets.find(maxGain);
        if (bucketIter != gainBuckets.end() && !bucketIter->second.empty()) {
            Cell* cell = bucketIter->second.back();
            bucketIter->second.pop_back();
            
            // If bucket is now empty, find next highest gain
            if (bucketIter->second.empty()) {
                gainBuckets.erase(maxGain);
                maxGain = -1001; // Start below our limit
                for (const auto& bucket : gainBuckets) {
                    if (bucket.first > maxGain) {
                        maxGain = bucket.first;
                    }
                }
            }
            
            return cell;
        }
        maxGain--;
    }
    return nullptr;
}

void BucketList::updateCell(Cell* cell, int newGain) {
    if (cell == nullptr) {
        std::cerr << "Warning: Attempted to update null cell in bucket list" << std::endl;
        return;
    }
    
    if (cell->locked) {
        // Don't update locked cells
        return;
    }
    
    int oldGain = cell->gain;
    
    // Check if the cell exists in its current bucket
    auto bucketIter = gainBuckets.find(oldGain);
    if (bucketIter != gainBuckets.end()) {
        auto& bucket = bucketIter->second;
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (*it == cell) {
                // Remove from current bucket
                bucket.erase(it);
                if (bucket.empty()) {
                    gainBuckets.erase(oldGain);
                    // Need to update maxGain if we removed the highest bucket
                    if (oldGain == maxGain) {
                        maxGain = -1001; // Start below our limit
                        for (const auto& b : gainBuckets) {
                            if (b.first > maxGain) {
                                maxGain = b.first;
                            }
                        }
                    }
                }
                
                // Update cell's gain
                cell->gain = newGain;
                
                // Insert into new bucket
                gainBuckets[newGain].push_back(cell);
                if (newGain > maxGain) {
                    maxGain = newGain;
                }
                
                return;
            }
        }
    }
    
    // If cell was not found in its supposed bucket, just insert it
    cell->gain = newGain;
    gainBuckets[newGain].push_back(cell);
    if (newGain > maxGain) {
        maxGain = newGain;
    }
}

bool BucketList::isEmpty() {
    return gainBuckets.empty() || maxGain < -1000;
}