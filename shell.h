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
constexpr int8_t kEntryTypeOffset{ kLoadedAssemblyOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kEntryMethodOffset{ kEntryTypeOffset - static_cast<int8_t>(sizeof(void*)) };
constexpr int8_t kParamOffset{ kEntryMethodOffset - static_cast<int8_t>(sizeof VARIANT) };
constexpr int8_t kRetOffset{ kParamOffset - static_cast<int8_t>(sizeof VARIANT) };
constexpr uint8_t kReservedStack{ -kRetOffset };
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

    // release_offset{ 0x08 };

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

    // create entry point type name string
    uint8_t push__e{ 0x68 }; void* entry_type_name{ nullptr };
    uint8_t call2{ 0xE8 }; uintptr_t alloc_rel_call{ NULL };

    // already push entry type ptr and allocated string ptr onto stack
    uint16_t lea_edx_deref_add_ebp__g{ 0x558D }; int8_t add_n{ kEntryTypeOffset };
    uint8_t push_edx__g{ 0x52 };
    uint8_t push_eax2{ 0x50 };

    // get entry point type
    uint16_t mov_ecx_deref_add_ebp__f{ 0x4D8B }; int8_t add_o{ kLoadedAssemblyOffset };
    uint16_t mov_eax_deref_ecx__f{ 0x018B }; // vtable = *loaded_assembly
    uint8_t push_ecx__f{ 0x51 };
    uint16_t get_type_call_deref_add_eax{ 0x50FF }; int8_t get_type_offset{ 0x44 };

    // create entry point method name string
    uint8_t push__f{ 0x68 }; void* entry_method_name{ nullptr };
    uint8_t call3{ 0xE8 }; uintptr_t alloc2_rel_call{ NULL };

    // already push entry method ptr and allocated string ptr onto stack
    uint16_t lea_edx_deref_add_ebp__h{ 0x558D }; int8_t add_p{ kEntryMethodOffset };
    uint8_t push_edx__h{ 0x52 };
    uint16_t push_u8_flags{ 0x386A }; // Public | NonPublic | Static
    uint8_t push_eax3{ 0x50 };

    // get entry point method
    uint16_t mov_ecx_deref_add_ebp__g{ 0x4D8B }; int8_t add_q{ kEntryTypeOffset };
    uint16_t mov_eax_deref_ecx__g{ 0x018B }; // vtable = *entry_type
    uint8_t push_ecx__g{ 0x51 };
    uint16_t get_method_call_deref_add_eax{ 0x90FF }; uint32_t get_method_offset{ 0xB4 };

    // Init param and ret variants
    uint16_t lea_edx_deref_add_ebp__i{ 0x558D }; int8_t add_r{ kParamOffset };
    uint8_t push_edx__i{ 0x52 };
    uint8_t call4{ 0xE8 }; uintptr_t init_rel_call{ NULL };
    uint8_t mov_eax{ 0xB8 }; uint32_t vt_null{ 0x1 };
    uint16_t mov_deref_add_ebp{ 0x8966 }; uint8_t ax{ 0x45 }; int8_t add_s{ kParamOffset };

    // ret
    uint16_t lea_edx_deref_add_ebp__j{ 0x558D }; int8_t add_t{ kRetOffset };
    uint8_t push_edx__j{ 0x52 };
    uint8_t call5{ 0xE8 }; uintptr_t init2_rel_call{ NULL };

    uint16_t push_size{ 0x016A };
    uint16_t push_lbound{ 0x006A };
    uint16_t push_vt{ 0x0C6A }; // vt_variant
    uint8_t call6{ 0xE8 }; uintptr_t create_vector_rel_call{ NULL };
    uint8_t push_eax4{ 0x50 }; // save array ptr

    // push 01 // size
    // push 00 // lbound
    // push 0C // vt_variant
    // call SafeArrayCreateVector
    // push eax 
    
    // Construct Variant on stack
    uint16_t sub_esp3{ 0xEC83 }; uint8_t sub{ 0x10 };
    uint16_t mov_deref{ 0x04C7 }; uint8_t esp{ 0x24 }; uint32_t vt{ 0x8 }; // vt_bstr
    uint32_t mov_deref_add8_esp{ 0x082444C7 }; uint32_t val{ 0 };
    uint8_t push_esp{ 0x54 };

    // sub esp, 0x10
    // mov [esp], 8 // vt_bstr
    // mov [esp+8], 0
    // push esp // push as an arg for SafeArrayPutElement
    
    uint32_t lea_edx_deref_addc_esp{ 0x0C24548D };
    uint8_t push_edx__k{ 0x52 };
    uint8_t push_eax5{ 0x50 }; // save array ptr
    uint8_t call7{ 0xE8 }; uintptr_t put_elem_rel_call{ NULL };
    // lea edx, [esp+0C]
    // push edx
    // push eax
    // call SafeArrayPutElement
    
    // Destroy Variant on Stack
    uint16_t add_esp{ 0xC483 }; uint8_t add_1337{ 0x10 };
    // add esp, 0x10 // [esp] is now array ptr
    uint8_t pop_eax{ 0x58 };
    // pop eax

    // Push args
    uint16_t lea_edx_deref_add_ebp__k{ 0x558D }; int8_t add_v{ kRetOffset };
    uint8_t push_edx__l{ 0x52 }; // push return value ptr
    uint8_t push_eax6{ 0x50 }; // push array ptr

    // Get Invoke from vtable
    uint16_t mov_ecx_deref_add_ebp__h{ 0x4D8B }; int8_t add_u{ kEntryMethodOffset };
    uint16_t mov_eax_deref_ecx__h{ 0x018B }; // vtable = *entry_method

    // Move Invoke params to stack (pass by value)
    uint16_t movups_xmm0_deref_add{ 0x100F }; uint8_t ebp{ 0x45 }; int8_t add_w{ kParamOffset };
    uint16_t sub_esp2{ 0xEC83 }; uint8_t sub2{ 0x10 };
    uint32_t movups_deref_esp_xmm0{ 0x2404110F };

    // Push instance ptr and call
    uint8_t push_ecx__h{ 0x51 };
    uint16_t invoke_call_deref_add_eax{ 0x90FF }; uint32_t invoke_offset{ 0x94 };

    uint16_t mov_eax_deref_add_ebp{ 0x458B }; int8_t add_z{ kMetaHostOffset }; // -4
    uint16_t mov_esp_ebp{ 0xE58B };
    uint8_t pop_ebp{ 0x5D };
    uint8_t ret{ 0xC3 };

    InjectShell(
        void* at,
        uintptr_t clr_create_instance,
        uintptr_t sys_alloc_string,
        uintptr_t variant_init,
        uintptr_t create_vector,
        uintptr_t put_elem,

        void* clsid_clrmetahost,
        void* iid_iclrmetahost,
        void* clsid_corruntimehost,
        void* iid_icorruntimehost,
        void* iid_appdomain,
        void* dll_managed_array,
        void* entry_type_name,
        void* entry_method_name) {
        this->clsid_clrmetahost = clsid_clrmetahost;
        this->iid_iclrmetahost = iid_iclrmetahost;

        this->clsid_corruntimehost = clsid_corruntimehost;
        this->iid_icorruntimehost = iid_icorruntimehost;

        this->iid_appdomain = iid_appdomain;

        this->managed_array = dll_managed_array;
        this->entry_type_name = entry_type_name;
        this->entry_method_name = entry_method_name;

        const auto ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, rel_call) + sizeof uintptr_t };
        rel_call = (clr_create_instance - ret_addr);

        const auto alloc_ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, alloc_rel_call) + sizeof uintptr_t };
        alloc_rel_call = (sys_alloc_string - alloc_ret_addr);

        const auto alloc2_ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, alloc2_rel_call) + sizeof uintptr_t };
        alloc2_rel_call = (sys_alloc_string - alloc2_ret_addr);

        const auto init_ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, init_rel_call) + sizeof uintptr_t };
        init_rel_call = (variant_init - init_ret_addr);

        const auto init2_ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, init2_rel_call) + sizeof uintptr_t };
        init2_rel_call = (variant_init - init2_ret_addr);

        const auto cv_ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, create_vector_rel_call) + sizeof uintptr_t };
        create_vector_rel_call = (create_vector - cv_ret_addr);
        
        const auto put_ret_addr{ reinterpret_cast<uintptr_t>(at) + offsetof(InjectShell, put_elem_rel_call) + sizeof uintptr_t };
        put_elem_rel_call = (put_elem - put_ret_addr);
    }
};

#pragma pack(pop)   /* restore original alignment from stack */