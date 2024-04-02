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

// const static uint64_t entries = 1UL << (addressSpaceBits - pageBits);
const static uint64_t entries = 2; // DELETE THIS: TEMPORARILY SET TO 2 FOR SIMPLE IMPLEMENTATION PLEASE DONT FORGET :)

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
      (kernel->allocateMemory(entries * sizeof(TableEntry), pageTableAlign));
  bytesAllocated += entries * sizeof(TableEntry);

  for (int i = 0; i < (int)entries; ++i)
    {
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
  /* Ensure unused address bits are zero */
  vAddr &= (1UL << addressSpaceBits) - 1;

  int entry = vAddr / pageSize;

  initPageTableEntry(pageTables[PID][entry], pPage.addr);
  pPage.driverData = &pageTables[PID][entry];
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
