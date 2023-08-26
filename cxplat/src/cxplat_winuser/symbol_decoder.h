// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "winuser_internal.h"
#include <windows.h>
#include <DbgHelp.h>
#include <optional>
#include <string>
#include <vector>

static bool _symbol_decoder_initialized = false;

inline cxplat_status_t
_cxplat_symbol_decoder_initialize()
{
    // Initialize DbgHelp.dll.
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(GetCurrentProcess(), nullptr, TRUE)) {
        int err = GetLastError();
        if (err == ERROR_INVALID_PARAMETER) {
            // The process already did SymInitialize(), which is fine.
            return CXPLAT_STATUS_SUCCESS;
        }
        return CXPLAT_STATUS_FROM_WIN32(GetLastError());
    }
    _symbol_decoder_initialized = true;
    return CXPLAT_STATUS_SUCCESS;
}

inline void
_cxplat_symbol_decoder_deinitialize()
{
    if (_symbol_decoder_initialized) {
        SymCleanup(GetCurrentProcess());
    }
}

inline cxplat_status_t
_cxplat_decode_symbol(
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
            return CXPLAT_STATUS_FROM_WIN32(GetLastError());
        }

        name = symbol->Name;
        unsigned long displacement32 = (unsigned long)displacement;

        if (!SymGetLineFromAddr64(GetCurrentProcess(), address, &displacement32, &line)) {
            line_number = std::nullopt;
            file_name = std::nullopt;
            return CXPLAT_STATUS_SUCCESS;
        }

        line_number = line.LineNumber;
        file_name = line.FileName;
        return CXPLAT_STATUS_SUCCESS;
    } catch (std::bad_alloc&) {
        return CXPLAT_STATUS_NO_MEMORY;
    }
}
