#ifndef WIN_MEDIA_PLAYER_H_
#define WIN_MEDIA_PLAYER_H_

#include <Windows.h>
#include <mfmediaengine.h>

#include <string>
#include <vector>
#include <mutex>

class CMediaPlayerNotify : public IMFMediaEngineNotify
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		if (riid == __uuidof(IMFMediaEngineNotify))
		{
			*ppv = static_cast<IMFMediaEngineNotify*>(this);
		}
		else
		{
			*ppv = nullptr;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}
	STDMETHODIMP EventNotify(DWORD Event, DWORD_PTR param1, DWORD param2)
	{
		if (Event == MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE)
		{
			::SetEvent(reinterpret_cast<HANDLE>(param1));
		}
		else
		{
			OnMediaEngineEvent(Event, param1, param2);
		}
		return S_OK;
	}
	STDMETHODIMP_(ULONG) AddRef() { return ::InterlockedIncrement(&m_lRef); }
	STDMETHODIMP_(ULONG) Release()
	{
		LONG lRef = ::InterlockedDecrement(&m_lRef);
		if (!lRef)delete this;
		return lRef;
	}

	void SetPlayer(void* arg);
private:
	LONG m_lRef = 0;
	void* m_pPlayer = nullptr;

	void OnMediaEngineEvent(DWORD Event, DWORD_PTR param1, DWORD param2);
};

class CMediaPlayer
{
public:
	CMediaPlayer(HWND hWnd);
	~CMediaPlayer();
	bool SetFiles(const std::vector<std::wstring>& filePaths);
	bool Play();
	void Next();
	void Back();
	BOOL SwitchLoop();
	BOOL SwitchMute();
	double GetCurrentVolume();
	double GetCurrentRate();
	bool SetCurrentVolume(double dbVolume);
	bool SetCurrentRate(double dbRate); 
	void AutoNext();
	void SwitchSizeLore(bool bBarHidden);
	void ResizeWindow();
	void SetDisplayArea(RECT *rect);
	void ResetZoom();
	void UpScale();
	void DownScale();
private:
	HWND m_hRetWnd = nullptr;
	
	HRESULT m_hrComInit = E_FAIL;
	HRESULT m_hrMfStart = E_FAIL;
	CMediaPlayerNotify* m_pmfNotify = nullptr;
	IMFMediaEngineEx* m_pmfEngineEx = nullptr;
	std::mutex m_mutex;

	std::vector<std::wstring> m_media_files;
	size_t m_nIndex = 0;

	BOOL m_iLoop = FALSE;
	BOOL m_iMute = FALSE;

	RECT m_srcRect{};
	double m_dbScale = 1.0;
	bool m_bBarHidden = false;

	void Clear();
	void ResizeBuffer();
};

#endif //WIN_MEDIA_PLAYER_H_
