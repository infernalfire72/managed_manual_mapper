#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <minwindef.h>
#include <Psapi.h>

DWORD GetProcessIdFromExecutableName(const std::wstring_view& executable_name) {
    const auto snapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };

    PROCESSENTRY32W process;
    ZeroMemory(&process, sizeof PROCESSENTRY32W);
    process.dwSize = sizeof PROCESSENTRY32W;
    auto success{ Process32FirstW(snapshot, &process) };

    while (success) {
        if (lstrcmpiW(process.szExeFile, executable_name.data()) == 0) {
            CloseHandle(snapshot);
            return process.th32ProcessID;
        }
        success = Process32NextW(snapshot, &process);
    }

    CloseHandle(snapshot);
    return NULL;
}

std::optional<PROCESS_INFORMATION> start_unelevated(std::wstring& args) {
    const auto explorer_id{ GetProcessIdFromExecutableName(L"explorer.exe") };
    if (explorer_id == NULL) {
        return std::nullopt;
    }

    HANDLE explorer{ OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, explorer_id) };
    if (explorer == NULL) {
        return std::nullopt;
    }

    HANDLE explorer_token;
    const auto open_success{ OpenProcessToken(explorer, TOKEN_DUPLICATE, &explorer_token) };
    CloseHandle(explorer);
    if (!open_success) {
        return std::nullopt;
    }

    HANDLE dup_explorer_token;
    const auto dup_success{ DuplicateTokenEx(explorer_token, MAXIMUM_ALLOWED, NULL,
        SecurityImpersonation, TokenPrimary, &dup_explorer_token) };
    CloseHandle(explorer_token);
    if (!dup_success) {
        return std::nullopt;
    }

    STARTUPINFOW startup_info;
    ZeroMemory(&startup_info, sizeof STARTUPINFOW);
    startup_info.cb = sizeof STARTUPINFOW;

    PROCESS_INFORMATION process_info;
    ZeroMemory(&process_info, sizeof PROCESS_INFORMATION);

    const auto create_process_success{ CreateProcessWithTokenW(
        dup_explorer_token,
        LOGON_WITH_PROFILE,
        nullptr,
        args.data(),
        CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED,
        nullptr,
        nullptr,
        &startup_info,
        &process_info
    ) };
    CloseHandle(dup_explorer_token);
    if (!create_process_success) {
        return std::nullopt;
    }

    return process_info;
}

bool wsuffixi(wchar_t* a, const std::wstring_view& b) {
    const auto a_len{ lstrlenW(a) };
    const auto a_offset{ a_len - b.size() };
    return lstrcmpiW(&a[a_offset], b.data()) == 0;
}

template <size_t N>
auto get_modules(
    HANDLE process,
    const std::array<std::wstring_view, N>& module_names
) -> std::array<HMODULE, N> {
    HMODULE modules[64];
    DWORD needed;

    std::array<HMODULE, N> results{ 0 };
    const auto result{ EnumProcessModulesEx(
        process,
        modules,
        sizeof(modules),
        &needed,
        LIST_MODULES_ALL) };
    if (!result) {
        return results;
    }
    
    int count = needed / sizeof(HMODULE);
    if (count > 64) {
        count = 64;
    }

    for (int i = 0; i < count; i++) {
        wchar_t name[MAX_PATH];
        const auto module_handle{ modules[i] };
        GetModuleFileNameExW(process, module_handle, name, MAX_PATH);

        wprintf(L"%s\n", name);
        for (size_t j{ 0 }; j < N; j++) {
            if (wsuffixi(name, module_names[j])) {
                results[j] = module_handle;
                break;
            }
        }
    }

    return results;
}

bool RPM(HANDLE process, const HMODULE module_base, ptrdiff_t offset, void* dst, size_t size) {
    const auto address{ reinterpret_cast<uintptr_t>(module_base) + offset };
    return ReadProcessMemory(process, reinterpret_cast<void*>(address), dst, size, nullptr);
}

template<size_t N>
auto GetProcAddressesEx(
    HANDLE process,
    const HMODULE module_base,
    std::array<const char*, N> function_names
) -> std::optional<std::array<uintptr_t, N>> {
    IMAGE_DOS_HEADER dos;
    if (!RPM(process, module_base, 0, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE)
        return std::nullopt;

    IMAGE_NT_HEADERS32 nt;
    if (!RPM(process, module_base, dos.e_lfanew, &nt, sizeof(nt)))
        return std::nullopt;

    DWORD exp_rva{ nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress };
    if (!exp_rva)
        return std::nullopt;

    IMAGE_EXPORT_DIRECTORY exp;
    if (!RPM(process, module_base, exp_rva, &exp, sizeof(exp)))
        return std::nullopt;

    DWORD* functions{ (DWORD*)malloc(exp.NumberOfFunctions * sizeof(DWORD)) };
    DWORD* name_rvas{ (DWORD*)malloc(exp.NumberOfNames * sizeof(DWORD)) };
    WORD* ordinals{ (WORD*)malloc(exp.NumberOfNames * sizeof(WORD)) };

    RPM(process, module_base, exp.AddressOfFunctions, functions, exp.NumberOfFunctions * sizeof(DWORD));
    RPM(process, module_base, exp.AddressOfNames, name_rvas, exp.NumberOfNames * sizeof(DWORD));
    RPM(process, module_base, exp.AddressOfNameOrdinals, ordinals, exp.NumberOfNames * sizeof(WORD));

    std::array<uintptr_t, N> results{ 0 };
    for (DWORD i = 0; i < exp.NumberOfNames; i++) {
        if (name_rvas[i] == 0) {
            continue;
        }
        char name[256];
        RPM(process, module_base, name_rvas[i], name, sizeof(name));

        for (size_t j{ 0 }; j < N; j++) {
            if (strcmp(name, function_names[j]) == 0) {
                WORD ord{ ordinals[i] };
                DWORD func_rva{ functions[ord] };

                uintptr_t func_addr{ reinterpret_cast<uintptr_t>(module_base) + func_rva };
                results[j] = func_addr;
                break;
            }
        }
    }

    free(name_rvas);
    free(ordinals);
    free(functions);
    return results;
}