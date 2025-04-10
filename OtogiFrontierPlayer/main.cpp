

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <locale.h>

#include "win_dialogue.h"
#include "win_filesystem.h"
#include "win_text.h"
#include "json_minimal.h"

#include "sfml_spine_player.h"


namespace otogi
{
	struct SPlayerSetting
	{
		std::wstring wstrAtlasExtension = L".atlas";
		std::wstring wstrSkelExtension = L".json";
		std::wstring wstrVoiceExtension = L".m4a";
	};

	static SPlayerSetting g_playerSetting;

	static bool ReadSettingFile(SPlayerSetting& playerSetting)
	{
		std::wstring wstrFilePath = win_filesystem::GetCurrentProcessPath() + L"\\setting.txt";
		std::string strFile = win_filesystem::LoadFileAsString(wstrFilePath.c_str());
		if (strFile.empty())return false;

		char* p = &strFile[0];
		auto pp = std::make_unique<char*>();

		json_minimal::ExtractJsonObject(&p, "extension", &*pp);
		if (*pp == nullptr)return false;

		std::vector<char> vBuffer(1024, '\0');
		bool bRet = json_minimal::GetJsonElementValue(*pp, "atlas", vBuffer.data(), vBuffer.size());
		if (!bRet)return false;
		playerSetting.wstrAtlasExtension = win_text::WidenUtf8(vBuffer.data());

		bRet = json_minimal::GetJsonElementValue(*pp, "skel", vBuffer.data(), vBuffer.size());
		if (!bRet)return false;
		playerSetting.wstrSkelExtension = win_text::WidenUtf8(vBuffer.data());

		bRet = json_minimal::GetJsonElementValue(*pp, "voice", vBuffer.data(), vBuffer.size());
		if (!bRet)return false;
		playerSetting.wstrVoiceExtension = win_text::WidenUtf8(vBuffer.data());

		return true;
	}

	static bool InitialiseSetting()
	{
		SPlayerSetting playerSetting;
		bool bRet = ReadSettingFile(playerSetting);
		if (bRet)
		{
			g_playerSetting = std::move(playerSetting);
		}

		return g_playerSetting.wstrAtlasExtension != g_playerSetting.wstrSkelExtension;
	}

	static bool IsSkelBinary()
	{
		const wchar_t* wszBinaryCandidates[] =
		{
			L".skel", L".bin"
		};
		for (size_t i = 0; i < sizeof(wszBinaryCandidates) / sizeof(wszBinaryCandidates[0]); ++i)
		{
			if (g_playerSetting.wstrSkelExtension.find(wszBinaryCandidates[i]) != std::wstring::npos)
			{
				return true;
			}
		}
		return false;
	}

	static void GetSpineList(const std::wstring& wstrFolderPath, std::vector<std::string>& atlasPaths, std::vector<std::string>& skelPaths)
	{
		bool bAtlasLonger = g_playerSetting.wstrAtlasExtension.size() > g_playerSetting.wstrSkelExtension.size();

		std::wstring& wstrLongerExtesion = bAtlasLonger ? g_playerSetting.wstrAtlasExtension : g_playerSetting.wstrSkelExtension;
		std::wstring& wstrShorterExtension = bAtlasLonger ? g_playerSetting.wstrSkelExtension : g_playerSetting.wstrAtlasExtension;
		std::vector<std::string>& strLongerPaths = bAtlasLonger ? atlasPaths : skelPaths;
		std::vector<std::string>& strShorterPaths = bAtlasLonger ? skelPaths : atlasPaths;

		std::vector<std::wstring> wstrFilePaths;
		win_filesystem::CreateFilePathList(wstrFolderPath.c_str(), L"*", wstrFilePaths);

		for (const auto& filePath : wstrFilePaths)
		{
			const auto EndsWith = [filePath](const std::wstring& str)
				-> bool
				{
					if (filePath.size() < str.size()) return false;
					return std::equal(str.rbegin(), str.rend(), filePath.rbegin());
				};

			if (EndsWith(wstrLongerExtesion))
			{
				strLongerPaths.push_back(win_text::NarrowUtf8(filePath));
			}
			else if (EndsWith(wstrShorterExtension))
			{
				strShorterPaths.push_back(win_text::NarrowUtf8(filePath));
			}
		}
	}

	static const std::wstring& GetVoiceExtension()
	{
		return g_playerSetting.wstrVoiceExtension;
	}

	static bool GetAudioFolderPathFromAtlasFolderPath(const std::wstring& wstrAtlasFolderPath, std::wstring& wstrAudioFolderPath)
	{
		/*
		* Example
		* Atlas path: assetbundleresources/chara/still/221491
		* Audio path: assetbundleresources/sound/voice/still/voice_still_221491
		*/
		size_t nPos = wstrAtlasFolderPath.find_last_of(L"\\/");
		if (nPos == std::wstring::npos)return false;

		std::wstring wstrCharacterId = wstrAtlasFolderPath.substr(nPos + 1);

		nPos = wstrAtlasFolderPath.find(L"chara");
		if (nPos == std::wstring::npos)return false;

		std::wstring wstrVoiceFolder = wstrAtlasFolderPath.substr(0, nPos);
		wstrVoiceFolder += L"sound\\voice\\still";

		std::vector<std::wstring> folders;
		win_filesystem::CreateFilePathList(wstrVoiceFolder.c_str(), nullptr, folders);
		auto IsContained = [&wstrCharacterId](const std::wstring& wstr)
			-> bool
			{
				return wcsstr(wstr.c_str(), wstrCharacterId.c_str()) != nullptr;
			};

		auto iter = std::find_if(folders.begin(), folders.end(), IsContained);
		if (iter == folders.end())return false;
		size_t nIndex = std::distance(folders.begin(), iter);

		wstrAudioFolderPath = folders[nIndex];
		return true;
	}
} /* namespace otogi */


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	setlocale(LC_ALL, ".utf8");

	std::wstring wstrPickedFolder = win_dialogue::SelectWorkFolder();
	if (wstrPickedFolder.empty())return 0;

	if (!otogi::InitialiseSetting())return 0;

	CSfmlSpinePlayer SfmlPlayer;
	SfmlPlayer.SetFont("C:\\Windows\\Fonts\\yumindb.ttf", true, true);

	std::vector<std::wstring> folderPaths;
	size_t nFolderIndex = 0;
	win_filesystem::GetFilePathListAndIndex(wstrPickedFolder, nullptr, folderPaths, &nFolderIndex);
	for (;;)
	{
		std::wstring wstrFolderPath = folderPaths[nFolderIndex];

		std::vector<std::string> atlasPaths;
		std::vector<std::string> skelPaths;
		otogi::GetSpineList(wstrFolderPath, atlasPaths, skelPaths);
		if (skelPaths.empty())break;

		bool bRet = SfmlPlayer.SetSpine(atlasPaths, skelPaths, otogi::IsSkelBinary());
		if (!bRet)break;

		std::wstring wstrAudioFolderPath;
		otogi::GetAudioFolderPathFromAtlasFolderPath(wstrFolderPath, wstrAudioFolderPath);

		std::vector<std::wstring> audioFilePaths;
		win_filesystem::CreateFilePathList(wstrAudioFolderPath.c_str(), otogi::GetVoiceExtension().c_str(), audioFilePaths);
		SfmlPlayer.SetAudioFiles(audioFilePaths);

		int iRet = SfmlPlayer.Display();
		if (iRet == 1)
		{
			++nFolderIndex;
			if (nFolderIndex > folderPaths.size() - 1)nFolderIndex = 0;
		}
		else if (iRet == 2)
		{
			--nFolderIndex;
			if (nFolderIndex > folderPaths.size() - 1)nFolderIndex = folderPaths.size() - 1;
		}
		else
		{
			break;
		}
	}

	return 0;
}
