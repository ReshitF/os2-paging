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

  
  TableEntry *Table_0 = reinterpret_cast<TableEntry*>(root);
  if (Table_0[level_0].valid == 0)
    return false;

  TableEntry *Table_1 = reinterpret_cast<TableEntry*>(Table_0[level_0].physicalPage << pageBits);
  if (Table_1[level_1].valid == 0)
    return false;

  TableEntry *Table_2 = reinterpret_cast<TableEntry*>(Table_1[level_1].physicalPage << pageBits);
  if (Table_2[level_2].valid == 0)
    return false;

  TableEntry *Table_3 = reinterpret_cast<TableEntry*>(Table_2[level_2].physicalPage << pageBits);
  if (Table_3[level_3].valid == 0)
    return false;

  pPage = Table_3[level_3].physicalPage;

  return true;
}

TLB::TLB(const size_t nEntries, const MMU &mmu):
          nEntries(nEntries), mmu(mmu), nLookups(0),
          nHits(0), nEvictions(0), nFlush(0), nFlushEvictions(0),
          temp({})
{
}

TLB::~TLB(){
}

bool
TLB::lookup(const uint64_t vPage, uint64_t &pPage){
  return true;
}

void
TLB::add(const uint64_t vPage, const uint64_t pPage){

}

void
TLB::flush(void){

}

void
TLB::clear(void){

}

void
TLB::getStatistics(int &nLookups, int &nHits, int &nEvictions,
                       int &nFlush, int &nFlushEvictions) const{

}