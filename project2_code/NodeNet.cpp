#include "NodeNet.h"

/******************************************************************************
 * NetList method implementations
 *****************************************************************************/

// Add a new net at the tail of the NetList
Net* NetList::addNet(const std::string &name) {
    Net* newNet = new Net(name);
    if (!head) {
        head = tail = newNet;
    } else {
        tail->next = newNet;
        newNet->prev = tail;
        tail = newNet;
    }
    return newNet;
}

// Find a net by name
Net* NetList::findNet(const std::string &netName) {
    Net* current = head;
    while (current) {
        if (current->name == netName) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Count how many Net objects in the list
int NetList::count() {
    int c = 0;
    Net* current = head;
    while (current) {
        c++;
        current = current->next;
    }
    return c;
}

// Destructor: delete all Net objects and their Node objects
NetList::~NetList() {
    Net* currentNet = head;
    while (currentNet) {
        Net* nextNet = currentNet->next;
        
        // Delete all nodes in this net
        Node* currentNode = currentNet->head;
        while (currentNode) {
            Node* nextNode = currentNode->next;
            delete currentNode;
            currentNode = nextNode;
        }
        delete currentNet;
        currentNet = nextNet;
    }
}