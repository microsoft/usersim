// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "net_platform.h"
#include "usersim/fwp_test.h"

#include <shared_mutex>
#include <unordered_map>

typedef std::unique_lock<std::shared_mutex> exclusive_lock_t;
typedef std::shared_lock<std::shared_mutex> shared_lock_t;

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
        fwpm_callouts.insert({id, *callout});
        return id;
    }

    bool
    remove_fwpm_callout(size_t id)
    {
        exclusive_lock_t l(lock);
        return fwpm_callouts.erase(id) == 1;
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
            fwpm_filters.insert({id, *filter});

            callout = get_fwps_callout(&filter->action.calloutKey);
            CXPLAT_DEBUG_ASSERT(callout != nullptr);
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
                    callout = get_fwps_callout(&it.second.action.calloutKey);
                    CXPLAT_DEBUG_ASSERT(callout != nullptr);
                    fwps_filter.context = it.second.rawContext;
                    break;
                }
            }

            return_value = fwpm_filters.erase(id) == 1;
        }

        CXPLAT_DEBUG_ASSERT(callout != nullptr);
        __analysis_assume(callout != nullptr);
        // Invoke filter delete notification callback.
        callout->notifyFn(FWPS_CALLOUT_NOTIFY_DELETE_FILTER, &callout->calloutKey, &fwps_filter);

        return return_value;
    }

    _Requires_lock_not_held_(this->lock) void add_fwpm_provider(_In_ const FWPM_PROVIDER* provider)
    {
        UNREFERENCED_PARAMETER(provider);
        return;
    }

    _Requires_lock_not_held_(this->lock) uint32_t add_fwpm_sub_layer(_In_ const FWPM_SUBLAYER0* sub_layer)
    {
        exclusive_lock_t l(lock);
        uint32_t id = next_id++;
        fwpm_sub_layers.insert({id, *sub_layer});
        return id;
    }

    _Requires_lock_not_held_(this->lock) bool remove_fwpm_sub_layer(size_t id)
    {
        exclusive_lock_t l(lock);
        return fwpm_sub_layers.erase(id) == 1;
    }

    FWP_ACTION_TYPE
    classify_test_packet(_In_ const GUID* layer_guid, NET_IFINDEX if_index);

    FWP_ACTION_TYPE
    test_bind_ipv4(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet4_recv_accept(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet6_recv_accept(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet4_connect(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_cgroup_inet6_connect(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_sock_ops_v4(_In_ fwp_classify_parameters_t* parameters);

    FWP_ACTION_TYPE
    test_sock_ops_v6(_In_ fwp_classify_parameters_t* parameters);

    static fwp_engine_t*
    get()
    {
        if (!_engine) {
            _engine = std::make_unique<fwp_engine_t>();
        }
        return _engine.get();
    }

  private:
    _Requires_lock_not_held_(this->lock) FWP_ACTION_TYPE test_callout(
        uint16_t layer_id,
        _In_ const GUID& layer_guid,
        _In_ const GUID& sublayer_guid,
        _In_ FWPS_INCOMING_VALUE0* incoming_value);

    _Ret_maybenull_ const FWPM_FILTER*
    get_fwpm_filter_with_context_under_lock(_In_ const GUID& layer_guid)
    {
        for (auto& [first, filter] : fwpm_filters) {
            if (memcmp(&filter.layerKey, &layer_guid, sizeof(GUID)) == 0 && filter.rawContext != 0) {
                return &filter;
            }
        }
        return nullptr;
    }

    _Ret_maybenull_ const FWPM_FILTER*
    get_fwpm_filter_with_context_under_lock(_In_ const GUID& layer_guid, _In_ const GUID& sublayer_guid)
    {
        for (auto& [first, filter] : fwpm_filters) {
            if (memcmp(&filter.layerKey, &layer_guid, sizeof(GUID)) == 0 &&
                memcmp(&filter.subLayerKey, &sublayer_guid, sizeof(GUID)) == 0 && filter.rawContext != 0) {
                return &filter;
            }
        }
        return nullptr;
    }

    _Ret_maybenull_ const GUID*
    get_callout_key_from_layer_guid_under_lock(_In_ const GUID* layer_guid)
    {
        for (auto& [first, callout] : fwpm_callouts) {
            if (callout.applicableLayer == *layer_guid) {
                return &callout.calloutKey;
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

    static std::unique_ptr<fwp_engine_t> _engine;

    std::shared_mutex lock;
    uint32_t next_id = 1;
    uint32_t next_flow_id = 1;
    std::unordered_map<size_t, FWPS_CALLOUT3> fwps_callouts;
    std::unordered_map<size_t, FWPM_CALLOUT0> fwpm_callouts;
    std::unordered_map<size_t, FWPM_FILTER0> fwpm_filters;
    std::unordered_map<size_t, FWPM_SUBLAYER0> fwpm_sub_layers;
    std::unordered_map<uint64_t, uint64_t> fwpm_flow_contexts;
    GUID _default_sublayer = {};
    GUID _connect_v4_sublayer = {};
    GUID _connect_v6_sublayer = {};
} fwp_engine_t;
