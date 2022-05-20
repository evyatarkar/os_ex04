//
// Created by Evyatar on 15/05/2022.
//

#include "VirtualMemory.h"
#include <iostream>


uint64_t getMask(const uint64_t virtualAddress, const int startInd, const int bitWidth=OFFSET_WIDTH){
  uint64_t subAddress = 1LL << (bitWidth);
  subAddress--;
  subAddress = subAddress << startInd;
  return subAddress & virtualAddress;
}


/*
 * Initialize the virtual memory.
 */
void VMinitialize();

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value);

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value);

//int main(int argc, char **argv) {
////  getMask()
//  uint64_t test = 45789;
//  printf(reinterpret_cast<const char *>(test));
//}