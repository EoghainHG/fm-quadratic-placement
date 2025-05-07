#ifndef NET_H
#define NET_H

#include "cell.h"
#include <vector>
#include <string>

class Node; // Forward declaration for compatibility

class Net {
public:
    // Common attributes
    int id;
    std::string name;
    std::vector<Cell*> cells;

    // Additional attributes for NodeNet compatibility
    Node* head;  // Head of nodes in net (NodeNet compatibility)
    Node* tail;  // Tail of nodes in net (NodeNet compatibility)
    Net* next;   // Next net in NetList (NodeNet compatibility)
    Net* prev;   // Previous net in NetList (NodeNet compatibility)

    // Constructors
    Net(int id);
    Net(const std::string& net_name);

    // Methods
    void addCell(Cell* cell);
    void addNode(Node* node); // For NodeNet compatibility
    
    // Debug output
    void print();
};

#endif // NET_H