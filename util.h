#pragma once

#include <string>
#include <string_view>

bool wsuffixi(wchar_t* a, const std::wstring_view& b);

std::string GetErrorMessage(unsigned long error_id);