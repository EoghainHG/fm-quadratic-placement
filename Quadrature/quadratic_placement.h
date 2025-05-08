#ifndef QUADRATIC_PLACEMENT_H
#define QUADRATIC_PLACEMENT_H

#include "cell.h"
#include "net.h"
#include "algorithm.h"
#include "placement_common.h"  // Include the common header
#include <vector>
#include <unordered_map>
#include <set>
#include <Eigen/Sparse>
#include <Eigen/Dense>

class QuadraticPlacer {
public:
    QuadraticPlacer(std::vector<Cell>& cells, std::vector<Net>& nets);
    
    void addFixedCell(int cellId, double x, double y, double width = 0.03, double height = 0.03);
    void runPlacement();
    double calculateWirelength() const;
    void outputPlacement(const std::string& filename);
    
private:
    // References to input data
    std::vector<Cell>& cells;
    std::vector<Net>& nets;
    
    std::vector<Eigen::Triplet<double>> A_triplets; // Triplets for sparse matrix
    std::vector<double> diagonal;                  // Diagonal elements of matrix A
    
    // Cell positions
    std::vector<CellPosition> cellPositions;
    std::unordered_map<int, CellPosition> fixedCells;
    
    // Quadratic placement specific data
    std::unordered_map<int, int> cellIdToMatrixIndex;
    std::unordered_map<int, int> matrixIndexToCellId;
    int movableCellCount;
    
    // Matrices for quadratic placement
    Eigen::SparseMatrix<double> A_x;
    Eigen::SparseMatrix<double> A_y;
    Eigen::VectorXd b_x;
    Eigen::VectorXd b_y;
    
    // Solution vectors
    Eigen::VectorXd cell_x;
    Eigen::VectorXd cell_y;
    
    // Methods
    void preprocessFixedCells();
    void createCellMappings();
    void buildConnectivityMatrices();
    void buildCliqueModel(const Net& net, 
                        std::vector<Eigen::Triplet<double>>& A_triplets,
                        Eigen::VectorXd& b_x,
                        Eigen::VectorXd& b_y);
    void buildStarModel(const Net& net, 
                       std::vector<Eigen::Triplet<double>>& A_triplets,
                       Eigen::VectorXd& b_x,
                       Eigen::VectorXd& b_y);
    void makeMatrixDiagonalDominant(Eigen::SparseMatrix<double>& A);
    void solveQuadraticPlacement();
    void spreadCells();
    void legalizePlacement();
    std::pair<double, double> getCellDimensions(int cellId) const;
};

#endif // QUADRATIC_PLACEMENT_H