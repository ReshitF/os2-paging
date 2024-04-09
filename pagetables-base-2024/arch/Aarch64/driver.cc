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
  // TableEntry *table0 = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesTable0 * sizeof(TableEntry), pageTableAlign));
  // for (int level0 = 0; level0 < entriesTable0; level0++){
  //   table0[level0] = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  //   for (int level1 = 0; level1 < entriesNextTables; level1++){
  //     table0[level0][level1] = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  //     for (int level2 = 0; level2 < entriesNextTables; level2++){
  //       table0[level0][level1][level2] = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  //       for (int level3 = 0; level3 < entriesNextTables; level3++){
  //         table0[level0][level1][level2][level3] = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  //       }
  //     }
  //   }
  // }
  // TableEntry *table3 = reinterpret_cast<TableEntry *> (kernel->allocateMemory(entriesNextTables * sizeof(TableEntry), pageTableAlign));
  // TableEntry* table[entriesTable0][entriesNextTables][entriesNextTables][entriesNextTables];
  // for (int level0 = 0; level0 < entriesTable0; level0++){
  //   for (int level1 = 0; level1 < entriesNextTables; level1++){
  //     for (int level2 = 0; level2 < entriesNextTables; level2++){
  //       for (int level3 = 0; level3 < entriesNextTables; level3++){
  //         table[level0][level1][level2][level3] = reinterpret_cast<TableEntry *> (kernel->allocateMemory(sizeof(TableEntry), pageTableAlign));
  //       }
  //     }
  //   }
  // }

  TableEntry *table = reinterpret_cast<TableEntry *>
      (kernel->allocateMemory(entriesTable0 * entriesNextTables * entriesNextTables * entriesNextTables * sizeof(TableEntry), pageTableAlign));
  bytesAllocated += sizeof(table);

  // for (int i = 0; i < (int)entries; ++i)
  //   {
  //     table0[i].valid = 0;
  //     table3[i].dirty = 0;
  //   }
  for (int level0 = 0; level0 < entriesTable0; level0++){
    for (int level1 = 0; level1 < entriesNextTables; level1++){
      for (int level2 = 0; level2 < entriesNextTables; level2++){
        for (int level3 = 0; level3 < entriesNextTables; level3++){
          table[level0*level1*level2*level3].valid = 0;
          table[level0*level1*level2*level3].dirty = 0;
        }
      }
    }
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
  const uint64_t vPage = vAddr >> pageBits;
  const uint64_t mask = (1 << 11) -1;             // 11-bit bitmask
  const uint64_t level_3 = vPage & mask;          // bits 0-10
  const uint64_t level_2 = (vPage >> 11) & mask;  // bits 11-21
  const uint64_t level_1 = (vPage >> 22) & mask;  // bits 22-32
  const uint64_t level_0 = (vPage >> 33) & 1;     // bit 33 

  initPageTableEntry(pageTables[PID][level_0*2048*2048*2048 + level_1*2048*2048 + level_2*2048 + level_3], pPage.addr);
  pPage.driverData = &pageTables[PID][level_0*2048*2048*2048 + level_1*2048*2048 + level_2*2048 + level_3];
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
