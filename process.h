#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string_view>

#include "util.h"

namespace Process {
    std::optional<PROCESS_INFORMATION> start_unelevated(std::wstring& args);

    bool resolve_module_handles(
        HANDLE process,
        std::function<void(wchar_t*, HMODULE)> callback);

    bool resolve_function_names(
        HANDLE process, const HMODULE module_base,
        std::function<void(char*, uintptr_t)> callback);

    template <size_t N>
    auto get_modules(
        HANDLE process,
        const std::array<std::wstring_view, N>& module_names
    ) -> std::array<HMODULE, N> {
        std::array<HMODULE, N> results{ 0 };
        const auto callback = [&](wchar_t* name, HMODULE module_handle) {
            for (size_t j{ 0 }; j < N; j++) {
                if (wsuffixi(name, module_names[j])) {
                    results[j] = module_handle;
                    break;
                }
            }
            };

        resolve_module_handles(process, callback);
        return results;
    }

    template<size_t N>
    auto resolve_functions(
        HANDLE process,
        const HMODULE module_base,
        std::array<const char*, N> function_names
    ) -> std::optional<std::array<uintptr_t, N>> {
        std::array<uintptr_t, N> results{ 0 };
        const auto callback = [&](char* name, uintptr_t func_addr) {
            for (size_t j{ 0 }; j < N; j++) {
                if (strcmp(name, function_names[j]) == 0) {
                    results[j] = func_addr;
                    break;
                }
            }
            };

        if (resolve_function_names(process, module_base, callback)) {
            return results;
        }
        else {
            return std::nullopt;
        }
    }
}