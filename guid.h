#pragma once

#include <cstdint>

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