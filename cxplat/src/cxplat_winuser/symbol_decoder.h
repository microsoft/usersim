// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "platform.h"

#include <DbgHelp.h>
#include <optional>
#include <string>
#include <vector>

inline NTSTATUS
_usersim_symbol_decoder_initialize()
{
    // Initialize DbgHelp.dll.
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(GetCurrentProcess(), nullptr, TRUE)) {
        return STATUS_NO_MEMORY;
    }
    return STATUS_SUCCESS;
}

inline void
_usersim_symbol_decoder_deinitialize()
{
    SymCleanup(GetCurrentProcess());
}

inline NTSTATUS
_usersim_decode_symbol(
    uintptr_t address,
    std::string& name,
    uint64_t& displacement,
    std::optional<uint32_t>& line_number,
    std::optional<std::string>& file_name)
{
    try {

        std::vector<uint8_t> symbol_buffer(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR));
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer.data());
        IMAGEHLP_LINE64 line;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        if (!SymFromAddr(GetCurrentProcess(), address, &displacement, symbol)) {
            return STATUS_NO_MEMORY;
        }

        name = symbol->Name;
        unsigned long displacement32 = (unsigned long)displacement;

        if (!SymGetLineFromAddr64(GetCurrentProcess(), address, &displacement32, &line)) {
            line_number = std::nullopt;
            file_name = std::nullopt;
            return STATUS_SUCCESS;
        }

        line_number = line.LineNumber;
        file_name = line.FileName;
        return STATUS_SUCCESS;
    } catch (std::bad_alloc&) {
        return STATUS_NO_MEMORY;
    }
}
