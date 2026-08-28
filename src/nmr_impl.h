// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "kernel_um.h"
#include "platform.h"

#include <../km/netioddk.h>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <netiodef.h>
#include <optional>
#include <vector>

typedef class nmr_t
{
  public:
    typedef void* nmr_provider_handle;
    typedef void* nmr_client_handle;
    typedef void* nmr_binding_handle;

    nmr_t() = default;
    ~nmr_t() = default;

    /**
     * @brief Register a provider.
     *
     * @param[in] characteristics Characteristics of the provider.
     * @param[in] context Context passed to the provider.
     * @return Handle to the provider module.
     */
    nmr_provider_handle
    register_provider(_In_ const NPI_PROVIDER_CHARACTERISTICS& characteristics, _In_opt_ const void* context);

    /**
     * @brief Deregister a provider. The caller must wait for completion.
     *
     * @param[in] provider_handle Handle to the provider.
     */
    void
    deregister_provider(_In_ nmr_provider_handle provider_handle);

    /**
     * @brief Wait for deregistration to complete.
     *
     * @param[in] provider_handle Handle to the provider.
     */
    void
    wait_for_deregister_provider(_In_ nmr_provider_handle provider_handle);

    /**
     * @brief Register a client.
     *
     * @param[in] characteristics Characteristics of the client.
     * @param[in] context Context passed to the client.
     * @return Handle to the client module.
     */
    nmr_client_handle
    register_client(_In_ const NPI_CLIENT_CHARACTERISTICS& characteristics, _In_opt_ const void* context);

    /**
     * @brief Deregister a client. The caller must wait for completion.
     *
     * @param[in] client_handle Handle to the client.
     */
    void
    deregister_client(_In_ nmr_client_handle client_handle);

    /**
     * @brief Wait for deregistration to complete.
     *
     * @param[in] client_handle Handle to the client.
     */
    void
    wait_for_deregister_client(_In_ nmr_client_handle client_handle);

    /**
     * @brief Signal that a client detach is complete.
     *
     * @param[in] binding_handle NMR binding handle.
     */
    void
    binding_detach_client_complete(_In_ nmr_binding_handle binding_handle);

    /**
     * @brief Signal that a provider detach is complete.
     *
     * @param[in] binding_handle NMR binding handle.
     */
    void
    binding_detach_provider_complete(_In_ nmr_binding_handle binding_handle);

    /**
     * @brief Callback from the client to complete an attach.
     *
     * @param[in] binding_handle Binding handle passed to the client.
     * @param[in] client_binding_context Client's per binding context.
     * @param[in] client_dispatch Client's dispatch table.
     * @param[out] provider_binding_context Provider's per binding context.
     * @param[out] provider_dispatch Provider's dispatch table.
     * @retval STATUS_SUCCESS The client module was successfully attached to the provider module.
     * @retval STATUS_NOINTERFACE The provider module did not attach to the client module.
     * @retval Other status codes An error occurred.
     */
    NTSTATUS
    client_attach_provider(
        _In_ nmr_binding_handle binding_handle,
        _In_ __drv_aliasesMem const void* client_binding_context,
        _In_ const void* client_dispatch,
        _Outptr_ const void** provider_binding_context,
        _Outptr_ const void** provider_dispatch);

    static nmr_t&
    get()
    {
        return singleton;
    }

  private:
    struct binding;

    struct client_module
    {
        const NPI_CLIENT_CHARACTERISTICS characteristics = {};
        const void* context = nullptr;
        size_t pending_bind_ops = 0;
        std::vector<std::shared_ptr<binding>> bindings;
        bool deregistering = false;
    };

    struct provider_module
    {
        const NPI_PROVIDER_CHARACTERISTICS characteristics = {};
        const void* context = nullptr;
        size_t pending_bind_ops = 0;
        std::vector<std::shared_ptr<binding>> bindings;
        bool deregistering = false;
    };

    enum binding_status
    {
        Start = 0,   ///< Initial state. Binding has been created but ClientAttachProvider has not been called.
        Ready,       ///< ClientAttachProvider has been called and returned STATUS_SUCCESS.
        LateBind,    ///< Attach completed after one of the modules started deregistering.
        BeginUnbind, ///< Client or provider has called NmrDeregisterClient or NmrDeregisterProvider but detach has not
                     ///< yet been called.
        UnbindPending, ///< Client or provider detach returned STATUS_PENDING.
        UnbindComplete ///< Client or provider detach returned STATUS_SUCCESS or called NmrBindingDetachClientComplete
                       ///< or NmrBindingDetachProviderComplete.
    };
    struct binding
    {
        provider_module& provider;
        client_module& client;
        const void* provider_binding_context = nullptr;
        const void* provider_dispatch = nullptr;
        binding_status provider_binding_status = Start;
        const void* client_binding_context = nullptr;
        const void* client_dispatch = nullptr;
        binding_status client_binding_status = Start;
        bool attached = false;
        bool cleanup_started = false;
    };
    typedef std::function<void()> pending_action_t;

    // The NMR operations are mostly symmetric with respect to providers and
    // clients. As a result, the operations are implemented as a single set
    // templated function with the template parameter being the type of the
    // NMR entity being acted on (provider or client).

    /**
     * @brief Add a provider or client to the correct collection.
     *
     * @param[in, out] collection Collection to add to.
     * @param[in] characteristics Characteristics of the provider or client.
     * @param[in] context Context handle to return to the caller.
     * @return Handle to the provider or client.
     */
    template <typename collection_t, typename characteristics_t>
    collection_t::value_type::first_type
    add(_Inout_ collection_t& collection, _In_ const characteristics_t& characteristics, _In_opt_ const void* context);

    /**
     * @brief Begin the process of deregistering a provider or client.
     *
     * @param[in, out] collection Collection to deregister from.
     * @param[in] handle Handle to the provider or client.
     */
    template <typename collection_t>
    void
    deactivate(_Inout_ collection_t& collection, _In_ collection_t::value_type::first_type handle);

    /**
     * @brief Finish removing a provider or client from the correct collection.
     *
     * @param[in, out] collection Collection to remove from.
     * @param[in] handle Handle to the provider or client.
     */
    template <typename collection_t>
    void
    remove(_Inout_ collection_t& collection, _In_ collection_t::value_type::first_type handle);

    /**
     * @brief Perform a bind using an entry from the initiator_collection
     * and all entries from the target_collection.
     *
     * @param[in, out] initiator_collection Collection containing the initiator (can be either provider or client).
     * @param[in] handle Handle to the initiator.
     * @param[in, out] target_collection Collection containing all the targets (can be either provider or client).
     */
    template <typename initiator_collection_t, typename target_collection_t>
    void
    perform_bind(
        _Inout_ initiator_collection_t& initiator_collection,
        _In_ initiator_collection_t::value_type::first_type initiator_handle,
        _Inout_ target_collection_t& target_collection);

    /**
     * @brief Unbind a provider or client from all other providers or clients.
     *
     * @param[in, out] initiator_collection Collection containing the initiator (can be either provider or client).
     * @param[in] handle Handle to the initiator (can be either provider or client).
     */
    template <typename initiator_collection_t>
    void
    perform_unbind(
        _Inout_ initiator_collection_t& initiator_collection,
        _In_ initiator_collection_t::value_type::first_type initiator_handle);

    /**
     * @brief Attempt to bind a client to a provider.
     *
     * @param[in, out] client Client to attempt to bind.
     * @param[in, out] provider Provider to attempt to bind to.
     * @return Contains a function to perform the bind if accepted.
     */
    std::optional<pending_action_t>
    bind(_Inout_ client_module& client, _Inout_ provider_module& provider);

    /**
     * @brief Finish the process of unbinding a client from a provider.
     *
     * @param[in] binding_handle Binding handle to unbind.
     */
    void
    unbind_complete(_Inout_ binding& binding);

    /**
     * @brief Start the process of unbinding a client from a provider.
     *
     * @param[in] binding_handle Binding handle to unbind.
     */
    void
    begin_unbind(_Inout_ binding& binding);

    // Binding handle is a pointer to the binding.
    std::map<nmr_binding_handle, std::shared_ptr<binding>> bindings;
    // Provider and client handles.
    std::map<nmr_provider_handle, provider_module> providers;
    std::map<nmr_client_handle, client_module> clients;

    size_t next_handle = 1;

    std::condition_variable bindings_changed;
    std::mutex lock; // Protects all of the instance variables above, as well as
                     // the client_binding_status and provider_binding_status of
                     // each binding.

    static nmr_t singleton;
} nmr_t;
