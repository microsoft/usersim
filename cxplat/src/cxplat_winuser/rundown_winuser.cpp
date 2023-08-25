// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"
#include <map>
#include <memory>
#include <mutex>
#include <windows.h>

/***
 * @brief This following class implements a mock of the Windows Kernel's rundown reference implementation.
 * 1) It uses a map to track the number of references to a given cxplat_rundown_reference_t structure.
 * 2) The address of the cxplat_rundown_reference_t structure is used as the key to the map.
 * 3) A single condition variable is used to wait for the ref count of any cxplat_rundown_reference_t structure to reach 0.
 * 4) The class is a singleton and is created during static initialization and destroyed during static destruction.
 */
typedef class _rundown_ref_table
{
  private:
    static std::unique_ptr<_rundown_ref_table> _instance;

  public:
    // The constructor and destructor should be private to ensure that the class is a singleton, but that is not
    // possible because the singleton instance is stored in a unique_ptr which requires the constructor to be public.
    // The instance of the class can be accessed using the instance() method.
    _rundown_ref_table() = default;
    ~_rundown_ref_table() = default;

    /**
     * @brief Get the singleton instance of the rundown ref table.
     *
     * @return The singleton instance of the rundown ref table.
     */
    static _rundown_ref_table&
    instance()
    {
        return *_instance;
    }

    /**
     * @brief Initialize the rundown ref table entry for the given context.
     *
     * @param[in] context The address of a cxplat_rundown_reference_t structure.
     */
    void
    initialize_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Re-initialize the entry if it already exists.
        if (_rundown_ref_counts.find((uint64_t)context) != _rundown_ref_counts.end()) {
            _rundown_ref_counts.erase((uint64_t)context);
        }

        _rundown_ref_counts[(uint64_t)context] = {false, 0};
    }

    /**
     * @brief Reinitialize the rundown ref table entry for the given context.
     *
     * @param[in] context The address of a previously run down cxplat_rundown_reference_t structure.
     */
    void
    reinitialize_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        auto& [rundown, ref_count] = _rundown_ref_counts[(uint64_t)context];

        // Check if the entry is not rundown.
        if (!rundown) {
            throw std::runtime_error("rundown ref table not rundown");
        }

        if (ref_count != 0) {
            throw std::runtime_error("rundown ref table corruption");
        }

        rundown = false;
    }

    /**
     * @brief Acquire a rundown ref for the given context.
     *
     * @param[in] context The address of a cxplat_rundown_reference_t structure.
     * @retval true Rundown has not started.
     * @retval false Rundown has started.
     */
    bool
    acquire_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        // Check if the entry is already rundown.
        if (std::get<0>(_rundown_ref_counts[(uint64_t)context])) {
            return false;
        }

        // Increment the ref count if the entry is not rundown.
        std::get<1>(_rundown_ref_counts[(uint64_t)context])++;

        return true;
    }

    /**
     * @brief Release a rundown ref for the given context.
     *
     * @param[in] context The address of a cxplat_rundown_reference_t structure.
     */
    void
    release_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        if (std::get<1>(_rundown_ref_counts[(uint64_t)context]) == 0) {
            throw std::runtime_error("rundown ref table already released");
        }

        std::get<1>(_rundown_ref_counts[(uint64_t)context])--;

        if (std::get<1>(_rundown_ref_counts[(uint64_t)context]) == 0) {
            _rundown_ref_cv.notify_all();
        }
    }

    /**
     * @brief Wait for the rundown ref count to reach 0 for the given context.
     *
     * @param[in] context The address of a cxplat_rundown_reference_t structure.
     */
    void
    wait_for_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        auto& [rundown, ref_count] = _rundown_ref_counts[(uint64_t)context];
        rundown = true;
        // Wait for the ref count to reach 0.
        _rundown_ref_cv.wait(lock, [&ref_count] { return ref_count == 0; });
    }

  private:
    std::mutex _lock;
    std::map<uint64_t, std::tuple<bool, uint64_t>> _rundown_ref_counts;
    std::condition_variable _rundown_ref_cv;
} rundown_ref_table_t;

/**
 * @brief The singleton instance of the rundown ref table. Created during static initialization and destroyed during
 * static destruction.
 */
std::unique_ptr<_rundown_ref_table> rundown_ref_table_t::_instance = std::make_unique<rundown_ref_table_t>();

void
cxplat_initialize_rundown_protection(_Out_ cxplat_rundown_reference_t* rundown_reference)
{
#pragma warning(push)
#pragma warning(suppress : 6001) // Uninitialized memory. The rundown_ref is used as a key in a map and is not
                                 // dereferenced.
    rundown_ref_table_t::instance().initialize_rundown_ref(rundown_reference);
#pragma warning(pop)
}

void
cxplat_reinitialize_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    rundown_ref_table_t::instance().reinitialize_rundown_ref(rundown_reference);
}

void
cxplat_wait_for_rundown_protection_release(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    rundown_ref_table_t::instance().wait_for_rundown_ref(rundown_reference);
}

int
cxplat_acquire_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    if (rundown_ref_table_t::instance().acquire_rundown_ref(rundown_reference)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void
cxplat_release_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    rundown_ref_table_t::instance().release_rundown_ref(rundown_reference);
}
