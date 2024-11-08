// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat_fault_injection.h"
#include "fwp_um.h"
#define htonl(x) _byteswap_ulong(x)  /* TODO: replace with RtlUlongByteSwap */
#define htons(x) _byteswap_ushort(x) /* TODO: replace with RtlUShortByteSwap */
#define ntohl(x) _byteswap_ulong(x)
#define ntohs(x) _byteswap_ushort(x)
#include "net_platform.h"
#include "usersim/ex.h"

thread_local static FWPS_CONNECT_REQUEST0* _fwp_um_connect_request = nullptr;

bool
_is_connection_redirected(
    _In_ const fwp_classify_parameters_t* parameters,
    _In_ const FWPS_CONNECT_REQUEST0* request,
    _Out_ uint16_t* redirected_port,
    _Out_ uint8_t** redirected_address)
{
    // Only check the port to know if redirection happened.
    *redirected_port = INETADDR_PORT((PSOCKADDR)&request->remoteAddressAndPort);
    *redirected_address = INETADDR_ADDRESS((PSOCKADDR)&request->remoteAddressAndPort);

    if (parameters->destination_port == *redirected_port) {
        return false;
    }

    if (request->remoteAddressAndPort.ss_family == AF_INET) {
        if (parameters->destination_ipv4_address == *((uint32_t*)*redirected_address)) {
            return false;
        }
    } else if (memcmp(*redirected_address, parameters->destination_ipv6_address.byteArray16, 16) == 0) {
        return false;
    }

    return true;
}

// Allocate and initialize FWPS_CONNECT_REQUEST0.
void static _allocate_and_initialize_connection_request(
    ADDRESS_FAMILY family, _In_ const fwp_classify_parameters_t* parameters)
{
    CXPLAT_DEBUG_ASSERT(_fwp_um_connect_request == nullptr);
    _fwp_um_connect_request = (FWPS_CONNECT_REQUEST0*)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, sizeof(FWPS_CONNECT_REQUEST0), USERSIM_TAG_FWPS_CONNECT_REQUEST0);
    if (_fwp_um_connect_request == nullptr) {
        // Most likely we are under fault injection simulation. Return.
        return;
    }

    _fwp_um_connect_request->remoteAddressAndPort.ss_family = family;
    INETADDR_SET_PORT((PSOCKADDR)&_fwp_um_connect_request->remoteAddressAndPort, parameters->destination_port);
    return;
}

void static _free_connection_request()
{
    cxplat_free(_fwp_um_connect_request, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_FWPS_CONNECT_REQUEST0);
    _fwp_um_connect_request = nullptr;
}

#pragma region fwp_engine_t

std::unique_ptr<fwp_engine_t> fwp_engine_t::_engine;

// Attempt to classify a test packet at a given WFP layer on a given interface index.
// This is used to test the xdp hook.
FWP_ACTION_TYPE
fwp_engine_t::classify_test_packet(_In_ const GUID* layer_guid, NET_IFINDEX if_index)
{
    shared_lock_t l(lock);
    const GUID* callout_key = get_callout_key_from_layer_guid_under_lock(layer_guid);
    if (callout_key == nullptr) {
        return FWP_ACTION_CALLOUT_UNKNOWN;
    }
    const FWPS_CALLOUT3* callout = get_callout_from_key_under_lock(callout_key);
    if (callout == nullptr) {
        return FWP_ACTION_CALLOUT_UNKNOWN;
    }
    FWPS_CLASSIFY_OUT0 result = {};
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_MAX] = {};
    FWPS_INCOMING_VALUES incoming_fixed_values = {.incomingValue = incoming_value};
    incoming_fixed_values.incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_INTERFACE_INDEX].value.uint32 = if_index;
    FWPS_INCOMING_METADATA_VALUES incoming_metadata_values = {};
    const FWPM_FILTER* fwpm_filter = get_fwpm_filter_with_context_under_lock(*layer_guid);
    if (!fwpm_filter) {
        return FWP_ACTION_CALLOUT_UNKNOWN;
    }
    FWPS_FILTER fwps_filter = {.context = fwpm_filter->rawContext};
    NET_BUFFER_LIST_POOL_PARAMETERS pool_parameters = {};
    std::unique_ptr<void, decltype(&NdisFreeNetBufferListPool)> nbl_pool_handle(
        NdisAllocateNetBufferListPool(nullptr, &pool_parameters), NdisFreeNetBufferListPool);
    if (!nbl_pool_handle) {
        return FWP_ACTION_CALLOUT_UNKNOWN;
    }

    std::unique_ptr<NET_BUFFER_LIST, decltype(&NdisFreeNetBufferList)> nbl(
        NdisAllocateNetBufferList(nbl_pool_handle.get(), 0, 0), NdisFreeNetBufferList);
    if (!nbl) {
        return FWP_ACTION_CALLOUT_UNKNOWN;
    }

    unsigned long data = 0;
    std::unique_ptr<MDL, decltype(&IoFreeMdl)> mdl_chain(
        IoAllocateMdl(&data, sizeof(data), FALSE, FALSE, nullptr), IoFreeMdl);
    if (!mdl_chain) {
        return FWP_ACTION_CALLOUT_UNKNOWN;
    }

    std::unique_ptr<NET_BUFFER, decltype(&NdisFreeNetBuffer)> nb(
        NdisAllocateNetBuffer(nbl_pool_handle.get(), mdl_chain.get(), 0, sizeof(data)), NdisFreeNetBuffer);
    if (!nb) {
        return FWP_ACTION_CALLOUT_UNKNOWN;
    }

    nbl->FirstNetBuffer = nb.get();
    callout->classifyFn(
        &incoming_fixed_values,
        &incoming_metadata_values,
        nbl.get(),
        nullptr, // classifyContext,
        &fwps_filter,
        0, // flowContext,
        &result);

    return result.actionType;
}

// This is used to test the bind hook.
FWP_ACTION_TYPE
fwp_engine_t::test_bind_ipv4(_In_ fwp_classify_parameters_t* parameters)
{
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_MAX] = {};
    incoming_value[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_PORT].value.uint16 = parameters->destination_port;
    incoming_value[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_ADDRESS].value.uint32 =
        parameters->destination_ipv4_address;
    incoming_value[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_APP_ID].value.byteBlob = &parameters->app_id;

    return test_callout(
        FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4,
        FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4,
        _default_sublayer,
        incoming_value);
}

_Requires_lock_not_held_(this->lock) FWP_ACTION_TYPE fwp_engine_t::test_callout(
    uint16_t layer_id,
    _In_ const GUID& layer_guid,
    _In_ const GUID& sublayer_guid,
    _In_ FWPS_INCOMING_VALUE0* incoming_value)
{
    FWPS_INCOMING_VALUES incoming_fixed_values = {.layerId = layer_id, .incomingValue = incoming_value};
    FWPS_INCOMING_METADATA_VALUES incoming_metadata_values = {};
    FWPS_FILTER fwps_filter = {};
    const FWPS_CALLOUT3* callout = nullptr;

    {
        shared_lock_t l(lock);
        const FWPM_FILTER* fwpm_filter = get_fwpm_filter_with_context_under_lock(layer_guid, sublayer_guid);
        if (!fwpm_filter) {
            return FWP_ACTION_CALLOUT_UNKNOWN;
        }
        fwps_filter.context = fwpm_filter->rawContext;

        const GUID* callout_key = get_callout_key_from_layer_guid_under_lock(&layer_guid);
        if (callout_key == nullptr) {
            return FWP_ACTION_CALLOUT_UNKNOWN;
        }

        callout = get_callout_from_key_under_lock(callout_key);
        if (callout == nullptr) {
            return FWP_ACTION_CALLOUT_UNKNOWN;
        }

        incoming_metadata_values.flowHandle = next_flow_id++;
    }

    FWPS_CLASSIFY_OUT0 result = {};
    result.rights = FWPS_RIGHT_ACTION_WRITE;
    callout->classifyFn(
        &incoming_fixed_values,
        &incoming_metadata_values,
        nullptr, // layer_data
        nullptr, // classify_context,
        &fwps_filter,
        0, // flow_context,
        &result);

    return result.actionType;
}

// This is used to test the INET4_RECV_ACCEPT hook.
FWP_ACTION_TYPE
fwp_engine_t::test_cgroup_inet4_recv_accept(_In_ fwp_classify_parameters_t* parameters)
{
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_MAX] = {};
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS].value.uint32 =
        parameters->destination_ipv4_address;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT].value.uint16 = parameters->destination_port;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS].value.uint32 = parameters->source_ipv4_address;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT].value.uint16 = parameters->source_port;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_INTERFACE].value.uint64 =
        const_cast<UINT64*>(&parameters->interface_luid);
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_APP_ID].value.byteBlob =
        const_cast<FWP_BYTE_BLOB*>(&parameters->app_id);
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_USER_ID].value.byteBlob =
        const_cast<FWP_BYTE_BLOB*>(&parameters->user_id);
    incoming_value[FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS].value.uint32 = parameters->reauthorization_flag;

    return test_callout(
        FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, _default_sublayer, incoming_value);
}

// This is used to test the INET6_RECV_ACCEPT hook.
FWP_ACTION_TYPE
fwp_engine_t::test_cgroup_inet6_recv_accept(_In_ fwp_classify_parameters_t* parameters)
{
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_MAX] = {};
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS].value.byteArray16 =
        &parameters->destination_ipv6_address;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT].value.uint16 = parameters->destination_port;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS].value.byteArray16 =
        &parameters->source_ipv6_address;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT].value.uint16 = parameters->source_port;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_INTERFACE].value.uint64 = &parameters->interface_luid;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_ALE_APP_ID].value.byteBlob = &parameters->app_id;
    incoming_value[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_ALE_USER_ID].value.byteBlob = &parameters->user_id;
    incoming_value[FWPS_FIELD_ALE_AUTH_CONNECT_V6_FLAGS].value.uint32 = parameters->reauthorization_flag;

    return test_callout(
        FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, _default_sublayer, incoming_value);
}

// This is used to test the INET4_CONNECT hook.
FWP_ACTION_TYPE
fwp_engine_t::test_cgroup_inet4_connect(_In_ fwp_classify_parameters_t* parameters)
{
    FWP_ACTION_TYPE action;
    bool redirected = false;
    uint16_t redirected_port = 0;
    uint8_t* redirected_address = nullptr;
    bool fault_injection_enabled = cxplat_fault_injection_is_enabled();

    _allocate_and_initialize_connection_request(AF_INET, parameters);

    // For CGROUP_CONNECT* attach type, first CONNECT_REDIRECT callout is invoked, followed by
    // AUTH_CONNECT.
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_MAX] = {};
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_ADDRESS].value.uint32 = parameters->source_ipv4_address;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_PORT].value.uint16 = parameters->source_port;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_ADDRESS].value.uint32 =
        parameters->destination_ipv4_address;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_PORT].value.uint16 = parameters->destination_port;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value.byteBlob = &parameters->app_id;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_USER_ID].value.byteBlob = &parameters->user_id;

    action = test_callout(
        FWPS_LAYER_ALE_CONNECT_REDIRECT_V4, FWPM_LAYER_ALE_CONNECT_REDIRECT_V4, _default_sublayer, incoming_value);
    CXPLAT_DEBUG_ASSERT(action == FWP_ACTION_PERMIT || action == FWP_ACTION_CONTINUE || fault_injection_enabled);

    if (_fwp_um_connect_request != nullptr) {
        redirected =
            _is_connection_redirected(parameters, _fwp_um_connect_request, &redirected_port, &redirected_address);
    }

    FWPS_INCOMING_VALUE0 incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_MAX] = {};
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS].value.uint32 = parameters->source_ipv4_address;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT].value.uint16 = parameters->source_port;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS].value.uint32 =
        parameters->destination_ipv4_address;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT].value.uint16 = parameters->destination_port;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_APP_ID].value.byteBlob = &parameters->app_id;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_USER_ID].value.byteBlob = &parameters->user_id;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_INTERFACE].value.uint64 = &parameters->interface_luid;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS].value.uint32 = parameters->reauthorization_flag;

    action = test_callout(
        FWPS_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V4, _default_sublayer, incoming_value2);

    if (redirected) {
        // In case the connection is redirected, AUTH_CONNECT callout will be invoked twice.
        CXPLAT_DEBUG_ASSERT(action == FWP_ACTION_PERMIT || action == FWP_ACTION_CONTINUE || fault_injection_enabled);

        incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT].value.uint16 = ntohs(redirected_port);
        incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS].value.uint32 =
            ntohl(*((uint32_t*)redirected_address));

        action = test_callout(
            FWPS_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V4, _default_sublayer, incoming_value2);
    }

    _free_connection_request();

    return action;
}

// This is used to test the INET6_CONNECT hook.
FWP_ACTION_TYPE
fwp_engine_t::test_cgroup_inet6_connect(_In_ fwp_classify_parameters_t* parameters)
{
    FWP_ACTION_TYPE action;
    bool redirected = false;
    uint16_t redirected_port = 0;
    uint8_t* redirected_address = nullptr;
    bool fault_injection_enabled = cxplat_fault_injection_is_enabled();

    _allocate_and_initialize_connection_request(AF_INET6, parameters);

    // For CGROUP_CONNECT* attach type, first CONNECT_REDIRECT callout is invoked, followed by
    // AUTH_CONNECT.
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_MAX] = {};
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_LOCAL_ADDRESS].value.byteArray16 =
        &parameters->source_ipv6_address;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_LOCAL_PORT].value.uint16 = parameters->source_port;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_REMOTE_ADDRESS].value.byteArray16 =
        &parameters->destination_ipv6_address;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_REMOTE_PORT].value.uint16 = parameters->destination_port;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_APP_ID].value.byteBlob = &parameters->app_id;
    incoming_value[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_USER_ID].value.byteBlob = &parameters->user_id;

    // TODO: why does this use _connect_v6_sublayer but test_cgroup_inet4_connect uses _default_sublayer?
    action = test_callout(
        FWPS_LAYER_ALE_CONNECT_REDIRECT_V6, FWPM_LAYER_ALE_CONNECT_REDIRECT_V6, _connect_v6_sublayer, incoming_value);
    CXPLAT_DEBUG_ASSERT(action == FWP_ACTION_PERMIT || action == FWP_ACTION_CONTINUE || fault_injection_enabled);

    if (_fwp_um_connect_request != nullptr) {
        redirected =
            _is_connection_redirected(parameters, _fwp_um_connect_request, &redirected_port, &redirected_address);
    }

    FWPS_INCOMING_VALUE0 incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_MAX] = {};
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS].value.byteArray16 =
        &parameters->source_ipv6_address;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT].value.uint16 = parameters->source_port;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS].value.byteArray16 =
        &parameters->destination_ipv6_address;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT].value.uint16 = parameters->destination_port;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_ALE_APP_ID].value.byteBlob = &parameters->app_id;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_ALE_USER_ID].value.byteBlob = &parameters->user_id;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_INTERFACE].value.uint64 = &parameters->interface_luid;
    incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_FLAGS].value.uint32 = parameters->reauthorization_flag;

    action = test_callout(
        FWPS_LAYER_ALE_AUTH_CONNECT_V6, FWPM_LAYER_ALE_AUTH_CONNECT_V6, _default_sublayer, incoming_value2);

    if (redirected) {
        // In case the connection is redirected, AUTH_CONNECT callout will be invoked twice.
        CXPLAT_DEBUG_ASSERT(action == FWP_ACTION_PERMIT || action == FWP_ACTION_CONTINUE || fault_injection_enabled);

        FWP_BYTE_ARRAY16 destination_ip = {0};
        memcpy(destination_ip.byteArray16, redirected_address, 16);

        incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT].value.uint16 = ntohs(redirected_port);
        incoming_value2[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS].value.byteArray16 = &destination_ip;

        action = test_callout(
            FWPS_LAYER_ALE_AUTH_CONNECT_V6, FWPM_LAYER_ALE_AUTH_CONNECT_V6, _default_sublayer, incoming_value2);
    }

    _free_connection_request();

    return action;
}

// This is used to test the SOCK_OPS hook for IPv4 traffic.
FWP_ACTION_TYPE
fwp_engine_t::test_sock_ops_v4(_In_ fwp_classify_parameters_t* parameters)
{
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_MAX] = {};
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS].value.uint32 =
        parameters->destination_ipv4_address;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_PORT].value.uint16 = parameters->destination_port;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_ADDRESS].value.uint32 = parameters->source_ipv4_address;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_PORT].value.uint16 = parameters->source_port;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_INTERFACE].value.uint64 = &parameters->interface_luid;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value.byteBlob = &parameters->app_id;

    return test_callout(
        FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4, FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4, _default_sublayer, incoming_value);
}

// This is used to test the SOCK_OPS hook for IPv6 traffic.
FWP_ACTION_TYPE
fwp_engine_t::test_sock_ops_v6(_In_ fwp_classify_parameters_t* parameters)
{
    FWPS_INCOMING_VALUE0 incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_MAX] = {};
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_ADDRESS].value.byteArray16 =
        &parameters->destination_ipv6_address;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_PORT].value.uint16 = parameters->destination_port;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_REMOTE_ADDRESS].value.byteArray16 =
        &parameters->source_ipv6_address;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_REMOTE_PORT].value.uint16 = parameters->source_port;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL].value.uint8 = parameters->protocol;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_COMPARTMENT_ID].value.uint32 = parameters->compartment_id;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_INTERFACE].value.uint64 = &parameters->interface_luid;
    incoming_value[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_ALE_APP_ID].value.byteBlob = &parameters->app_id;

    return test_callout(
        FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6, FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6, _default_sublayer, incoming_value);
}

#pragma endregion fwp_engine_t

#pragma region fwpm_apis

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmFilterDeleteById0(_In_ HANDLE engine_handle, _In_ uint64_t id)
{
    // Skip fault injection for this API because return failure status requires to remove filter from the list.
    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    if (engine.remove_fwpm_filter(id)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_INVALID_PARAMETER;
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS
    FwpmTransactionBegin0(_In_ _Acquires_lock_(_Curr_) HANDLE engine_handle, _In_ uint32_t flags)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    UNREFERENCED_PARAMETER(engine_handle);
    UNREFERENCED_PARAMETER(flags);
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmFilterAdd0(
    _In_ HANDLE engine_handle,
    _In_ const FWPM_FILTER0* filter,
    _In_opt_ PSECURITY_DESCRIPTOR sd,
    _Out_opt_ uint64_t* id)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    UNREFERENCED_PARAMETER(sd);

    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    auto id_returned = engine.add_fwpm_filter(filter);

    if (id) {
        *id = id_returned;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmTransactionCommit0(_In_ _Releases_lock_(_Curr_) HANDLE engine_handle)
{
    // Skip fault injection for this API because return failure status requires cleanup.
    UNREFERENCED_PARAMETER(engine_handle);
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmTransactionAbort0(_In_ _Releases_lock_(_Curr_) HANDLE engine_handle)
{
    // Skip fault injection for this API because return failure status requires cleanup.
    UNREFERENCED_PARAMETER(engine_handle);
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmCalloutAdd0(
    _In_ HANDLE engine_handle,
    _In_ const FWPM_CALLOUT0* callout,
    _In_opt_ PSECURITY_DESCRIPTOR sd,
    _Out_opt_ uint32_t* id)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    auto id_returned = engine.add_fwpm_callout(callout);

    if (id) {
        *id = id_returned;
    }
    UNREFERENCED_PARAMETER(sd);
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmCalloutDeleteByKey0(_In_ HANDLE engine_handle, _In_ const GUID* key)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    if (!engine.remove_fwpm_callout(key)) {
        return STATUS_NOT_FOUND;
    }
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmEngineOpen0(
    _In_opt_ const wchar_t* server_name,
    _In_ uint32_t authn_service,
    _In_opt_ SEC_WINNT_AUTH_IDENTITY_W* auth_identity,
    _In_opt_ const FWPM_SESSION0* session,
    _Out_ HANDLE* engine_handle)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    UNREFERENCED_PARAMETER(server_name);
    UNREFERENCED_PARAMETER(authn_service);
    UNREFERENCED_PARAMETER(auth_identity);
    UNREFERENCED_PARAMETER(session);

    *engine_handle = fwp_engine_t::get()->get();
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS
    FwpmProviderAdd0(_In_ HANDLE engine_handle, _In_ const FWPM_PROVIDER0* provider, _In_opt_ PSECURITY_DESCRIPTOR sd)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    engine.add_fwpm_provider(provider);

    UNREFERENCED_PARAMETER(sd);
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmProviderDeleteByKey0(_In_ HANDLE engine_handle, _In_ const GUID* key)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    engine.remove_fwpm_provider(key);
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS
    FwpmSubLayerAdd0(_In_ HANDLE engine_handle, _In_ const FWPM_SUBLAYER0* sub_layer, _In_opt_ PSECURITY_DESCRIPTOR sd)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    UNREFERENCED_PARAMETER(sd);
    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    engine.add_fwpm_sub_layer(sub_layer);

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS
    FwpmSubLayerDeleteByKey0(_In_ HANDLE engine_handle, _In_ const GUID* sub_layer_key)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    auto& engine = *reinterpret_cast<fwp_engine_t*>(engine_handle);

    if (!engine.remove_fwpm_sub_layer(sub_layer_key)) {
        return STATUS_NOT_FOUND;
    }
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpmEngineClose0(_Inout_ HANDLE engine_handle)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    if (engine_handle != fwp_engine_t::get()->get()) {
        return STATUS_INVALID_PARAMETER;
    } else {
        return STATUS_SUCCESS;
    }
}

#pragma endregion fwpm_apis

#pragma region fwps_apis

typedef struct _fwp_injection_handle
{
    ADDRESS_FAMILY address_family;
    uint32_t flags;
} fwp_injection_handle;

static std::unique_ptr<fwp_injection_handle> _injection_handle;

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS
    FwpsCalloutRegister3(_Inout_ void* device_object, _In_ const FWPS_CALLOUT3* callout, _Out_opt_ uint32_t* callout_id)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    UNREFERENCED_PARAMETER(device_object);

    auto& engine = *fwp_engine_t::get()->get();

    auto id_returned = engine.register_fwps_callout(callout);

    if (callout_id) {
        *callout_id = id_returned;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpsCalloutUnregisterById0(_In_ const uint32_t callout_id)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    auto& engine = *fwp_engine_t::get()->get();

    if (engine.remove_fwps_callout(callout_id)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_INVALID_PARAMETER;
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpsInjectionHandleCreate0(
    _In_opt_ ADDRESS_FAMILY address_family, _In_ uint32_t flags, _Out_ HANDLE* injection_handle)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    _injection_handle = std::make_unique<_fwp_injection_handle>(address_family, flags);
    *injection_handle = _injection_handle.get();

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS FwpsInjectionHandleDestroy0(_In_ HANDLE injection_handle)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    if (injection_handle != _injection_handle.get()) {
        return STATUS_INVALID_PARAMETER;
    } else {
        _injection_handle.reset();
        return STATUS_SUCCESS;
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS
    FwpsFlowRemoveContext0(_In_ uint64_t flow_id, _In_ UINT16 layer_id, _In_ uint32_t callout_id)
{
    // Skip fault injection.
    auto& engine = *fwp_engine_t::get()->get();
    engine.delete_flow_context(flow_id, layer_id, callout_id);

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS FwpsFlowAssociateContext0(
    _In_ uint64_t flow_id, _In_ UINT16 layer_id, _In_ uint32_t callout_id, _In_ uint64_t flowContext)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    UNREFERENCED_PARAMETER(layer_id);

    auto& engine = *fwp_engine_t::get()->get();
    engine.associate_flow_context(flow_id, callout_id, flowContext);

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS FwpsAllocateNetBufferAndNetBufferList0(
    _In_ NDIS_HANDLE pool_handle,
    _In_ uint16_t context_size,
    _In_ uint16_t context_backfill,
    _In_opt_ MDL* mdl_chain,
    _In_ unsigned long data_offset,
    _In_ size_t data_length,
    _Outptr_ NET_BUFFER_LIST** net_buffer_list)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    NTSTATUS status;
    NET_BUFFER_LIST* new_net_buffer_list = NULL;

    UNREFERENCED_PARAMETER(pool_handle);
    UNREFERENCED_PARAMETER(context_size);
    UNREFERENCED_PARAMETER(context_backfill);
    UNREFERENCED_PARAMETER(data_offset);

    new_net_buffer_list = (NET_BUFFER_LIST*)(ExAllocatePoolUninitialized(
        NonPagedPoolNx, sizeof(NET_BUFFER_LIST), USERSIM_TAG_NET_BUFFER_LIST));
    if (!new_net_buffer_list) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    RtlZeroMemory(new_net_buffer_list, sizeof(NET_BUFFER_LIST));

    new_net_buffer_list->FirstNetBuffer =
        (NET_BUFFER*)(ExAllocatePoolUninitialized(NonPagedPoolNx, sizeof(NET_BUFFER), USERSIM_TAG_NET_BUFFER));
    if (!new_net_buffer_list->FirstNetBuffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    RtlZeroMemory(new_net_buffer_list->FirstNetBuffer, sizeof(NET_BUFFER));

    new_net_buffer_list->FirstNetBuffer->MdlChain = mdl_chain;
    new_net_buffer_list->FirstNetBuffer->DataLength = (unsigned long)data_length;

    *net_buffer_list = new_net_buffer_list;
    new_net_buffer_list = NULL;
    status = STATUS_SUCCESS;

Done:
    if (new_net_buffer_list) {
        FwpsFreeNetBufferList0(new_net_buffer_list);
    }
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL) void FwpsFreeNetBufferList0(_In_ NET_BUFFER_LIST* net_buffer_list)
{
    if (!net_buffer_list) {
        return;
    }

    if (net_buffer_list->FirstNetBuffer) {
        ExFreePoolWithTag(net_buffer_list->FirstNetBuffer, USERSIM_TAG_NET_BUFFER);
    }

    ExFreePoolWithTag(net_buffer_list, USERSIM_TAG_NET_BUFFER_LIST);
}

_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(DISPATCH_LEVEL) _Must_inspect_result_ NTSTATUS
    FwpsInjectMacReceiveAsync0(
        _In_ HANDLE injection_handle,
        _In_opt_ HANDLE injection_context,
        _In_ uint32_t flags,
        _In_ UINT16 layer_id,
        _In_ IF_INDEX interface_index,
        _In_ NDIS_PORT_NUMBER ndis_port_number,
        _Inout_ NET_BUFFER_LIST* net_buffer_lists,
        _In_ FWPS_INJECT_COMPLETE completion_function,
        _In_opt_ HANDLE completion_context)
{
    UNREFERENCED_PARAMETER(injection_handle);
    UNREFERENCED_PARAMETER(injection_context);
    UNREFERENCED_PARAMETER(flags);
    UNREFERENCED_PARAMETER(layer_id);
    UNREFERENCED_PARAMETER(interface_index);
    UNREFERENCED_PARAMETER(ndis_port_number);
    UNREFERENCED_PARAMETER(net_buffer_lists);
    UNREFERENCED_PARAMETER(completion_function);
    UNREFERENCED_PARAMETER(completion_context);
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL) void FwpsFreeCloneNetBufferList0(
    _In_ NET_BUFFER_LIST* net_buffer_list, _In_ unsigned long free_clone_flags)
{
    NdisFreeCloneNetBufferList(net_buffer_list, free_clone_flags);
}

_IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS FwpsAllocateCloneNetBufferList0(
    _Inout_ NET_BUFFER_LIST* original_net_buffer_list,
    _In_opt_ NDIS_HANDLE net_buffer_list_pool_handle,
    _In_opt_ NDIS_HANDLE net_buffer_pool_handle,
    _In_ unsigned long allocate_clone_flags,
    _Outptr_ NET_BUFFER_LIST** net_buffer_list)
{
    // Skip fault injection, as it is already handled in NdisAllocateCloneNetBufferList
    if (net_buffer_list_pool_handle == nullptr || net_buffer_pool_handle == nullptr) {
        return STATUS_INVALID_PARAMETER;
    }
    *net_buffer_list = NdisAllocateCloneNetBufferList(
        original_net_buffer_list, net_buffer_list_pool_handle, net_buffer_pool_handle, allocate_clone_flags);
    return (*net_buffer_list) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(DISPATCH_LEVEL) _Must_inspect_result_ NTSTATUS
    FwpsInjectMacSendAsync0(
        _In_ HANDLE injection_handle,
        _In_opt_ HANDLE injection_context,
        _In_ uint32_t flags,
        _In_ UINT16 layer_id,
        _In_ IF_INDEX interface_index,
        _In_ NDIS_PORT_NUMBER ndis_port_number,
        _Inout_ NET_BUFFER_LIST* net_buffer_lists,
        _In_ FWPS_INJECT_COMPLETE completion_function,
        _In_opt_ HANDLE completion_context)
{
    UNREFERENCED_PARAMETER(injection_handle);
    UNREFERENCED_PARAMETER(injection_context);
    UNREFERENCED_PARAMETER(flags);
    UNREFERENCED_PARAMETER(layer_id);
    UNREFERENCED_PARAMETER(interface_index);
    UNREFERENCED_PARAMETER(ndis_port_number);
    UNREFERENCED_PARAMETER(net_buffer_lists);
    UNREFERENCED_PARAMETER(completion_function);
    UNREFERENCED_PARAMETER(completion_context);
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS NTAPI FwpsAcquireWritableLayerDataPointer0(
    _In_ UINT64 classifyHandle,
    _In_ UINT64 filterId,
    _In_ UINT32 flags,
    _Out_ void** writableLayerData,
    _Inout_opt_ FWPS_CLASSIFY_OUT0* classifyOut)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(classifyHandle);
    UNREFERENCED_PARAMETER(filterId);
    UNREFERENCED_PARAMETER(flags);
    UNREFERENCED_PARAMETER(classifyOut);

    *writableLayerData = _fwp_um_connect_request;
    if (*writableLayerData == nullptr) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS NTAPI
    FwpsAcquireClassifyHandle0(_In_ void* classifyContext, _In_ UINT32 flags, _Out_ UINT64* classifyHandle)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(flags);
    UNREFERENCED_PARAMETER(classifyHandle);

    *classifyHandle = 0;

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL) void NTAPI FwpsReleaseClassifyHandle0(_In_ UINT64 classifyHandle)
{
    UNREFERENCED_PARAMETER(classifyHandle);
}

_IRQL_requires_max_(DISPATCH_LEVEL) void NTAPI
    FwpsApplyModifiedLayerData0(_In_ UINT64 classifyHandle, _In_ void* modifiedLayerData, _In_ UINT32 flags)
{
    UNREFERENCED_PARAMETER(classifyHandle);
    UNREFERENCED_PARAMETER(flags);

    CXPLAT_DEBUG_ASSERT(modifiedLayerData != nullptr);
}

_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS NTAPI
    FwpsRedirectHandleCreate0(_In_ const GUID* providerGuid, _Reserved_ UINT32 flags, _Out_ HANDLE* redirectHandle)
{
    UNREFERENCED_PARAMETER(providerGuid);
    UNREFERENCED_PARAMETER(flags);

    // Fault injection is implicitly introduced by cxplat_allocate_with_tag().
    *redirectHandle = (HANDLE)cxplat_allocate(CXPLAT_POOL_FLAG_NON_PAGED, 1, USERSIM_TAG_HANDLE);
    if (*redirectHandle == nullptr) {
        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL) void NTAPI FwpsRedirectHandleDestroy0(_In_ HANDLE redirectHandle)
{
    cxplat_free(redirectHandle, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_HANDLE);
}

_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(DISPATCH_LEVEL) FWPS_CONNECTION_REDIRECT_STATE NTAPI
    FwpsQueryConnectionRedirectState0(
        _In_ HANDLE redirectRecords, _In_ HANDLE redirectHandle, _Outptr_opt_result_maybenull_ void** redirectContext)
{
    if (redirectContext) {
        *redirectContext = NULL;
    }

    if (cxplat_fault_injection_inject_fault()) {
        return FWPS_CONNECTION_REDIRECTED_BY_SELF;
    }

    UNREFERENCED_PARAMETER(redirectRecords);
    UNREFERENCED_PARAMETER(redirectHandle);

    return FWPS_CONNECTION_NOT_REDIRECTED;
}

#pragma endregion fwps_apis

#pragma region test_fwp

FWP_ACTION_TYPE
usersim_fwp_classify_packet(_In_ const GUID* layer_guid, NET_IFINDEX if_index)
{
    return fwp_engine_t::get()->classify_test_packet(layer_guid, if_index);
}

FWP_ACTION_TYPE
usersim_fwp_bind_ipv4(_In_ fwp_classify_parameters_t* parameters)
{
    return fwp_engine_t::get()->test_bind_ipv4(parameters);
}

FWP_ACTION_TYPE
usersim_fwp_cgroup_inet4_recv_accept(_In_ fwp_classify_parameters_t* parameters)
{
    return fwp_engine_t::get()->test_cgroup_inet4_recv_accept(parameters);
}

FWP_ACTION_TYPE
usersim_fwp_cgroup_inet6_recv_accept(_In_ fwp_classify_parameters_t* parameters)
{
    return fwp_engine_t::get()->test_cgroup_inet6_recv_accept(parameters);
}

FWP_ACTION_TYPE
usersim_fwp_cgroup_inet4_connect(_In_ fwp_classify_parameters_t* parameters)
{
    return fwp_engine_t::get()->test_cgroup_inet4_connect(parameters);
}

FWP_ACTION_TYPE
usersim_fwp_cgroup_inet6_connect(_In_ fwp_classify_parameters_t* parameters)
{
    return fwp_engine_t::get()->test_cgroup_inet6_connect(parameters);
}

FWP_ACTION_TYPE
usersim_fwp_sock_ops_v4(_In_ fwp_classify_parameters_t* parameters)
{
    return fwp_engine_t::get()->test_sock_ops_v4(parameters);
}

FWP_ACTION_TYPE
usersim_fwp_sock_ops_v6(_In_ fwp_classify_parameters_t* parameters)
{
    return fwp_engine_t::get()->test_sock_ops_v6(parameters);
}

void
usersim_fwp_set_sublayer_guids(
    _In_ const GUID& default_sublayer, _In_ const GUID& connect_v4_sublayer, _In_ const GUID& connect_v6_sublayer)
{
    fwp_engine_t::get()->set_sublayer_guids(default_sublayer, connect_v4_sublayer, connect_v6_sublayer);
}

#pragma endregion test_fwp
