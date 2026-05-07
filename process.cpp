#include <cassert>
#include <functional>
#include <optional>
#include <string_view>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

namespace Process {
    DWORD get_process_id(const std::wstring_view& executable_name) {
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

    HANDLE steal_explorer_token() {
        const auto explorer_id{ get_process_id(L"explorer.exe") };
        if (explorer_id == NULL) {
            return INVALID_HANDLE_VALUE;
        }

        HANDLE explorer{ OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, explorer_id) };
        if (explorer == NULL) {
            return INVALID_HANDLE_VALUE;
        }

        HANDLE explorer_token;
        const auto open_success{ OpenProcessToken(explorer, TOKEN_DUPLICATE, &explorer_token) };
        CloseHandle(explorer);
        if (!open_success) {
            return INVALID_HANDLE_VALUE;
        }

        HANDLE dup_explorer_token;
        const auto dup_success{ DuplicateTokenEx(explorer_token, MAXIMUM_ALLOWED, NULL,
            SecurityImpersonation, TokenPrimary, &dup_explorer_token) };
        CloseHandle(explorer_token);
        if (!dup_success) {
            return INVALID_HANDLE_VALUE;
        }

        return dup_explorer_token;
    }

    std::optional<PROCESS_INFORMATION> start_unelevated(std::wstring& args) {
        const auto explorer_token{ steal_explorer_token() };

        STARTUPINFOW startup_info;
        ZeroMemory(&startup_info, sizeof STARTUPINFOW);
        startup_info.cb = sizeof STARTUPINFOW;

        PROCESS_INFORMATION process_info;
        ZeroMemory(&process_info, sizeof PROCESS_INFORMATION);

        const auto create_process_success{ CreateProcessWithTokenW(
            explorer_token,
            LOGON_WITH_PROFILE,
            nullptr,
            args.data(),
            CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED,
            nullptr,
            nullptr,
            &startup_info,
            &process_info
        ) };
        CloseHandle(explorer_token);
        if (!create_process_success) {
            return std::nullopt;
        }

        return process_info;
    }    

    bool resolve_module_handles(
        HANDLE process,
        std::function<void(wchar_t*, HMODULE)> callback
    ) {
        HMODULE modules[64];
        DWORD needed;
        const auto result{ EnumProcessModulesEx(
            process,
            modules,
            sizeof(modules),
            &needed,
            LIST_MODULES_ALL) };
        if (!result) {
            return false;
        }

        int count = needed / sizeof(HMODULE);
        if (count > 64) {
            count = 64;
        }

        for (int i = 0; i < count; i++) {
            wchar_t name[MAX_PATH];
            const auto module_handle{ modules[i] };
            GetModuleFileNameExW(process, module_handle, name, MAX_PATH);

            callback(name, module_handle);
        }

        return true;
    }

    bool resolve_function_names(
        HANDLE process,
        const HMODULE module_base,
        std::function<void(char*, uintptr_t)> callback
    ) {
        const auto rpm = [&](ptrdiff_t offset, void* dst, size_t size) -> bool {
            const auto address{ reinterpret_cast<uintptr_t>(module_base) + offset };
            return ReadProcessMemory(process, reinterpret_cast<void*>(address), dst, size, nullptr);
        };

        IMAGE_DOS_HEADER dos;
        if (!rpm(0, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE)
            return false;

        IMAGE_NT_HEADERS32 nt;
        if (!rpm(dos.e_lfanew, &nt, sizeof(nt)))
            return false;

        DWORD exp_rva{ nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress };
        if (!exp_rva)
            return false;

        IMAGE_EXPORT_DIRECTORY exp;
        if (!rpm(exp_rva, &exp, sizeof(exp)))
            return false;

        DWORD* functions{ (DWORD*)malloc(exp.NumberOfFunctions * sizeof(DWORD)) };
        DWORD* name_rvas{ (DWORD*)malloc(exp.NumberOfNames * sizeof(DWORD)) };
        WORD* ordinals{ (WORD*)malloc(exp.NumberOfNames * sizeof(WORD)) };

        __assume(functions && name_rvas && ordinals); // lol

        rpm(exp.AddressOfFunctions, functions, exp.NumberOfFunctions * sizeof(DWORD));
        rpm(exp.AddressOfNames, name_rvas, exp.NumberOfNames * sizeof(DWORD));
        rpm(exp.AddressOfNameOrdinals, ordinals, exp.NumberOfNames * sizeof(WORD));

        for (DWORD i = 0; i < exp.NumberOfNames; i++) {
            if (name_rvas[i] == 0) {
                continue;
            }
            char name[256];
            rpm(name_rvas[i], name, sizeof(name));

            WORD ord{ ordinals[i] };
            DWORD func_rva{ functions[ord] };
            uintptr_t func_addr{ reinterpret_cast<uintptr_t>(module_base) + func_rva };
            callback(name, func_addr);
        }

        free(name_rvas);
        free(ordinals);
        free(functions);
        return true;
    }
}