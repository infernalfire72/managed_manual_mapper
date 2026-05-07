// LePatchus.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <array>
#include <iostream>

#include <fstream>
#include <string_view>
#include <optional>
#include <vector>
#include <Windows.h>


#include "guid.h"
#include "shell.h"

#include "util.h"
#include "process.h"

using namespace std::literals;

struct Allocator {
    void* page;
    size_t offset;

    void* alloc(ptrdiff_t size) {
        const auto start{ reinterpret_cast<uintptr_t>(page) + offset };
        offset += size;
        return reinterpret_cast<void*>(start);
    }
};

std::vector<uint8_t> load_from_file(std::string_view path) {
    std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) return std::vector<uint8_t>();
    const size_t pos = file.tellg();
    if (!pos) return std::vector<uint8_t>();
    std::vector<uint8_t> vec(pos);
    file.seekg(0, std::ios::beg);
    file.read((char*)&vec[0], pos);
    file.close();
    return vec;
}

bool start_and_inject(
    std::wstring& cmdline,
    std::vector<uint8_t> dll_bytes,
    const std::wstring_view& entry_type,
    const std::wstring_view& entry_method
) {
    const auto start_result{ Process::start_unelevated(cmdline) };
    if (!start_result.has_value()) {
        const auto error_id{ GetLastError() };
        std::cout << "Failed starting: (" << error_id << ") " << GetErrorMessage(error_id) << std::endl;
        return false;
    }

    const auto process{ start_result.value() };

    std::cout << process.dwProcessId << std::endl;
    ResumeThread(process.hThread);
    Sleep(100);
    SuspendThread(process.hThread);

    const auto [mscoree, oleaut32] = Process::get_modules(process.hProcess, std::array{ L"mscoree.dll"sv, L"oleaut32.dll"sv });
    if (!mscoree || !oleaut32) {
        std::cout << "Modules not found" << std::endl;
        return 0;
    }

    std::cout << "mscoree: " << mscoree << std::endl;
    std::cout << "oleaut32: " << oleaut32 << std::endl;

    const auto mscoree_result{ Process::resolve_functions(process.hProcess, mscoree,
        std::array{"CLRCreateInstance"}) };
    if (!mscoree_result.has_value()) {
        return 0;
    }
    const auto clr_create_instance{ mscoree_result.value()[0] };
    printf("%p\n", (void*)clr_create_instance);

    const auto oleaut32_result{ Process::resolve_functions(process.hProcess, oleaut32,
        std::array{"SafeArrayCreateVector", "SafeArrayDestroy", "SafeArrayPutElement", "SysAllocString", "VariantInit"}) };
    if (!oleaut32_result.has_value()) {
        return 0;
    }
    const auto& [create_vector, destroy, put_elem, sys_alloc_string, variant_init] { oleaut32_result.value() };
    printf("%p %p %p %p\n", create_vector, destroy, sys_alloc_string, variant_init);


    // Allocate and prepare remote memory
    const auto shared_memory{ VirtualAllocEx(process.hProcess, nullptr, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE) };
    Allocator code_allocator = { .page = shared_memory, .offset = 0 };
    const auto data_start{ reinterpret_cast<uintptr_t>(shared_memory) + 2048 };
    Allocator data_allocator = { .page = reinterpret_cast<void*>(data_start), .offset = 0 };

    // Setup Data
    const auto iid_iclrmetahost{ data_allocator.alloc(sizeof Guid) };
    WriteProcessMemory(process.hProcess, iid_iclrmetahost, reinterpret_cast<void*>(&IID_ICLRMetaHost), sizeof Guid, nullptr);

    const auto clsid_clrmetahost{ data_allocator.alloc(sizeof Guid) };
    WriteProcessMemory(process.hProcess, clsid_clrmetahost, reinterpret_cast<void*>(&CLSID_CLRMetaHost), sizeof Guid, nullptr);

    const auto iid_icorruntimehost{ data_allocator.alloc(sizeof Guid) };
    WriteProcessMemory(process.hProcess, iid_icorruntimehost, reinterpret_cast<void*>(&IID_ICorRuntimeHost), sizeof Guid, nullptr);

    const auto clsid_corruntimehost{ data_allocator.alloc(sizeof Guid) };
    WriteProcessMemory(process.hProcess, clsid_corruntimehost, reinterpret_cast<void*>(&CLSID_CorRuntimeHost), sizeof Guid, nullptr);

    const auto iid_appdomain{ data_allocator.alloc(sizeof Guid) };
    WriteProcessMemory(process.hProcess, iid_appdomain, reinterpret_cast<void*>(&IID_AppDomain), sizeof Guid, nullptr);

    const auto entry_type_name_size{ (entry_type.size() + 1) * sizeof(wchar_t) };
    const auto entry_type_name{ data_allocator.alloc(entry_type_name_size) };
    WriteProcessMemory(process.hProcess, entry_type_name, entry_type.data(), entry_type_name_size, nullptr);

    const auto entry_method_name_size{ (entry_method.size() + 1) * sizeof(wchar_t) };
    const auto entry_method_name{ data_allocator.alloc(entry_method_name_size) };
    WriteProcessMemory(process.hProcess, entry_method_name, entry_method.data(), entry_method_name_size, nullptr);

    // Setup Code
    const auto create_vector_shell{ code_allocator.alloc(sizeof CreateVectorShell) };
    CreateVectorShell cv_shell(create_vector_shell, create_vector, dll_bytes.size());
    WriteProcessMemory(process.hProcess, create_vector_shell, &cv_shell, sizeof CreateVectorShell, nullptr);

    // Create managed byte array to hold the dll bytes
    const auto alloc_thread{ CreateRemoteThreadEx(process.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)create_vector_shell, nullptr, NULL, nullptr, nullptr) };
    WaitForSingleObject(alloc_thread, INFINITE);

    // Get the remote pointer to the managed byte array
    DWORD exit_code{ 0 };
    GetExitCodeThread(alloc_thread, &exit_code);
    CloseHandle(alloc_thread);
    void* safe_array{ reinterpret_cast<void*>(exit_code) };
    void* safe_array_data{ reinterpret_cast<void*>(exit_code + 0x18) };
    WriteProcessMemory(process.hProcess, safe_array_data, dll_bytes.data(), dll_bytes.size(), nullptr);

    const auto inject_shell{ code_allocator.alloc(sizeof InjectShell) };
    InjectShell i_shell(inject_shell,
        clr_create_instance, sys_alloc_string, variant_init, create_vector, put_elem,
        clsid_clrmetahost, iid_iclrmetahost,
        clsid_corruntimehost, iid_icorruntimehost,
        iid_appdomain, safe_array, entry_type_name, entry_method_name);
    WriteProcessMemory(process.hProcess, inject_shell, &i_shell, sizeof InjectShell, nullptr);

    std::cout << inject_shell << std::endl;
    const auto inject_thread{ CreateRemoteThreadEx(process.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)inject_shell, nullptr, NULL, nullptr, nullptr) };
    WaitForSingleObject(inject_thread, INFINITE);

    // Get the remote pointer to the managed byte array
    DWORD inject_exit_code{ 0 };
    GetExitCodeThread(inject_thread, &inject_exit_code);
    CloseHandle(inject_thread);

    ResumeThread(process.hThread);

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (!inject_exit_code) {
        std::cout << "Failed injecting..." << std::endl;
    }
}

int main()
{
    const auto dll = load_from_file("patcher.dll");
    const std::wstring_view entry_type{ L"Akatsuki.Patcher.Main"sv };
    const std::wstring_view entry_method{ L"Initialize"sv };

    std::wstring args{ L"C:\\Users\\Ye\\AppData\\Local\\osu!\\osu!.exe -devserver akatsuki.gg" };
    start_and_inject(args, dll, entry_type, entry_method);
    
}