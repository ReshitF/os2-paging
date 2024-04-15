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

const static uint64_t entries = 1UL << (addressSpaceBits - pageBits);

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
  bytesAllocated += sizeof(table);

  for (int i = 0; i < 2; ++i) {
    table[i].valid = 0;
    table[i].dirty = 0;
  }

  pageTables.emplace(PID, table);
}

void
AArch64MMUDriver::releasePageTable(const uint64_t PID)
{
  auto it = pageTables.find(PID);
  kernel->releaseMemory(it->second, entries * sizeof(TableEntry));
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
  std::cout << "Starting SetMapping\n";
  /* Ensure unused address bits are zero */
  vAddr &= (1UL << addressSpaceBits) - 1;
  const uint64_t vPage = vAddr >> pageBits;
  const uint64_t mask = (1 << 11) -1;             // 11-bit bitmask
  const uint64_t level_3 = vPage & mask;          // bits 0-10
  const uint64_t level_2 = (vPage >> 11) & mask;  // bits 11-21
  const uint64_t level_1 = (vPage >> 22) & mask;  // bits 22-32
  const uint64_t level_0 = (vPage >> 33) & 1;     // bit 33 
  if (pageTables[PID][level_0].dirty == 0){
     // doe wat
     TableEntry *table = reinterpret_cast<TableEntry *>
      (kernel->allocateMemory(2048 * sizeof(TableEntry), pageTableAlign));
    table[412].dirty = 1;
    pageTables[PID][level_0].physicalPage = reinterpret_cast<uint64_t>(table) >> pageBits;

    
    TableEntry *temp = reinterpret_cast<TableEntry *>(pageTables[PID][level_0].physicalPage << pageBits);
    std::cout << "Our set dirty bit: " << (int)temp[412].dirty << "\n";
    std::cout << "NOT Our set dirty bit: " << (int)temp[410].dirty << "\n";

  } else {
    std::cout << "Valid was true\n";
    TableEntry *nextLevel = reinterpret_cast<TableEntry *>(pageTables[PID][level_0].physicalPage);
  }

  initPageTableEntry(pageTables[PID][0], pPage.addr);
  pPage.driverData = &pageTables[PID][0];
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
