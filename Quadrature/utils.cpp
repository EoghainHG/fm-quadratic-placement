#include "utils.h"
#include <fstream>
#include <iostream>

std::vector<Cell> loadCellsFromFile(std::string filename) {
    std::vector<Cell> cells;
    std::ifstream file(filename);
    int id;
    while (file >> id) {
        cells.push_back(Cell(id));
    }
    return cells;
}

std::vector<Net> loadNetsFromFile(std::string filename) {
    std::vector<Net> nets;
    std::ifstream file(filename);
    int id;
    while (file >> id) {
        nets.push_back(Net(id));
    }
    return nets;
}