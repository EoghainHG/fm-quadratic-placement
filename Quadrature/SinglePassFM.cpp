#include "SinglePassFM.h"
#include "NodeNet.h"
#include <iostream>
#include <cmath>
#include <random>
#include <ctime>

// Define the extern maps declared in SinglePassFM.h
std::unordered_map<std::string, int>  nodeArea;
std::unordered_map<std::string, bool> nodePart;
std::unordered_map<std::string, int>  nodeGain;
std::unordered_map<std::string, bool> nodeLocked;
int totalArea = 0;

/******************************************************************************
 * areaBalance: check if new area is within +/-31% ratio
 *****************************************************************************/
static bool areaBalance(int A, int B, int T)
{
    double r = (double)A / (double)(A + B);
    double left  = (r*T) - (0.31*T);
    double right = (r*T) + (0.31*T);
    return (A >= left && A <= right);
}

/******************************************************************************
 * computeGainForNodeName:
 *   We'll see how many nodes in partition A vs. B in each net that has 'nodeName'
 *   If net is cut => external or internal
 *****************************************************************************/
static int computeGainForNodeName(const std::string &nodeName,
                                  int &fsCount,
                                  const std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap)
{
    // if node not in any net => no external edges
    auto it = nodeToNetsMap.find(nodeName);
    if (it == nodeToNetsMap.end()) {
        fsCount += 0;
        return 0;
    }

    bool myPart = nodePart[nodeName];
    int fs = 0, te = 0;

    // for each net referencing this nodeName
    for (Net* netPtr : it->second) {
        // count how many are in A vs. B
        int netA = 0, netB = 0;
        Node* c = netPtr->head;
        while (c) {
            bool cpart = nodePart[c->name];
            if (cpart) netA++;
            else       netB++;
            c = c->next;
        }
        if (netA > 0 && netB > 0) {
            // net is cut
            if (myPart) {
                if (netB > 0) fs++;
                else te++;
            } else {
                if (netA > 0) fs++;
                else te++;
            }
        } else {
            te++;
        }
    }
    fsCount += fs;
    return (fs - te);
}

/******************************************************************************
 * computeCutsize: sum external edges from perspective of each nodeName => /2
 *****************************************************************************/
static int computeCutsize(const std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap)
{
    int totalFS = 0;
    for (auto &pr : nodeArea) {
        const std::string &nm = pr.first;
        int localFS = 0;
        computeGainForNodeName(nm, localFS, nodeToNetsMap);
        totalFS += localFS;
    }
    return (totalFS / 2);
}

/******************************************************************************
 * singlePassFM: 
 *   1) compute gains for each node
 *   2) build bucket
 *   3) pick from top gain down, move if area ok, lock
 *****************************************************************************/
static int singlePassFM(const std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap)
{
    // partition area
    int partA = 0, partB = 0;
    // unlock all
    for (auto &pr : nodeArea) {
        if (nodePart[pr.first]) partA += pr.second;
        else                    partB += pr.second;
        nodeLocked[pr.first] = false;
    }

    // compute gains
    int highestGain = -1;
    std::vector<std::string> allNodes;
    allNodes.reserve(nodeArea.size());

    for (auto &pr : nodeArea) {
        const std::string &nm = pr.first;
        int fsTmp = 0;
        int g = computeGainForNodeName(nm, fsTmp, nodeToNetsMap);
        nodeGain[nm] = g;
        if (std::abs(g) > highestGain) highestGain = std::abs(g);
        allNodes.push_back(nm);
    }

    if (highestGain < 0) {
        // no net is cut => no moves
        return computeCutsize(nodeToNetsMap);
    }

    // build bucket from -highest.. +highest => size=2*highest+1
    int bucketSize = (2*highestGain) + 1;
    std::vector<std::vector<std::string>> bucket(bucketSize);
    int offset = highestGain;

    for (auto &nm : allNodes) {
        int idx = nodeGain[nm] + offset;
        bucket[idx].push_back(nm);
    }

    // move from top gain to lower
    for (int gpos = (2*highestGain); gpos >= 0; gpos--) {
        auto &bckt = bucket[gpos];
        while (!bckt.empty()) {
            std::string c = bckt.back();
            bckt.pop_back();
            if (nodeLocked[c]) continue;

            // recalc area
            partA = 0; partB = 0;
            for (auto &p2 : nodeArea) {
                if (nodePart[p2.first]) partA += p2.second;
                else                    partB += p2.second;
            }
            int cArea = nodeArea[c];

            if (nodePart[c]) {
                // A->B
                int newA = partA - cArea;
                int newB = partB + cArea;
                if (areaBalance(newA, newB, totalArea)) {
                    nodePart[c] = false;
                }
            } else {
                // B->A
                int newA = partA + cArea;
                int newB = partB - cArea;
                if (areaBalance(newA, newB, totalArea)) {
                    nodePart[c] = true;
                }
            }
            nodeLocked[c] = true;

            // skip neighbor updates in this example
            break;
        }
    }

    return computeCutsize(nodeToNetsMap);
}

/******************************************************************************
 * runSinglePassFM:
 *   - random initial partition
 *   - do singlePassFM
 *****************************************************************************/
void runSinglePassFM(const std::unordered_map<std::string, std::vector<Net*>> &nodeToNetsMap)
{
    // random partition
    std::mt19937 rng((unsigned)time(nullptr));
    std::uniform_int_distribution<int> dist(0,1);

    for (auto &pr : nodeArea) {
        nodePart[pr.first] = (dist(rng) == 1);
    }

    int initCut = computeCutsize(nodeToNetsMap);
    std::cout << "Initial cutsize = " << initCut << "\n";

    int finalCut = singlePassFM(nodeToNetsMap);
    std::cout << "Final cutsize after single-pass FM = " << finalCut << "\n";
}