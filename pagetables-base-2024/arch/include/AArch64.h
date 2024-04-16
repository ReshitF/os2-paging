/* pagetables -- A framework to experiment with memory management
 *
 *    AArch64.h - A AArch64, 4-level page table to serve as example.
 *
 * Copyright (C) 2017-2020 Leiden University, The Netherlands.
 */

// Authors:
// R. Fazlija (s3270831)
// A. Kooiker (s2098199)


#ifndef __ARCH_AARCH64__
#define __ARCH_AARCH64__

#include "mmu.h"
#include "oskernel.h"

#include <map>

namespace AArch64 {

/* Like in x86_64, we consider that only 48 bits of the virtual address
 * may be used.
 */
const static uint64_t addressSpaceBits = 48;

/* Table entry definitions and assorted constants. */
const static uint64_t pageBits = 14; /* 16 KiB / page */
const static uint64_t pageSize = 1UL << pageBits;

/* Page tables contain 2^10 entries and are 2^12 bytes in size.
 * log2(sizeof(TableEntry)) = 2.
 */
const static uint64_t pageTableAlign = pageSize;

struct __attribute__ ((__packed__)) TableEntry
{
  uint8_t valid : 1;
  uint8_t read : 1;
  uint8_t write : 1;

  uint8_t dirty : 1;
  uint8_t referenced : 1;

  uint32_t reserved : 25;

  uint64_t physicalPage : 34;
};

/*
 * MMU hardware part (arch/AArch64/mmu.cc)
 */

class AArch64MMU : public MMU
{
  public:
    AArch64MMU();
    virtual ~AArch64MMU();

    virtual uint8_t getPageBits(void) const override
    {
      return pageBits;
    }

    virtual uint64_t getPageSize(void) const override
    {
      return pageSize;
    }

    virtual uint8_t getAddressSpaceBits(void) const override
    {
      return addressSpaceBits;
    }

    virtual bool performTranslation(const uint64_t vPage,
                                    uint64_t &pPage,
                                    bool isWrite) override;
};


/*
 * OS driver part (arch/AArch64/driver.cc)
 */

class AArch64MMUDriver : public MMUDriver
{
  protected:
    std::map<uint64_t, TableEntry *> pageTables;
    uint64_t bytesAllocated;
    OSKernel *kernel;  /* no ownership */

  public:
    AArch64MMUDriver();
    virtual ~AArch64MMUDriver() override;

    virtual void      setHostKernel(OSKernel *kernel) override;

    virtual uint64_t  getPageSize(void) const override;

    virtual void      allocatePageTable(const uint64_t PID) override;
    virtual void      releasePageTable(const uint64_t PID) override;
    virtual uintptr_t getPageTable(const uint64_t PID) override;

    virtual void      setMapping(const uint64_t PID,
                                 uintptr_t vAddr,
                                 PhysPage &pPage) override;

    virtual void      setPageValid(PhysPage &pPage,
                                   bool setting) override;

    virtual uint64_t  getBytesAllocated(void) const override;

    /* Disallow objects from being copied, since it has a pointer member. */
    AArch64MMUDriver(const AArch64MMUDriver &driver) = delete;
    void operator=(const AArch64MMUDriver &driver) = delete;
};

} /* namespace AArch64 */

#endif /* __ARCH_AARCH64__ */
