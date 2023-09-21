// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "cxplat_fault_injection.h"
#include "symbol_decoder.h"

#include <DbgHelp.h>
#include <cstddef>
#include <fstream>
#include <map>
#include <mutex>
#include <psapi.h>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

/**
 * @brief This class is used to track potential fault points and fail them in
 * a deterministic manner. Increasing the number of stack frames examined will
 * increase the accuracy of the test, but also increase the time it takes to run
 * the test.
 */
typedef class _cxplat_fault_injection
{
  public:
    /**
     * @brief Construct a new fault injection object.
     * @param[in] stack_depth The number of stack frames to compare when tracking faults.
     */
    _cxplat_fault_injection(size_t stack_depth);

    /**
     * @brief Destroy the fault injection object.
     */
    ~_cxplat_fault_injection();

    bool
    inject_fault();

    /**
     * @brief Reset the fault injection state, both in memory and on disk.
     */
    void
    reset();

    void
    add_module_under_test(uintptr_t module_base_address, size_t module_size)
    {
        std::unique_lock lock(_mutex);
        auto new_module = std::make_pair(module_base_address, module_base_address + module_size);
        // On first insertion, the count will be 0.
        _modules_under_test[new_module]++;
    }

    void
    remove_module_under_test(uintptr_t module_base_address, size_t module_size)
    {
        std::unique_lock lock(_mutex);
        auto module_key = std::make_pair(module_base_address, module_base_address + module_size);
        if (_modules_under_test.contains(module_key)) {
            _modules_under_test[module_key]--;
            if (_modules_under_test[module_key] == 0) {
                _modules_under_test.erase(module_key);
            }
        }
    }

  private:
    /**
     * @brief Compute a hash over the current stack.
     */
    struct _stack_hasher
    {
        size_t
        operator()(const std::vector<uintptr_t>& key) const
        {
            size_t hash_value = 0;
            for (const auto value : key) {
                hash_value ^= std::hash<uintptr_t>{}(value);
            }
            return hash_value;
        }
    };

    /**
     * @brief Determine if this path is new.
     * If it is new, then inject the fault, add it to the set of known
     * fault paths and return true.
     */
    bool
    is_new_stack();

    /**
     * @brief Write the current stack to the log file.
     */
    void
    log_stack_trace(const std::vector<uintptr_t>& canonical_stack, const std::vector<uintptr_t>& stack);

    /**
     * @brief Load the list of known faults from the log file.
     */
    void
    load_fault_log();

    /**
     * @brief Find the base address of the module containing the given address or 0 if not found.
     *
     * @param[in] address Address to find the base address for.
     * @return Base address of the module containing the given address or 0 if not found.
     */
    uintptr_t
    find_base_address(uintptr_t address);

    /**
     * @brief Base address and size of the modules being tested.
     */
    std::map<std::pair<uintptr_t, uintptr_t>, size_t> _modules_under_test;

    /**
     * @brief The iteration number of the current test pass.
     */
    size_t _iteration = 0;

    /**
     * @brief The log file for faults that have been injected.
     */
    _Guarded_by_(_mutex) std::ofstream _log_file;

    /**
     * @brief The set of known fault paths.
     */
    _Guarded_by_(_mutex) std::unordered_set<std::vector<uintptr_t>, _stack_hasher> _fault_hash;

    /**
     * @brief The mutex to protect the set of known fault paths.
     */
    std::mutex _mutex;

    size_t _stack_depth;
    _Guarded_by_(_mutex) std::vector<std::string> _last_fault_stack;

    std::string _log_file_name;

} cxplat_fault_injection_t;

static std::unique_ptr<cxplat_fault_injection_t> _cxplat_fault_injection_singleton;

// Link with DbgHelp.lib
#pragma comment(lib, "dbghelp.lib")

/**
 * @brief The number of stack frames to write to the human readable log.
 */
#define CXPLAT_FAULT_STACK_CAPTURE_FRAME_COUNT 16

/**
 * @brief The number of stack frames to capture to uniquely identify an fault path.
 */
#define CXPLAT_FAULT_STACK_CAPTURE_FRAME_COUNT_FOR_HASH 4

/**
 * @brief Thread local storage to track recursing from the fault injection callback.
 */
static thread_local int _cxplat_fault_injection_recursion = 0;

/**
 * @brief Class to automatically increment and decrement the recursion count.
 */
class cxplat_fault_injection_recursion_guard
{
  public:
    cxplat_fault_injection_recursion_guard() { _cxplat_fault_injection_recursion++; }
    ~cxplat_fault_injection_recursion_guard() { _cxplat_fault_injection_recursion--; }
    /**
     * @brief Return true if the current thread is recursing from the fault injection callback.
     * @retval true The current thread is recursing from the fault injection callback.
     * @retval false The current thread is not recursing from the fault injection callback.
     */
    bool
    is_recursing()
    {
        return (_cxplat_fault_injection_recursion > 1);
    }
};

_cxplat_fault_injection::_cxplat_fault_injection(size_t stack_depth) : _stack_depth(stack_depth)
{
    if (_stack_depth == 0) {
        _stack_depth = CXPLAT_FAULT_STACK_CAPTURE_FRAME_COUNT_FOR_HASH;
    }

    // Get the path to the executable being run.
    char process_name[MAX_PATH];
    GetModuleFileNameA(nullptr, process_name, MAX_PATH);

    _log_file_name = process_name + std::string(".fault.log");
    load_fault_log();
}

_cxplat_fault_injection::~_cxplat_fault_injection()
{
    _log_file.flush();
    _log_file.close();
}

bool
_cxplat_fault_injection::inject_fault()
{
    return is_new_stack();
}

void
_cxplat_fault_injection::reset()
{
    std::unique_lock lock(_mutex);
    _fault_hash.clear();

    // Reset the iteration number.
    _iteration = 0;

    // Close and reopen the log file to clear it.
    _log_file.close();
    _log_file.open(_log_file_name, std::ios::out | std::ios::trunc);
}

bool
_cxplat_fault_injection::is_new_stack()
{
    // Prevent infinite recursion during fault injection.
    cxplat_fault_injection_recursion_guard recursion_guard;
    if (recursion_guard.is_recursing()) {
        return false;
    }
    bool new_stack = false;

    std::vector<uintptr_t> stack(CXPLAT_FAULT_STACK_CAPTURE_FRAME_COUNT);
    std::vector<uintptr_t> canonical_stack;

    unsigned long hash;
    // Capture CXPLAT_FAULT_STACK_CAPTURE_FRAME_COUNT_FOR_HASH frames of the current stack trace.
    if (CaptureStackBackTrace(
            0, static_cast<unsigned int>(stack.size()), reinterpret_cast<void**>(stack.data()), &hash) > 0) {
        // Form the canonical stack.
        for (size_t i = 0; i < _stack_depth; i++) {
            uintptr_t frame = stack[i];
            uintptr_t base_address = find_base_address(frame);
            // Only consider frames in the modules being tested.
            if (base_address) {
                canonical_stack.push_back(stack[i] - base_address);
            }
        }

        std::unique_lock lock(_mutex);
        // Check if the stack trace is already in the hash.
        if (!_fault_hash.contains(canonical_stack)) {
            _fault_hash.insert(canonical_stack);
            new_stack = true;
        }
    }
    if (new_stack) {
        log_stack_trace(canonical_stack, stack);
    }

    return new_stack;
}

void
_cxplat_fault_injection::log_stack_trace(
    const std::vector<uintptr_t>& canonical_stack, const std::vector<uintptr_t>& stack)
{
    // Decode stack trace outside of the lock.
    std::ostringstream log_record;
    for (auto i : canonical_stack) {
        log_record << std::hex << i << " ";
    }
    log_record << std::endl;

    std::vector<std::string> local_last_fault_stack;

    for (auto frame : stack) {
        std::string name;
        std::string string_stack_frame;
        uint64_t displacement;
        std::optional<uint32_t> line_number;
        std::optional<std::string> file_name;
        if (frame == 0) {
            break;
        }
        log_record << "# ";
        if (CXPLAT_SUCCEEDED(_cxplat_decode_symbol(frame, name, displacement, line_number, file_name))) {
            log_record << std::hex << frame << " " << name << " + " << displacement;
            string_stack_frame = name + " + " + std::to_string(displacement);
            if (line_number.has_value() && file_name.has_value()) {
                log_record << " " << file_name.value() << " " << line_number.value();
                string_stack_frame += " " + file_name.value() + " " + std::to_string(line_number.value());
            }
        }
        log_record << std::endl;
        local_last_fault_stack.push_back(string_stack_frame);
    }
    log_record << std::endl;

    {
        std::unique_lock lock(_mutex);
        _last_fault_stack = local_last_fault_stack;
        _log_file << log_record.str();
        // Flush the file after every write to prevent loss on crash.
        _log_file.flush();
    }
}

void
_cxplat_fault_injection::load_fault_log()
{
    {
        std::ifstream fault_log(_log_file_name);
        std::string line;
        std::string frame;
        while (std::getline(fault_log, line)) {
            // Count the iterations to correlate crashes with the last failed fault.
            if (line.starts_with("# Iteration: ")) {
                _iteration++;
                continue;
            }
            // Skip the stack trace.
            if (line.starts_with("#")) {
                continue;
            }
            // Parse the stack frame.
            std::vector<uintptr_t> stack;
            auto stream = std::istringstream(line);
            while (std::getline(stream, frame, ' ')) {
                stack.push_back(std::stoull(frame, nullptr, 16));
            }
            _fault_hash.insert(stack);
        }
        fault_log.close();
    }

    // Re-open the log file in append mode to record the faults that are failed in this run.
    _log_file.open(_log_file_name, std::ios_base::app);

    // Add the current iteration number to the log file.
    _log_file << "# Iteration: " << ++_iteration << std::endl;
}

uintptr_t
_cxplat_fault_injection::find_base_address(uintptr_t address)
{

    // Determine which module this offset is in.
    // The _modules_under_test set is sorted by start and end offset of the module.
    // The lower_bound function returns the first entry where the (start, end) offset is >=
    // (offset, 0). Because this range has an invalid end offset, it will never be an exact match
    // and will always return the first module that starts after the offset.

    auto iter = _modules_under_test.lower_bound(std::make_pair(address, 0));

    // Boundary conditions are:
    // 1. The offset is before the first module -> iter == _modules_under_test.begin()
    // 2. The offset is after the last module -> iter == _modules_under_test.end()

    // _modules_under_test cannot be empty because there is at least one module.

    if (iter == _modules_under_test.begin()) {
        // The offset is before the first module.
        return 0;
    }

    // Select the previous module.
    iter--;

    auto& module = iter->first;

    // Check if the offset is in the module.
    return (address >= module.first && address < module.first + module.second) ? module.first : 0;
}

cxplat_status_t
cxplat_fault_injection_initialize(size_t stack_depth) noexcept
{
    if (_cxplat_fault_injection_singleton) {
        return CXPLAT_STATUS_INVALID_STATE;
    }

    try {
        _cxplat_fault_injection_singleton = std::make_unique<_cxplat_fault_injection>(stack_depth);
    } catch (...) {
        return CXPLAT_STATUS_NO_MEMORY;
    }
    return CXPLAT_STATUS_SUCCESS;
}

void
cxplat_fault_injection_uninitialize() noexcept
{
    _cxplat_fault_injection_singleton.reset();
}

bool
cxplat_fault_injection_inject_fault() noexcept
{
    try {
        if (_cxplat_fault_injection_singleton) {
            return _cxplat_fault_injection_singleton->inject_fault();
        }
        return false;
    } catch (...) {
        return false;
    }
}

bool
cxplat_fault_injection_is_enabled() noexcept
{
    return _cxplat_fault_injection_singleton != nullptr;
}

void
cxplat_fault_injection_reset() noexcept
{
    if (_cxplat_fault_injection_singleton) {
        _cxplat_fault_injection_singleton->reset();
    }
}

cxplat_status_t
cxplat_fault_injection_add_module(_In_ void* module_under_test) noexcept
{
    try {
        if (_cxplat_fault_injection_singleton) {
            MODULEINFO module_info = {0};
            if (module_under_test == nullptr) {
                // If the module under test is not specified, use the current process.
                module_under_test = GetModuleHandle(nullptr);
            }
            if (!GetModuleInformation(
                    GetCurrentProcess(),
                    reinterpret_cast<const HMODULE>(module_under_test),
                    &module_info,
                    sizeof(module_info))) {
                throw std::runtime_error("GetModuleInformation failed");
            }
            _cxplat_fault_injection_singleton->add_module_under_test(
                reinterpret_cast<uintptr_t>(module_info.lpBaseOfDll), module_info.SizeOfImage);
        }
        return CXPLAT_STATUS_SUCCESS;
    } catch (...) {
        return CXPLAT_STATUS_NO_MEMORY;
    }
}

cxplat_status_t
cxplat_fault_injection_remove_module(void* module_under_test) noexcept
{
    try {
        if (_cxplat_fault_injection_singleton) {
            MODULEINFO module_info = {0};
            if (module_under_test == nullptr) {
                // If the module under test is not specified, use the current process.
                module_under_test = GetModuleHandle(nullptr);
            }
            if (!GetModuleInformation(
                    GetCurrentProcess(),
                    reinterpret_cast<HMODULE>(module_under_test),
                    &module_info,
                    sizeof(module_info))) {
                throw std::runtime_error("GetModuleInformation failed");
            }
            _cxplat_fault_injection_singleton->remove_module_under_test(
                reinterpret_cast<uintptr_t>(module_info.lpBaseOfDll), module_info.SizeOfImage);
        }
        return CXPLAT_STATUS_SUCCESS;
    } catch (...) {
        return CXPLAT_STATUS_NO_MEMORY;
    }
}
