/* pagetables -- A framework to experiment with memory management
 *
 *    process.h - Process abstraction
 *
 * Copyright (C) 2017-2020  Leiden University, The Netherlands.
 */

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <fstream>
#include <memory>
#include <list>

#include <stdint.h>

class TraceReader;


/*
 * A process performs memory accesses
 */

enum class MemAccessType
{
  Instr,
  Store,
  Load,
  /* According to the Lackey documentation, a Modify is generated by
   * an instruction that modifies a memory value in place. For example,
   * this is done by "incl (%ecx)".
   */
  Modify
};

struct MemAccess
{
  MemAccessType type;
  uint64_t      addr;
  uint8_t       size;
};

std::ostream &operator<<(std::ostream &stream, const MemAccess &access);


/*
 * Process abstraction.
 */

class Process
{
  private:
    std::unique_ptr<TraceReader> reader;

  public:
    Process(std::istream &input);
    ~Process();

    uint64_t getPID(void) const;

    void getMemoryAccess(MemAccess &access);
    bool finished(void) const;
};

using ProcessList = std::list<std::shared_ptr<Process>>;

#endif /* __PROCESS_H__ */
