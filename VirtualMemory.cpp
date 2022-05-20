//
// Created by Evyatar on 15/05/2022.
//

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <iostream>
#include <stdlib.h>

int getStartBit (int level)
{
  return OFFSET_WIDTH * (TABLES_DEPTH - level);
}

uint64_t getSubAddress (uint64_t virtualAddress, int level, int bitWidth=OFFSET_WIDTH)
{
  int startInd = getStartBit (level);
  uint64_t mask = 1LL << (bitWidth);
  mask--;
  mask = mask << startInd;
  return (mask & virtualAddress) >> startInd;
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
  word_t *value;
  for (int i = 0; i < PAGE_SIZE; i++)
    {
      PMread (frameIndex * PAGE_SIZE + i, value);
    }
  if (*value != 0)
    {
      return 0;
    }
  return 1;
}

int getDistanceFromCurFrameIndex(uint64_t curFrameIndex, uint64_t logicalAddressPageIndex)
{
  int a, b;
  a = abs((int) (logicalAddressPageIndex - curFrameIndex));
  b = NUM_PAGES - a;
  if (a < b){
    return a;
  }
  return b;
}

void DFS (uint64_t fatherFrameIndex, uint64_t curFrameIndex, int level, uint64_t *res,
          int *maxDist, uint64_t *maxDistIndex, uint64_t logicalAddressPageIndex)
{

  // makes sure to NOT delete the current frame
  if (fatherFrameIndex != curFrameIndex)
    {
      if (isFrameEmpty (curFrameIndex))
        {
          *res = curFrameIndex;
          return;
        }
    }

  if (level == TABLES_DEPTH)
    {
      auto dist = reinterpret_cast<unsigned long>(getDistanceFromCurFrameIndex);
      if (*maxDist < dist){
          *maxDist = dist;
          *maxDistIndex = curFrameIndex;
      }
    }

    for (int i = 0; i < PAGE_SIZE; i++)
      {
        word_t *value;
        PMread (curFrameIndex * PAGE_SIZE + i, value);
        if (*value == 0)
          {
            DFS (fatherFrameIndex, (uint64_t) value, level + 1, res, maxDist, maxDistIndex, logicalAddressPageIndex);
          }
      }
}

uint64_t findFreeFrameIndex (uint64_t fatherFrameIndex, int *nextIndex, uint64_t logicalAddressPageIndex)
{
  if (*nextIndex + 1 < NUM_FRAMES)
    {
      return ++(*nextIndex); // todo: check that ++ before return not after
    }
  uint64_t *res;
  *res = -1;
  int *maxDist;
  *maxDist = -1;
  uint64_t *maxDistIndex;
  DFS (fatherFrameIndex, 0, 0, res, maxDist, maxDistIndex, logicalAddressPageIndex);
  if (*res != -1){
    return *res;
  }
  return *maxDistIndex;
}

word_t getFinalPage (uint64_t virtualAddress)
{
  uint64_t p_i;
  int nextFreeIndex = 0;
  word_t addresses[TABLES_DEPTH + 1];
  addresses[0] = 0;
  uint64_t logicalAddressPageIndex = getSubAddress(virtualAddress, TABLES_DEPTH-1, VIRTUAL_ADDRESS_WIDTH-OFFSET_WIDTH);
  for (int i = 0; i < TABLES_DEPTH; i++)
    {
      p_i = getSubAddress (virtualAddress, i);
      PMread (addresses[i] * PAGE_SIZE + p_i, addresses + i + 1);
      if (addresses[i + 1] == 0)
        {
          // find room for next kid
          uint64_t freePageTableIndex = findFreeFrameIndex (addresses[i], &nextFreeIndex, logicalAddressPageIndex);
//        initializeTable(freePageTableIndex);
        }
    }
  return addresses[TABLES_DEPTH];
}

/*
 * Initialize the virtual memory.
 */
void VMinitialize ()
{
  initializeTable (0);
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread (uint64_t, word_t *value);

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite (uint64_t virtualAddress, word_t value);

int main (int argc, char **argv)
{
  uint64_t add = 57;
  uint64_t add1 = 60;
//  auto res = getSubAddress (add, 4);
//  std::cout << res << " " << add << std::endl;

  auto r = getDistanceFromCurFrameIndex(add, add1);
  std::cout << r << " " << std::endl;

}