
#include "sfml_spine_player.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "winmm.lib")

#ifdef  _DEBUG
#pragma comment(lib, "sfml-system-d.lib")
#pragma comment(lib, "sfml-graphics-d.lib")
#pragma comment(lib, "sfml-window-d.lib")
#else
#pragma comment(lib, "sfml-system.lib")
#pragma comment(lib, "sfml-graphics.lib")
#pragma comment(lib, "sfml-window.lib")
#endif // _DEBUG

#include "win_media_player.h"


spSkeletonData* readSkeletonJsonData(const char* filename, spAtlas* atlas, float scale)
{
	spSkeletonJson* json = spSkeletonJson_create(atlas);
	json->scale = scale;
	spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, filename);
	spSkeletonJson_dispose(json);
	return skeletonData;
}

spSkeletonData* readSkeletonBinaryData(const char* filename, spAtlas* atlas, float scale)
{
	spSkeletonBinary* binary = spSkeletonBinary_create(atlas);
	binary->scale = scale;
	spSkeletonData* skeletonData = spSkeletonBinary_readSkeletonDataFile(binary, filename);
	spSkeletonBinary_dispose(binary);
	return skeletonData;
}

CSfmlSpinePlayer::CSfmlSpinePlayer()
{

}

CSfmlSpinePlayer::~CSfmlSpinePlayer()
{
	for (size_t i = 0; i < m_atlases.size(); ++i)
	{
		if (m_atlases.at(i) != nullptr)
		{
			spAtlas_dispose(m_atlases.at(i));
		}
	}
	m_atlases.clear();
	for (size_t i = 0; i < m_skeletonData.size(); ++i)
	{
		if (m_skeletonData.at(i) != nullptr)
		{
			spSkeletonData_dispose(m_skeletonData.at(i));
		}
	}
	m_skeletonData.clear();
}

bool CSfmlSpinePlayer::SetSpine(const std::vector<std::string>& atlasPaths, const std::vector<std::string>& skelPaths, bool bIsBinary)
{
	if (atlasPaths.size() != skelPaths.size())return false;

	for (size_t i = 0; i < atlasPaths.size(); ++i)
	{
		const std::string& strAtlasPath = atlasPaths.at(i);
		const std::string& strSkeletonPath = skelPaths.at(i);

		spAtlas* atlas = spAtlas_createFromFile(strAtlasPath.c_str(), nullptr);
		if (atlas == nullptr)continue;

		spSkeletonData* skeletonData = bIsBinary ? readSkeletonBinaryData(strSkeletonPath.c_str(), atlas, 1.f) : readSkeletonJsonData(strSkeletonPath.c_str(), atlas, 1.f);
		if (skeletonData == nullptr)
		{
			if (atlas != nullptr)
			{
				spAtlas_dispose(atlas);
			}
			continue;
		}

		m_atlases.emplace_back(atlas);
		m_skeletonData.emplace_back(skeletonData);
	}

	if (m_skeletonData.empty())return false;

	m_fMaxWidth = m_skeletonData.at(0)->width;
	m_fMaxHeight = m_skeletonData.at(0)->height;

	WorkOutDefaultScale();

	return SetupDrawer();
}
/*音声ファイル設定*/
void CSfmlSpinePlayer::SetAudios(std::vector<std::wstring>& filePaths)
{
	m_audio_files = filePaths;
}
/*ウィンドウ表示*/
int CSfmlSpinePlayer::Display()
{
	sf::Vector2i iMouseStartPos;

	bool bOnWindowMove = false;
	bool bSpeedHavingChanged = false;

	int iRet = 0;
	m_window = std::make_unique< sf::RenderWindow>(sf::VideoMode(static_cast<unsigned int>(m_fMaxWidth), static_cast<unsigned int>(m_fMaxHeight)), "Otogi spine player", sf::Style::None);
	m_window->setPosition(sf::Vector2i(0, 0));
	m_window->setFramerateLimit(0);
	ResetScale();

	/*The media player is based on Microsoft Media Foundation because SFML does not support .m4a file.*/
	std::unique_ptr<CMediaPlayer> pMediaPlayer = std::make_unique<CMediaPlayer>(m_window->getSystemHandle());
	pMediaPlayer->SetFiles(m_audio_files);
	double dbAudioRate = 1.0;

	sf::Event event;
	sf::Clock deltaClock;
	while (m_window->isOpen())
	{
		while (m_window->pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				m_window->close();
				break;
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					iMouseStartPos.x = event.mouseButton.x;
					iMouseStartPos.y = event.mouseButton.y;
				}
				break;
			case sf::Event::MouseButtonReleased:
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					/*速度変更完了*/
					if (bSpeedHavingChanged)
					{
						bSpeedHavingChanged = false;
						break;
					}

					/*窓枠移動開始・完了*/
					if (bOnWindowMove || sf::Mouse::isButtonPressed(sf::Mouse::Right))
					{
						bOnWindowMove ^= true;
						break;
					}

					int iX = iMouseStartPos.x - event.mouseButton.x;
					int iY = iMouseStartPos.y - event.mouseButton.y;

					if (iX == 0 && iY == 0)
					{
						ShiftScene();
					}
					else
					{
						MoveViewPoint(iX, iY);
					}
				}
				if (event.mouseButton.button == sf::Mouse::Middle)
				{
					ResetScale();
				}
				break;
			case sf::Event::MouseWheelScrolled:
				if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
				{
					/*速度変更*/
					if (event.mouseWheelScroll.delta < 0)
					{
						m_fTimeScale += 0.05f;
					}
					else
					{
						m_fTimeScale -= 0.05f;
					}
					RescaleTime();
					bSpeedHavingChanged = true;
				}
				else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
				{
					/*音声送り・戻し*/
					if (pMediaPlayer.get() != nullptr)
					{
						if (event.mouseWheelScroll.delta < 0)
						{
							pMediaPlayer->Next();
						}
						else
						{
							pMediaPlayer->Back();
						}
					}
				}
				else
				{
					/*拡縮変更*/
					if (event.mouseWheelScroll.delta < 0)
					{
						m_fSkeletonScale += 0.025f;
					}
					else
					{
						m_fSkeletonScale -= 0.025f;
						if (m_fSkeletonScale < 0.25f)m_fSkeletonScale = 0.25f;
					}
					RescaleSkeleton();
				}
				break;
			case sf::Event::KeyReleased:
				switch (event.key.code)
				{
				case sf::Keyboard::Key::S:
					for (size_t i = 0; i < m_drawables.size(); ++i)
					{
						m_drawables.at(i).get()->SwitchSelectivePma();
					}
					break;
				case sf::Keyboard::Key::Escape:
					m_window->close();
					break;
				case sf::Keyboard::Key::PageUp:
					if (dbAudioRate < 2.41)
					{
						dbAudioRate += 0.1;
					}
					if (pMediaPlayer.get() != nullptr)pMediaPlayer->SetCurrentRate(dbAudioRate);
					break;
				case sf::Keyboard::Key::PageDown:
					if (dbAudioRate > 0.59)
					{
						dbAudioRate -= 0.1;
					}
					if (pMediaPlayer.get() != nullptr)pMediaPlayer->SetCurrentRate(dbAudioRate);
					break;
				case sf::Keyboard::Key::Home:
					dbAudioRate = 1.0;
					if (pMediaPlayer.get() != nullptr)pMediaPlayer->SetCurrentRate(dbAudioRate);
					break;
				case sf::Keyboard::Key::Up:
					iRet = 2;
					m_window->close();
					break;
				case sf::Keyboard::Key::Down:
					iRet = 1;
					m_window->close();
					break;
				default:
					break;
				}
				break;
			}
		}

		float delta = deltaClock.getElapsedTime().asSeconds();
		deltaClock.restart();
		Redraw(delta);

		if (bOnWindowMove)
		{
			int iPosX = sf::Mouse::getPosition().x - m_window->getSize().x / 2;
			int iPosY = sf::Mouse::getPosition().y - m_window->getSize().y / 2;
			m_window->setPosition(sf::Vector2i(iPosX, iPosY));
		}
	}
	return iRet;
}
/*描画器設定*/
bool CSfmlSpinePlayer::SetupDrawer()
{
	/*
	 * The spine outline shader, an extension by Unity, cannot be brought forth by SFML. 
	 * Some scene gets closer to natural with premultiplied alpha disabled, some with enabled.
	 */
	const std::vector<std::string> pmaSelectiveList
	{ "penis", "tama", "vagina", "manko", "penice", "chin", "ccitsu", "chitsu", "manco", "vibrator", "cover"};

	for (size_t i = 0; i < m_skeletonData.size(); ++i)
	{
		m_drawables.emplace_back(std::make_shared<CSfmlSpineDrawableC>(m_skeletonData.at(i)));

		CSfmlSpineDrawableC* drawable = m_drawables.at(i).get();
		drawable->timeScale = 1.0f;
		drawable->skeleton->x = m_fMaxWidth / 2;
		drawable->skeleton->y = m_fMaxHeight / 2;
		spSkeleton_setToSetupPose(drawable->skeleton);
		spSkeleton_updateWorldTransform(drawable->skeleton);

		drawable->SetSelectivePmaList(pmaSelectiveList);

		for (size_t ii = 0; ii < m_skeletonData.at(i)->animationsCount; ++ii)
		{
			spAnimation* animation = m_skeletonData.at(i)->animations[ii];
			std::string strAnimationName = animation->name;
			auto iter = std::find(m_animationNames.begin(), m_animationNames.end(), strAnimationName);
			if (iter == m_animationNames.cend())m_animationNames.push_back(strAnimationName);
		}
	}

	if (!m_animationNames.empty())
	{
		for (size_t i = 0; i < m_skeletonData.size(); ++i)
		{
			spAnimationState_setAnimationByName(m_drawables.at(i).get()->state, 0, m_animationNames.at(0).c_str(), true);
		}
	}

	return m_animationNames.size() > 0;
}
/*標準尺度算出*/
void CSfmlSpinePlayer::WorkOutDefaultScale()
{
	if (m_skeletonData.empty())return;

	unsigned int uiSkeletonWidth = static_cast<unsigned int>(m_skeletonData.at(0)->width);
	unsigned int uiSkeletonHeight = static_cast<unsigned int>(m_skeletonData.at(0)->height);

	unsigned int uiDesktopWidth = sf::VideoMode::getDesktopMode().width;
	unsigned int uiDesktopHeight = sf::VideoMode::getDesktopMode().height;

	if (uiSkeletonWidth > uiDesktopWidth || uiSkeletonHeight > uiDesktopHeight)
	{
		if (uiDesktopWidth > uiDesktopHeight)
		{
			m_fDefaultWindowScale = static_cast<float>(uiDesktopHeight) / uiSkeletonHeight;
			m_fThresholdScale = static_cast<float>(uiDesktopWidth) / uiSkeletonWidth;
		}
		else
		{
			m_fDefaultWindowScale = static_cast<float>(uiDesktopWidth) / uiSkeletonWidth;
			m_fThresholdScale = static_cast<float>(uiDesktopHeight) / uiSkeletonHeight;
		}
		m_fSkeletonScale = m_fDefaultWindowScale;
	}
}
/*尺度設定*/
void CSfmlSpinePlayer::RescaleSkeleton()
{
	/* 
	 * spSkeleton struct for spine-3.6 had not scaleX and scaleY yet; they were added in 3.7-c. 
	 * So here the window is boundlessly scaled up as if were the skeleton zoomed.
	 */
	ResizeWindow();
}
/*速度設定*/
void CSfmlSpinePlayer::RescaleTime()
{
	if (m_fTimeScale < 0.f)m_fTimeScale = 0.f;
	for (size_t i = 0; i < m_drawables.size(); ++i)
	{
		m_drawables.at(i).get()->timeScale = m_fTimeScale;
	}
}
/*速度・尺度・視点初期化*/
void CSfmlSpinePlayer::ResetScale()
{
	m_fTimeScale = 1.0f;
	m_fSkeletonScale = m_fDefaultWindowScale;
	m_iOffset = sf::Vector2i{};

	RescaleSkeleton();
	RescaleTime();
	MoveViewPoint(0, 0);
	ResizeWindow();
}
/*窓寸法調整*/
void CSfmlSpinePlayer::ResizeWindow()
{
	if (m_window.get() != nullptr)
	{
		m_window->setSize(sf::Vector2u(static_cast<unsigned int>(m_fMaxWidth * m_fSkeletonScale), static_cast<unsigned int>(m_fMaxHeight * m_fSkeletonScale)));
	}
}
/*視点移動*/
void CSfmlSpinePlayer::MoveViewPoint(int iX, int iY)
{
	m_iOffset.x += iX;
	m_iOffset.y += iY;
	for (size_t i = 0; i < m_drawables.size(); ++i)
	{
		m_drawables.at(i)->skeleton->x = (m_fMaxWidth - m_iOffset.x) / 2;
		m_drawables.at(i)->skeleton->y = (m_fMaxHeight - m_iOffset.y) / 2;
	}
}
/*場面移行*/
void CSfmlSpinePlayer::ShiftScene()
{
	++m_nAnimationIndex;
	if (m_nAnimationIndex > m_animationNames.size() - 1)m_nAnimationIndex = 0;
	for (size_t i = 0; i < m_drawables.size(); ++i)
	{
		spAnimationState_setAnimationByName(m_drawables.at(i).get()->state, 0, m_animationNames.at(m_nAnimationIndex).c_str(), true);
	}
}
/*再描画*/
void CSfmlSpinePlayer::Redraw(float fDelta)
{
	if (m_window.get() != nullptr)
	{
		m_window->clear();
		for (size_t i = 0; i < m_drawables.size(); ++i)
		{
			m_drawables.at(i).get()->Update(fDelta);
			m_window->draw(*m_drawables.at(i).get(), sf::RenderStates(sf::BlendAlpha));
		}
		m_window->display();
	}
}
