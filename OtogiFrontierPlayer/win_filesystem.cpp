
#include <Windows.h>
#include <shlwapi.h>

#include "win_filesystem.h"

#pragma comment(lib, "Shlwapi.lib")

/*�w��K�w�̃t�@�C���E�t�H���_�ꗗ����*/
bool win_filesystem::CreateFilePathList(const wchar_t* pwzFolderPath, const wchar_t* pwzFileExtension, std::vector<std::wstring>& paths)
{
	if (pwzFolderPath == nullptr)return false;

	std::wstring wstrParent = pwzFolderPath;
	wstrParent += L"\\";
	std::wstring wstrPath = wstrParent + L'*';
	if (pwzFileExtension != nullptr)
	{
		wstrPath += pwzFileExtension;
	}

	WIN32_FIND_DATA sFindData;
	std::vector<std::wstring> wstrNames;

	HANDLE hFind = ::FindFirstFile(wstrPath.c_str(), &sFindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		if (pwzFileExtension != nullptr)
		{
			do
			{
				/*�t�@�C���ꗗ*/
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
				/*�t�H���_�ꗗ*/
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

	/*���O���ɐ���*/
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
