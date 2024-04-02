/* pagetables -- A framework to experiment with memory management
 *
 *    AArch64/mmu.cc - A AArch64, 4-level page table to serve as
 *                    example. Hardware MMU part.
 *
 * Copyright (C) 2017-2020  Leiden University, The Netherlands.
 */

#include "AArch64.h"
using namespace AArch64;

/*
 * MMU part. The MMU simply performs a walk of the page table and
 * attempts the translation from a virtual to a physical address.
 */

AArch64MMU::AArch64MMU()
{
}

AArch64MMU::~AArch64MMU()
{
}

bool
AArch64MMU::performTranslation(const uint64_t vPage,
                              uint64_t &pPage,
                              bool isWrite)
{
  /* Validate alignment of page table */
  if ((root & (pageTableAlign - 1)) != 0)
    throw std::runtime_error("Unaligned page table access");

  const uint64_t mask = (1 << 11) -1;             // 11-bit bitmask
  const uint64_t level_3 = vPage & mask;          // bits 0-10
  const uint64_t level_2 = (vPage >> 11) & mask;  // bits 11-21
  const uint64_t level_1 = (vPage >> 22) & mask;  // bits 22-32
  const uint64_t level_0 = (vPage >> 33) & 1;     // bit 33

  TableEntry *table_0 = reinterpret_cast<TableEntry *>(root);
  TableEntry *table_1 = reinterpret_cast<TableEntry *>( &table_0[level_0] ); 
  TableEntry *table_2 = reinterpret_cast<TableEntry *>( &table_1[level_1] );  
  TableEntry *table_3 = reinterpret_cast<TableEntry *>( &table_2[level_2] );  

  if (not table_3[level_3].valid)
    return false;

  pPage = table_3[level_3].physicalPage;

  return true;
}
