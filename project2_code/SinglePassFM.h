#ifndef SINGLE_PASS_FM_H
#define SINGLE_PASS_FM_H

#include <string>
#include <unordered_map>
#include <vector>

class Net; // forward
class NetList;

// We store node attributes in global maps keyed by nodeName:
extern std::unordered_map<std::string, int>  nodeArea;   // area
extern std::unordered_map<std::string, bool> nodePart;   // partition
extern std::unordered_map<std::string, int>  nodeGain;   // gain
extern std::unordered_map<std::string, bool> nodeLocked; // locked

extern int totalArea; // sum of all nodeArea

// A function to run a single-pass FM approach
// We'll parse all node names, compute gains, build bucket, do moves, etc.
void runSinglePassFM(const std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap);

#endif // SINGLE_PASS_FM_H