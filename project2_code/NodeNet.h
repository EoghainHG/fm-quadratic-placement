#ifndef NODE_NET_H
#define NODE_NET_H

#include <string>
#include <iostream>
#include "net.h" // Include our consolidated Net class

/******************************************************************************
 * Node class
 *****************************************************************************/
class Node {
public:
    std::string name;
    int width;
    int height;
    std::string movetype;
    Node* next;
    Node* prev;

    Node(std::string node_name, int w, int h, std::string type = "movable")
        : name(node_name), width(w), height(h), movetype(type),
          next(nullptr), prev(nullptr) {}
};

/******************************************************************************
 * NetList class
 *  - Doubly linked list of Net objects
 *****************************************************************************/
class NetList {
public:
    Net* head;
    Net* tail;

    NetList() : head(nullptr), tail(nullptr) {}

    // Add a new net at the tail
    Net* addNet(const std::string &name);

    Net* findNet(const std::string &netName);

    int count();

    // Destructor cleans up all Net objects and their Nodes
    ~NetList();
};

#endif