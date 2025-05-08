#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include "cell.h"
#include "net.h"

std::vector<Cell> loadCellsFromFile(std::string filename);
std::vector<Net> loadNetsFromFile(std::string filename);
void printCircuitStats(const std::vector<Cell>& cells, const std::vector<Net>& nets);

#endif // UTILS_H

