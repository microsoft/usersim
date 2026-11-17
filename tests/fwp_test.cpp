// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include <initguid.h>

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "../src/net_platform.h"
#include "usersim/fwp_test.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

namespace {
struct callback_context_t
{
    std::promise<void> classify_entered;
    std::promise<void> release_classify;
    std::shared_future<void> classify_release = release_classify.get_future().share();
    std::atomic<uint32_t> classify_count = 0;
    std::atomic<uint32_t> filter_delete_count = 0;
    std::atomic<uint32_t> flow_delete_count = 0;
    std::atomic<bool> context_alive = true;
    bool context_alive_at_classify_return = false;
    bool classify_arguments_valid = false;
    bool flow_arguments_valid = false;
    NTSTATUS reentrant_remove_status = STATUS_SUCCESS;
    uint32_t callout_id = 0;
    uint64_t flow_id = 0;
};

void NTAPI
classify(
    const FWPS_INCOMING_VALUES0*,
    const FWPS_INCOMING_METADATA_VALUES0* metadata,
    void* layer_data,
    const void*,
    const FWPS_FILTER3* filter,
    uint64_t,
    FWPS_CLASSIFY_OUT0* result)
{
    auto& context = *reinterpret_cast<callback_context_t*>(filter->context);
    context.classify_arguments_valid = layer_data
                                           ? (static_cast<NET_BUFFER_LIST*>(layer_data)->FirstNetBuffer != nullptr &&
                                              metadata->flowHandle == 0 && result->rights == FWPS_RIGHT_ACTION_WRITE)
                                           : (metadata->flowHandle != 0 && result->rights == FWPS_RIGHT_ACTION_WRITE);
    if (context.classify_count.fetch_add(1) == 0) {
        context.classify_entered.set_value();
        context.classify_release.wait();
        context.context_alive_at_classify_return = context.context_alive.load();
    }
    result->actionType = FWP_ACTION_PERMIT;
}

NTSTATUS NTAPI
notify(FWPS_CALLOUT_NOTIFY_TYPE type, const GUID*, FWPS_FILTER3* filter)
{
    if (type == FWPS_CALLOUT_NOTIFY_DELETE_FILTER) {
        auto& context = *reinterpret_cast<callback_context_t*>(filter->context);
        context.context_alive = false;
        ++context.filter_delete_count;
        // Reenter the engine to verify notifications run without its lock.
        usersim_fwp_get_fwpm_filter_count();
    }
    return STATUS_SUCCESS;
}

void NTAPI
flow_delete(uint16_t layer_id, uint32_t callout_id, uint64_t flow_context)
{
    auto& context = *reinterpret_cast<callback_context_t*>(flow_context);
    ++context.flow_delete_count;
    context.flow_arguments_valid = layer_id == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 && callout_id == context.callout_id;
    context.reentrant_remove_status = FwpsFlowRemoveContext0(context.flow_id, layer_id, callout_id);
}

// Registers a test callout and a filter whose rawContext points to the fixture's callback state.
// Keeps fault injection suspended for the fixture's lifetime and cleans up WFP registrations on destruction.
struct fwp_fixture_t
{
    explicit fwp_fixture_t(const GUID& layer_guid = FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4)
    {
        usersim_fault_injection_suspend();
        REQUIRE(FwpmEngineOpen0(nullptr, 0, nullptr, nullptr, &engine) == STATUS_SUCCESS);
        FWPS_CALLOUT3 callout = {};
        callout.calloutKey = callout_key;
        callout.classifyFn = classify;
        callout.notifyFn = notify;
        callout.flowDeleteFn = flow_delete;
        REQUIRE(FwpsCalloutRegister3(nullptr, &callout, &context.callout_id) == STATUS_SUCCESS);
        FWPM_CALLOUT0 management_callout = {};
        management_callout.calloutKey = callout_key;
        management_callout.applicableLayer = layer_guid;
        REQUIRE(FwpmCalloutAdd0(engine, &management_callout, nullptr, nullptr) == STATUS_SUCCESS);
        FWPM_FILTER0 filter = {};
        filter.layerKey = layer_guid;
        filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
        filter.action.calloutKey = callout_key;
        filter.rawContext = reinterpret_cast<uint64_t>(&context);
        REQUIRE(FwpmFilterAdd0(engine, &filter, nullptr, &filter_id) == STATUS_SUCCESS);
    }

    ~fwp_fixture_t()
    {
        if (filter_id != 0) {
            CHECK(FwpmFilterDeleteById0(engine, filter_id) == STATUS_SUCCESS);
        }
        CHECK(FwpmCalloutDeleteByKey0(engine, &callout_key) == STATUS_SUCCESS);
        CHECK(FwpsCalloutUnregisterById0(context.callout_id) == STATUS_SUCCESS);
        CHECK(FwpmEngineClose0(engine) == STATUS_SUCCESS);
        usersim_fault_injection_resume();
    }

    callback_context_t context;
    HANDLE engine = nullptr;
    uint64_t filter_id = 0;
    const GUID callout_key = {0x873409be, 0xb15b, 0x4630, {0x9a, 0xcf, 0x90, 0x40, 0xf1, 0xa2, 0x9e, 0x07}};
};
} // namespace

// Filter deletion defers its notification until admitted classification completes.
TEST_CASE("wfp_filter_context_lifetime", "[fwp]")
{
    const bool packet_classify = GENERATE(false, true);
    fwp_fixture_t fixture(packet_classify ? FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE : FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4);
    auto classify_once = [&]() {
        if (packet_classify) {
            return usersim_fwp_classify_packet(&FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE, 1);
        }
        fwp_classify_parameters_t parameters = {};
        return usersim_fwp_sock_ops_v4(&parameters, nullptr);
    };
    auto entered = fixture.context.classify_entered.get_future();
    FWP_ACTION_TYPE action = FWP_ACTION_CALLOUT_UNKNOWN;
    std::thread classifier([&]() { action = classify_once(); });

    CHECK(entered.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    const auto delete_status = FwpmFilterDeleteById0(fixture.engine, fixture.filter_id);
    if (delete_status == STATUS_SUCCESS) {
        fixture.filter_id = 0;
    }
    CHECK(delete_status == STATUS_SUCCESS);
    CHECK(fixture.context.filter_delete_count == 0);
    CHECK(classify_once() == FWP_ACTION_CALLOUT_UNKNOWN);

    fixture.context.release_classify.set_value();
    classifier.join();
    REQUIRE(action == FWP_ACTION_PERMIT);
    REQUIRE(fixture.context.context_alive_at_classify_return);
    REQUIRE(fixture.context.classify_arguments_valid);
    REQUIRE(fixture.context.classify_count == 1);
    REQUIRE(fixture.context.filter_delete_count == 1);
}

// Concurrent and repeated flow-context removal delivers exactly one delete notification.
TEST_CASE("wfp_flow_context_removal", "[fwp]")
{
    fwp_fixture_t fixture;
    for (uint64_t iteration = 1; iteration <= 16; ++iteration) {
        fixture.context.flow_id = iteration;
        REQUIRE(
            FwpsFlowAssociateContext0(
                iteration,
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4,
                fixture.context.callout_id,
                reinterpret_cast<uint64_t>(&fixture.context)) == STATUS_SUCCESS);
        std::promise<void> start;
        auto ready = start.get_future().share();
        NTSTATUS remove_status = STATUS_SUCCESS;
        std::thread cleanup([&]() {
            ready.wait();
            remove_status =
                FwpsFlowRemoveContext0(iteration, FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4, fixture.context.callout_id);
        });
        std::thread external_delete([&]() {
            ready.wait();
            usersim_fwp_sock_ops_v4_remove_flow_context(iteration);
        });
        start.set_value();
        cleanup.join();
        external_delete.join();

        REQUIRE((remove_status == STATUS_SUCCESS || remove_status == STATUS_NOT_FOUND));
        REQUIRE(fixture.context.flow_delete_count == iteration);
        REQUIRE(fixture.context.flow_arguments_valid);
        REQUIRE(fixture.context.reentrant_remove_status == STATUS_NOT_FOUND);
        REQUIRE(
            FwpsFlowRemoveContext0(iteration, FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4, fixture.context.callout_id) ==
            STATUS_NOT_FOUND);
        usersim_fwp_sock_ops_v4_remove_flow_context(iteration);
        REQUIRE(fixture.context.flow_delete_count == iteration);
    }
}
