#include "Parser.h"
#include "NodeNet.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// parseNetsFile implementation
bool parseNetsFile(const std::string &filename,
                   NetList &netList,
                   std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open nets file: " << filename << std::endl;
        return false;
    }

    std::cout << "Reading nets file: " << filename << std::endl;

    std::string line;
    int lineCount = 0;
    int validNetCount = 0;

    while (std::getline(file, line)) {
        lineCount++;
        if (line.empty() || line[0] == '#') continue; // skip empty/comment

        std::istringstream iss(line);
        std::string token;
        if (iss >> token && token == "NetDegree") {
            int netDegree;
            std::string netName;
            if (!(iss >> token >> netDegree >> netName)) {
                std::cerr << "Warning: Malformed NetDegree line " << lineCount << std::endl;
                continue;
            }
            // add a new net
            Net* newNet = netList.addNet(netName);
            validNetCount++;

            // read netDegree lines for node names
            for (int i = 0; i < netDegree; i++) {
                if (!std::getline(file, line)) {
                    std::cerr << "Warning: Unexpected EOF while reading net " << netName << std::endl;
                    break;
                }
                lineCount++;
                if (line.empty() || line[0] == '#') {
                    i--;
                    continue;
                }
                std::istringstream nodeIss(line);
                std::string nodeName;
                if (nodeIss >> nodeName) {
                    // record that nodeName belongs to this net
                    nodeToNetsMap[nodeName].push_back(newNet);
                } else {
                    std::cerr << "Warning: Malformed node line for net " << netName << std::endl;
                }
            }
        }
    }
    file.close();

    std::cout << "Nets file done. " << lineCount << " lines, found "
              << validNetCount << " valid nets.\n";
    std::cout << "Node-to-nets map has " << nodeToNetsMap.size() << " unique nodes.\n";

    return (validNetCount > 0);
}

// parseNodesFile implementation
bool parseNodesFile(const std::string &filename,
                    const std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open nodes file: " << filename << std::endl;
        return false;
    }

    std::cout << "Reading nodes file: " << filename << std::endl;

    std::string line;
    int lineCount = 0, validLineCount = 0, nodesAddedToNets = 0;

    while (std::getline(file, line)) {
        lineCount++;
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string nodeName;
        int width, height;
        std::string movetype = "movable";

        if (!(iss >> nodeName >> width >> height)) {
            std::cerr << "Warning: Malformed line " << lineCount << " in nodes file\n";
            continue;
        }
        validLineCount++;

        // check leftover for movetype
        {
            std::string leftover;
            if (std::getline(iss, leftover)) {
                size_t first = leftover.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    movetype = leftover.substr(first);
                }
            }
        }

        // see if nodeName belongs to any nets
        auto it = nodeToNetsMap.find(nodeName);
        if (it != nodeToNetsMap.end()) {
            for (Net* netPtr : it->second) {
                // create a Node* and add to netPtr
                Node* newNode = new Node(nodeName, width, height, movetype);
                netPtr->addNode(newNode);
                nodesAddedToNets++;
            }
        } else {
            // node not in any net
        }
    }
    file.close();

    std::cout << "Nodes file done. " << lineCount << " lines, " << validLineCount
              << " valid nodes.\n";
    std::cout << "Added " << nodesAddedToNets << " node instances to nets.\n";

    return (validLineCount > 0);
}