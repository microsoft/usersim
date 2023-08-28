// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "leak_detector.h"
#include "symbol_decoder.h"

#include <iostream>

void
_cxplat_leak_detector::register_allocation(uintptr_t address, size_t size)
{
    std::unique_lock<std::mutex> lock(_mutex);
    allocation_t allocation = {address, size, 0};
    std::vector<uintptr_t> stack(1 + _stack_depth);
    if (CaptureStackBackTrace(
            1,
            static_cast<unsigned int>(stack.size()),
            reinterpret_cast<void**>(stack.data()),
            &allocation.alloc_stack_hash) == 0) {
        allocation.alloc_stack_hash = 0;
    }
    _allocations[address] = allocation;
    if (!_stack_hashes.contains(allocation.alloc_stack_hash)) {
        _stack_hashes[allocation.alloc_stack_hash] = stack;
    }
}

void
_cxplat_leak_detector::flush_output(std::ostringstream& output)
{
    _in_memory_log.push_back(output.str());
    std::cout << output.str();
    output.str("");
}

void
_cxplat_leak_detector::output_stack_trace(std::ostringstream& output, std::string label, unsigned long stack_hash)
{
    std::vector<uintptr_t> stack = _stack_hashes[stack_hash];
    std::string name;
    uint64_t displacement;
    std::optional<uint32_t> line_number;
    std::optional<std::string> file_name;
    output << "  " << label << " stack:" << std::endl;
    for (auto address : stack) {
        if (CXPLAT_SUCCEEDED(_cxplat_decode_symbol(address, name, displacement, line_number, file_name))) {
            output << "    " << name << " + " << displacement;
            if (line_number.has_value() && file_name.has_value()) {
                output << " (" << file_name.value() << ":" << line_number.value() << ")";
            }
            output << std::endl;
        }
        flush_output(output);
    }
}

void
_cxplat_leak_detector::unregister_allocation(uintptr_t address)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (!_allocations.contains(address)) {
        auto allocation = _freed_allocations[address];
        std::ostringstream output;
        output << "Double-free of " << allocation.size << " bytes at " << allocation.address << std::endl;
        flush_output(output);
        output_stack_trace(output, "Allocation", allocation.alloc_stack_hash);
        output_stack_trace(output, "Free", allocation.free_stack_hash);
    }
    CXPLAT_RUNTIME_ASSERT(_allocations.contains(address));
    _freed_allocations[address] = _allocations[address];
    _allocations.erase(address);

    // Capture stack trace.
    auto& allocation = _freed_allocations[address];
    std::vector<uintptr_t> stack(1 + _stack_depth);
    if (CaptureStackBackTrace(
            1,
            static_cast<unsigned int>(stack.size()),
            reinterpret_cast<void**>(stack.data()),
            &allocation.free_stack_hash) == 0) {
        allocation.free_stack_hash = 0;
    }
    if (!_stack_hashes.contains(allocation.free_stack_hash)) {
        _stack_hashes[allocation.free_stack_hash] = stack;
    }
}

void
_cxplat_leak_detector::dump_leaks()
{
    std::unique_lock<std::mutex> lock(_mutex);
    for (auto& allocation : _allocations) {
        std::ostringstream output;
        output << "Leak of " << allocation.second.size << " bytes at " << allocation.second.address << std::endl;
        flush_output(output);
        output_stack_trace(output, "Allocation", allocation.second.alloc_stack_hash);
    }

    // assert to make sure that a leaking test throws an exception thereby failing the test.
    CXPLAT_DEBUG_ASSERT(_allocations.empty());

    _allocations.clear();
    _freed_allocations.clear();
    _stack_hashes.clear();
}
