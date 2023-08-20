// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "leak_detector.h"
#include "symbol_decoder.h"

#include <iostream>
#include <sstream>

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
            &allocation.stack_hash) == 0) {
        allocation.stack_hash = 0;
    }
    _allocations[address] = allocation;
    if (!_stack_hashes.contains(allocation.stack_hash)) {
        _stack_hashes[allocation.stack_hash] = stack;
    }
}

void
_cxplat_leak_detector::unregister_allocation(uintptr_t address)
{
    std::unique_lock<std::mutex> lock(_mutex);
    CXPLAT_RUNTIME_ASSERT(_allocations.contains(address));
    _allocations.erase(address);
}

void
_cxplat_leak_detector::dump_leaks()
{
    std::unique_lock<std::mutex> lock(_mutex);
    for (auto& allocation : _allocations) {
        std::ostringstream output;
        std::vector<uintptr_t> stack = _stack_hashes[allocation.second.stack_hash];
        output << "Leak of " << allocation.second.size << " bytes at " << allocation.second.address << std::endl;
        _in_memory_log.push_back(output.str());
        std::cout << output.str();
        output.str("");
        std::string name;
        uint64_t displacement;
        std::optional<uint32_t> line_number;
        std::optional<std::string> file_name;
        for (auto address : stack) {
            if (CXPLAT_SUCCEEDED(_cxplat_decode_symbol(address, name, displacement, line_number, file_name))) {
                output << "    " << name << " + " << displacement;
                if (line_number.has_value() && file_name.has_value()) {
                    output << " (" << file_name.value() << ":" << line_number.value() << ")";
                }
                output << std::endl;
            }
            _in_memory_log.push_back(output.str());
            std::cout << output.str();
            output.str("");
        }
        _in_memory_log.push_back(output.str());
        std::cout << output.str();
        output.str("");
    }

    // assert to make sure that a leaking test throws an exception thereby failing the test.
    CXPLAT_DEBUG_ASSERT(_allocations.empty());

    _allocations.clear();
    _stack_hashes.clear();
}
