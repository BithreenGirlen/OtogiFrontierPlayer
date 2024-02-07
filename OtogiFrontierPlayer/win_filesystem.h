#ifndef WIN_FILESYSTEM_H_
#define WIN_FILESYSTEM_H_

#include <string>
#include <vector>

namespace win_filesystem
{
	bool CreateFilePathList(const wchar_t* pwzFolderPath, const wchar_t* pwzFileExtension, std::vector<std::wstring>& paths);
}
#endif // WIN_FILESYSTEM_H_
