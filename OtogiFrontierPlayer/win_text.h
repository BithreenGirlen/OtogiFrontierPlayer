#ifndef WIN_TEXT_H_
#define WIN_TEXT_H_

#include <string>

namespace win_text
{
    std::wstring WidenUtf8(const std::string& str);
    std::string NarrowUtf8(const std::wstring& wstr);

    std::wstring WidenANSI(const std::string& str);
    std::string NarrowANSI(const std::wstring& wstr);
}

#endif //WIN_TEXT_H_
