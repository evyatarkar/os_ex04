//
// Created by Evyatar on 15/05/2022.
//

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <stdlib.h>

int nextFreeIndex;

int getStartBit (int level, int bitWidth)
{
  return OFFSET_WIDTH * (TABLES_DEPTH - level);
}

uint64_t
getSubAddress (uint64_t virtualAddress, int level, int bitWidth = OFFSET_WIDTH)
{
  int startInd = getStartBit (level, bitWidth);
  uint64_t mask = 1LL << (bitWidth);
  mask--;
  mask = mask << startInd;
  auto a = (mask & virtualAddress) >> startInd;
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
  uint64_t newLogicalAddress = oldLogicalAddress << OFFSET_WIDTH;

  if (currentStepDirection != 0)
    {
      newLogicalAddress = newLogicalAddress | currentStepDirection;
    }
  return newLogicalAddress;
}

void
DFS (uint64_t fatherFrameIndex, uint64_t curFrameIndex, int level, uint64_t *indexOfEmptyFrameFound,
     int *maxDist, uint64_t *maxDistIndex, uint64_t logicalAddressPageIndex,
     uint64_t *fatherIndexOfFinalIndex, uint64_t *logicalAddressOfFinalFrameIndex,
     uint64_t *frameIndexOfFinalIndexInFather, int *done, uint64_t currentLogicalAddress, uint64_t fatherIndexInt,  uint64_t indexINFatherIndexInt)
{
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
      auto dist = getDistanceFromCurFrameIndex (currentLogicalAddress, logicalAddressPageIndex);
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
      word_t value;
      for (int i = 0; i < PAGE_SIZE; i++)
        {

          PMread (curFrameIndex * PAGE_SIZE + i, &value);
          if (value >= NUM_FRAMES)
            {
              *done = 1;
              return;
            }
          if (value != 0)
            {

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

  uint64_t indexOfEmptyFrameFound = 0;
  int done = 0;
  int maxDist = 0;
  uint64_t currentFrameIndexInDFS = 0;
  uint64_t logicalAddressOfFinalFrameIndex = 0;
  uint64_t maxDistIndex = 0;
  uint64_t fatherIndexOfFinalIndex = 0;
  uint64_t frameIndexOfFinalIndexInFather = 0;

  DFS (fatherFrameIndex, currentFrameIndexInDFS, 0, &indexOfEmptyFrameFound, &maxDist, &maxDistIndex,
       logicalAddressPageIndex, &fatherIndexOfFinalIndex, &logicalAddressOfFinalFrameIndex,
       &frameIndexOfFinalIndexInFather, &done, 0, 0, 0);

  if (indexOfEmptyFrameFound != 0)
    {
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
  word_t freeFrameIndex = 0;
  word_t lastFreeFrameIndex = 0;
  uint64_t logicalAddressPageIndex = getSubAddress (virtualAddress,
                                                    TABLES_DEPTH - 1,
                                                    VIRTUAL_ADDRESS_WIDTH
                                                    - OFFSET_WIDTH);
  for (int i = 0; i < TABLES_DEPTH; i++)
    {
      p_i = getSubAddress (virtualAddress, i, OFFSET_WIDTH);

      PMread (freeFrameIndex * PAGE_SIZE + p_i, addresses + i);

      if (addresses[i] == 0)
        {
          // find room for next kid
          lastFreeFrameIndex = freeFrameIndex;
          freeFrameIndex = findFreeFrameIndex (lastFreeFrameIndex, &nextFreeIndex, logicalAddressPageIndex);
          // link the new empty frame to father
          PMwrite (lastFreeFrameIndex * PAGE_SIZE + p_i, freeFrameIndex);
        }
      else
        {
          lastFreeFrameIndex = freeFrameIndex;
          freeFrameIndex = addresses[i];
        }
    }

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
  uint64_t newFrameIndex = insertPageToFrame (virtualAddress);
  auto d = getSubAddress (virtualAddress, TABLES_DEPTH);
  PMread (newFrameIndex * PAGE_SIZE + d, value);
  return 1;
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite (uint64_t virtualAddress, word_t value)
{
  uint64_t newFrameIndex = insertPageToFrame (virtualAddress);
  auto d = getSubAddress (virtualAddress, TABLES_DEPTH);
  PMwrite (newFrameIndex * PAGE_SIZE + d, value);
  return 1;
}
