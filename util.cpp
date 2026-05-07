#include "util.h"
#include <cwchar>

#include <Windows.h>

bool wsuffixi(wchar_t* a, const std::wstring_view& b)
{
    const auto a_len{ wcslen(a) };
    const auto a_offset{ a_len - b.size() };
    return _wcsicmp(&a[a_offset], b.data()) == 0;
}

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