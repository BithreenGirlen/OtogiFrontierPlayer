
#include <shobjidl.h>
#include <atlbase.h>
#include <mfapi.h>

#include "win_media_player.h"

#pragma comment (lib,"Mfplat.lib")

/*�Đ��@�N���X�|�C���^�i�[*/
void CMediaPlayerNotify::SetPlayer(void* arg)
{
	m_pPlayer = arg;
}
/*�Đ��C�x���g����*/
void CMediaPlayerNotify::OnMediaEngineEvent(DWORD Event, DWORD_PTR param1, DWORD param2)
{
	CMediaPlayer* pPlayer = static_cast<CMediaPlayer*>(m_pPlayer);
	if (pPlayer != nullptr)
	{
		switch (Event)
		{
		case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
			pPlayer->ResizeWindow();
			break;
		case MF_MEDIA_ENGINE_EVENT_ENDED:
			pPlayer->AutoNext();
			break;
		}
	}
}

CMediaPlayer::CMediaPlayer(HWND hWnd)
{
	m_hrComInit = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(m_hrComInit))return;

	m_hrMfStart = ::MFStartup(MF_VERSION);
	if (FAILED(m_hrMfStart))return;

	CComPtr<IMFMediaEngineClassFactory> pmfFactory;

	HRESULT hr = ::CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pmfFactory));
	if (FAILED(hr))return;

	CComPtr<IMFAttributes> pmfAttributes;

	hr = ::MFCreateAttributes(&pmfAttributes, 1);
	if (FAILED(hr))return;

	if (hWnd != nullptr)
	{
		hr = pmfAttributes->SetUINT64(MF_MEDIA_ENGINE_PLAYBACK_HWND, reinterpret_cast<UINT64>(hWnd));
		if (FAILED(hr))return;
		m_hRetWnd = hWnd;
	}

	m_pmfNotify = new CMediaPlayerNotify();
	hr = pmfAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, reinterpret_cast<IUnknown*>(m_pmfNotify));
	if (FAILED(hr))
	{
		m_pmfNotify->Release();
		m_pmfNotify = nullptr;
		return;
	}

	m_pmfNotify->SetPlayer(this);

	CComPtr<IMFMediaEngine> pMfMediaEngine;
	hr = pmfFactory->CreateInstance(MF_MEDIA_ENGINE_REAL_TIME_MODE, pmfAttributes, &pMfMediaEngine);
	if (FAILED(hr))
	{
		m_pmfNotify->Release();
		m_pmfNotify = nullptr;
		return;
	}

	hr = pMfMediaEngine->QueryInterface(__uuidof(IMFMediaEngineEx), (void**)&m_pmfEngineEx);
	if (FAILED(hr))
	{
		m_pmfNotify->Release();
		m_pmfNotify = nullptr;
		return;
	}

	m_pmfEngineEx->SetVolume(0.5);
	m_pmfEngineEx->SetPreload(MF_MEDIA_ENGINE_PRELOAD_METADATA);

}

CMediaPlayer::~CMediaPlayer()
{
	/*Shutdown()�ɈϔC����B*/
	//if (m_pmfNotify != nullptr)
	//{
	//	m_pmfNotify->Release();
	//	m_pmfNotify = nullptr;
	//}

	if (m_pmfEngineEx != nullptr)
	{
		m_pmfEngineEx->Shutdown();
		//m_pmfEngineEx->Release();
		m_pmfEngineEx = nullptr;
	}

	if (SUCCEEDED(m_hrMfStart))
	{
		::MFShutdown();
	}

	if (SUCCEEDED(m_hrComInit))
	{
		::CoUninitialize();
	}
}
/*�t�@�C���ݒ�*/
bool CMediaPlayer::SetFiles(const std::vector<std::wstring>& filePaths)
{
	Clear();
	m_media_files = filePaths;
	ResetZoom();

	return Play();
}
/*�Đ�*/
bool CMediaPlayer::Play()
{
	if (m_pmfEngineEx != nullptr && !m_media_files.empty() && m_nIndex <= m_media_files.size())
	{
		HRESULT hr = m_pmfEngineEx->SetSource(const_cast<BSTR>(m_media_files.at(m_nIndex).c_str()));
		if (SUCCEEDED(hr))
		{
			return SUCCEEDED(m_pmfEngineEx->Play());
		}
	}
	return false;
}
/*����*/
void CMediaPlayer::Next()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_nIndex;
	if (m_nIndex >= m_media_files.size())m_nIndex = 0;
	Play();
}
/*�߂�*/
void CMediaPlayer::Back()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	--m_nIndex;
	if (m_nIndex >= m_media_files.size())m_nIndex = m_media_files.size() - 1;
	Play();
}
/*�Đ����[�v�ݒ�*/
BOOL CMediaPlayer::SwitchLoop()
{
	if (m_pmfEngineEx != nullptr)
	{
		m_iLoop ^= TRUE;
		if (FAILED(m_pmfEngineEx->SetLoop(m_iLoop)))
		{
			m_iLoop ^= TRUE;
		}
	}
	return m_iLoop;
}
/*�����ݒ�*/
BOOL CMediaPlayer::SwitchMute()
{
	if (m_pmfEngineEx != nullptr)
	{
		m_iMute ^= TRUE;
		if (FAILED(m_pmfEngineEx->SetMuted(m_iMute)))
		{
			m_iMute ^= TRUE;
		}
	}
	return m_iMute;
}
/*���ʎ擾*/
double CMediaPlayer::GetCurrentVolume()
{
	if (m_pmfEngineEx != nullptr)
	{
		return m_pmfEngineEx->GetVolume();
	}
	return 100.0;
}
/*�Đ����x�擾*/
double CMediaPlayer::GetCurrentRate()
{
	if (m_pmfEngineEx != nullptr)
	{
		return m_pmfEngineEx->GetPlaybackRate();
	}
	return 1.0;
}
/*���ʐݒ�*/
bool CMediaPlayer::SetCurrentVolume(double dbVolume)
{
	if (m_pmfEngineEx != nullptr)
	{
		return SUCCEEDED(m_pmfEngineEx->SetVolume(dbVolume));
	}
	return false;
}
/*�Đ����x�ݒ�*/
bool CMediaPlayer::SetCurrentRate(double dbRate)
{
	if (m_pmfEngineEx != nullptr)
	{
		if (dbRate != m_pmfEngineEx->GetDefaultPlaybackRate())
		{
			m_pmfEngineEx->SetPlaybackRate(dbRate);
		}
		return SUCCEEDED(m_pmfEngineEx->SetDefaultPlaybackRate(dbRate));
	}
	return false;
}
/*��������*/
void CMediaPlayer::AutoNext()
{
	if (m_nIndex < m_media_files.size()-1)Next();
}
/*���@�v�Z�@�؂�ւ�*/
void CMediaPlayer::SwitchSizeLore(bool bBarHidden)
{
	m_bBarHidden = bBarHidden;
	ResizeWindow();
}
/*���g���@����*/
void CMediaPlayer::ResizeWindow()
{
	if (m_pmfEngineEx != nullptr && m_hRetWnd != nullptr)
	{
		BOOL iRet = m_pmfEngineEx->HasVideo();
		if (iRet)
		{
			DWORD dwWidth = 0;
			DWORD dwHeight = 0;
			HRESULT hr = m_pmfEngineEx->GetNativeVideoSize(&dwWidth, &dwHeight);

			RECT rect;
			if (!m_bBarHidden)
			{
				::GetWindowRect(m_hRetWnd, &rect);
			}
			else
			{
				::GetClientRect(m_hRetWnd, &rect);
			}

			int iX = static_cast<int>(::round(dwWidth * (m_dbScale * 1000) / 1000));
			int iY = static_cast<int>(::round(dwHeight * (m_dbScale * 1000) / 1000));
			rect.right = iX + rect.left;
			rect.bottom = iY + rect.top;

			if (!m_bBarHidden)
			{
				::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
				::SetWindowPos(m_hRetWnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
			}
			else
			{
				RECT rc;
				::GetWindowRect(m_hRetWnd, &rc);
				::MoveWindow(m_hRetWnd, rc.left, rc.top, rect.right, rect.bottom, TRUE);
			}
			ResizeBuffer();
		}

	}
}
/*�\������ݒ�*/
void CMediaPlayer::SetDisplayArea(RECT *rect)
{
	if (m_srcRect.left == 0 && m_srcRect.right == 0 && m_srcRect.top == 0 && m_srcRect.bottom == 0)
	{
		m_srcRect = *rect;
		ResizeBuffer();
	}
}
/*�S��\��*/
void CMediaPlayer::ResetZoom()
{
	m_srcRect = RECT{};
	ResizeBuffer();
}
/*�g��*/
void CMediaPlayer::UpScale()
{
	if (m_dbScale < 0.99)
	{
		m_dbScale += 0.05;
		ResizeWindow();
	}
}
/*�k��*/
void CMediaPlayer::DownScale()
{
	if (m_dbScale > 0.51)
	{
		m_dbScale -= 0.05;
		ResizeWindow();
	}
}
/*����*/
void CMediaPlayer::Clear()
{
	m_media_files.clear();
	m_nIndex = 0;
}
/*���Ő��@�ύX*/
void CMediaPlayer::ResizeBuffer()
{
	if (m_pmfEngineEx != nullptr)
	{
		BOOL iRet = m_pmfEngineEx->HasVideo();
		if (iRet)
		{
			RECT rc;
			::GetClientRect(m_hRetWnd, &rc);
			int iClientWidth = rc.right - rc.left;
			int iClientHeight = rc.bottom - rc.top;

			float x1 = static_cast<float>(m_srcRect.left) / iClientWidth;
			float y1 = static_cast<float>(m_srcRect.top) / iClientHeight;
			float x2 = static_cast<float>(m_srcRect.right) / iClientWidth;
			float y2 = static_cast<float>(m_srcRect.bottom) / iClientHeight;

			MFVideoNormalizedRect srcRect{ x1, y1, x2, y2 };
			MFARGB bg{ 0, 0, 0, 0 };
			m_pmfEngineEx->UpdateVideoStream(&srcRect, &rc, &bg);
		}

	}
}