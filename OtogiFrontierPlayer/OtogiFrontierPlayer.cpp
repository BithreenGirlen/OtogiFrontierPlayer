// OtogiFrontierPlayer.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "OtogiFrontierPlayer.h"

#include "win_dialogue.h"
#include "win_filesystem.h"
#include "win_text.h"

#include "sfml_spine_player.h"

void GetSpineList(const std::wstring& wstrFolderPath, std::vector<std::string>& atlasPaths, std::vector<std::string>& skelPaths)
{
    std::vector<std::wstring> filePaths;
    win_filesystem::CreateFilePathList(wstrFolderPath.c_str(), L".txt", filePaths);
    for (const std::wstring& filePath : filePaths)
    {
        /*---------------------
         * 種類  | 名称形式
         * atlas | xx.atlas.txt
         * skel  | xx.skel.txt
         * json  | xx.txt
         *---------------------*/
        if (filePath.rfind(L"#") != std::wstring::npos)continue;

        if (filePath.rfind(L".atlas") != std::wstring::npos)
        {
            atlasPaths.push_back(win_text::NarrowUtf8(filePath));
        }
        else
        {
            skelPaths.push_back(win_text::NarrowUtf8(filePath));
        }
    }
}

void GetFolderList(const std::wstring& wstrFolderPath, std::vector<std::wstring>& folders, size_t* nIndex)
{
    std::wstring wstrParent;
    std::wstring wstrCurrent;

    size_t nPos = wstrFolderPath.find_last_of(L"\\/");
    if (nPos != std::wstring::npos)
    {
        wstrParent = wstrFolderPath.substr(0, nPos);
        wstrCurrent = wstrFolderPath.substr(nPos + 1);
    }

    win_filesystem::CreateFilePathList(wstrParent.c_str(), nullptr, folders);

    auto iter = std::find(folders.begin(), folders.end(), wstrFolderPath);
    if (iter != folders.end())
    {
        *nIndex = std::distance(folders.begin(), iter);
    }
}

bool GetAudioFolderPathFromAtlasFolderPath(const std::wstring& wstrAtlasFolderPath, std::wstring& wstrAudioFolderPath)
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

    wstrAudioFolderPath = folders.at(nIndex);
    return true;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    std::wstring wstrPickedFolder = win_dialogue::SelectWorkFolder();
    if (!wstrPickedFolder.empty())
    {
        std::vector<std::wstring> folders;
        size_t nFolderIndex = 0;
        GetFolderList(wstrPickedFolder, folders, &nFolderIndex);
        for (;;)
        {
            std::wstring wstrFolderPath = folders.at(nFolderIndex);

            std::vector<std::string> atlasPaths;
            std::vector<std::string> skelPaths;
            GetSpineList(wstrFolderPath, atlasPaths, skelPaths);
            if (skelPaths.empty())break;

            std::wstring wstrAudioFolderPath;
            GetAudioFolderPathFromAtlasFolderPath(wstrFolderPath, wstrAudioFolderPath);

            std::vector<std::wstring> audioFilePaths;
            win_filesystem::CreateFilePathList(wstrAudioFolderPath.c_str(), L".m4a", audioFilePaths);

            CSfmlSpinePlayer SfmlPlayer;
            bool bRet = SfmlPlayer.SetSpine(atlasPaths, skelPaths, skelPaths.at(0).rfind(".skel") != std::string::npos);
            if (bRet)
            {
                SfmlPlayer.SetAudios(audioFilePaths);

                int iRet = SfmlPlayer.Display();
                if (iRet == 1)
                {
                    ++nFolderIndex;
                    if (nFolderIndex > folders.size() - 1)nFolderIndex = 0;
                }
                else if (iRet == 2)
                {
                    --nFolderIndex;
                    if (nFolderIndex > folders.size() - 1)nFolderIndex = folders.size() - 1;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return 0;
}
