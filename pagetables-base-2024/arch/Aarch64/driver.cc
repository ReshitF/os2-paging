/* pagetables -- A framework to experiment with memory management
 *
 *    AArch64/driver.cc - A AArch64, 4-level page table to serve as
 *                       example. OS driver part.
 *
 * Copyright (C) 2017-2020  Leiden University, The Netherlands.
 */

#include "AArch64.h"
using namespace AArch64;

#include <iostream>

/*
 * Helper functions for generating assertions and forming page table
 * entries and addresses.
 */

static inline void
initPageTableEntry(TableEntry &entry, const uintptr_t address)
{
  entry.physicalPage = address >> pageBits;
  entry.read = 1;
  entry.valid = 1;
  entry.dirty = 0;
}

static inline uintptr_t
getAddress(TableEntry &entry)
{
  return (uintptr_t)entry.physicalPage << pageBits;
}

/* OSkernel part. The OS kernel is in charge of actually allocating and
 * organizing the page tables.
 */

const static uint64_t entries = 2048;

AArch64MMUDriver::AArch64MMUDriver()
  : pageTables(), bytesAllocated(0), kernel(nullptr)
{
}

AArch64MMUDriver::~AArch64MMUDriver()
{
  if (not pageTables.empty())
    std::cerr << "MMUDriver: error: kernel did not release all page tables."
              << std::endl;
}


void
AArch64MMUDriver::setHostKernel(OSKernel *kernel)
{
  this->kernel = kernel;
}

uint64_t
AArch64MMUDriver::getPageSize(void) const
{
  return pageSize;
}

void
AArch64MMUDriver::allocatePageTable(const uint64_t PID)
{
  TableEntry *table = reinterpret_cast<TableEntry *>
      (kernel->allocateMemory(2 * sizeof(TableEntry), pageTableAlign));
  bytesAllocated += 2 * sizeof(TableEntry);

  for (int i = 0; i < 2; ++i) {
    table[i].valid = 0;
    table[i].dirty = 0;
  }

  pageTables.emplace(PID, table);
}

void
AArch64MMUDriver::releasePageTable(const uint64_t PID)
{
  for (int it0 = 0; it0 < 2; it0++){
    TableEntry *Table_0 = pageTables[PID];
    if (Table_0[it0].valid == 1){
      TableEntry *Table_1 = reinterpret_cast<TableEntry*>(Table_0[it0].physicalPage << pageBits);
      for (int it1 = 0; it1 < (int)entries; it1++){
        if (Table_1[it1].valid == 1){
          TableEntry *Table_2 = reinterpret_cast<TableEntry*>(Table_1[it1].physicalPage << pageBits);
          for (int it2 = 0; it2 < (int)entries; it2++){
            if (Table_2[it2].valid == 1){
              TableEntry *Table_3 = reinterpret_cast<TableEntry*>(Table_2[it2].physicalPage << pageBits);
              for (int it3 = 0; it3 < (int)entries; it3++){
                if (Table_3[it3].valid == 1){
                  kernel->releaseMemory(reinterpret_cast<void*>(&Table_3[it3]), 1 * sizeof(TableEntry));
                }
              }
              kernel->releaseMemory(reinterpret_cast<void*>(Table_2[it2].physicalPage << pageBits), entries * sizeof(TableEntry));
            }
          }
          kernel->releaseMemory(reinterpret_cast<void*>(Table_1[it1].physicalPage << pageBits), entries * sizeof(TableEntry));
        }
      }
      kernel->releaseMemory(reinterpret_cast<void*>(Table_0[it0].physicalPage << pageBits), entries*sizeof(TableEntry));
    }
  }
  auto it = pageTables.find(PID);
  kernel->releaseMemory(it->second, 2 * sizeof(TableEntry));
  pageTables.erase(it);
}

uintptr_t
AArch64MMUDriver::getPageTable(const uint64_t PID)
{
  auto kv = pageTables.find(PID);
  if (kv == pageTables.end())
    return 0x0;
  /* else */
  return reinterpret_cast<uintptr_t>(kv->second);
}

void
AArch64MMUDriver::setMapping(const uint64_t PID,
                            uintptr_t vAddr,
                            PhysPage &pPage)
{
  // std::cout << "Starting SetMapping: ";
  /* Ensure unused address bits are zero */
  vAddr &= (1UL << addressSpaceBits) - 1;
  const uint64_t vPage = vAddr >> pageBits;
  const uint64_t mask = (1 << 11) -1;             // 11-bit bitmask
  const uint64_t level_3 = vPage & mask;          // bits 0-10
  const uint64_t level_2 = (vPage >> 11) & mask;  // bits 11-21
  const uint64_t level_1 = (vPage >> 22) & mask;  // bits 22-32
  const uint64_t level_0 = (vPage >> 33) & 1;     // bit 33 

  TableEntry *Table_0 = pageTables[PID];
  if (Table_0[level_0].valid == 0){
    TableEntry *table = reinterpret_cast<TableEntry *>
      (kernel->allocateMemory(entries * sizeof(TableEntry), pageTableAlign));
    bytesAllocated += entries * sizeof(TableEntry);
    Table_0[level_0].physicalPage = reinterpret_cast<uint64_t>(table) >> pageBits;
    Table_0[level_0].valid = 1;
    // std::cout << "0, ";
  }
  
  TableEntry *Table_1 = reinterpret_cast<TableEntry*>(Table_0[level_0].physicalPage << pageBits);
  if (Table_1[level_1].valid == 0){
    TableEntry *table = reinterpret_cast<TableEntry *>
      (kernel->allocateMemory(entries * sizeof(TableEntry), pageTableAlign));
    bytesAllocated += entries * sizeof(TableEntry);
    Table_1[level_1].physicalPage = reinterpret_cast<uint64_t>(table) >> pageBits;
    Table_1[level_1].valid = 1;
    // std::cout << "1, ";
  }
  TableEntry *Table_2 = reinterpret_cast<TableEntry*>(Table_1[level_1].physicalPage << pageBits);

  if (Table_2[level_2].valid == 0){
    TableEntry *table = reinterpret_cast<TableEntry *>
      (kernel->allocateMemory(entries * sizeof(TableEntry), pageTableAlign));
    bytesAllocated += entries * sizeof(TableEntry);
    Table_2[level_2].physicalPage = reinterpret_cast<uint64_t>(table) >> pageBits;
    Table_2[level_2].valid = 1;
    // std::cout << "2, ";
  }
  TableEntry *Table_3 = reinterpret_cast<TableEntry*>(Table_2[level_2].physicalPage << pageBits);

  if (Table_3[level_3].valid == 0){
    initPageTableEntry(Table_3[level_3], pPage.addr);
    pPage.driverData = &Table_3[level_3];
    // std::cout << "3\n";
  }
}

uint64_t
AArch64MMUDriver::getBytesAllocated(void) const
{
  return bytesAllocated;
}

void
AArch64MMUDriver::setPageValid(PhysPage &pPage, bool setting)
{
  TableEntry *entry = reinterpret_cast<TableEntry *>(pPage.driverData);
  if (not entry->valid)
    throw std::runtime_error("Access to invalid page table entry attempted.");

  entry->valid = setting;
}
