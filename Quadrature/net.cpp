#include "net.h"
#include "NodeNet.h"
#include <iostream>

// Constructor for FM algorithm
Net::Net(int id) 
    : id(id), name("net_" + std::to_string(id)), 
      head(nullptr), tail(nullptr), next(nullptr), prev(nullptr) {}

// Constructor for NodeNet
Net::Net(const std::string& net_name) 
    : id(-1), name(net_name), 
      head(nullptr), tail(nullptr), next(nullptr), prev(nullptr) {}

void Net::addCell(Cell* cell) {
    cells.push_back(cell);
}

void Net::addNode(Node* node) {
    if (!head) {
        head = tail = node;
    } else {
        tail->next = node;
        node->prev = tail;
        tail = node;
    }
}

void Net::print() {
    std::cout << "Net: " << name << " (ID: " << id << ") connects: ";
    
    // Print cells if available
    if (!cells.empty()) {
        for (Cell* cell : cells) {
            std::cout << "Cell_" << cell->id << " ";
        }
    }
    
    // Also print nodes if available (NodeNet compatibility)
    if (head) {
        Node* current = head;
        while (current) {
            std::cout << current->name << " ";
            current = current->next;
        }
    }
    
    std::cout << std::endl;
}