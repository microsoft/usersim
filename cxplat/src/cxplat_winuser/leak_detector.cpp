// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "leak_detector.h"
#include "symbol_decoder.h"
#include "usersim/ke.h"

#include <iostream>
#include <sstream>

void
_usersim_leak_detector::register_allocation(uintptr_t address, size_t size)
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
_usersim_leak_detector::unregister_allocation(uintptr_t address)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (!_allocations.contains(address)) {
        KeBugCheck(0);
    }
    _allocations.erase(address);
}

void
_usersim_leak_detector::dump_leaks()
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
            if (_usersim_decode_symbol(address, name, displacement, line_number, file_name) == STATUS_SUCCESS) {
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
    usersim_assert(_allocations.empty());

    _allocations.clear();
    _stack_hashes.clear();
}
