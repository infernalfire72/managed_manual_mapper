// LePatchus.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <array>
#include <iostream>
#include <string_view>
#include <optional>

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

std::string GetErrorMessage(DWORD error_id) {
    if (error_id == 0) return std::string();

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

#include "shell.h"
#include "process.h"

struct Allocator {
    void* page;
    size_t offset;

    void* alloc(ptrdiff_t size) {
        const auto start{ reinterpret_cast<uintptr_t>(page) + offset };
        offset += size;
        return reinterpret_cast<void*>(start);
    }
};

struct Guid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
    uint8_t g;
    uint8_t h;
    uint8_t i;
    uint8_t j;
    uint8_t k;

    Guid(uint32_t a,
        uint16_t b,
        uint16_t c,
        uint8_t d,
        uint8_t e,
        uint8_t f,
        uint8_t g,
        uint8_t h,
        uint8_t i,
        uint8_t j,
        uint8_t k) {
        this->a = a;
        this->b = b;
        this->c = c;
        this->d = d;
        this->e = e;
        this->f = f;
        this->g = g;
        this->h = h;
        this->i = i;
        this->j = j;
        this->k = k;
    }
};

Guid IID_ICLRMetaHost{ 0xD332DB9E, 0xB9B3, 0x4125, 0x82, 0x07, 0xA1, 0x48, 0x84, 0xF5, 0x32, 0x16 };
Guid CLSID_CLRMetaHost{ 0x9280188d, 0xe8e, 0x4867, 0xb3, 0xc, 0x7f, 0xa8, 0x38, 0x84, 0xe8, 0xde };
Guid IID_ICorRuntimeHost{ 0xcb2f6722, 0xab3a, 0x11d2, 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e };
Guid CLSID_CorRuntimeHost{ 0xcb2f6723, 0xab3a, 0x11d2, 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e };

Guid IID_AppDomain{ 0x05f696dc, 0x2b29, 0x3663, 0xad, 0x8b, 0xc4, 0x38, 0x9c, 0xf2, 0xa7, 0x13 };

#include <vector>
#include <fstream>

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

int main()
{
    /*InjectShell* shell = new InjectShell(nullptr, 0xDEADBEEF, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    printf("%p %p\n", LeCLR, shell);
    Sleep(-1);
    return 0;*/

    const auto dll = load_from_file("patcher.dll");
    std::cout << dll.size() << std::endl;

    std::wstring args{ L"C:\\Users\\Ye\\AppData\\Local\\osu!\\osu!.exe -devserver akatsuki.gg" };
    const auto start_result{ start_unelevated(args) };
    if (!start_result.has_value()) {
        const auto error_id{ GetLastError() };
        std::cout << "Failed starting: (" << error_id << ") " << GetErrorMessage(error_id) << std::endl;
        return -1;
    }

    const auto process{ start_result.value() };

    std::cout << process.dwProcessId << std::endl;
    ResumeThread(process.hThread);
    Sleep(100);
    SuspendThread(process.hThread);

    using namespace std::literals;
    const auto [mscoree, oleaut32] = get_modules(process.hProcess, std::array{L"mscoree.dll"sv, L"oleaut32.dll"sv});
    if (!mscoree || !oleaut32) {
        std::cout << "Modules not found" << std::endl;
        return 0;
    }

    std::cout << "mscoree: " << mscoree << std::endl;
    std::cout << "oleaut32: " << oleaut32 << std::endl;

    const auto mscoree_result{ GetProcAddressesEx(process.hProcess, mscoree,
        std::array{"CLRCreateInstance"}) };
    if (!mscoree_result.has_value()) {
        return 0;
    }
    const auto clr_create_instance{ mscoree_result.value()[0] };
    printf("%p\n", (void*)clr_create_instance);

    const auto oleaut32_result{ GetProcAddressesEx(process.hProcess, oleaut32,
        std::array{"SafeArrayAccessData", "SafeArrayCreateVector", "SafeArrayDestroy", "SafeArrayUnaccessData"})};
    if (!oleaut32_result.has_value()) {
        return 0;
    }
    const auto& [access_data, create_vector, destroy, unaccess_data] { oleaut32_result.value() };
    printf("%p %p %p %p\n", access_data, create_vector, destroy, unaccess_data);


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

    // Setup Code
    const auto create_vector_shell{code_allocator.alloc(sizeof CreateVectorShell)};
    CreateVectorShell cv_shell(create_vector_shell, create_vector, dll.size());
    WriteProcessMemory(process.hProcess, create_vector_shell, &cv_shell, sizeof CreateVectorShell, nullptr);

    // Create managed byte array to hold the dll bytes
    const auto thread{ CreateRemoteThreadEx(process.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)create_vector_shell, nullptr, NULL, nullptr, nullptr) };
    WaitForSingleObject(thread, INFINITE);

    // Get the remote pointer to the data store of the managed byte array
    DWORD exit_code{ 0 };
    GetExitCodeThread(thread, &exit_code);
    void* safe_array{ reinterpret_cast<void*>(exit_code) };
    void* safe_array_data{ reinterpret_cast<void*>(exit_code + 0x18) };
    WriteProcessMemory(process.hProcess, safe_array_data, dll.data(), dll.size(), nullptr);

    const auto inject_shell{ code_allocator.alloc(sizeof InjectShell) };
    InjectShell i_shell(inject_shell, clr_create_instance, 
        clsid_clrmetahost, iid_iclrmetahost,
        clsid_corruntimehost, iid_icorruntimehost,
        iid_appdomain, safe_array);
    WriteProcessMemory(process.hProcess, inject_shell, &i_shell, sizeof InjectShell, nullptr);

    std::cout << inject_shell << std::endl;

    printf("%p\n", safe_array_data);

    ResumeThread(process.hThread);

    // LeCLR(process.hProcess, process.hThread);

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}