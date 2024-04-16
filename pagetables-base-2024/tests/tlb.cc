/* pagetables -- A framework to experiment with memory management
 *
 *    tests/simple.cc - unit tests for simple page table.
 *
 * Copyright (C) 2022  Leiden University, The Netherlands.
 */

#define BOOST_TEST_MODULE AArch64
#include <boost/test/unit_test.hpp>

#include "arch/include/AArch64.h"
#include "include/settings.h"
using namespace AArch64;

constexpr static uint64_t MemorySize = (1 * 1024 * 1024) << 10;
constexpr static int entries = 2048;


BOOST_AUTO_TEST_SUITE(aarch64_test)

/*
 * Test MMU class separately
 */

struct MMUFixture
{
  AArch64MMU mmu;
  TableEntry *table_0;
  TableEntry *table_1;
  TableEntry *table_2;
  TableEntry *table_3;

  MMUFixture()
    : mmu(), table_0(nullptr), table_1(nullptr), table_2(nullptr), table_3(nullptr)
  {
    /* 
    Allocate page table, by simply setting up a 1 dimensional 
    line of tables, since only addres 0x0 is ever accessed, which does a tablewalk 
    down the 0th index of each table 
    */
    table_0 = new (std::align_val_t{ pageSize }, std::nothrow) TableEntry[2];
    table_1 = new (std::align_val_t{ pageSize }, std::nothrow) TableEntry[entries];
    table_2 = new (std::align_val_t{ pageSize }, std::nothrow) TableEntry[entries];
    table_3 = new (std::align_val_t{ pageSize }, std::nothrow) TableEntry[entries];
    std::fill_n(table_0, 2, TableEntry{ 0 });
    std::fill_n(table_1, entries, TableEntry{ 0 });
    std::fill_n(table_2, entries, TableEntry{ 0 });
    std::fill_n(table_3, entries, TableEntry{ 0 });

    table_0[0].physicalPage = reinterpret_cast<uint64_t>(table_1) >> pageBits;
    table_1[0].physicalPage = reinterpret_cast<uint64_t>(table_2) >> pageBits;
    table_2[0].physicalPage = reinterpret_cast<uint64_t>(table_3) >> pageBits;
    table_0[0].valid = 1;
    table_1[0].valid = 1;
    table_2[0].valid = 1;

    mmu.setPageTablePointer(reinterpret_cast<uintptr_t>(table_0));
  }

  ~MMUFixture()
  {
    operator delete[](table_0, std::align_val_t{ pageSize}, std::nothrow);
  }

  MMUFixture(const MMUFixture &) = delete;
  MMUFixture &operator=(const MMUFixture &) = delete;
};


/* Test empty page table */
BOOST_FIXTURE_TEST_CASE( empty_page_table, MMUFixture )
{
  /* Traverse through address space in increments of 1/4 pages, all
   * translations should fail silently.
   */
  for (uintptr_t addr = 0x0; addr < 16 * pageSize; addr += pageSize / 4)
    {
      MemAccess access{ .type = MemAccessType::Load, .addr = addr, .size = 8 };
      uint64_t pAddr = 0;
      BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), false);
    }
}

/* Test whether MMU correctly translates page numbers */
BOOST_FIXTURE_TEST_CASE( mmu_translate_page_numbers, MMUFixture )
{
  int nLookups{}, nHits{}, nEvictions{}, nFlush{}, nFlushEvictions{};

  // Initialize final table with empty values, except for 2 examples that are not empty.
  for (int i = 0; i < entries; i++){
    table_3[i].valid = true;
    table_3[i].physicalPage = i;
  }
  table_3[0].physicalPage = 0xf00;
  table_3[6].physicalPage = 0xa00;

  /* Access virtual page 0x0, should not hit and increase nlookups to 1 */
  MemAccess access{ .type = MemAccessType::Load, .addr = 0, .size = 8 };
  uint64_t pAddr = 0;
  BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), true);
  BOOST_CHECK_EQUAL(pAddr, 0xf00UL << pageBits);
  
  mmu.getTLBStatistics(nLookups, nHits, nEvictions, nFlush, nFlushEvictions);
  BOOST_CHECK_EQUAL(nLookups, 1);
  BOOST_CHECK_EQUAL(nHits, 0);

  /* Access virtual page 0x0 again, which should HIT in the tlb. */
  BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), true);
  mmu.getTLBStatistics(nLookups, nHits, nEvictions, nFlush, nFlushEvictions);
  BOOST_CHECK_EQUAL(nLookups, 2);
  BOOST_CHECK_EQUAL(nHits, 1);

  /* Access virtual page 0x6, won't hit in the tlb, but will increase nlookups */
  access.addr = 0x6 << pageBits;
  BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), true);
  mmu.getTLBStatistics(nLookups, nHits, nEvictions, nFlush, nFlushEvictions);
  BOOST_CHECK_EQUAL(pAddr, 0xa00UL << pageBits);
  BOOST_CHECK_EQUAL(nLookups, 3);
  BOOST_CHECK_EQUAL(nHits, 1);

  // Context switch, but ASID is enabled by default, so no flushes happen
  mmu.setASIDEnabled(true);
  mmu.setPageTablePointer(reinterpret_cast<uintptr_t>(table_1));

  // Normally excuted inside interruptHandler, but test implementation 
  // is very rudementary. Instead, we copy the relevant code straight 
  // into this test case, as the program would normally execute it.
  if (!mmu.getASIDEnabled()){
    mmu.flush();
  }

  mmu.getTLBStatistics(nLookups, nHits, nEvictions, nFlush, nFlushEvictions);
  BOOST_CHECK_EQUAL(nLookups, 3);
  BOOST_CHECK_EQUAL(nHits, 1);
  BOOST_CHECK_EQUAL(nEvictions, 0);
  BOOST_CHECK_EQUAL(nFlush, 0);
  BOOST_CHECK_EQUAL(nFlushEvictions, 0);

  // Test is the TLB flushes when ASID is disabled (it should!)
  mmu.setASIDEnabled(false);
  mmu.setPageTablePointer(reinterpret_cast<uintptr_t>(table_0));

  // Normally excuted inside interruptHandler, but test implementation 
  // is very rudementary. Instead, we copy the relevant code straight 
  // into this test case, as the program would normally execute it.
  if (!mmu.getASIDEnabled()){
    mmu.flush();
  }

  mmu.getTLBStatistics(nLookups, nHits, nEvictions, nFlush, nFlushEvictions);
  BOOST_CHECK_EQUAL(nLookups, 3);
  BOOST_CHECK_EQUAL(nHits, 1);
  BOOST_CHECK_EQUAL(nEvictions, 2);
  BOOST_CHECK_EQUAL(nFlush, 1);
  BOOST_CHECK_EQUAL(nFlushEvictions, 2);

  // Accessing a bunch of addresses, until tlb is entirely filled. The 33rd 
  // address should overflow the standard 32-sized buffer, resulting in 1 extra eviction.
  for (int i = 0; i < (int)TLBEntries+1; i ++){
    access.addr = i << pageBits;
    mmu.getTranslation(access, pAddr);
    mmu.getTLBStatistics(nLookups, nHits, nEvictions, nFlush, nFlushEvictions);
    if (i != (int)TLBEntries){
      BOOST_CHECK_EQUAL(nEvictions, 2);
    } else {
      BOOST_CHECK_EQUAL(nEvictions, 3);
    }
  }
  
}

BOOST_AUTO_TEST_SUITE_END()
