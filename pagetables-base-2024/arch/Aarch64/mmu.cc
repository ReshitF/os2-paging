/* pagetables -- A framework to experiment with memory management
 *
 *    AArch64/mmu.cc - A AArch64, 4-level page table to serve as
 *                    example. Hardware MMU part.
 *
 * Copyright (C) 2017-2020  Leiden University, The Netherlands.
 */

// Authors:
// R. Fazlija (s3270831)
// A. Kooiker (s2098199)

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

  // Helper variables to decode the addres for the page walk
  const uint64_t mask = (1 << 11) -1;             // 11-bit bitmask
  const uint64_t level_3 = vPage & mask;          // bits 0-10
  const uint64_t level_2 = (vPage >> 11) & mask;  // bits 11-21
  const uint64_t level_1 = (vPage >> 22) & mask;  // bits 22-32
  const uint64_t level_0 = (vPage >> 33) & 1;     // bit 33

  TableEntry *Table_0 = reinterpret_cast<TableEntry*>(root);
  if (Table_0[level_0].valid == 0)
    return false;

  // Here the reinterpret cast of the driver is reversed and 
  // the physicalPage is left shifted to get a full pointer to the next level 
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

// The code below facilitates the TLB functionality
TLB::TLB(const size_t nEntries, const MMU &mmu):
          nEntries(nEntries), mmu(mmu), nLookups(0),
          nHits(0), nEvictions(0), nFlush(0), nFlushEvictions(0),
          Buffer({}){
}

TLB::~TLB(){
}

bool
TLB::lookup(const uint64_t vPage, uint64_t &pPage){
  nLookups++;
  for (int i = Buffer.size()-1; i >= 0; i--){
    if (std::get<0>(Buffer[i]) == ASID && std::get<1>(Buffer[i]) == vPage){
      pPage = std::get<2>(Buffer[i]);
      // Found element is used, so is moved to back (most recently used)
      Buffer.push_back(Buffer[i]);
      Buffer.erase(Buffer.begin()+i);
      nHits++;
      return true;
    }
  }
  return false;
}

void
TLB::add(const uint64_t vPage, const uint64_t pPage){
  std::tuple<uint64_t,uint64_t,uint64_t> newEntry = std::make_tuple(ASID, vPage, pPage);
  if (Buffer.size() == nEntries){
    nEvictions++;
    Buffer.erase(Buffer.begin());
  }
  Buffer.push_back(newEntry);
}

void
TLB::flush(void){
  nFlush++;
  nFlushEvictions += Buffer.size();
  nEvictions += Buffer.size();
  Buffer.clear();
}

void
TLB::clear(void){
  flush();
  nLookups = 0;
  nHits = 0;
  nEvictions = 0;
  nFlush = 0;
  nFlushEvictions = 0;
  ASID = 0;
}

void
TLB::getStatistics(int &nLookups, int &nHits, int &nEvictions,
                       int &nFlush, int &nFlushEvictions) const
{
  nLookups = this->nLookups;
  nHits = this->nHits;
  nEvictions = this->nEvictions;
  nFlush = this->nFlush;
  nFlushEvictions = this->nFlushEvictions;
}

void
TLB::setASID(uint64_t newASID){
  this->ASID = newASID;
}

void
TLB::setASIDEnabled(bool enable){
  this->ASIDEnabled = enable;
}