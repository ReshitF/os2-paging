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

  TableEntry *table = reinterpret_cast<TableEntry *>(root);
  if (not table[vPage].valid)
    return false;

  pPage = table[vPage].physicalPage;

  return true;
}
