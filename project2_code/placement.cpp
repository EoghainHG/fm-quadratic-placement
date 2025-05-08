#include "placement.h"
#include <iostream>
#include <fstream>
#include <execution>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <queue>
#include <set>
#include <mutex>    // Add this for std::mutex
#include <sstream>  // Add this for std::ostringstream
#include <atomic>   // Add this for std::atomic

Placement::Placement(std::vector<Cell>& cells, std::vector<Net>& nets)
    : cells(cells), nets(nets) {
    // Initialize with empty positions
    cellPositions.reserve(cells.size());
}

void Placement::addFixedCell(int cellId, double x, double y, double width, double height) {
    fixedCells[cellId] = CellPosition(cellId, x, y, width, height, true);
}

void Placement::preprocessFixedCells() {
    // First, add all fixed cells to the cell positions vector
    for (const auto& pair : fixedCells) {
        cellPositions.push_back(pair.second);
    }
    
    // Create a set of fixed cell IDs for quick lookup
    std::set<int> fixedCellIds;
    for (const auto& pair : fixedCells) {
        fixedCellIds.insert(pair.first);
    }
    
    // Mark fixed cells in the original cell vector
    for (auto& cell : cells) {
        if (fixedCellIds.find(cell.id) != fixedCellIds.end()) {
            cell.movable = false;
            cell.locked = true;
        }
    }
}

std::pair<double, double> Placement::getCellDimensions(int cellId) const {
    // First check if it's a fixed cell
    auto it = fixedCells.find(cellId);
    if (it != fixedCells.end()) {
        return {it->second.width, it->second.height};
    }
    
    // Otherwise use default dimensions
    // You could replace this with actual cell dimensions from your data
    return {0.01, 0.01};
}

void Placement::runPlacement() {
    std::cout << "Starting parallel placement algorithm..." << std::endl;
    
    // Pre-process fixed cells
    preprocessFixedCells();
    
    // Create initial region
    Region initialRegion(0.0, 0.0, 1.0, 1.0);
    
    // First, divide cells into top-level clusters based on connectivity
    const size_t CLUSTER_SIZE = 10; // Adjust as needed
    std::vector<std::vector<int>> clusters;
    
    // Group cells by connectivity or simply by ID ranges
    std::vector<int> currentCluster;
    for (size_t i = 0; i < cells.size(); ++i) {
        if (cells[i].movable) {
            currentCluster.push_back(i);
            
            if (currentCluster.size() >= CLUSTER_SIZE) {
                clusters.push_back(currentCluster);
                currentCluster.clear();
            }
        }
    }
    
    // Add the last cluster if not empty
    if (!currentCluster.empty()) {
        clusters.push_back(currentCluster);
    }
    
    std::cout << "Created " << clusters.size() << " top-level clusters" << std::endl;
    
    // Process top-level clusters in parallel
    std::vector<Region> clusterRegions(clusters.size(), initialRegion);
    
    // First: determine regions for each cluster (sequentially)
    // You could divide the space evenly among clusters
    for (size_t i = 0; i < clusters.size(); ++i) {
        // Create a subregion based on cluster index
        double x1, y1, x2, y2;
        int rowCount = std::ceil(std::sqrt(clusters.size()));
        int colCount = std::ceil(clusters.size() / (double)rowCount);
        
        int row = i / colCount;
        int col = i % colCount;
        
        x1 = col / (double)colCount;
        y1 = row / (double)rowCount;
        x2 = (col + 1) / (double)colCount;
        y2 = (row + 1) / (double)rowCount;
        
        clusterRegions[i] = Region(x1, y1, x2, y2);
    }
    
    // Then process each cluster in parallel
    std::for_each(
        std::execution::par,
        clusters.begin(), clusters.end(),
        [this, &clusterRegions, &clusters](const std::vector<int>& cluster) {
            size_t clusterIndex = &cluster - &clusters[0];
            recursiveBipartition(clusterRegions[clusterIndex], cluster, 0);
        }
    );
    
    // Legalize the placement to remove any overlaps
    legalizePlacement();
    std::cout << "crash before wirelength" << std::endl;
    // Calculate and report wirelength
    double wirelength = calculateWirelength();
    std::cout << "Placement complete. Total wirelength: " << wirelength << std::endl;
}

void Placement::recursiveBipartition(const Region& region, const std::vector<int>& cellIndices, int depth) {
    // Base case: if we have few cells, region is too small, or depth limit reached
    if (cellIndices.size() <= 10 || depth > 6) {  // Reduced depth limit and increased cell count for base case
        double centerX = (region.x1 + region.x2) / 2.0;
        double centerY = (region.y1 + region.y2) / 2.0;
        
        // Spread cells around center point instead of putting them all at the exact same spot
        double spread = 0.02; // Small spread factor
        int cellCount = cellIndices.size();
        
        for (int i = 0; i < cellCount; ++i) {
            int idx = cellIndices[i];
            // Skip if this is a fixed cell
            if (fixedCells.find(idx) != fixedCells.end()) {
                continue;
            }
            
            // Get cell dimensions
            auto dimensions = getCellDimensions(idx);
            double cellWidth = dimensions.first;
            double cellHeight = dimensions.second;
            
            // Calculate position with some spread to avoid all cells at exactly the same spot
            double angle = 2.0 * M_PI * i / cellCount;
            double offsetX = spread * std::cos(angle);
            double offsetY = spread * std::sin(angle);
            
            // Store the cell position
            cellPositions.emplace_back(idx, centerX + offsetX, centerY + offsetY, cellWidth, cellHeight);
        }
        return;
    }
    
    // For large cell groups, use a simpler partitioning approach
    if (cellIndices.size() > 500) {  // For very large groups, avoid creating too many temp objects
        // Simple geometric bipartitioning without creating temp cells/nets
        bool cutHorizontal = (depth % 2 == 0);
        std::vector<int> partition0, partition1;
        
        for (int idx : cellIndices) {
            // For fixed cells, decide based on their fixed position
            auto fixedIt = fixedCells.find(idx);
            if (fixedIt != fixedCells.end()) {
                double x = fixedIt->second.x;
                double y = fixedIt->second.y;
                
                if (cutHorizontal) {
                    double midX = (region.x1 + region.x2) / 2.0;
                    (x < midX) ? partition0.push_back(idx) : partition1.push_back(idx);
                } else {
                    double midY = (region.y1 + region.y2) / 2.0;
                    (y < midY) ? partition0.push_back(idx) : partition1.push_back(idx);
                }
            } else {
                // For movable cells, decide based on existing partition or random
                (cells[idx].partition == 0) ? partition0.push_back(idx) : partition1.push_back(idx);
            }
        }
        
        // Balance the partitions if they're too uneven
        int diff = static_cast<int>(partition0.size()) - static_cast<int>(partition1.size());
        if (std::abs(diff) > static_cast<int>(cellIndices.size() / 4)) {
            // Move some cells from larger to smaller partition
            auto& larger = (diff > 0) ? partition0 : partition1;
            auto& smaller = (diff > 0) ? partition1 : partition0;
            
            int cellsToMove = std::abs(diff) / 2;
            for (int i = 0; i < cellsToMove && !larger.empty(); ++i) {
                smaller.push_back(larger.back());
                larger.pop_back();
            }
        }
        
        // Split the region
        std::pair<Region, Region> subRegions;
        if (cutHorizontal) {
            subRegions = region.splitHorizontal();
        } else {
            subRegions = region.splitVertical();
        }
        
        // Recursively place each partition in its subregion
        recursiveBipartition(subRegions.first, partition0, depth + 1);
        recursiveBipartition(subRegions.second, partition1, depth + 1);
        return;
    }
    
    // Normal FM partitioning for smaller groups (this is the original code)
    // Create temporary vectors for FM partitioning
    std::vector<Cell> tempCells;
    std::vector<Net> tempNets;
    tempCells.reserve(cellIndices.size() + 100); // Reserve more to avoid reallocation
    
    // [... Rest of the original implementation ...]
    // Continue with the existing FM partitioning logic as before
    
    // Map of original cell indices to temp indices
    std::unordered_map<int, int> origToTemp;
    std::unordered_map<int, int> tempToOrig;
    
    // Build temporary cells for partitioning
    for (size_t i = 0; i < cellIndices.size(); ++i) {
        int origIdx = cellIndices[i];
        tempCells.push_back(cells[origIdx]);
        tempCells.back().id = i;  // Give sequential IDs in the temporary structure
        origToTemp[origIdx] = i;
        tempToOrig[i] = origIdx;
    }
    
    // Build temporary nets for partitioning
    std::unordered_map<int, int> origNetToTemp;
    int tempNetId = 0;
    
    for (const auto& net : nets) {
        // Check if this net connects cells in our current set
        bool connects = false;
        for (Cell* cell : net.cells) {
            if (cell && origToTemp.find(cell->id) != origToTemp.end()) {
                connects = true;
                break;
            }
        }
        
        if (connects) {
            tempNets.push_back(Net(tempNetId));
            origNetToTemp[net.id] = tempNetId;
            
            // Add cells to the net
            for (Cell* cell : net.cells) {
                if (!cell) continue; // Skip null pointers
                
                auto it = origToTemp.find(cell->id);
                if (it != origToTemp.end()) {
                    tempNets.back().addCell(&tempCells[it->second]);
                    tempCells[it->second].connectedNets.push_back(tempNetId);
                } else {
                    // This is a fixed cell or a cell in another region
                    // We need to add a weight to bias the partitioning
                    
                    // Find the fixed cell's position
                    double fixedX = 0.5, fixedY = 0.5; // Default center
                    bool found = false;
                    
                    // Check if it's in our fixed cells
                    auto fixedIt = fixedCells.find(cell->id);
                    if (fixedIt != fixedCells.end()) {
                        fixedX = fixedIt->second.x;
                        fixedY = fixedIt->second.y;
                        found = true;
                    } else {
                        // Check if it's already placed
                        for (const auto& pos : cellPositions) {
                            if (pos.cellId == cell->id) {
                                fixedX = pos.x;
                                fixedY = pos.y;
                                found = true;
                                break;
                            }
                        }
                    }
                    
                    if (found) {
                        // Determine which region partition this fixed cell is in or closest to
                        bool horizontalCut = (depth % 2 == 0);
                        int preferredPartition;
                        
                        if (horizontalCut) {
                            // Region will be split left/right
                            double midX = (region.x1 + region.x2) / 2.0;
                            preferredPartition = (fixedX < midX) ? 0 : 1;
                        } else {
                            // Region will be split bottom/top
                            double midY = (region.y1 + region.y2) / 2.0;
                            preferredPartition = (fixedY < midY) ? 0 : 1;
                        }
                        
                        // Bias the partitioning by adding a "dummy" cell that's pre-assigned
                        // to the preferred partition
                        Cell dummyCell(tempCells.size());
                        dummyCell.movable = true;
                        dummyCell.locked = true;
                        dummyCell.partition = preferredPartition;
                        
                        // Connect it to this net
                        tempNets.back().addCell(&dummyCell);
                        tempCells.push_back(dummyCell);
                    }
                }
            }
            
            tempNetId++;
        }
    }
    
    // Run FM partitioning on the temporary structures
    FM_Partitioner partitioner(tempCells, tempNets);
    partitioner.runPartitioning();
    
    // Collect cells for each partition
    std::vector<int> partition0;
    std::vector<int> partition1;
    partition0.reserve(cellIndices.size());
    partition1.reserve(cellIndices.size());
    
    for (const auto& cell : tempCells) {
        if (cell.id >= static_cast<int>(cellIndices.size())) {
            // Skip dummy cells
            continue;
        }
        
        if (cell.partition == 0) {
            partition0.push_back(tempToOrig[cell.id]);
        } else {
            partition1.push_back(tempToOrig[cell.id]);
        }
    }
    
    // Determine cut direction (alternate between horizontal and vertical)
    bool cutHorizontal = (depth % 2 == 0);
    
    // Split the region
    std::pair<Region, Region> subRegions;
    if (cutHorizontal) {
        subRegions = region.splitHorizontal();
    } else {
        subRegions = region.splitVertical();
    }
    
    // Recursively place each partition in its subregion
    recursiveBipartition(subRegions.first, partition0, depth + 1);
    recursiveBipartition(subRegions.second, partition1, depth + 1);
}

void Placement::legalizePlacement() {
    std::cout << "Starting parallel legalization..." << std::endl;
    
    // Simple greedy legalization - shift cells that overlap
    bool overlapsExist = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 100;
    
    // Create a grid for spatial partitioning to reduce the number of overlap checks
    const int GRID_SIZE = 100;
    std::vector<std::vector<size_t>> grid(GRID_SIZE * GRID_SIZE);
    
    // Function to map cell position to grid index
    auto getGridIndex = [GRID_SIZE](double x, double y) -> size_t {
        int gridX = std::min(GRID_SIZE - 1, std::max(0, static_cast<int>(x * GRID_SIZE)));
        int gridY = std::min(GRID_SIZE - 1, std::max(0, static_cast<int>(y * GRID_SIZE)));
        return gridY * GRID_SIZE + gridX;
    };
    
    // Initially populate grid
    for (size_t i = 0; i < cellPositions.size(); ++i) {
        size_t gridIdx = getGridIndex(cellPositions[i].x, cellPositions[i].y);
        grid[gridIdx].push_back(i);
    }
    
    while (overlapsExist && iterations < MAX_ITERATIONS) {
        std::cout << "Iteration: " << iterations << std::endl;
        iterations++;
        
        // Initialize with false, will be set to true if any overlaps are found
        std::atomic<bool> anyOverlaps{false};
        
        // Process each grid cell in parallel
        std::for_each(std::execution::par, grid.begin(), grid.end(), 
            [&anyOverlaps, &grid, this](const std::vector<size_t>& cellIndices) {
                // Process cells within this grid cell
                for (size_t i = 0; i < cellIndices.size(); ++i) {
                    size_t cellIdx1 = cellIndices[i];
                    
                    // Skip fixed cells
                    if (cellPositions[cellIdx1].isFixed) continue;
                    
                    for (size_t j = i+1; j < cellIndices.size(); ++j) {
                        size_t cellIdx2 = cellIndices[j];
                        
                        if (cellPositions[cellIdx1].overlaps(cellPositions[cellIdx2])) {
                            anyOverlaps.store(true);
                            
                            // Handle overlap (similar to your original code)
                            if (cellPositions[cellIdx2].isFixed) {
                                // Move cell1 away from fixed cell2
                                // Logic similar to your original code
                                double dx = cellPositions[cellIdx1].x - cellPositions[cellIdx2].x;
                                double dy = cellPositions[cellIdx1].y - cellPositions[cellIdx2].y;
                                
                                // Normalize
                                double dist = std::sqrt(dx*dx + dy*dy);
                                if (dist < 0.001) {
                                    dx = 0.001 * (rand() % 100 - 50) / 50.0;
                                    dy = 0.001 * (rand() % 100 - 50) / 50.0;
                                } else {
                                    dx /= dist;
                                    dy /= dist;
                                }
                                
                                // Minimum distance needed
                                double minDist = (cellPositions[cellIdx1].width + cellPositions[cellIdx2].width) / 2.0 +
                                                (cellPositions[cellIdx1].height + cellPositions[cellIdx2].height) / 2.0;
                                minDist *= 0.55;
                                
                                // Move cell1 away
                                cellPositions[cellIdx1].x += dx * minDist;
                                cellPositions[cellIdx1].y += dy * minDist;
                                
                                // Keep within bounds
                                cellPositions[cellIdx1].x = std::max(cellPositions[cellIdx1].width/2, 
                                                           std::min(1.0 - cellPositions[cellIdx1].width/2, 
                                                                   cellPositions[cellIdx1].x));
                                cellPositions[cellIdx1].y = std::max(cellPositions[cellIdx1].height/2, 
                                                           std::min(1.0 - cellPositions[cellIdx1].height/2, 
                                                                   cellPositions[cellIdx1].y));
                            } else {
                                // Both movable
                                // Logic similar to your original code
                                double dx = cellPositions[cellIdx2].x - cellPositions[cellIdx1].x;
                                double dy = cellPositions[cellIdx2].y - cellPositions[cellIdx1].y;
                                
                                // Normalize
                                double dist = std::sqrt(dx*dx + dy*dy);
                                if (dist < 0.001) {
                                    dx = 0.001 * (rand() % 100 - 50) / 50.0;
                                    dy = 0.001 * (rand() % 100 - 50) / 50.0;
                                } else {
                                    dx /= dist;
                                    dy /= dist;
                                }
                                
                                // Minimum distance needed
                                double minDist = (cellPositions[cellIdx1].width + cellPositions[cellIdx2].width) / 2.0 +
                                                (cellPositions[cellIdx1].height + cellPositions[cellIdx2].height) / 2.0;
                                minDist *= 0.55;
                                
                                // Move both cells
                                cellPositions[cellIdx1].x -= dx * minDist * 0.5;
                                cellPositions[cellIdx1].y -= dy * minDist * 0.5;
                                cellPositions[cellIdx2].x += dx * minDist * 0.5;
                                cellPositions[cellIdx2].y += dy * minDist * 0.5;
                                
                                // Keep within bounds
                                cellPositions[cellIdx1].x = std::max(cellPositions[cellIdx1].width/2, 
                                                           std::min(1.0 - cellPositions[cellIdx1].width/2, 
                                                                   cellPositions[cellIdx1].x));
                                cellPositions[cellIdx1].y = std::max(cellPositions[cellIdx1].height/2, 
                                                           std::min(1.0 - cellPositions[cellIdx1].height/2, 
                                                                   cellPositions[cellIdx1].y));
                                
                                cellPositions[cellIdx2].x = std::max(cellPositions[cellIdx2].width/2, 
                                                           std::min(1.0 - cellPositions[cellIdx2].width/2, 
                                                                   cellPositions[cellIdx2].x));
                                cellPositions[cellIdx2].y = std::max(cellPositions[cellIdx2].height/2, 
                                                           std::min(1.0 - cellPositions[cellIdx2].height/2, 
                                                                   cellPositions[cellIdx2].y));
                            }
                        }
                    }
                }
            });
        
        // Update grid after each iteration
        std::vector<std::vector<size_t>> newGrid(GRID_SIZE * GRID_SIZE);
        
        for (size_t i = 0; i < cellPositions.size(); ++i) {
            size_t gridIdx = getGridIndex(cellPositions[i].x, cellPositions[i].y);
            newGrid[gridIdx].push_back(i);
        }
        
        grid = std::move(newGrid);
        overlapsExist = anyOverlaps.load();
    }
    
    std::cout << "Parallel legalization completed in " << iterations << " iterations." << std::endl;
}

double Placement::calculateWirelength() const {
    std::cout << "Starting wirelength calculation..." << std::endl;
    double totalWL = 0.0;
    size_t processedCount = 0;
    size_t totalNets = nets.size();  // Define this variable!
    
    std::cout << "Total nets to process: " << totalNets << std::endl;
    
    for (size_t i = 0; i < totalNets; i++) {
        // Log progress periodically
        if (i % 10000 == 0) {
            std::cout << "Processing net " << i << " of " << totalNets << std::endl;
        }
        
        const auto& net = nets[i];
        
        // Skip empty nets
        if (net.cells.empty()) continue;
        
        // Calculate actual wirelength here instead of using unit wirelength
        totalWL += 1;
        processedCount++;
    }
    
    std::cout << "Completed wirelength calculation for " << processedCount << " nets" << std::endl;
    return totalWL;
}

void Placement::outputPlacement(const std::string& filename) {
    std::cout << "Starting to write placement results to file..." << std::endl;
    
    // Write output in chunks to avoid memory issues
    const size_t CHUNK_SIZE = 10000;
    
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file " << filename << std::endl;
        return;
    }
    
    // Write header
    outFile << "CellID,X,Y,Width,Height,IsFixed" << std::endl;
    
    // Write in chunks
    size_t totalCells = cellPositions.size();
    size_t processed = 0;
    
    while (processed < totalCells) {
        size_t chunkEnd = std::min(processed + CHUNK_SIZE, totalCells);
        
        // Process this chunk
        for (size_t i = processed; i < chunkEnd; ++i) {
            const auto& pos = cellPositions[i];
            outFile << pos.cellId << "," 
                    << pos.x << "," 
                    << pos.y << "," 
                    << pos.width << "," 
                    << pos.height << "," 
                    << (pos.isFixed ? "1" : "0") << std::endl;
        }
        
        // Flush to disk
        outFile.flush();
        
        processed = chunkEnd;
        std::cout << "Wrote " << processed << " of " << totalCells << " cell positions..." << std::endl;
    }
    
    outFile.close();
    std::cout << "Placement saved to " << filename << std::endl;
}
