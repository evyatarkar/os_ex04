//
// Created by Evyatar on 15/05/2022.
//

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <iostream>
#include <stdlib.h>

int nextFreeIndex;

int getStartBit (int level, int bitWidth)
{
  return OFFSET_WIDTH * (TABLES_DEPTH - level);
}

uint64_t
getSubAddress (uint64_t virtualAddress, int level, int bitWidth = OFFSET_WIDTH)
{
//  std::cout << " ~~~ getting sub address" << std::endl;
//  std::cout << "     virtualAddress: " << virtualAddress << std::endl;
//  std::cout << "     level: " << level << std::endl;
//  std::cout << "     bitWidth: " << bitWidth << std::endl;
  int startInd = getStartBit (level, bitWidth);
//  std::cout << "     startInd: " << startInd << std::endl;
  uint64_t mask = 1LL << (bitWidth);
//  std::cout << "     mask: " << mask << std::endl;
  mask--;
//  std::cout << "     mask: " << mask << std::endl;
  mask = mask << startInd;
//  std::cout << "     mask: " << mask << std::endl;
//  std::cout << "     mask & virtualAddress: " << (mask & virtualAddress)
//            << std::endl;
  auto a = (mask & virtualAddress) >> startInd;
//  std::cout << "     RESULT: " << a << std::endl;
  return a;
}

void initializeTable (uint64_t frameIndex)
{
  for (int i = 0; i < PAGE_SIZE; i++)
    {
      PMwrite (frameIndex * PAGE_SIZE + i, 0);
    }
}

int isFrameEmpty (uint64_t frameIndex)
{
  word_t value;
  value = 0;
  for (int i = 0; i < PAGE_SIZE; i++)
    {
      PMread (frameIndex * PAGE_SIZE + i, &value);
      if (value != 0)
        {
          return 0;
        }
    }
  return 1;
}

int
getDistanceFromCurFrameIndex (uint64_t curLogicalAddressIndex, uint64_t logicalAddressPageIndex)
{
//  std::cout << "!\ngetting distance between : " << curLogicalAddressIndex
//            << " and " << logicalAddressPageIndex << std::endl;
  uint64_t a, b;
  a = abs (int (logicalAddressPageIndex - curLogicalAddressIndex));
  b = NUM_PAGES - a;
  if (a < b)
    {
      return a;
    }
  return b;
}
uint64_t
getNextLogicalAddress (uint64_t oldLogicalAddress, uint64_t currentStepDirection)
{
//  std::cout << " 0 - oldLogicalAddress " << oldLogicalAddress
//            << ", currentStepDirection " << currentStepDirection << std::endl;
  uint64_t newLogicalAddress = oldLogicalAddress << OFFSET_WIDTH;

//  std::cout << "   - after pushing by offset: " << newLogicalAddress
//            << std::endl;
  if (currentStepDirection != 0)
    {
      newLogicalAddress = newLogicalAddress | currentStepDirection;
    }
//  std::cout << "   - after or with offset: " << newLogicalAddress << std::endl;
  return newLogicalAddress;
}

void
DFS (uint64_t fatherFrameIndex, uint64_t curFrameIndex, int level, uint64_t *indexOfEmptyFrameFound,
     int *maxDist, uint64_t *maxDistIndex, uint64_t logicalAddressPageIndex,
     uint64_t *fatherIndexOfFinalIndex, uint64_t *logicalAddressOfFinalFrameIndex,
     uint64_t *frameIndexOfFinalIndexInFather, int *done, uint64_t currentLogicalAddress, uint64_t fatherIndexInt,  uint64_t indexINFatherIndexInt)
{

  // makes sure to NOT delete the current frame
//  std::cout << "getting sub address for current logical address in DFS"
//            << std::endl;

  if (fatherFrameIndex != curFrameIndex && level != TABLES_DEPTH)
    {
      if (isFrameEmpty (curFrameIndex))
        {
          *indexOfEmptyFrameFound = curFrameIndex;
          *logicalAddressOfFinalFrameIndex = currentLogicalAddress;
          *done = 1;
          *fatherIndexOfFinalIndex = fatherIndexInt;
          *frameIndexOfFinalIndexInFather = indexINFatherIndexInt;
          return;
        }
    }

  if (level == TABLES_DEPTH)
    {
//      std::cout << "Got to LEAF!" << std::endl;
//      std::cout << " - current logical address: " << currentLogicalAddress
//                << std::endl;
//      std::cout << " - logical adress where were trying to add: "
//                << logicalAddressPageIndex << std::endl;
      auto dist = getDistanceFromCurFrameIndex (currentLogicalAddress, logicalAddressPageIndex);
//      std::cout << " $ distance between them is: " << dist << std::endl;
      if (*maxDist < dist)
        {
          *maxDist = dist;
          *maxDistIndex = curFrameIndex;
          *logicalAddressOfFinalFrameIndex = currentLogicalAddress;
          *fatherIndexOfFinalIndex = fatherIndexInt;
          *frameIndexOfFinalIndexInFather = indexINFatherIndexInt;
        }
    }
  else
    {
//      std::cout << " GOT TO ELSE" << std::endl;
      word_t value;
      for (int i = 0; i < PAGE_SIZE; i++)
        {
          PMread (curFrameIndex * PAGE_SIZE + i, &value);
//          std::cout << "-- reading from frame index: " << curFrameIndex <<
//                    " row num is: " << i << " value from read is: " << value
//                    << std::endl;
          if (value >= NUM_FRAMES)
            {
              *done = 1;
              return;
            }
          if (value != 0)
            {
//              std::cout << "value != 0" << std::endl;
//              *curFrameIndex = value;
//          auto nextLogicalAddress = getNextLogicalAddress(oldLogicalAddress, i);

              DFS (fatherFrameIndex, value, level + 1, indexOfEmptyFrameFound,
                   maxDist, maxDistIndex, logicalAddressPageIndex,
                   fatherIndexOfFinalIndex, logicalAddressOfFinalFrameIndex,
                   frameIndexOfFinalIndexInFather, done,
                   getNextLogicalAddress (currentLogicalAddress, i), curFrameIndex, i);
              if (*done)
                {
                  return;
                }
            }
        }
      // TODO something needs to happen here to single out the last frame we took while trying to build a tree and the were none left
      // because now its zeros so its exchanging itself.
      // freeFrameIndex is 3 and should be 4
    }

}

uint64_t
findFreeFrameIndex (uint64_t fatherFrameIndex, int *nextIndex, uint64_t logicalAddressPageIndex)
{
  if ((*nextIndex + 1) < NUM_FRAMES)
    {
      *nextIndex = *nextIndex + 1;
      return *nextIndex;
    }

//  std::cout << "No more indexes left. this is good." << std::endl;
  uint64_t indexOfEmptyFrameFound = 0;
  int done = 0;
  int maxDist = 0;
  uint64_t currentFrameIndexInDFS = 0;
  uint64_t logicalAddressOfFinalFrameIndex = 0;
  uint64_t maxDistIndex = 0;
  uint64_t fatherIndexOfFinalIndex = 0;
  uint64_t frameIndexOfFinalIndexInFather = 0;

//  std::cout << "starting DFS" << std::endl;
  DFS (fatherFrameIndex, currentFrameIndexInDFS, 0, &indexOfEmptyFrameFound, &maxDist, &maxDistIndex,
       logicalAddressPageIndex, &fatherIndexOfFinalIndex, &logicalAddressOfFinalFrameIndex,
       &frameIndexOfFinalIndexInFather, &done, 0, 0, 0);
//  std::cout << "finished DFS" << std::endl;

  if (indexOfEmptyFrameFound != 0)
    {
//      PMevict (indexOfEmptyFrameFound, logicalAddressOfFinalFrameIndex);
      PMwrite ((fatherIndexOfFinalIndex) * PAGE_SIZE + frameIndexOfFinalIndexInFather, 0);
      return indexOfEmptyFrameFound;
    }

  PMevict (maxDistIndex, logicalAddressOfFinalFrameIndex);
  initializeTable (maxDistIndex);
  PMwrite ((fatherIndexOfFinalIndex) * PAGE_SIZE + frameIndexOfFinalIndexInFather, 0);
  return maxDistIndex;
}

uint64_t insertPageToFrame (uint64_t virtualAddress)
{
  uint64_t p_i;
  word_t addresses[TABLES_DEPTH];
//    addresses[0] = 0;
  word_t freeFrameIndex = 0;
  word_t lastFreeFrameIndex = 0;
//  std::cout << "getting sub address for logical address of OG" << std::endl;

  uint64_t logicalAddressPageIndex = getSubAddress (virtualAddress,
                                                    TABLES_DEPTH - 1,
                                                    VIRTUAL_ADDRESS_WIDTH
                                                    - OFFSET_WIDTH);
  for (int i = 0; i < TABLES_DEPTH; i++)
    {
//      std::cout << "getting sub address p_i. i = " << i << std::endl;
      p_i = getSubAddress (virtualAddress, i, OFFSET_WIDTH);
//      std::cout << "!! current depth is: " << i << ". offset is: Pi: " << p_i
//                << ". freeFrameIndex is: " << freeFrameIndex << std::endl;
      PMread (freeFrameIndex * PAGE_SIZE + p_i, addresses + i);

//      std::cout << "next address from PMread: " << addresses[i] << std::endl;
      if (addresses[i] == 0)
        {
//            std::cout << "   going to find frame" << std::endl;
          // find room for next kid
          lastFreeFrameIndex = freeFrameIndex;
          freeFrameIndex = findFreeFrameIndex (lastFreeFrameIndex, &nextFreeIndex, logicalAddressPageIndex);
//            addresses[i] = freeFrameIndex;
//            std::cout << "   freeFrameIndex found after DFS: " << freeFrameIndex << std::endl;
          // link the new empty frame to father
//          std::cout << " linking new frame " << freeFrameIndex << " to father "
//                    << lastFreeFrameIndex << " in index: " << p_i << std::endl;
          PMwrite (lastFreeFrameIndex * PAGE_SIZE + p_i, freeFrameIndex);
        }
      else
        {
//          std::cout << "GOT TO ELSE FROM addresses[i] == " << addresses[i] << std::endl;
          lastFreeFrameIndex = freeFrameIndex; //todo: what is that?
          freeFrameIndex = addresses[i];
//          PMwrite (lastFreeFrameIndex * PAGE_SIZE + p_i, freeFrameIndex);
        }
    }

//  std::cout << "   restoring with: " << freeFrameIndex << ", "
//            << logicalAddressPageIndex << std::endl;
  PMrestore (freeFrameIndex, logicalAddressPageIndex);
  return freeFrameIndex;
}

/*
 * Initialize the virtual memory.
 */
void VMinitialize ()
{
  initializeTable (0);
  nextFreeIndex = 0;
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread (uint64_t virtualAddress, word_t *value)
{
//  std::cout << "\n + VMread called" << std::endl;
  uint64_t newFrameIndex = insertPageToFrame (virtualAddress);
//  std::cout << "\n + from VMread got ne empty frame: " << newFrameIndex
//            << ". getting subaddress" << std::endl;
  auto d = getSubAddress (virtualAddress, TABLES_DEPTH);
  PMread (newFrameIndex * PAGE_SIZE + d, value);
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite (uint64_t virtualAddress, word_t value)
{
//  std::cout << "\n + VMwrite called" << std::endl;

  uint64_t newFrameIndex = insertPageToFrame (virtualAddress);
  auto d = getSubAddress (virtualAddress, TABLES_DEPTH);
  PMwrite (newFrameIndex * PAGE_SIZE + d, value);
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