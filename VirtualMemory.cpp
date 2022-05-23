//
// Created by Evyatar on 15/05/2022.
//

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <iostream>
#include <stdlib.h>


int nextFreeIndex;


int getStartBit(int level) {
    return PAGE_SIZE * (TABLES_DEPTH - level - 1) + OFFSET_WIDTH;
}

uint64_t
getSubAddress(uint64_t virtualAddress, int level, int bitWidth = OFFSET_WIDTH) {
    int startInd = getStartBit(level);
    uint64_t mask = 1LL << (bitWidth);
    mask--;
    mask = mask << startInd;
    return (mask & virtualAddress) >> startInd;
}

void initializeTable(uint64_t frameIndex) {
    for (int i = 0; i < PAGE_SIZE; i++) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

int isFrameEmpty(uint64_t frameIndex) {
    word_t value;
    value = 0;
    for (int i = 0; i < PAGE_SIZE; i++) {
        PMread(frameIndex * PAGE_SIZE + i, &value);
        if (value != 0) {
            return 0;
        }
    }
    return 1;
}

int
getDistanceFromCurFrameIndex(uint64_t curLogicalAddressIndex, uint64_t logicalAddressPageIndex) {
    uint64_t a, b;
    a = abs(int(logicalAddressPageIndex - curLogicalAddressIndex));
    b = NUM_PAGES - a;
    if (a < b) {
        return a;
    }
    return b;
}

void
DFS(uint64_t fatherFrameIndex, uint64_t curFrameIndex, int level, uint64_t *res,
    int *maxDist, uint64_t *maxDistIndex, uint64_t logicalAddressPageIndex,
    uint64_t *fatherIndexOfFinalIndex, uint64_t *logicalAddressOfFinalFrameIndex,
    uint64_t *frameIndexOfFinalIndexInFather, int *done) {

    // makes sure to NOT delete the current frame
    auto currentLogicalAddress = getSubAddress(curFrameIndex, TABLES_DEPTH - 1,
                                               VIRTUAL_ADDRESS_WIDTH
                                               - OFFSET_WIDTH);
    if (fatherFrameIndex != curFrameIndex && level != TABLES_DEPTH) {
        if (isFrameEmpty(curFrameIndex)) {
            *res = curFrameIndex;
            *logicalAddressOfFinalFrameIndex = currentLogicalAddress;
//            *done = 1;
            return;
        }
    }

    if (level == TABLES_DEPTH) {
        std::cout << "Got to LEAF!" << std::endl;
        std::cout << " - current logical address: " << currentLogicalAddress << std::endl;
        std::cout << " - logical adress where were trying to add: " << logicalAddressPageIndex << std::endl;
        auto dist = getDistanceFromCurFrameIndex(currentLogicalAddress, logicalAddressPageIndex);
        std::cout << " $ distance between them is: " << dist << std::endl;
        if (*maxDist < dist) {
            *maxDist = dist;
            *maxDistIndex = curFrameIndex;
            *logicalAddressOfFinalFrameIndex = currentLogicalAddress;
        }
    } else {
        std::cout << " GOT TO ELSE" << std::endl;
        word_t value;
        for (int i = 0; i < PAGE_SIZE; i++) {
            PMread(curFrameIndex * PAGE_SIZE + i, &value);
            std::cout << "-- reading from frame index: " << curFrameIndex <<
            " row num is: " << i << " value from read is: " << value << std::endl;
            if (value >= NUM_FRAMES)
            {
                *done = 1;
                return;
            }
            if (value != 0) {
                std::cout << "value != 0" << std::endl;
                *fatherIndexOfFinalIndex = curFrameIndex;
                *frameIndexOfFinalIndexInFather = i;
                DFS(fatherFrameIndex, (uint64_t) value, level + 1, res,
                    maxDist, maxDistIndex, logicalAddressPageIndex,
                    fatherIndexOfFinalIndex, logicalAddressOfFinalFrameIndex, frameIndexOfFinalIndexInFather, done);
                if (*done){
                    return;
                }
            }
        }
    }

}

uint64_t
findFreeFrameIndex(uint64_t fatherFrameIndex, int *nextIndex, uint64_t logicalAddressPageIndex) {
    if ((*nextIndex + 1) < NUM_FRAMES) {
        *nextIndex = *nextIndex + 1;
//        std::cout << "found free frame from regular indexes. at: " << *nextIndex << std::endl;
//        std::cout << "address at: " << nextIndex << std::endl;
        return *nextIndex; // todo: check that ++ before return not after
    }
    std::cout << "No more indexes left. this is good." << std::endl;
    uint64_t res = -1;
    int done = 0;
    int maxDist = 0;
    int currentFrameIndexInDFS = 0;
    uint64_t logicalAddressOfFinalFrameIndex = -1;
    uint64_t maxDistIndex = -1;
    uint64_t fatherIndexOfFinalIndex = -1;
    uint64_t frameIndexOfFinalIndexInFather = -1;

    std::cout << "starting DFS" << std::endl;
    DFS(fatherFrameIndex, currentFrameIndexInDFS, 0, &res, &maxDist, &maxDistIndex,
        logicalAddressPageIndex, &fatherIndexOfFinalIndex, &logicalAddressOfFinalFrameIndex,
        &frameIndexOfFinalIndexInFather, &done);
    std::cout << "finished DFS" << std::endl;

    if (res != -1) {
        PMevict(res, logicalAddressPageIndex);
        PMwrite((fatherIndexOfFinalIndex) * PAGE_SIZE + frameIndexOfFinalIndexInFather, 0);
        return res;
    }
    PMevict(maxDistIndex, logicalAddressPageIndex);
    initializeTable(maxDistIndex);
    PMwrite((fatherIndexOfFinalIndex) * PAGE_SIZE
            + frameIndexOfFinalIndexInFather, 0);
    return maxDistIndex;
}


uint64_t insertPageToFrame(uint64_t virtualAddress) {
    uint64_t p_i;
    word_t addresses[TABLES_DEPTH];
//    addresses[0] = 0;
    word_t freeFrameIndex = 0;
    word_t lastFreeFrameIndex = 0;
    uint64_t logicalAddressPageIndex = getSubAddress(virtualAddress,
                                                     TABLES_DEPTH - 1,
                                                     VIRTUAL_ADDRESS_WIDTH
                                                     - OFFSET_WIDTH);
    for (int i = 0; i < TABLES_DEPTH; i++) {
        p_i = getSubAddress(virtualAddress, i, 2);
        std::cout << "i: " << i << ". Pi: " << p_i << std::endl;
        PMread(lastFreeFrameIndex * PAGE_SIZE + p_i, addresses + i);
        std::cout << "next address from PMread: " << addresses[i + 1] << std::endl;
        if (addresses[i] == 0) {
            std::cout << "going to find frame" << std::endl;
            // find room for next kid
            lastFreeFrameIndex = freeFrameIndex;
            freeFrameIndex = findFreeFrameIndex(addresses[i], &nextFreeIndex, logicalAddressPageIndex);
//            addresses[i] = freeFrameIndex;
            std::cout << "freeFrameIndex:" << freeFrameIndex << std::endl;
            // link the new empty frame to father
            PMwrite(lastFreeFrameIndex * PAGE_SIZE + p_i, freeFrameIndex);
        }
    }

    std::cout << "FINAL - restoring index :" << freeFrameIndex << std::endl;
    PMrestore(freeFrameIndex, logicalAddressPageIndex);
    return freeFrameIndex;
}

/*
 * Initialize the virtual memory.
 */
void VMinitialize() {
    initializeTable(0);
    nextFreeIndex = 0;
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t *value) {
    uint64_t newFrameIndex = insertPageToFrame(virtualAddress);
    auto d = getSubAddress(virtualAddress, TABLES_DEPTH);
    PMread(newFrameIndex * PAGE_SIZE + d, value);
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value) {
    uint64_t newFrameIndex = insertPageToFrame(virtualAddress);
    auto d = getSubAddress(virtualAddress, TABLES_DEPTH);
    PMwrite(newFrameIndex * PAGE_SIZE + d, value);
}
//
//int main (int argc, char **argv)
//{
//  uint64_t add = 57;
//  uint64_t add1 = 60;
////  auto res = getSubAddress (add, 4);
////  std::cout << res << " " << add << std::endl;
//
//  auto r = getDistanceFromCurFrameIndex (add, add1);
//  std::cout << r << " " << std::endl;
//
//}