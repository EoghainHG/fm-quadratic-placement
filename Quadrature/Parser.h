#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <unordered_map>
#include <vector>

class NetList;
class Net;

// Parse the .nets file into a NetList, building a mapping nodeName -> vector of Net*
bool parseNetsFile(const std::string &filename,
                   NetList &netList,
                   std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap);

// Parse the .nodes file, adding Node objects to the Net objects
bool parseNodesFile(const std::string &filename,
                    const std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap);

#endif // PARSER_H