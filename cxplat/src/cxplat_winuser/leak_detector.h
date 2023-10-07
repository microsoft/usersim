// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

typedef class cxplat_leak_detector_t
{
  public:
    cxplat_leak_detector_t() = default;
    ~cxplat_leak_detector_t() = default;

    void
    register_allocation(uintptr_t address, size_t size);

    void
    unregister_allocation(uintptr_t address);

    void
    dump_leaks();

  private:
    void
    flush_output(std::ostringstream& output);

    void
    output_stack_trace(std::ostringstream& output, std::string label, unsigned long stack_hash);

    typedef struct allocation_t
    {
        uintptr_t address;
        size_t size;
        unsigned long alloc_stack_hash;
        unsigned long free_stack_hash;
    } allocation_t;

    std::unordered_map<unsigned long, std::vector<uintptr_t>> _stack_hashes;
    std::unordered_map<uintptr_t, allocation_t> _allocations;
    std::unordered_map<uintptr_t, allocation_t> _freed_allocations;
    std::mutex _mutex;
    const size_t _stack_depth = 32;
    std::vector<std::string> _in_memory_log;
} cxplat_leak_detector_t;

typedef std::unique_ptr<cxplat_leak_detector_t> cxplat_leak_detector_ptr;
