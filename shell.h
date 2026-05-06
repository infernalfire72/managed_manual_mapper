#pragma once

#include <cstddef>
#include <cstdint>

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

struct CreateVectorShell {
    uint8_t push{ 0x68 };
    uint32_t size{ 0 };
    uint8_t push2{ 0x6a };
    uint8_t lbound{ 0 };
    uint8_t push3{ 0x6a };
    uint8_t vt{ 0x11 };
    uint8_t call{ 0xe8 };
    uint32_t rel_call{ 0 };
    uint8_t ret{ 0xc3 };

    CreateVectorShell(void* at, uintptr_t create_vector, uint32_t size) {
        this->size = size;
        const auto ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(CreateVectorShell, rel_call) + sizeof uintptr_t };
        rel_call = (create_vector - ret_addr);
    }
};

constexpr int8_t kMetaHostOffset{ -static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kEnumeratorOffset{ kMetaHostOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kRuntimeCountOffset{ kEnumeratorOffset - static_cast<int8_t>(sizeof(ULONG)) };
constexpr int8_t kRuntimeInfoOffset{ kRuntimeCountOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kRuntimeHostOffset{ kRuntimeInfoOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kDefaultDomainOffset{ kRuntimeHostOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kAppDomainOffset{ kDefaultDomainOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kLoadedAssemblyOffset{ kAppDomainOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr uint8_t kReservedStack{ -kLoadedAssemblyOffset };
struct InjectShell {
    uint8_t push_ebp{ 0x55 };
    uint16_t mov_ebp_esp{ 0xEC8B };
    uint16_t sub_esp{ 0xEC83 }; uint8_t reserved_stack{ kReservedStack };

    // create clr meta host
    uint16_t lea_eax_deref_add_ebp{ 0x458D }; int8_t add_{ kMetaHostOffset }; // -4
    uint8_t push_eax{ 0x50 };
    uint8_t _push{ 0x68 }; void* iid_iclrmetahost{ nullptr };
    uint8_t __push{ 0x68 }; void* clsid_clrmetahost{ nullptr };
    uint8_t call{ 0xE8 }; uintptr_t rel_call{ NULL };

    // get EnumerateLoadedRuntimes from vtable
    uint16_t mov_ecx_deref_add_ebp{ 0x4D8B }; int8_t add_a{ kMetaHostOffset }; // -4 mov ecx, meta_host
    uint16_t mov_eax_deref_ecx{ 0x018B }; // vtable = *meta_host
    uint16_t lea_edx_deref_add_ebp{ 0x558D }; int8_t add_b{ kEnumeratorOffset }; // -8, enumerator
    uint8_t push_edx{ 0x52 };
    uint16_t push_u8_max{ 0xFF6A };
    uint8_t push_ecx{ 0x51 };
    // initialize runtimes enumerator ptr
    uint16_t call_deref_add_eax{ 0x50FF }; uint8_t enumerate_loaded_runtimes_offset{ 0x18 };

    // get Next from vtable
    uint16_t mov_ecx_deref_add_ebp__a{ 0x4D8B }; int8_t add_c{ kEnumeratorOffset };
    uint16_t mov_eax_deref_ecx__a{ 0x018B }; // vtable = *enumerator

    uint16_t lea_edx_deref_add_ebp__a{ 0x558D }; int8_t add_d{ kRuntimeCountOffset };
    uint8_t push_edx__a{ 0x52 };
    uint16_t lea_edx_deref_add_ebp__b{ 0x558D }; int8_t add_e{ kRuntimeInfoOffset };
    uint8_t push_edx__b{ 0x52 };
    uint16_t push_u8_one{ 0x016A };
    uint8_t push_ecx__a{ 0x51 };
    // call next and initialize runtime info ptr
    uint16_t next_call_deref_add_eax{ 0x50FF }; int8_t next_offset{ 0x0C };

    // get GetInterface from vtable
    uint16_t mov_ecx_deref_add_ebp__b{ 0x4D8B }; int8_t add_f{ kRuntimeInfoOffset };
    uint16_t mov_eax_deref_ecx__b{ 0x018B }; // vtable = *runtime_info

    uint16_t lea_edx_deref_add_ebp__c{ 0x558D }; int8_t add_g{ kRuntimeHostOffset };
    uint8_t push_edx__c{ 0x52 };
    uint8_t push__a{ 0x68 }; void* iid_icorruntimehost{ nullptr };
    uint8_t push__b{ 0x68 }; void* clsid_corruntimehost{ nullptr };
    uint8_t push_ecx__b{ 0x51 };
    // call GetInterface and initialize runtime host ptr
    uint16_t get_interface_call_deref_add_eax{ 0x50FF }; int8_t get_interface_offset{ 0x24 };

    // get default domain
    uint16_t mov_ecx_deref_add_ebp__c{ 0x4D8B }; int8_t add_h{ kRuntimeHostOffset };
    uint16_t mov_eax_deref_ecx__c{ 0x018B }; // vtable = *runtime_host

    uint16_t lea_edx_deref_add_ebp__d{ 0x558D }; int8_t add_i{ kDefaultDomainOffset };
    uint8_t push_edx__d{ 0x52 };
    uint8_t push_ecx__c{ 0x51 };
    uint16_t get_default_domain_call_deref_add_eax{ 0x50FF }; int8_t get_default_domain_offset{ 0x34 };

    // get app domain
    uint16_t mov_ecx_deref_add_ebp__d{ 0x4D8B }; int8_t add_j{ kDefaultDomainOffset };
    uint16_t mov_eax_deref_ecx__d{ 0x018B }; // vtable = *default_domain

    uint16_t lea_edx_deref_add_ebp__e{ 0x558D }; int8_t add_k{ kAppDomainOffset };
    uint8_t push_edx__e{ 0x52 };
    uint8_t push__c{ 0x68 }; void* iid_appdomain{ nullptr };
    uint8_t push_ecx__d{ 0x51 };
    uint16_t call_deref_eax{ 0x10FF }; // calls QueryInterface

    // load assembly from bytes
    uint16_t mov_ecx_deref_add_ebp__e{ 0x4D8B }; int8_t add_l{ kAppDomainOffset };
    uint16_t mov_eax_deref_ecx__e{ 0x018B }; // vtable = *app_domain

    uint16_t lea_edx_deref_add_ebp__f{ 0x558D }; int8_t add_m{ kLoadedAssemblyOffset };
    uint8_t push_edx__f{ 0x52 };
    uint8_t push__d{ 0x68 }; void* managed_array{ nullptr };
    uint8_t push_ecx__e{ 0x51 };
    uint16_t load_call_deref_add_eax{ 0x90FF }; uint32_t load_offset{ 0xB4 };

    uint16_t mov_eax_deref_add_ebp{ 0x458B }; int8_t add_z{ kMetaHostOffset }; // -4
    uint16_t mov_esp_ebp{ 0xE58B };
    uint8_t pop_ebp{ 0x5D };
    uint8_t ret{ 0xC3 };

    InjectShell(
        void* at,
        uintptr_t clr_create_instance,
        void* clsid_clrmetahost,
        void* iid_iclrmetahost,
        void* clsid_corruntimehost,
        void* iid_icorruntimehost,
        void* iid_appdomain,
        void* dll_managed_array) {
        this->clsid_clrmetahost = clsid_clrmetahost;
        this->iid_iclrmetahost = iid_iclrmetahost;

        this->clsid_corruntimehost = clsid_corruntimehost;
        this->iid_icorruntimehost = iid_icorruntimehost;

        this->iid_appdomain = iid_appdomain;

        this->managed_array = dll_managed_array;

        const auto ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, rel_call) + sizeof uintptr_t };
        rel_call = (clr_create_instance - ret_addr);
    }
};

#pragma pack(pop)   /* restore original alignment from stack */