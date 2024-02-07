
#include <shobjidl.h>
#include <atlbase.h>

#include "win_dialogue.h"

struct ComInit
{
	HRESULT m_hrComInit;
	ComInit() : m_hrComInit(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
	~ComInit() { if (SUCCEEDED(m_hrComInit)) ::CoUninitialize(); }
};

/*フォルダ選択ダイアログ*/
std::wstring win_dialogue::SelectWorkFolder()
{
	ComInit sInit;
	CComPtr<IFileOpenDialog> pFolderDlg;
	HRESULT hr = pFolderDlg.CoCreateInstance(CLSID_FileOpenDialog);

	if (SUCCEEDED(hr)) {
		FILEOPENDIALOGOPTIONS opt{};
		pFolderDlg->GetOptions(&opt);
		pFolderDlg->SetOptions(opt | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);

		if (SUCCEEDED(pFolderDlg->Show(nullptr)))
		{
			CComPtr<IShellItem> pSelectedItem;
			pFolderDlg->GetResult(&pSelectedItem);

			wchar_t* pPath;
			pSelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);

			if (pPath != nullptr)
			{
				std::wstring wstrPath = pPath;
				::CoTaskMemFree(pPath);
				return wstrPath;
			}

		}
	}

	return std::wstring();
}
