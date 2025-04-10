
#include <Windows.h>
#include <shlwapi.h>

#include "win_filesystem.h"

#pragma comment(lib, "Shlwapi.lib")

namespace win_filesystem
{
	/*ファイルのメモリ展開*/
	char* LoadExistingFile(const wchar_t* pwzFilePath, unsigned long* ulSize)
	{
		HANDLE hFile = ::CreateFile(pwzFilePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwSize = ::GetFileSize(hFile, nullptr);
			if (dwSize != INVALID_FILE_SIZE)
			{
				char* pBuffer = static_cast<char*>(malloc(static_cast<size_t>(dwSize + 1ULL)));
				if (pBuffer != nullptr)
				{
					DWORD dwRead = 0;
					BOOL iRet = ::ReadFile(hFile, pBuffer, dwSize, &dwRead, nullptr);
					if (iRet)
					{
						::CloseHandle(hFile);
						*(pBuffer + dwRead) = '\0';
						*ulSize = dwRead;

						return pBuffer;
					}
					else
					{
						free(pBuffer);
					}
				}
			}
			::CloseHandle(hFile);
		}

		return nullptr;
	}
	/*指定階層のファイル・フォルダ名一覧取得*/
	bool CreateFilaNameList(const wchar_t* pwzFolderPath, const wchar_t* pwzFileNamePattern, std::vector<std::wstring>& wstrNames)
	{
		if (pwzFolderPath == nullptr)return false;

		std::wstring wstrPath = pwzFolderPath;
		if (pwzFileNamePattern != nullptr)
		{
			if (wcschr(pwzFileNamePattern, L'*') == nullptr)
			{
				wstrPath += L'*';
			}
			wstrPath += pwzFileNamePattern;
		}
		else
		{
			wstrPath += L'*';
		}

		WIN32_FIND_DATA sFindData;

		HANDLE hFind = ::FindFirstFile(wstrPath.c_str(), &sFindData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			if (pwzFileNamePattern != nullptr)
			{
				do
				{
					/*ファイル一覧*/
					if (!(sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						wstrNames.push_back(sFindData.cFileName);
					}
				} while (::FindNextFile(hFind, &sFindData));
			}
			else
			{
				do
				{
					/*フォルダ一覧*/
					if ((sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (wcscmp(sFindData.cFileName, L".") != 0 && wcscmp(sFindData.cFileName, L"..") != 0)
						{
							wstrNames.push_back(sFindData.cFileName);
						}
					}
				} while (::FindNextFile(hFind, &sFindData));
			}

			::FindClose(hFind);
		}
		return wstrNames.size() > 0;
	}
}

/*指定階層のファイル・フォルダ一覧作成*/
bool win_filesystem::CreateFilePathList(const wchar_t* pwzFolderPath, const wchar_t* pwzFileSpec, std::vector<std::wstring>& paths)
{
	if (pwzFolderPath == nullptr || wcslen(pwzFolderPath) == 0)return false;

	std::wstring wstrParent = pwzFolderPath;
	if (wstrParent.back() != L'\\')
	{
		wstrParent += L"\\";
	}
	std::vector<std::wstring> wstrNames;

	if (pwzFileSpec != nullptr)
	{
		const auto SplitSpecs = [](const wchar_t* pwzFileSpec, std::vector<std::wstring>& specs)
			-> void
			{
				std::wstring wstrTemp;
				for (const wchar_t* p = pwzFileSpec; *p != L'\0' && p != nullptr; ++p)
				{
					if (*p == L';')
					{
						if (!wstrTemp.empty())
						{
							specs.push_back(wstrTemp);
							wstrTemp.clear();
						}
						continue;
					}

					wstrTemp.push_back(*p);
				}

				if (!wstrTemp.empty())
				{
					specs.push_back(wstrTemp);
				}
			};
		std::vector<std::wstring> specs;
		SplitSpecs(pwzFileSpec, specs);

		for (const auto& spec : specs)
		{
			CreateFilaNameList(wstrParent.c_str(), spec.c_str(), wstrNames);
		}
	}
	else
	{
		CreateFilaNameList(wstrParent.c_str(), pwzFileSpec, wstrNames);
	}

	/*名前順に整頓*/
	for (size_t i = 0; i < wstrNames.size(); ++i)
	{
		size_t nIndex = i;
		for (size_t j = i; j < wstrNames.size(); ++j)
		{
			if (::StrCmpLogicalW(wstrNames.at(nIndex).c_str(), wstrNames.at(j).c_str()) > 0)
			{
				nIndex = j;
			}
		}
		std::swap(wstrNames.at(i), wstrNames.at(nIndex));
	}

	for (const std::wstring& wstr : wstrNames)
	{
		paths.push_back(wstrParent + wstr);
	}

	return paths.size() > 0;
}
/*指定経路と同階層のファイル・フォルダ一覧作成・相対位置取得*/
bool win_filesystem::GetFilePathListAndIndex(const std::wstring& wstrPath, const wchar_t* pwzFileSpec, std::vector<std::wstring>& paths, size_t* nIndex)
{
	std::wstring wstrParent;
	std::wstring wstrCurrent;

	size_t nPos = wstrPath.find_last_of(L"\\/");
	if (nPos != std::wstring::npos)
	{
		wstrParent = wstrPath.substr(0, nPos);
		wstrCurrent = wstrPath.substr(nPos + 1);
	}

	win_filesystem::CreateFilePathList(wstrParent.c_str(), pwzFileSpec, paths);

	auto iter = std::find(paths.begin(), paths.end(), wstrPath);
	if (iter != paths.end())
	{
		*nIndex = std::distance(paths.begin(), iter);
	}

	return iter != paths.end();
}
/*文字列としてファイル読み込み*/
std::string win_filesystem::LoadFileAsString(const wchar_t* pwzFilePath)
{
	DWORD ulSize = 0;
	char* pBuffer = LoadExistingFile(pwzFilePath, &ulSize);
	if (pBuffer != nullptr)
	{
		std::string str;
		str.resize(ulSize);
		memcpy(&str[0], pBuffer, ulSize);

		free(pBuffer);
		return str;
	}

	return std::string();
}

std::wstring win_filesystem::GetCurrentProcessPath()
{
	wchar_t pwzPath[MAX_PATH]{};
	::GetModuleFileName(nullptr, pwzPath, MAX_PATH);
	std::wstring::size_type nPos = std::wstring(pwzPath).find_last_of(L"\\/");
	return std::wstring(pwzPath).substr(0, nPos);
}

std::wstring win_filesystem::CreateWorkFolder(const std::wstring& wstrRelativePath)
{
	if (wstrRelativePath.empty())return std::wstring();

	std::wstring wstrPath = GetCurrentProcessPath();
	wstrPath.push_back(L'\\');
	size_t nRead = 0;
	if (wstrRelativePath.front() == L'\\')++nRead;

	for (; nRead < wstrRelativePath.size();)
	{
		const wchar_t* pPos = wcspbrk(&wstrRelativePath[nRead], L"\\/");
		if (pPos == nullptr)
		{
			wstrPath += wstrRelativePath.substr(nRead) + L"\\";
			::CreateDirectoryW(wstrPath.c_str(), nullptr);
			break;
		}
		size_t nPos = pPos - &wstrRelativePath[nRead];
		wstrPath += wstrRelativePath.substr(nRead, nPos) + L"\\";
		::CreateDirectoryW(wstrPath.c_str(), nullptr);

		nRead += nPos + 1;
	}
	return wstrPath;
}

bool win_filesystem::SaveStringToFile(const wchar_t* pwzFilePath, const char* szData, unsigned long ulDataLength, bool bOverWrite)
{
	if (pwzFilePath != nullptr)
	{
		HANDLE hFile = ::CreateFile(pwzFilePath, GENERIC_WRITE, 0, nullptr, bOverWrite ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			::SetFilePointer(hFile, NULL, nullptr, FILE_END);
			DWORD dwWritten = 0;
			BOOL iRet = ::WriteFile(hFile, szData, ulDataLength, &dwWritten, nullptr);
			::CloseHandle(hFile);
			return iRet == TRUE;
		}
	}
	return false;
}

bool win_filesystem::DoesFileExist(const wchar_t* pwzFilePath)
{
	return ::PathFileExistsW(pwzFilePath) == TRUE;
}
