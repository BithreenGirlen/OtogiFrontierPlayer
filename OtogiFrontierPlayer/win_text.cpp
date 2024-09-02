
#include <shlwapi.h>

#include "win_text.h"

namespace win_text
{
    enum class CCodePage
    {
        kAnsi,
        kUtf8,
    };

    unsigned int ToWin32CodePage(CCodePage codePage)
    {
        switch (codePage)
        {
        case CCodePage::kAnsi:
            return CP_OEMCP;
            break;
        case CCodePage::kUtf8:
            return CP_UTF8;
            break;
        default:
            return CP_ACP;
        }
    }

    std::wstring Widen(const std::string& str, CCodePage codePage)
    {
        if (!str.empty())
        {
            unsigned int uiCodePage = ToWin32CodePage(codePage);
            int iLen = ::MultiByteToWideChar(uiCodePage, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
            if (iLen > 0)
            {
                std::wstring wstr(iLen, 0);
                ::MultiByteToWideChar(uiCodePage, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], iLen);
                return wstr;
            }
        }

        return std::wstring();
    }

    std::string Narrowen(const std::wstring& wstr, CCodePage codePage)
    {
        if (!wstr.empty())
        {
            unsigned int uiCodePage = ToWin32CodePage(codePage);
            int iLen = ::WideCharToMultiByte(uiCodePage, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
            if (iLen > 0)
            {
                std::string str(iLen, 0);
                ::WideCharToMultiByte(uiCodePage, 0, wstr.c_str(), static_cast<int>(wstr.length()), &str[0], iLen, nullptr, nullptr);
                return str;
            }
        }
        return std::string();
    }
}

/*std::string to std::wstring*/
std::wstring win_text::WidenUtf8(const std::string& str)
{
    return Widen(str, CCodePage::kUtf8);
}
/*std::wstring to std::string*/
std::string win_text::NarrowUtf8(const std::wstring& wstr)
{
    return Narrowen(wstr, CCodePage::kUtf8);
}

std::wstring win_text::WidenANSI(const std::string& str)
{
    return Widen(str, CCodePage::kAnsi);
}

std::string win_text::NarrowANSI(const std::wstring& wstr)
{
    return Narrowen(wstr, CCodePage::kAnsi);
}
