// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "usersim/common.h"

#include <ws2def.h>
#include <fwpmtypes.h>
#include <ifdef.h>
#include <stdint.h>

CXPLAT_EXTERN_C_BEGIN

typedef struct _fwp_classify_parameters
{
    ADDRESS_FAMILY family;
    uint32_t destination_ipv4_address;
    FWP_BYTE_ARRAY16 destination_ipv6_address;
    uint16_t destination_port;
    uint32_t source_ipv4_address;
    FWP_BYTE_ARRAY16 source_ipv6_address;
    uint16_t source_port;
    uint8_t protocol;
    uint32_t compartment_id;
    FWP_BYTE_BLOB app_id;
    uint64_t interface_luid;
    TOKEN_ACCESS_INFORMATION token_access_information;
    FWP_BYTE_BLOB user_id;
    uint32_t reauthorization_flag;
} fwp_classify_parameters_t;

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_classify_packet(_In_ const GUID* layer_guid, NET_IFINDEX if_index);

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_bind_ipv4(_In_ fwp_classify_parameters_t* parameters);

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_cgroup_inet4_recv_accept(_In_ fwp_classify_parameters_t* parameters);

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_cgroup_inet6_recv_accept(_In_ fwp_classify_parameters_t* parameters);

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_cgroup_inet4_connect(_In_ fwp_classify_parameters_t* parameters);

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_cgroup_inet6_connect(_In_ fwp_classify_parameters_t* parameters);

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_sock_ops_v4(_In_ fwp_classify_parameters_t* parameters, _Out_opt_ uint64_t* flow_id);

USERSIM_API FWP_ACTION_TYPE
usersim_fwp_sock_ops_v6(_In_ fwp_classify_parameters_t* parameters, _Out_opt_ uint64_t* flow_id);

USERSIM_API void
usersim_fwp_set_sublayer_guids(
    _In_ const GUID& default_sublayer, _In_ const GUID& connect_v4_sublayer, _In_ const GUID& connect_v6_sublayer);

USERSIM_API void
usersim_fwp_sock_ops_v4_remove_flow_context(_In_ uint64_t flow_id);

USERSIM_API void
usersim_fwp_sock_ops_v6_remove_flow_context(_In_ uint64_t flow_id);

CXPLAT_EXTERN_C_END
