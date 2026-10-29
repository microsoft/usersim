// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "net_platform.h"
#include "usersim/fwp_test.h"

#include <shared_mutex>
#include <unordered_map>
#include <vector>

typedef std::unique_lock<std::shared_mutex> exclusive_lock_t;
typedef std::shared_lock<std::shared_mutex> shared_lock_t;

// A WFP filter as stored by the mock engine.
//
// FWPM_FILTER0::providerKey is a pointer into caller-owned memory. Storing the FWPM_FILTER0 by value would
// therefore alias whatever the caller happened to pass, and any later read of providerKey (for example when
// enumerating filters by provider) would dereference memory the caller may already have released. The mock
// deep-copies the GUID into the entry and re-points the stored filter at its own copy. std::unordered_map is
// node-based, so the address of provider_key remains stable across rehashing.
typedef struct _fwpm_filter_entry
{
    FWPM_FILTER0 filter;
    GUID provider_key;
} fwpm_filter_entry_t;

// A WFP callout as stored by the mock engine. FWPM_CALLOUT0::providerKey has the same caller-owned-pointer
// problem as FWPM_FILTER0::providerKey, and is deep-copied for the same reason.
typedef struct _fwpm_callout_entry
{
    FWPM_CALLOUT0 callout;
    GUID provider_key;
} fwpm_callout_entry_t;

// A WFP sub-layer as stored by the mock engine. FWPM_SUBLAYER0::providerKey has the same caller-owned-pointer
// problem as the filter and callout cases, and is deep-copied for the same reason: the reference checks that back
// FWP_E_IN_USE dereference it.
typedef struct _fwpm_sub_layer_entry
{
    FWPM_SUBLAYER0 sub_layer;
    GUID provider_key;
} fwpm_sub_layer_entry_t;

// An in-progress enumeration. Real WFP enumerations are snapshots taken when the enum handle is created, so
// objects deleted while an enumeration is open are still returned and objects added are not. The mock models
// that explicitly, which also makes the common "enumerate everything, then delete each entry" pattern safe.
template <typename T> struct fwpm_enum_state_t
{
    std::vector<T> entries;
    size_t next_index = 0;
};

typedef class fwp_engine_t
{
  public:
    fwp_engine_t() = default;

    void
    set_sublayer_guids(
        _In_ const GUID& default_sublayer, _In_ const GUID& connect_v4_sublayer, _In_ const GUID& connect_v6_sublayer)
    {
        _default_sublayer = default_sublayer;
        _connect_v4_sublayer = connect_v4_sublayer;
        _connect_v6_sublayer = connect_v6_sublayer;
    }

    uint32_t
    add_fwpm_callout(_In_ const FWPM_CALLOUT0* callout)
    {
        exclusive_lock_t l(lock);
        uint32_t id = next_id++;
        auto& stored = fwpm_callouts.insert({id, fwpm_callout_entry_t{*callout, {}}}).first->second;

        // Re-point the stored callout at the entry's own copy of the provider key (see fwpm_callout_entry_t).
        if (callout->providerKey != nullptr) {
            stored.provider_key = *callout->providerKey;
            stored.callout.providerKey = &stored.provider_key;
        } else {
            stored.callout.providerKey = nullptr;
        }

        return id;
    }

    bool
    remove_fwpm_callout(size_t id)
    {
        exclusive_lock_t l(lock);
        return fwpm_callouts.erase(id) == 1;
    }

    _Requires_lock_not_held_(this->lock) NTSTATUS delete_fwpm_callout(_In_ const GUID* key)
    {
        exclusive_lock_t l(lock);

        // Report a missing object as not-found even if some filter still carries the key: an object that does not
        // exist cannot be in use.
        size_t id = 0;
        bool found = false;
        for (auto& [first, entry] : fwpm_callouts) {
            if (memcmp(&entry.callout.calloutKey, key, sizeof(GUID)) == 0) {
                id = first;
                found = true;
                break;
            }
        }

        if (!found) {
            return (NTSTATUS)FWP_E_CALLOUT_NOT_FOUND;
        }

        if (is_callout_referenced_under_lock(key)) {
            return (NTSTATUS)FWP_E_IN_USE;
        }

        fwpm_callouts.erase(id);
        return STATUS_SUCCESS;
    }

    uint32_t
    register_fwps_callout(_In_ const FWPS_CALLOUT3* callout)
    {
        exclusive_lock_t l(lock);
        uint32_t id = next_id++;
        fwps_callouts.insert({id, *callout});
        return id;
    }

    _Requires_lock_held_(this->lock) FWPS_CALLOUT3* get_fwps_callout(_In_ const GUID* callout_key)
    {
        for (auto& it : fwps_callouts) {
            if (memcmp(&it.second.calloutKey, callout_key, sizeof(GUID)) == 0) {
                return &it.second;
            }
        }

        return nullptr;
    }

    _Requires_lock_held_(this->lock) FWPS_CALLOUT3* get_fwps_callout(uint32_t callout_id)
    {
        for (auto& it : fwps_callouts) {
            if (it.first == callout_id) {
                return &it.second;
            }
        }

        return nullptr;
    }

    _Requires_lock_not_held_(this->lock) bool remove_fwps_callout(size_t id)
    {
        exclusive_lock_t l(lock);
        return fwps_callouts.erase(id) == 1;
    }

    _Requires_lock_not_held_(this->lock) void associate_flow_context(
        uint64_t flow_id, uint32_t callout_id, uint64_t flow_context)
    {
        UNREFERENCED_PARAMETER(callout_id);
        exclusive_lock_t l(lock);
        fwpm_flow_contexts.insert({flow_id, flow_context});
    }

    _Requires_lock_not_held_(this->lock) void delete_flow_context(
        uint64_t flow_id, uint16_t layer_id, uint32_t callout_id)
    {
        FWPS_CALLOUT3* callout = nullptr;
        FWPS_FILTER fwps_filter = {};
        uint64_t flow_context = 0;

        {
            exclusive_lock_t l(lock);
            for (auto& it : fwpm_flow_contexts) {
                if (it.first == flow_id) {
                    callout = get_fwps_callout(callout_id);
                    CXPLAT_DEBUG_ASSERT(callout != nullptr);
                    flow_context = it.second;
                    break;
                }
            }

            fwpm_flow_contexts.erase(flow_id);
        }

        CXPLAT_DEBUG_ASSERT(callout != nullptr);
        __analysis_assume(callout != nullptr);
        // Invoke flow delete notification callback.
        callout->flowDeleteFn(layer_id, callout_id, flow_context);
    }

    _Requires_lock_not_held_(this->lock) uint32_t add_fwpm_filter(_In_ const FWPM_FILTER0* filter)
    {
        FWPS_CALLOUT3* callout = nullptr;
        FWPS_FILTER fwps_filter = {};
        uint32_t id;

        {
            exclusive_lock_t l(lock);
            id = next_id++;
            auto& stored = fwpm_filters.insert({id, fwpm_filter_entry_t{*filter, {}}}).first->second;

            // Record the assigned run-time identifier in the stored filter. Real WFP populates FWPM_FILTER0::filterId
            // on add and reports it through enumeration, and it is the only handle a caller that did not add the
            // filter itself has for deleting it.
            stored.filter.filterId = id;

            // Re-point the stored filter at the entry's own copy of the provider key (see fwpm_filter_entry_t).
            if (filter->providerKey != nullptr) {
                stored.provider_key = *filter->providerKey;
                stored.filter.providerKey = &stored.provider_key;
            } else {
                stored.filter.providerKey = nullptr;
            }

            callout = get_fwps_callout(&filter->action.calloutKey);
            CXPLAT_DEBUG_ASSERT(callout != nullptr);
            fwps_filter.filterId = id;
            fwps_filter.context = filter->rawContext;
        }

        __analysis_assume(callout != nullptr);
        // Invoke filter add notification callback.
        callout->notifyFn(FWPS_CALLOUT_NOTIFY_ADD_FILTER, &filter->action.calloutKey, &fwps_filter);

        return id;
    }

    _Requires_lock_not_held_(this->lock) bool remove_fwpm_filter(size_t id)
    {
        FWPS_CALLOUT3* callout = nullptr;
        FWPS_FILTER fwps_filter = {};
        bool return_value = false;
        {
            exclusive_lock_t l(lock);
            for (auto& it : fwpm_filters) {
                if (it.first == id) {
                    // May be null if the callout function has already been unregistered (e.g., during driver
                    // unload); in that case WFP delivers no delete notification (handled below).
                    callout = get_fwps_callout(&it.second.filter.action.calloutKey);
                    fwps_filter.filterId = id;
                    fwps_filter.context = it.second.filter.rawContext;
                    break;
                }
            }

            return_value = fwpm_filters.erase(id) == 1;
        }

        // If the callout function is still registered, deliver the delete notification as real WFP does. Once the
        // callout has been unregistered (e.g., during driver unload), WFP delivers no delete notification.
        if (callout != nullptr) {
            callout->notifyFn(FWPS_CALLOUT_NOTIFY_DELETE_FILTER, &callout->calloutKey, &fwps_filter);
        }

        return return_value;
    }

    // Test-only: remove any WFP filters left in the engine (used to clean up after fault-injection tests that
    // intentionally leave filters undeletable). Does not issue notifications.
    void
    clear_fwpm_filters()
    {
        exclusive_lock_t l(lock);
        fwpm_filters.clear();
    }

    // Arms the deterministic FwpmFilterDeleteById failure counter (see usersim_fwp_set_filter_delete_failure_count
    // in fwp_test.h). Fails the next 'count' deletes; 0 disarms.
    void
    set_filter_delete_failure_count(uint32_t count)
    {
        exclusive_lock_t l(lock);
        _filter_delete_failure_count = count;
    }

    // Returns true (and consumes one) if the next FwpmFilterDeleteById call should be failed for fault injection.
    bool
    consume_filter_delete_failure()
    {
        exclusive_lock_t l(lock);
        if (_filter_delete_failure_count > 0) {
            _filter_delete_failure_count--;
            return true;
        }
        return false;
    }

    // Test-only: number of WFP filters currently present in the engine.
    size_t
    get_fwpm_filter_count()
    {
        shared_lock_t l(lock);
        return fwpm_filters.size();
    }

    // Creates a snapshot of the filters matching the (optional) enumeration template, and returns a handle to it.
    // A null template matches every filter, as it does in real WFP.
    _Requires_lock_not_held_(this->lock) uint64_t
        create_fwpm_filter_enum_handle(_In_opt_ const FWPM_FILTER_ENUM_TEMPLATE0* enum_template)
    {
        exclusive_lock_t l(lock);
        uint64_t handle = next_enum_handle++;
        auto& state = fwpm_filter_enums[handle];
        for (auto& [id, entry] : fwpm_filters) {
            if (!provider_key_matches(entry.filter.providerKey, enum_template ? enum_template->providerKey : nullptr)) {
                continue;
            }
            if (enum_template != nullptr && !is_null_guid(enum_template->layerKey) &&
                memcmp(&entry.filter.layerKey, &enum_template->layerKey, sizeof(GUID)) != 0) {
                continue;
            }

            state.entries.push_back(entry);
            rebind_filter_entry(state.entries.back());
        }
        return handle;
    }

    // Copies up to 'requested' entries from the snapshot into 'out', advancing the enumeration cursor.
    _Requires_lock_not_held_(this->lock) bool next_fwpm_filter_enum_entries(
        uint64_t handle, uint32_t requested, _Inout_ std::vector<fwpm_filter_entry_t>& out)
    {
        exclusive_lock_t l(lock);
        auto it = fwpm_filter_enums.find(handle);
        if (it == fwpm_filter_enums.end()) {
            return false;
        }

        auto& state = it->second;
        while (out.size() < requested && state.next_index < state.entries.size()) {
            out.push_back(state.entries[state.next_index++]);
            rebind_filter_entry(out.back());
        }
        return true;
    }

    _Requires_lock_not_held_(this->lock) bool destroy_fwpm_filter_enum_handle(uint64_t handle)
    {
        exclusive_lock_t l(lock);
        return fwpm_filter_enums.erase(handle) == 1;
    }

    // Rewinds the filter enumeration cursor by 'count' entries. Used when a batch was taken from the snapshot but
    // could not be handed to the caller, so those entries are enumerated again rather than silently skipped.
    _Requires_lock_not_held_(this->lock) void rewind_fwpm_filter_enum(uint64_t handle, size_t count)
    {
        exclusive_lock_t l(lock);
        auto it = fwpm_filter_enums.find(handle);
        if (it != fwpm_filter_enums.end()) {
            auto& state = it->second;
            state.next_index -= (count < state.next_index) ? count : state.next_index;
        }
    }

    // Creates a snapshot of the callouts matching the (optional) enumeration template, and returns a handle to it.
    _Requires_lock_not_held_(this->lock) uint64_t
        create_fwpm_callout_enum_handle(_In_opt_ const FWPM_CALLOUT_ENUM_TEMPLATE0* enum_template)
    {
        exclusive_lock_t l(lock);
        uint64_t handle = next_enum_handle++;
        auto& state = fwpm_callout_enums[handle];
        for (auto& [id, entry] : fwpm_callouts) {
            if (!provider_key_matches(
                    entry.callout.providerKey, enum_template ? enum_template->providerKey : nullptr)) {
                continue;
            }
            if (enum_template != nullptr && !is_null_guid(enum_template->layerKey) &&
                memcmp(&entry.callout.applicableLayer, &enum_template->layerKey, sizeof(GUID)) != 0) {
                continue;
            }

            state.entries.push_back(entry);
            rebind_callout_entry(state.entries.back());
        }
        return handle;
    }

    _Requires_lock_not_held_(this->lock) bool next_fwpm_callout_enum_entries(
        uint64_t handle, uint32_t requested, _Inout_ std::vector<fwpm_callout_entry_t>& out)
    {
        exclusive_lock_t l(lock);
        auto it = fwpm_callout_enums.find(handle);
        if (it == fwpm_callout_enums.end()) {
            return false;
        }

        auto& state = it->second;
        while (out.size() < requested && state.next_index < state.entries.size()) {
            out.push_back(state.entries[state.next_index++]);
            rebind_callout_entry(out.back());
        }
        return true;
    }

    _Requires_lock_not_held_(this->lock) bool destroy_fwpm_callout_enum_handle(uint64_t handle)
    {
        exclusive_lock_t l(lock);
        return fwpm_callout_enums.erase(handle) == 1;
    }

    // Rewinds the callout enumeration cursor by 'count' entries. See rewind_fwpm_filter_enum.
    _Requires_lock_not_held_(this->lock) void rewind_fwpm_callout_enum(uint64_t handle, size_t count)
    {
        exclusive_lock_t l(lock);
        auto it = fwpm_callout_enums.find(handle);
        if (it != fwpm_callout_enums.end()) {
            auto& state = it->second;
            state.next_index -= (count < state.next_index) ? count : state.next_index;
        }
    }

    // Adds a provider, rejecting a duplicate as real WFP does. Object identity matters here: a provider that
    // outlives the driver that created it is what makes a subsequent FwpmProviderAdd fail, so a mock that always
    // accepts the add cannot reproduce that class of bug.
    //
    // Unlike FWPM_FILTER0/FWPM_CALLOUT0, FWPM_PROVIDER0::providerKey is a GUID by value, so the stored copy owns
    // its own key and needs no re-binding. The remaining pointer members (displayData strings, serviceName,
    // providerData) are still shallow copies of caller memory, which is safe only because nothing reads them:
    // deep-copy them before adding any accessor or enumerator that hands a stored provider back to a caller.
    _Requires_lock_not_held_(this->lock) bool add_fwpm_provider(_In_ const FWPM_PROVIDER* provider)
    {
        exclusive_lock_t l(lock);
        if (get_fwpm_provider_under_lock(&provider->providerKey) != nullptr) {
            return false;
        }

        fwpm_providers.insert({next_id++, *provider});
        return true;
    }

    _Requires_lock_not_held_(this->lock) NTSTATUS delete_fwpm_provider(_In_ const GUID* key)
    {
        exclusive_lock_t l(lock);

        size_t id = 0;
        bool found = false;
        for (auto& [first, provider] : fwpm_providers) {
            if (memcmp(&provider.providerKey, key, sizeof(GUID)) == 0) {
                id = first;
                found = true;
                break;
            }
        }

        if (!found) {
            return (NTSTATUS)FWP_E_PROVIDER_NOT_FOUND;
        }

        if (is_provider_referenced_under_lock(key)) {
            return (NTSTATUS)FWP_E_IN_USE;
        }

        fwpm_providers.erase(id);
        return STATUS_SUCCESS;
    }

    _Requires_lock_not_held_(this->lock) uint32_t add_fwpm_sub_layer(_In_ const FWPM_SUBLAYER0* sub_layer)
    {
        exclusive_lock_t l(lock);
        uint32_t id = next_id++;
        auto& stored = fwpm_sub_layers.insert({id, fwpm_sub_layer_entry_t{*sub_layer, {}}}).first->second;

        // Re-point the stored sub-layer at the entry's own copy of the provider key (see fwpm_sub_layer_entry_t).
        if (sub_layer->providerKey != nullptr) {
            stored.provider_key = *sub_layer->providerKey;
            stored.sub_layer.providerKey = &stored.provider_key;
        } else {
            stored.sub_layer.providerKey = nullptr;
        }

        return id;
    }

    _Requires_lock_not_held_(this->lock) bool remove_fwpm_sub_layer(size_t id)
    {
        exclusive_lock_t l(lock);
        return fwpm_sub_layers.erase(id) == 1;
    }

    _Requires_lock_not_held_(this->lock) NTSTATUS delete_fwpm_sub_layer(_In_ const GUID* key)
    {
        exclusive_lock_t l(lock);

        size_t id = 0;
        bool found = false;
        for (auto& [first, entry] : fwpm_sub_layers) {
            if (memcmp(&entry.sub_layer.subLayerKey, key, sizeof(GUID)) == 0) {
                id = first;
                found = true;
                break;
            }
        }

        if (!found) {
            return (NTSTATUS)FWP_E_SUBLAYER_NOT_FOUND;
        }

        if (is_sub_layer_referenced_under_lock(key)) {
            return (NTSTATUS)FWP_E_IN_USE;
        }

        fwpm_sub_layers.erase(id);
        return STATUS_SUCCESS;
    }

    FWP_ACTION_TYPE
    classify_test_packet(_In_ const GUID* layer_guid, NET_IFINDEX if_index);

    FWP_ACTION_TYPE
    test_bind_ipv4(_In_ fwp_classify_parameters_t* parameters, _In_opt_ const GUID* callout_key = nullptr);

    FWP_ACTION_TYPE
    test_bind_ipv6(_In_ fwp_classify_parameters_t* parameters, _In_opt_ const GUID* callout_key = nullptr);

    FWP_ACTION_TYPE
    test_cgroup_inet4_recv_accept(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet6_recv_accept(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet4_connect(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet6_connect(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_sock_ops_v4(_In_ fwp_classify_parameters_t* parameters, _Out_opt_ uint64_t* flow_id);

    FWP_ACTION_TYPE
    test_sock_ops_v6(_In_ fwp_classify_parameters_t* parameters, _Out_opt_ uint64_t* flow_id);

    void 
    test_sock_ops_v4_remove_flow_context(_In_ uint64_t flow_id);

    void 
    test_sock_ops_v6_remove_flow_context(_In_ uint64_t flow_id);

    FWP_ACTION_TYPE
    test_cgroup_inet4_listen(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet6_listen(_In_ fwp_classify_parameters_t* parameters);

    static fwp_engine_t*
    get()
    {
        if (!_engine) {
            _engine = std::make_unique<fwp_engine_t>();
        }
        return _engine.get();
    }

  private:
    // Re-points a copied entry's providerKey at its own GUID copy. A byte-wise copy of an entry would otherwise
    // leave providerKey aliasing the GUID inside the entry it was copied from (see fwpm_filter_entry_t).
    static void
    rebind_filter_entry(_Inout_ fwpm_filter_entry_t& entry)
    {
        if (entry.filter.providerKey != nullptr) {
            entry.filter.providerKey = &entry.provider_key;
        }
    }

    static void
    rebind_callout_entry(_Inout_ fwpm_callout_entry_t& entry)
    {
        if (entry.callout.providerKey != nullptr) {
            entry.callout.providerKey = &entry.provider_key;
        }
    }

    static bool
    is_null_guid(_In_ const GUID& guid)
    {
        static const GUID null_guid = {};
        return memcmp(&guid, &null_guid, sizeof(GUID)) == 0;
    }

    // Applies an enumeration template's providerKey filter. A null template key matches every object, including
    // objects with no provider; a non-null template key matches only objects tagged with that exact provider.
    static bool
    provider_key_matches(_In_opt_ const GUID* object_key, _In_opt_ const GUID* template_key)
    {
        if (template_key == nullptr) {
            return true;
        }
        if (object_key == nullptr) {
            return false;
        }
        return memcmp(object_key, template_key, sizeof(GUID)) == 0;
    }

    _Requires_lock_not_held_(this->lock) FWP_ACTION_TYPE test_callout(
        uint16_t layer_id,
        _In_ const GUID& layer_guid,
        _In_ const GUID& sublayer_guid,
        _In_ FWPS_INCOMING_VALUE0* incoming_value,
        _Out_opt_ uint64_t* flow_handle,
        _In_opt_ const GUID* callout_key = nullptr);

    _Requires_lock_not_held_(this->lock) void test_remove_flow_context(
    uint64_t flow_id,
    uint16_t layer_id,
    _In_ const GUID& layer_guid);

    _Ret_maybenull_ const FWPM_FILTER*
    get_fwpm_filter_with_context_under_lock(_In_ const GUID& layer_guid)
    {
        for (auto& [first, entry] : fwpm_filters) {
            if (memcmp(&entry.filter.layerKey, &layer_guid, sizeof(GUID)) == 0 && entry.filter.rawContext != 0) {
                return &entry.filter;
            }
        }
        return nullptr;
    }

    _Ret_maybenull_ const FWPM_FILTER*
    get_fwpm_filter_with_context_under_lock(_In_ const GUID& layer_guid, _In_ const GUID& sublayer_guid)
    {
        for (auto& [first, entry] : fwpm_filters) {
            if (memcmp(&entry.filter.layerKey, &layer_guid, sizeof(GUID)) == 0 &&
                memcmp(&entry.filter.subLayerKey, &sublayer_guid, sizeof(GUID)) == 0 &&
                entry.filter.rawContext != 0) {
                return &entry.filter;
            }
        }
        return nullptr;
    }

    // Reference checks backing FWP_E_IN_USE. Real WFP refuses to delete an object that another object still
    // points at, which is the mechanism by which a filter that could not be deleted keeps its callout, sub-layer
    // and provider alive across a driver unload. Without this the mock would happily delete a referenced object
    // and no test could observe that failure mode.
    static bool
    is_callout_action(FWP_ACTION_TYPE action_type)
    {
        return action_type == FWP_ACTION_CALLOUT_TERMINATING || action_type == FWP_ACTION_CALLOUT_INSPECTION ||
               action_type == FWP_ACTION_CALLOUT_UNKNOWN;
    }

    bool
    is_callout_referenced_under_lock(_In_ const GUID* callout_key)
    {
        for (auto& [first, entry] : fwpm_filters) {
            // FWPM_ACTION0::calloutKey shares a union with filterType, so it only holds a callout key when the
            // action is a callout action. Comparing it for any other action type would match unrelated bytes and
            // report a spurious FWP_E_IN_USE.
            if (!is_callout_action(entry.filter.action.type)) {
                continue;
            }
            if (memcmp(&entry.filter.action.calloutKey, callout_key, sizeof(GUID)) == 0) {
                return true;
            }
        }
        return false;
    }

    bool
    is_sub_layer_referenced_under_lock(_In_ const GUID* sub_layer_key)
    {
        for (auto& [first, entry] : fwpm_filters) {
            if (memcmp(&entry.filter.subLayerKey, sub_layer_key, sizeof(GUID)) == 0) {
                return true;
            }
        }
        return false;
    }

    // A provider is referenced by any filter, callout or sub-layer tagged with it.
    bool
    is_provider_referenced_under_lock(_In_ const GUID* provider_key)
    {
        for (auto& [first, entry] : fwpm_filters) {
            if (entry.filter.providerKey != nullptr &&
                memcmp(entry.filter.providerKey, provider_key, sizeof(GUID)) == 0) {
                return true;
            }
        }
        for (auto& [first, entry] : fwpm_callouts) {
            if (entry.callout.providerKey != nullptr &&
                memcmp(entry.callout.providerKey, provider_key, sizeof(GUID)) == 0) {
                return true;
            }
        }
        for (auto& [first, entry] : fwpm_sub_layers) {
            if (entry.sub_layer.providerKey != nullptr &&
                memcmp(entry.sub_layer.providerKey, provider_key, sizeof(GUID)) == 0) {
                return true;
            }
        }
        return false;
    }

    _Ret_maybenull_ const FWPM_PROVIDER*
    get_fwpm_provider_under_lock(_In_ const GUID* provider_key)
    {
        for (auto& [first, provider] : fwpm_providers) {
            if (memcmp(&provider.providerKey, provider_key, sizeof(GUID)) == 0) {
                return &provider;
            }
        }
        return nullptr;
    }

    // Select a filter by its bound callout key at the given layer and sublayer. This disambiguates the
    // case where multiple callouts (each with its own filter) are registered at the same WFP layer and
    // sublayer (e.g., the legacy bind callout and the CGROUP_SOCK_ADDR bind callout both at
    // ALE_RESOURCE_ASSIGNMENT). Selecting the filter by its callout key lets a test target a specific
    // callout explicitly instead of relying on layer+sublayer first-match ordering. The sublayer is
    // still matched (mirroring get_fwpm_filter_with_context_under_lock) so selection stays unambiguous
    // even if a callout ever owns filters on more than one sublayer.
    _Ret_maybenull_ const FWPM_FILTER*
    get_fwpm_filter_by_callout_under_lock(
        _In_ const GUID& layer_guid, _In_ const GUID& sublayer_guid, _In_ const GUID& callout_key)
    {
        for (auto& [first, filter] : fwpm_filters) {
            if (memcmp(&filter.layerKey, &layer_guid, sizeof(GUID)) == 0 &&
                memcmp(&filter.subLayerKey, &sublayer_guid, sizeof(GUID)) == 0 &&
                memcmp(&filter.action.calloutKey, &callout_key, sizeof(GUID)) == 0 && filter.rawContext != 0) {
                return &filter;
            }
        }
        return nullptr;
    }

    _Ret_maybenull_ const GUID*
    get_callout_key_from_layer_guid_under_lock(_In_ const GUID* layer_guid)
    {
        for (auto& [first, entry] : fwpm_callouts) {
            if (entry.callout.applicableLayer == *layer_guid) {
                return &entry.callout.calloutKey;
            }
        }
        return nullptr;
    }

    _Ret_maybenull_ const FWPS_CALLOUT3*
    get_callout_from_key_under_lock(_In_ const GUID* callout_key)
    {
        for (auto& [first, callout] : fwps_callouts) {
            if (callout.calloutKey == *callout_key) {
                return &callout;
            }
        }
        return nullptr;
    }

    _Ret_maybenull_
    size_t get_callout_id_from_key_under_lock(_In_ const GUID* callout_key)
    {
        for (auto& [first, callout] : fwps_callouts) {
            if (callout.calloutKey == *callout_key) {
                return first;
            }
        }
        return 0;
    }

    static std::unique_ptr<fwp_engine_t> _engine;

    std::shared_mutex lock;
    uint32_t next_id = 1;
    uint32_t next_flow_id = 1;
    uint64_t next_enum_handle = 1;
    uint32_t _filter_delete_failure_count = 0; // Test-only WFP filter delete fault-injection counter.
    std::unordered_map<size_t, FWPS_CALLOUT3> fwps_callouts;
    std::unordered_map<size_t, fwpm_callout_entry_t> fwpm_callouts;
    std::unordered_map<size_t, fwpm_filter_entry_t> fwpm_filters;
    std::unordered_map<uint64_t, fwpm_enum_state_t<fwpm_filter_entry_t>> fwpm_filter_enums;
    std::unordered_map<uint64_t, fwpm_enum_state_t<fwpm_callout_entry_t>> fwpm_callout_enums;
    std::unordered_map<size_t, fwpm_sub_layer_entry_t> fwpm_sub_layers;
    std::unordered_map<size_t, FWPM_PROVIDER0> fwpm_providers;
    std::unordered_map<uint64_t, uint64_t> fwpm_flow_contexts;
    GUID _default_sublayer = {};
    GUID _connect_v4_sublayer = {};
    GUID _connect_v6_sublayer = {};
} fwp_engine_t;
