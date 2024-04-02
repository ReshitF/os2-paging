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
  int entriesTable0 = 2;
  int entriesNextTables = 2048;
  TableEntry *table0 = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesTable0 * sizeof(TableEntry), pageTableAlign));
  TableEntry *table1 = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  TableEntry *table2 = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  TableEntry *table3 = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  bytesAllocated += (entriesTable0 + 3 * entriesNextTables) * sizeof(TableEntry);

  for (int i = 0; i < (int)entries; ++i)
    {
      table3[i].valid = 1;
      table3[i].dirty = 0;
    }

  pageTables.emplace(PID, table0);
  pageTables.emplace(PID, table1);
  pageTables.emplace(PID, table2);
  pageTables.emplace(PID, table3);

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
