#include "algorithm.h"
#include "NodeNet.h"
#include "cell.h"
#include "net.h"
#include "Parser.h"
#include "SinglePassFM.h"
#include "placement.h"
#include "quadratic_placement.h" // Include our new quadratic placer
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <chrono> // For timing comparison

// Function to save FM results to a file
void saveFMCheckpoint(const std::vector<Cell>& cells, const std::vector<Net>& nets, const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open checkpoint file for writing: " << filename << std::endl;
        return;
    }
    
    // Save cell data
    outFile << "CELLS " << cells.size() << std::endl;
    for (const auto& cell : cells) {
        outFile << cell.id << " " << cell.gain << " " << cell.locked << " " 
                << cell.movable << " " << cell.partition << " ";
        
        // Save connected nets
        outFile << cell.connectedNets.size();
        for (int netId : cell.connectedNets) {
            outFile << " " << netId;
        }
        outFile << std::endl;
    }
    
    // Save nets data
    outFile << "NETS " << nets.size() << std::endl;
    for (const auto& net : nets) {
        outFile << net.id << " " << net.name << " " << net.cells.size();
        
        // Save connected cell pointers (by ID only, not the actual pointers)
        for (Cell* cell : net.cells) {
            outFile << " " << (cell ? cell->id : -1);
        }
        outFile << std::endl;
    }
    
    outFile.close();
    std::cout << "FM checkpoint saved to " << filename << std::endl;
}

// Function to load FM results from a file
bool loadFMCheckpoint(std::vector<Cell>& cells, std::vector<Net>& nets, const std::string& filename) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open checkpoint file for reading: " << filename << std::endl;
        return false;
    }
    
    cells.clear();
    nets.clear();
    
    std::string marker;
    int count;
    
    // Load cells
    inFile >> marker >> count;
    if (marker != "CELLS") {
        std::cerr << "Error: Invalid checkpoint file format" << std::endl;
        return false;
    }
    
    // Reserve space for better performance, but don't use resize()
    cells.reserve(count);
    
    for (int i = 0; i < count; i++) {
        int id, netCount;
        bool locked, movable;
        int partition, gain;
        
        inFile >> id >> gain >> locked >> movable >> partition >> netCount;
        
        // Create a new cell with the correct ID
        Cell cell(id);
        cell.gain = gain;
        cell.locked = locked;
        cell.movable = movable;
        cell.partition = partition;
        
        // Read the connected nets
        cell.connectedNets.clear();
        for (int j = 0; j < netCount; j++) {
            int netId;
            inFile >> netId;
            cell.connectedNets.push_back(netId);
        }
        
        // Add the cell to the vector
        cells.push_back(cell);
    }
    
    // Load nets
    inFile >> marker >> count;
    if (marker != "NETS") {
        std::cerr << "Error: Invalid checkpoint file format" << std::endl;
        return false;
    }
    
    // Reserve space for nets
    nets.reserve(count);
    
    // Create a map to find cells by ID quickly
    std::unordered_map<int, Cell*> cellMap;
    for (auto& cell : cells) {
        cellMap[cell.id] = &cell;
    }
    
    for (int i = 0; i < count; i++) {
        int id, cellCount;
        std::string netName;
        inFile >> id >> netName >> cellCount;
        
        // Create a new net with the correct ID
        Net net(id);
        net.name = netName;
        
        // Read the cell connections
        for (int j = 0; j < cellCount; j++) {
            int cellId;
            inFile >> cellId;
            
            if (cellId >= 0) {
                auto it = cellMap.find(cellId);
                if (it != cellMap.end()) {
                    net.addCell(it->second);
                }
            }
        }
        
        // Add the net to the vector
        nets.push_back(net);
    }
    
    inFile.close();
    std::cout << "FM checkpoint loaded from " << filename << std::endl;
    return true;
}

int main() {
    // Seed random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    // 1) Create NetList
    NetList netList;

    // 2) Create a map nodeName -> vector<Net*> for building adjacency
    std::unordered_map<std::string, std::vector<Net*>> nodeToNetsMap;

    // 3) Parse the .nets file - use a smaller benchmark for testing
    std::string netsFile = "superblue18.nets";
    std::string nodesFile = "superblue18.nodes";
    
    bool netsRead = parseNetsFile(netsFile, netList, nodeToNetsMap);
    std::cout << "Nets read: " << (netsRead ? "Yes" : "No") << std::endl;
    std::cout << "Net count: " << netList.count() << std::endl;

    // 4) Parse the .nodes file
    bool nodesRead = parseNodesFile(nodesFile, nodeToNetsMap);
    std::cout << "Nodes read: " << (nodesRead ? "Yes" : "No") << std::endl;

    // 5) Initialize node attributes for the FM algorithm
    std::cout << "\n--- Initializing FM Algorithm Data ---\n";
    totalArea = 0;
    
    for (auto& nodePair : nodeToNetsMap) {
        const std::string& nodeName = nodePair.first;
        
        // Initialize if not already set
        if (nodePart.find(nodeName) == nodePart.end()) {
            nodePart[nodeName] = rand() % 2;
        }
        
        nodeLocked[nodeName] = false;
        nodeGain[nodeName] = 0;
        
        // Get node area if possible, otherwise use default
        if (nodeArea.find(nodeName) == nodeArea.end()) {
            // Try to find the first node in the netlist to get its dimensions
            for (Net* net : nodePair.second) {
                Node* node = net->head;
                while (node) {
                    if (node->name == nodeName) {
                        nodeArea[nodeName] = node->width * node->height;
                        break;
                    }
                    node = node->next;
                }
                if (nodeArea.find(nodeName) != nodeArea.end()) break;
            }
            
            // If still not found, use default
            if (nodeArea.find(nodeName) == nodeArea.end()) {
                nodeArea[nodeName] = 1;
            }
        }
        
        totalArea += nodeArea[nodeName];
    }
    std::cout << "Total area: " << totalArea << std::endl;

    // 6) FM Partitioning - setup data structures
    std::cout << "\n--- Setting up FM Partitioner ---\n";
    
    // Create FM data structures (limit size as needed)
    const size_t MAX_CELLS = 1300000; // Adjust as needed
    std::vector<Cell> cells;
    std::vector<Net> fmNets;
    cells.reserve(MAX_CELLS);
    fmNets.reserve(MAX_CELLS);
    
    // Set up checkpoint file and flag
    const std::string checkpointFile = "fm_checkpoint18.dat";
    bool useCheckpoint = true; // Set to true to use checkpoint, false to rerun FM

    // Check if checkpoint file exists
    std::ifstream checkpointTest(checkpointFile);
    bool checkpointExists = checkpointTest.good();
    checkpointTest.close();
    
    if (useCheckpoint && checkpointExists) {
        // Load from checkpoint
        std::cout << "Loading FM results from checkpoint..." << std::endl;
        if (!loadFMCheckpoint(cells, fmNets, checkpointFile)) {
            std::cout << "Failed to load checkpoint, will need to run FM partitioning..." << std::endl;
            useCheckpoint = false;
        } else {
            std::cout << "FM partitioning results loaded successfully." << std::endl;
            std::cout << "Loaded " << cells.size() << " cells and " << fmNets.size() << " nets." << std::endl;
        }
    } else {
        useCheckpoint = false;
    }

    // Only run FM partitioning if not using checkpoint
    if (!useCheckpoint) {
        std::cout << "Creating node type map..." << std::endl;
        // Create a map to store node types
        std::unordered_map<std::string, std::string> nodeTypeMap;
        
        // Populate the node type map
        for (const auto& nodePair : nodeToNetsMap) {
            const std::string& nodeName = nodePair.first;
            
            // Try to find the node in the networks to get its type
            for (Net* net : nodePair.second) {
                Node* node = net->head;
                while (node) {
                    if (node->name == nodeName) {
                        nodeTypeMap[nodeName] = node->movetype;
                        break;
                    }
                    node = node->next;
                }
                if (nodeTypeMap.find(nodeName) != nodeTypeMap.end()) break;
            }
            
            // Default type if not found
            if (nodeTypeMap.find(nodeName) == nodeTypeMap.end()) {
                nodeTypeMap[nodeName] = "movable";
            }
        }
        
        std::cout << "Creating node to cell mapping..." << std::endl;
        
        // Create mapping from node names to cells with terminal information
        size_t cellCount = 0;
        std::unordered_map<std::string, Cell*> nodeNameToCell;
        
        for (const auto& nodePair : nodeToNetsMap) {
            if (cellCount >= MAX_CELLS) break;
            
            const std::string& nodeName = nodePair.first;
            
            // Create a new cell
            Cell cell(cellCount);
            
            // Set movable status based on node type
            auto typeIt = nodeTypeMap.find(nodeName);
            if (typeIt != nodeTypeMap.end()) {
                if (typeIt->second == "terminal" || typeIt->second == "terminal_NI") {
                    cell.movable = false;
                    cell.locked = true;  // Also mark as locked
                }
            }
            
            // Set partition from existing info or terminal status
            auto partIt = nodePart.find(nodeName);
            if (partIt != nodePart.end()) {
                cell.partition = partIt->second ? 1 : 0;
            } else {
                if (!cell.movable) {
                    // For terminals, assign specific partitions
                    cell.partition = 0;  // Default partition for fixed terminals
                } else {
                    cell.partition = rand() % 2; // Random partition for movable cells
                }
                nodePart[nodeName] = (cell.partition == 1);
            }
            
            size_t cellIndex = cells.size();
            cells.push_back(cell);
            nodeNameToCell[nodeName] = &cells[cellIndex];
            cellCount++;
        }
        
        std::cout << "Connecting cells to nets..." << std::endl;
        
        // Connect cells to nets (only for cells we've created)
        size_t netCount = 0;
        std::unordered_map<std::string, Net*> netNameToFMNet;
        
        for (const auto& nodePair : nodeToNetsMap) {
            const std::string& nodeName = nodePair.first;
            
            // Skip if this node doesn't have a cell
            auto cellIter = nodeNameToCell.find(nodeName);
            if (cellIter == nodeNameToCell.end()) continue;
            
            Cell* cell = cellIter->second;
            
            // Process each net connected to this node
            for (Net* parsedNet : nodePair.second) {
                // Skip if we've processed too many nets
                if (netCount >= MAX_CELLS) continue;
                
                // Check if we've already created an FM net for this
                if (netNameToFMNet.find(parsedNet->name) == netNameToFMNet.end()) {
                    // Create new FM net
                    fmNets.push_back(Net(fmNets.size()));
                    fmNets.back().name = parsedNet->name;
                    netNameToFMNet[parsedNet->name] = &fmNets.back();
                    netCount++;
                }
                
                Net* fmNet = netNameToFMNet[parsedNet->name];
                
                // Connect cell to net
                cell->connectedNets.push_back(fmNet->id);
                fmNet->addCell(cell);
            }
        }
        
        std::cout << "FM Model created: " << cells.size() << " cells, " 
                  << fmNets.size() << " nets." << std::endl;
        
        // Create and run the FM partitioner
        FM_Partitioner fmPartitioner(cells, fmNets);
        
        try {
            // Run partitioning
            std::cout << "Running FM partitioning algorithm..." << std::endl;
            fmPartitioner.runPartitioning();
            
            // Print results
            fmPartitioner.printResults();
            
            // Update the original node partitions
            std::cout << "Updating original node partitions..." << std::endl;
            for (const auto& nodeCellPair : nodeNameToCell) {
                const std::string& nodeName = nodeCellPair.first;
                const Cell* cell = nodeCellPair.second;
                
                // Update the node partition in the original data structure
                nodePart[nodeName] = (cell->partition == 1);
            }
            
            // Save checkpoint
            std::cout << "Saving FM partitioning checkpoint..." << std::endl;
            saveFMCheckpoint(cells, fmNets, checkpointFile);
        } catch (const std::exception& e) {
            std::cerr << "Exception during partitioning: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception during partitioning" << std::endl;
        }
    }

    // 7) Setup for placement comparison
    std::cout << "\n--- Setting up for placement algorithm comparison ---\n";
    
    // Identify fixed cells
    std::cout << "Identifying fixed cells..." << std::endl;
    std::vector<int> fixedCellIds;
    const size_t MAX_FIXED_CELLS = 200000;
    size_t fixedCellsFound = 0;

    for (size_t i = 0; i < cells.size(); i++) {
        if (!cells[i].movable) {
            fixedCellIds.push_back(cells[i].id);
            fixedCellsFound++;
            
            if (fixedCellsFound >= MAX_FIXED_CELLS) {
                std::cout << "Limiting to " << MAX_FIXED_CELLS << " fixed cells for memory reasons." << std::endl;
                break;
            }
        }
    }
    
    std::cout << "Processing " << fixedCellIds.size() << " fixed cells out of " 
              << fixedCellsFound << " total fixed cells identified." << std::endl;
              
    // Setup fixed cell positions around the periphery
    double perimeterPos = 0.0;
    double perimeterStep = 1.0 / (fixedCellIds.size() > 0 ? fixedCellIds.size() : 1);
    std::vector<std::tuple<int, double, double, double, double>> fixedCellPositions;
    
    for (size_t i = 0; i < fixedCellIds.size(); i++) {
        double x, y;
        
        // Place fixed cells around the periphery
        if (perimeterPos < 0.25) {
            // Bottom edge
            x = perimeterPos * 4.0;
            y = 0.0;
        } else if (perimeterPos < 0.5) {
            // Right edge
            x = 1.0;
            y = (perimeterPos - 0.25) * 4.0;
        } else if (perimeterPos < 0.75) {
            // Top edge
            x = 1.0 - (perimeterPos - 0.5) * 4.0;
            y = 1.0;
        } else {
            // Left edge
            x = 0.0;
            y = 1.0 - (perimeterPos - 0.75) * 4.0;
        }
        
        // Use a standard size for fixed cells
        double width = 0.03;
        double height = 0.03;
        
        // Store position for later use
        fixedCellPositions.emplace_back(fixedCellIds[i], x, y, width, height);
        
        perimeterPos += perimeterStep;
    }

    // 8) Run the recursive bipartitioning placer
    std::cout << "\n--- Running Recursive Bipartitioning Placement Algorithm ---\n";
    double bipartWL = 0.0;
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Create a copy of cells and nets for the bipartitioning placer
        std::vector<Cell> bipartCells = cells;
        std::vector<Net> bipartNets = fmNets;
        
        // Create placement algorithm object
        Placement bipartPlacer(bipartCells, bipartNets);
        
        // Add fixed cells with their positions
        for (const auto& [cellId, x, y, width, height] : fixedCellPositions) {
            bipartPlacer.addFixedCell(cellId, x, y, width, height);
        }
        
        // Run the placement algorithm
        bipartPlacer.runPlacement();
        
        // Calculate runtime
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Output the results
        bipartPlacer.outputPlacement("bipartitioning_placement_result.csv");
        
        std::cout << "Recursive bipartitioning placement completed in " 
                  << duration.count() / 1000.0 << " seconds." << std::endl;
        
        bipartWL = bipartPlacer.calculateWirelength();
        std::cout << "Bipartitioning placement wirelength: " << bipartWL << std::endl;
    }
    
    // 9) Run the quadratic placer
    std::cout << "\n--- Running Quadratic Placement Algorithm ---\n";
    double quadWL = 0.0;
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Create a copy of cells and nets for the quadratic placer
        std::vector<Cell> quadCells = cells;
        std::vector<Net> quadNets = fmNets;
        
        // Create quadratic placement algorithm object
        QuadraticPlacer quadPlacer(quadCells, quadNets);
        
        // Add fixed cells with their positions
        for (const auto& [cellId, x, y, width, height] : fixedCellPositions) {
            quadPlacer.addFixedCell(cellId, x, y, width, height);
        }
        
        // Run the quadratic placement algorithm
        quadPlacer.runPlacement();
        
        // Calculate runtime
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Output the results
        quadPlacer.outputPlacement("quadratic_placement_result18.csv");
        
        std::cout << "Quadratic placement completed in " 
                  << duration.count() / 1000.0 << " seconds." << std::endl;
        
        quadWL = quadPlacer.calculateWirelength();
        std::cout << "Quadratic placement wirelength: " << quadWL << std::endl;
    }
    
    // 10) Compare results
    std::cout << "\n--- Placement Comparison Results ---\n";
    std::cout << "Bipartitioning Wirelength: " << bipartWL << std::endl;
    std::cout << "Quadratic Wirelength: " << quadWL << std::endl;
    double improvement = ((bipartWL - quadWL) / bipartWL) * 100.0;
    std::cout << "Improvement with Quadratic Placement: " 
              << (improvement > 0 ? "+" : "") << improvement << "%" << std::endl;
    
    return 0;
}