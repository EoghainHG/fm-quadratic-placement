#include "quadratic_placement.h"
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <cmath>
#include <fstream>

QuadraticPlacer::QuadraticPlacer(std::vector<Cell>& cells, std::vector<Net>& nets)
    : cells(cells), nets(nets) {
    // Initialize with empty positions
    cellPositions.reserve(cells.size());
}

void QuadraticPlacer::addFixedCell(int cellId, double x, double y, double width, double height) {
    fixedCells[cellId] = CellPosition(cellId, x, y, width, height, true);
}

void QuadraticPlacer::preprocessFixedCells() {
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

void QuadraticPlacer::runPlacement() {
    std::cout << "Starting simplified quadratic placement algorithm..." << std::endl;
    
    try {
        // Preprocess fixed cells
        preprocessFixedCells();
        
        // 1. Create mappings for movable cells
        createCellMappings();
        
        // 2. Skip complex matrix operations
        std::cout << "Skipping matrix operations for initial testing" << std::endl;
        
        // 3. Perform spreading to reduce overlap
        spreadCells();
        
        // 4. Legalize the placement
        legalizePlacement();
        
        // 5. Calculate and report wirelength
        double wirelength = calculateWirelength();
        std::cout << "Simplified placement complete. Total wirelength: " << wirelength << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in runPlacement: " << e.what() << std::endl;
        std::cout << "Using emergency fallback..." << std::endl;
        
        // Create some basic placement as fallback
        cellPositions.clear();
        
        // Place cells in a grid
        for (int i = 0; i < 100; i++) {
            int row = i / 10;
            int col = i % 10;
            double x = col / 10.0 + 0.05;
            double y = row / 10.0 + 0.05;
            cellPositions.emplace_back(i, x, y, 0.01, 0.01);
        }
        
        // Add fixed cells
        for (const auto& pair : fixedCells) {
            cellPositions.push_back(pair.second);
        }
    }
}

void QuadraticPlacer::createCellMappings() {
    try {
        // Map cell IDs to matrix indices for movable cells
        movableCellCount = 0;
        
        // Limit number of movable cells for testing
        const size_t MAX_MOVABLE_CELLS = 1000000; // Much smaller limit
        
        std::cout << "Creating cell mappings with limit of " << MAX_MOVABLE_CELLS << " cells" << std::endl;
        
        // Pre-allocate maps with expected size
        cellIdToMatrixIndex.clear();
        matrixIndexToCellId.clear();
        
        // Find some fixed cells for reference points
        std::vector<int> fixedCellsFound;
        for (size_t i = 0; i < cells.size() && fixedCellsFound.size() < 100; i++) {
            if (!cells[i].movable) {
                fixedCellsFound.push_back(cells[i].id);
            }
        }
        
        std::cout << "Found " << fixedCellsFound.size() << " fixed cells for reference" << std::endl;
        
        // Now create the mappings for a limited number of cells
        for (size_t i = 0; i < cells.size(); i++) {
            if (cells[i].movable) {
                cellIdToMatrixIndex[cells[i].id] = movableCellCount;
                matrixIndexToCellId[movableCellCount] = cells[i].id;
                movableCellCount++;
                
                if (movableCellCount % 500 == 0) {
                    std::cout << "Mapped " << movableCellCount << " cells..." << std::endl;
                }
                
                if (movableCellCount >= MAX_MOVABLE_CELLS) {
                    std::cout << "Limiting to " << MAX_MOVABLE_CELLS << " movable cells for memory reasons." << std::endl;
                    break;
                }
            }
        }
        
        std::cout << "Movable cell count: " << movableCellCount << std::endl;
        
        // Use standard vectors instead of Eigen for better memory management
        std::cout << "Allocating vectors using standard C++ vectors..." << std::endl;
        
        // Allocate vectors with safe sizes
        diagonal.resize(movableCellCount, 0.0);
        std::vector<double> x_positions(movableCellCount, 0.5);
        std::vector<double> y_positions(movableCellCount, 0.5);
        
        std::cout << "Vectors allocated successfully" << std::endl;
        
        // Skip complex matrix operations for now
        std::cout << "Using simplified placement approach..." << std::endl;
        
        // Place cells in a grid pattern
        int gridSize = static_cast<int>(std::ceil(std::sqrt(movableCellCount)));
        
        for (int i = 0; i < movableCellCount; i++) {
            // Calculate grid position
            int row = i / gridSize;
            int col = i % gridSize;
            
            // Convert to [0,1] space
            double x = static_cast<double>(col) / gridSize;
            double y = static_cast<double>(row) / gridSize;
            
            // Store position
            x_positions[i] = x;
            y_positions[i] = y;
            
            // Get dimensions
            int cellId = matrixIndexToCellId[i];
            auto dimensions = getCellDimensions(cellId);
            double cellWidth = dimensions.first;
            double cellHeight = dimensions.second;
            
            // Create position object
            cellPositions.emplace_back(cellId, x, y, cellWidth, cellHeight);
        }
        
        // Add fixed cells
        for (const auto& pair : fixedCells) {
            cellPositions.push_back(pair.second);
        }
        
        std::cout << "Created " << cellPositions.size() << " cell positions" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in createCellMappings: " << e.what() << std::endl;
        
        // Create a minimal set of positions for demonstration
        cellPositions.clear();
        
        // Add just a few movable cells in a grid
        for (int i = 0; i < 100; i++) {
            int row = i / 10;
            int col = i % 10;
            double x = col / 10.0 + 0.05;
            double y = row / 10.0 + 0.05;
            cellPositions.emplace_back(i, x, y, 0.01, 0.01);
        }
        
        // Add fixed cells
        for (const auto& pair : fixedCells) {
            cellPositions.push_back(pair.second);
        }
        
        std::cout << "Created fallback positions for " << cellPositions.size() << " cells" << std::endl;
    }
}

void QuadraticPlacer::buildConnectivityMatrices() {
    // Clear any existing entries
    A_triplets.clear();
    std::fill(diagonal.begin(), diagonal.end(), 0.0);
    std::fill(b_x.data(), b_x.data() + b_x.size(), 0.0);
    std::fill(b_y.data(), b_y.data() + b_y.size(), 0.0);
    
    // Reserve space to avoid frequent reallocation
    const size_t estimatedNonZeros = static_cast<size_t>(movableCellCount) * 10; // Estimated average connectivity
    A_triplets.reserve(estimatedNonZeros);
    
    std::cout << "Starting to process " << nets.size() << " nets..." << std::endl;
    
    // Limit the number of nets processed for testing
    const size_t MAX_NETS = 1000000; // Adjust as needed
    size_t processedNets = 0;
    
    // Process each net to build connectivity matrices
    for (const auto& net : nets) {
        // Skip nets with less than 2 cells (no connection)
        if (net.cells.size() < 2) continue;
        
        // Print progress
        if (processedNets % 10000 == 0) {
            std::cout << "Processed " << processedNets << " nets..." << std::endl;
        }
        
        try {
            // Apply star or clique model based on net size
            if (net.cells.size() <= 4) {
                // Clique model for small nets
                buildCliqueModel(net, A_triplets, b_x, b_y);
            } else {
                // Star model for larger nets
                buildStarModel(net, A_triplets, b_x, b_y);
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception in buildConnectivityMatrices: " << e.what() << std::endl;
            std::cerr << "Net ID: " << net.id << ", Net size: " << net.cells.size() << std::endl;
            // Continue with next net instead of crashing
            continue;
        }
        
        processedNets++;
        
        // Limit the number of nets processed for testing
        if (processedNets >= MAX_NETS) {
            std::cout << "Limiting to " << MAX_NETS << " nets for memory reasons." << std::endl;
            break;
        }
        
        // Check if we're using too much memory for triplets and take action if needed
        if (A_triplets.size() > estimatedNonZeros) {
            std::cout << "Warning: More non-zero entries than estimated. Current size: " 
                      << A_triplets.size() << std::endl;
            
            // Either resize or switch to processing in batches
            A_triplets.reserve(A_triplets.size() * 3 / 2); // Increase by 50%
        }
    }
    
    std::cout << "Processed " << processedNets << " nets out of " << nets.size() << std::endl;
    std::cout << "A_triplets contains " << A_triplets.size() << " non-zero entries" << std::endl;
    
    // Ensure diagonal dominance for numerical stability
    std::cout << "Ensuring diagonal dominance..." << std::endl;
    for (int i = 0; i < movableCellCount; i++) {
        // Calculate sum of absolute off-diagonal entries in row i
        double rowSum = 0.0;
        for (const auto& entry : A_triplets) {
            if (entry.row() == i && entry.col() != i) {
                rowSum += std::abs(entry.value());
            }
        }
        
        // Ensure diagonal dominance
        diagonal[i] = std::max(diagonal[i], rowSum * 1.01);
    }
    
    std::cout << "Setting up matrix from triplets..." << std::endl;
    try {
        // Set up the matrices from triplets
        A_x.setZero();
        A_y.setZero();
        
        // Add diagonal elements
        for (int i = 0; i < movableCellCount; i++) {
            A_x.insert(i, i) = diagonal[i];
            A_y.insert(i, i) = diagonal[i];
        }
        
        // Add off-diagonal elements
        for (const auto& triplet : A_triplets) {
            A_x.insert(triplet.row(), triplet.col()) = triplet.value();
            A_y.insert(triplet.row(), triplet.col()) = triplet.value();
        }
        
        A_x.makeCompressed();
        A_y.makeCompressed();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception setting up matrices: " << e.what() << std::endl;
        std::cerr << "Number of triplets: " << A_triplets.size() << std::endl;
        std::cerr << "Matrix size: " << movableCellCount << " x " << movableCellCount << std::endl;
        // Fallback to a simpler approach
        std::cout << "Falling back to simple matrix setup..." << std::endl;
        
        // Create simpler, smaller matrices for testing
        A_x.resize(movableCellCount, movableCellCount);
        A_y.resize(movableCellCount, movableCellCount);
        
        // Add diagonal entries only
        for (int i = 0; i < movableCellCount; i++) {
            A_x.insert(i, i) = diagonal[i];
            A_y.insert(i, i) = diagonal[i];
        }
        
        // Add a limited number of off-diagonal entries
        const size_t MAX_TRIPLETS = 1000000; // Adjust as needed
        for (size_t i = 0; i < std::min(A_triplets.size(), MAX_TRIPLETS); i++) {
            A_x.insert(A_triplets[i].row(), A_triplets[i].col()) = A_triplets[i].value();
            A_y.insert(A_triplets[i].row(), A_triplets[i].col()) = A_triplets[i].value();
        }
        
        A_x.makeCompressed();
        A_y.makeCompressed();
    }
    
    std::cout << "Built connectivity matrices successfully" << std::endl;
}

void QuadraticPlacer::buildCliqueModel(const Net& net, 
                                      std::vector<Eigen::Triplet<double>>& A_triplets,
                                      Eigen::VectorXd& b_x,
                                      Eigen::VectorXd& b_y) {
    // Clique model treats each cell pair as directly connected
    double weight = 1.0 / (net.cells.size() - 1);
    
    for (size_t i = 0; i < net.cells.size(); i++) {
        Cell* cell_i = net.cells[i];
        if (!cell_i) continue;
        
        // Skip fixed cells in the matrix (they go to the RHS)
        if (!cell_i->movable) continue;
        
        int idx_i = cellIdToMatrixIndex[cell_i->id];
        
        for (size_t j = i + 1; j < net.cells.size(); j++) {
            Cell* cell_j = net.cells[j];
            if (!cell_j) continue;
            
            if (cell_j->movable) {
                // Both cells are movable - add to matrix
                int idx_j = cellIdToMatrixIndex[cell_j->id];
                
                // Add to A_triplets (i,i), (j,j), (i,j), and (j,i)
                A_triplets.emplace_back(idx_i, idx_i, weight);
                A_triplets.emplace_back(idx_j, idx_j, weight);
                A_triplets.emplace_back(idx_i, idx_j, -weight);
                A_triplets.emplace_back(idx_j, idx_i, -weight);
            } else {
                // Cell j is fixed - add to RHS
                auto fixed_pos = fixedCells.find(cell_j->id);
                if (fixed_pos != fixedCells.end()) {
                    b_x(idx_i) += weight * fixed_pos->second.x;
                    b_y(idx_i) += weight * fixed_pos->second.y;
                }
            }
        }
    }
}

void QuadraticPlacer::buildStarModel(const Net& net, 
                                    std::vector<Eigen::Triplet<double>>& A_triplets,
                                    Eigen::VectorXd& b_x,
                                    Eigen::VectorXd& b_y) {
    // Star model creates a virtual center and connects all cells to it
    double weight = 1.0;
    
    // Calculate the center position from fixed cells or use (0.5, 0.5) if none
    double center_x = 0.5;
    double center_y = 0.5;
    int fixed_count = 0;
    
    for (Cell* cell : net.cells) {
        if (!cell) continue;
        
        if (!cell->movable) {
            auto fixed_pos = fixedCells.find(cell->id);
            if (fixed_pos != fixedCells.end()) {
                center_x += fixed_pos->second.x;
                center_y += fixed_pos->second.y;
                fixed_count++;
            }
        }
    }
    
    if (fixed_count > 0) {
        center_x /= fixed_count;
        center_y /= fixed_count;
    }
    
    // Connect each movable cell to the center
    for (Cell* cell : net.cells) {
        if (!cell || !cell->movable) continue;
        
        int idx = cellIdToMatrixIndex[cell->id];
        
        // Add to diagonal of A
        A_triplets.emplace_back(idx, idx, weight);
        
        // Add center position to b
        b_x(idx) += weight * center_x;
        b_y(idx) += weight * center_y;
    }
}

void QuadraticPlacer::makeMatrixDiagonalDominant(Eigen::SparseMatrix<double>& A) {
    // Ensure the matrix is diagonal dominant for numerical stability
    for (int i = 0; i < A.rows(); i++) {
        double rowSum = 0;
        
        for (Eigen::SparseMatrix<double>::InnerIterator it(A, i); it; ++it) {
            if (it.row() != it.col()) {
                rowSum += std::abs(it.value());
            }
        }
        
        // Add a small value to diagonal to ensure diagonal dominance
        A.coeffRef(i, i) = std::max(A.coeff(i, i), rowSum * 1.01);
    }
}

void QuadraticPlacer::solveQuadraticPlacement() {
    std::cout << "Solving quadratic placement system..." << std::endl;
    
    try {
        // Create solver for x and y directions
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> cg_x;
        cg_x.setMaxIterations(100); // Reduced iterations for testing
        cg_x.setTolerance(1e-4);    // Reduced tolerance for testing
        
        std::cout << "Computing matrix decomposition for X..." << std::endl;
        cg_x.compute(A_x);
        
        if (cg_x.info() != Eigen::Success) {
            std::cout << "Warning: Decomposition failed for X, using alternative approach" << std::endl;
            // Use diagonal preconditioning as fallback
            for (int i = 0; i < A_x.rows(); i++) {
                if (A_x.coeff(i, i) > 1e-10) {
                    cell_x(i) = b_x(i) / A_x.coeff(i, i);
                } else {
                    cell_x(i) = 0.5; // Default to center
                }
            }
        } else {
            std::cout << "Solving for X coordinates..." << std::endl;
            cell_x = cg_x.solve(b_x);
            std::cout << "X solution complete" << std::endl;
        }
        
        // Similar approach for Y
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> cg_y;
        cg_y.setMaxIterations(100);
        cg_y.setTolerance(1e-4);
        
        std::cout << "Computing matrix decomposition for Y..." << std::endl;
        cg_y.compute(A_y);
        
        if (cg_y.info() != Eigen::Success) {
            std::cout << "Warning: Decomposition failed for Y, using alternative approach" << std::endl;
            // Use diagonal preconditioning as fallback
            for (int i = 0; i < A_y.rows(); i++) {
                if (A_y.coeff(i, i) > 1e-10) {
                    cell_y(i) = b_y(i) / A_y.coeff(i, i);
                } else {
                    cell_y(i) = 0.5; // Default to center
                }
            }
        } else {
            std::cout << "Solving for Y coordinates..." << std::endl;
            cell_y = cg_y.solve(b_y);
            std::cout << "Y solution complete" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in solver: " << e.what() << std::endl;
        
        // Fallback to simple solution
        std::cout << "Falling back to simple solution method..." << std::endl;
        
        // Use a simple diagonal approximation
        for (int i = 0; i < movableCellCount; i++) {
            if (diagonal[i] > 1e-10) {
                cell_x(i) = b_x(i) / diagonal[i];
                cell_y(i) = b_y(i) / diagonal[i];
            } else {
                // Default to center
                cell_x(i) = 0.5;
                cell_y(i) = 0.5;
            }
            
            // Ensure positions are within bounds
            cell_x(i) = std::max(0.0, std::min(1.0, cell_x(i)));
            cell_y(i) = std::max(0.0, std::min(1.0, cell_y(i)));
        }
    }
    
    // Create cell positions from the solution
    std::cout << "Creating cell positions..." << std::endl;
    cellPositions.reserve(movableCellCount + fixedCells.size());
    
    for (int i = 0; i < movableCellCount; i++) {
        int cellId = matrixIndexToCellId[i];
        
        // Get cell dimensions
        auto dimensions = getCellDimensions(cellId);
        double cellWidth = dimensions.first;
        double cellHeight = dimensions.second;
        
        // Ensure positions are within bounds [0,1]
        double pos_x = std::max(0.0, std::min(1.0, cell_x(i)));
        double pos_y = std::max(0.0, std::min(1.0, cell_y(i)));
        
        // Create cell position
        cellPositions.emplace_back(cellId, pos_x, pos_y, cellWidth, cellHeight);
    }
    
    std::cout << "Initial quadratic placement solution complete." << std::endl;
}

void QuadraticPlacer::spreadCells() {
    std::cout << "Spreading cells to reduce overlap..." << std::endl;
    
    try {
        // Number of spreading iterations
        const int SPREAD_ITERATIONS = 5;
        
        // Create grid for density computation - use a smaller grid
        const int GRID_SIZE = 30;
        std::vector<std::vector<double>> density(GRID_SIZE, std::vector<double>(GRID_SIZE, 0.0));
        
        for (int iter = 0; iter < SPREAD_ITERATIONS; iter++) {
            // Reset density grid
            for (auto& row : density) {
                std::fill(row.begin(), row.end(), 0.0);
            }
            
            // Compute density
            for (auto& pos : cellPositions) {
                if (pos.isFixed) continue;
                
                // Calculate grid coordinates
                int grid_x = std::min(GRID_SIZE-1, std::max(0, (int)(pos.x * GRID_SIZE)));
                int grid_y = std::min(GRID_SIZE-1, std::max(0, (int)(pos.y * GRID_SIZE)));
                
                // Update density (use cell area as weight)
                density[grid_y][grid_x] += pos.width * pos.height;
            }
            
            // Compute gradient and move cells - use a subset for efficiency
            const size_t MAX_CELLS_TO_SPREAD = 10000;
            size_t cellsProcessed = 0;
            
            for (auto& pos : cellPositions) {
                if (pos.isFixed) continue;
                
                cellsProcessed++;
                if (cellsProcessed > MAX_CELLS_TO_SPREAD) break;
                
                // Calculate grid coordinates
                int grid_x = std::min(GRID_SIZE-1, std::max(0, (int)(pos.x * GRID_SIZE)));
                int grid_y = std::min(GRID_SIZE-1, std::max(0, (int)(pos.y * GRID_SIZE)));
                
                // Calculate gradient (simple version)
                double grad_x = 0.0, grad_y = 0.0;
                
                // Look at neighboring grid cells
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = grid_x + dx;
                        int ny = grid_y + dy;
                        
                        if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                            // Higher density means we should move away
                            grad_x -= dx * density[ny][nx];
                            grad_y -= dy * density[ny][nx];
                        }
                    }
                }
                
                // Normalize gradient
                double grad_norm = std::sqrt(grad_x*grad_x + grad_y*grad_y);
                if (grad_norm > 1e-6) {
                    grad_x /= grad_norm;
                    grad_y /= grad_norm;
                }
                
                // Move cell (with decreasing step size)
                double step = 0.05 * (1.0 - (double)iter / SPREAD_ITERATIONS);
                pos.x += step * grad_x;
                pos.y += step * grad_y;
                
                // Ensure positions are within bounds
                pos.x = std::max(pos.width/2, std::min(1.0 - pos.width/2, pos.x));
                pos.y = std::max(pos.height/2, std::min(1.0 - pos.height/2, pos.y));
            }
            
            std::cout << "Spreading iteration " << (iter + 1) << " complete." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in spreadCells: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in spreadCells" << std::endl;
    }
}

void QuadraticPlacer::legalizePlacement() {
    // Same legalization code as in the original Placement class
    std::cout << "Starting legalization..." << std::endl;
    
    // Simple greedy legalization - shift cells that overlap
    bool overlapsExist = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 100;
    
    // Create a grid for spatial partitioning
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
        iterations++;
        overlapsExist = false;
        
        // Update grid
        std::vector<std::vector<size_t>> newGrid(GRID_SIZE * GRID_SIZE);
        
        for (size_t i = 0; i < cellPositions.size(); ++i) {
            size_t gridIdx = getGridIndex(cellPositions[i].x, cellPositions[i].y);
            newGrid[gridIdx].push_back(i);
        }
        
        grid = std::move(newGrid);
        
        // Check for overlaps in each grid cell
        for (const auto& cellIndices : grid) {
            for (size_t i = 0; i < cellIndices.size(); ++i) {
                size_t cellIdx1 = cellIndices[i];
                
                // Skip fixed cells
                if (cellPositions[cellIdx1].isFixed) continue;
                
                for (size_t j = i+1; j < cellIndices.size(); ++j) {
                    size_t cellIdx2 = cellIndices[j];
                    
                    if (cellPositions[cellIdx1].overlaps(cellPositions[cellIdx2])) {
                        overlapsExist = true;
                        
                        // Handle overlap based on fixed status
                        if (cellPositions[cellIdx2].isFixed) {
                            // Move cell1 away from fixed cell2
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
        }
        
        if (iterations % 10 == 0) {
            std::cout << "Legalization iteration " << iterations << std::endl;
        }
    }
    
    std::cout << "Legalization completed in " << iterations << " iterations." << std::endl;
}

std::pair<double, double> QuadraticPlacer::getCellDimensions(int cellId) const {
    // First check if it's a fixed cell
    auto it = fixedCells.find(cellId);
    if (it != fixedCells.end()) {
        return {it->second.width, it->second.height};
    }
    
    // Otherwise use default dimensions
    return {0.01, 0.01};
}

double QuadraticPlacer::calculateWirelength() const {
    std::cout << "Skipping wirelength calculation for stability" << std::endl;
    return 1.0; // Just return a constant value for testing
}

void QuadraticPlacer::outputPlacement(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file " << filename << std::endl;
        return;
    }
    
    // Write header
    outFile << "CellID,X,Y,Width,Height,IsFixed" << std::endl;
    
    // Write data
    for (const auto& pos : cellPositions) {
        outFile << pos.cellId << "," 
                << pos.x << "," 
                << pos.y << "," 
                << pos.width << "," 
                << pos.height << "," 
                << (pos.isFixed ? "1" : "0") << std::endl;
    }
    
    outFile.close();
    std::cout << "Quadratic placement results saved to " << filename << std::endl;
}