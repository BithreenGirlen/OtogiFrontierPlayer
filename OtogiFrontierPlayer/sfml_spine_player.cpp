
#include "sfml_spine_player.h"
#include "spine_loader_c.h"

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


CSfmlSpinePlayer::CSfmlSpinePlayer()
{

}

CSfmlSpinePlayer::~CSfmlSpinePlayer()
{

}

bool CSfmlSpinePlayer::SetSpine(const std::vector<std::string>& atlasPaths, const std::vector<std::string>& skelPaths, bool bIsBinary)
{
	if (atlasPaths.size() != skelPaths.size())return false;
	ClearDrawables();

	for (size_t i = 0; i < atlasPaths.size(); ++i)
	{
		const std::string& strAtlasPath = atlasPaths.at(i);
		const std::string& strSkeletonPath = skelPaths.at(i);

		std::shared_ptr<spAtlas> atlas = spine_loader_c::CreateAtlasFromFile(strAtlasPath.c_str(), nullptr);
		if (atlas.get() == nullptr)continue;

		std::shared_ptr<spSkeletonData> skeletonData = bIsBinary ?
			spine_loader_c::ReadBinarySkeletonFromFile(strSkeletonPath.c_str(), atlas.get()) :
			spine_loader_c::ReadTextSkeletonFromFile(strSkeletonPath.c_str(), atlas.get());
		if (skeletonData.get() == nullptr)continue;

		m_atlases.push_back(atlas);
		m_skeletonData.push_back(skeletonData);
	}

	WorkOutDefualtSize();
	WorkOutDefaultScale();

	return SetupDrawer();
}
/*字体ファイル設定*/
bool CSfmlSpinePlayer::SetFont(const std::string& strFilePath, bool bBold, bool bItalic)
{
	bool bRet = m_trackFont.loadFromFile(strFilePath);
	if (!bRet)
	{
		bRet = m_trackFont.loadFromFile("C:\\Windows\\Fonts\\arialnb.ttf");
		if (!bRet)return false;
	}

	constexpr float fOutLineThickness = 2.4f;

	m_trackText.setFont(m_trackFont);
	m_trackText.setFillColor(sf::Color::Black);
	m_trackText.setStyle((bBold ? sf::Text::Style::Bold : 0) | (bItalic ? sf::Text::Style::Italic : 0));
	m_trackText.setOutlineThickness(fOutLineThickness);
	m_trackText.setOutlineColor(sf::Color::White);

	return true;
}
/*音声ファイル設定*/
void CSfmlSpinePlayer::SetAudioFiles(const std::vector<std::wstring> &wstrAudioFilePaths)
{
	m_audioFilePaths = wstrAudioFilePaths;
	m_nAudioIndex = 0;

	m_trackText.setString("");

	/*The media player is based on Microsoft Media Foundation because SFML does not support .m4a file.*/
	m_pAudioPlayer = std::make_unique<CMfMediaPlayer>();
}

/*ウィンドウ表示*/
int CSfmlSpinePlayer::Display()
{
	int iRet = 0;
	m_window = std::make_unique< sf::RenderWindow>(sf::VideoMode(static_cast<unsigned int>(m_fBaseSize.x), static_cast<unsigned int>(m_fBaseSize.y)), "Otogi spine player", sf::Style::None);
	m_window->setPosition(sf::Vector2i(0, 0));
	m_window->setFramerateLimit(0);

	ResetScale();
	UpdateTrack();

	sf::Vector2i iMouseStartPos;

	bool bOnWindowMove = false;
	bool bSpeedHavingChanged = false;

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
						ShiftAnimation();
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
					StepOnTrack(event.mouseWheelScroll.delta < 0);
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
				case sf::Keyboard::Key::C:
					ToggleTextColor();
					break;
				case sf::Keyboard::Key::D:
					m_bFDefaultOffSetEnabled ^= true;
					ResetScale();
					break;
				case sf::Keyboard::Key::S:
					for (size_t i = 0; i < m_drawables.size(); ++i)
					{
						m_drawables.at(i).get()->SwitchSelectivePma();
					}
					break;
				case sf::Keyboard::Key::T:
					m_bTrackHidden ^= true;
					break;
				case sf::Keyboard::Key::Escape:
					m_window->close();
					break;
				case sf::Keyboard::Key::PageUp:
					ChangePlaybackRate(true);
					break;
				case sf::Keyboard::Key::PageDown:
					ChangePlaybackRate(false);
					break;
				case sf::Keyboard::Key::Home:
					if (m_pAudioPlayer.get() != nullptr)
					{
						m_pAudioPlayer->SetCurrentRate(1.0);
					}
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

		if (m_pAudioPlayer.get() != nullptr && m_pAudioPlayer->IsEnded())
		{
			if (m_nAudioIndex < m_audioFilePaths.size() - 1)
			{
				StepOnTrack();
			}
		}

		if (bOnWindowMove)
		{
			int iPosX = sf::Mouse::getPosition().x - m_window->getSize().x / 2;
			int iPosY = sf::Mouse::getPosition().y - m_window->getSize().y / 2;
			m_window->setPosition(sf::Vector2i(iPosX, iPosY));
		}
	}
	return iRet;
}

void CSfmlSpinePlayer::ClearDrawables()
{
	m_drawables.clear();
	m_atlases.clear();
	m_skeletonData.clear();

	m_animationNames.clear();
	m_nAnimationIndex = 0;
}
/*描画器設定*/
bool CSfmlSpinePlayer::SetupDrawer()
{
	/*
	 * Spine outline shader, an extension by Unity, cannot be brought forth by SFML. 
	 * Some scene gets closer to natural with premultiplied alpha disabled, some with enabled.
	 */
	const std::vector<std::string> pmaSelectiveList
	{ "penis", "tama", "vagina", "manko", "penice", "chin", "ccitsu", "chitsu", "manco", "vibrator", "cover"};

	for (const auto& pSkeletonData : m_skeletonData)
	{
		const auto pDrawable = std::make_shared<CSfmlSpineDrawableC>(pSkeletonData.get());
		if (pDrawable.get() == nullptr)continue;

		pDrawable->timeScale = 1.0f;
		pDrawable->skeleton->x = m_fBaseSize.x / 2;
		pDrawable->skeleton->y = m_fBaseSize.y / 2;
		spSkeleton_setToSetupPose(pDrawable->skeleton);
		spSkeleton_updateWorldTransform(pDrawable->skeleton);

		m_drawables.push_back(pDrawable);

		pDrawable->SetSelectivePmaList(pmaSelectiveList);

		for (size_t i = 0; i < pSkeletonData->animationsCount; ++i)
		{
			const std::string& strAnimationName = pSkeletonData->animations[i]->name;
			auto iter = std::find(m_animationNames.begin(), m_animationNames.end(), strAnimationName);
			if (iter == m_animationNames.cend())m_animationNames.push_back(strAnimationName);
		}
	}

	if (!m_animationNames.empty())
	{
		for (size_t i = 0; i < m_skeletonData.size(); ++i)
		{
			spAnimationState_setAnimationByName(m_drawables.at(i).get()->animationState, 0, m_animationNames.at(0).c_str(), true);
		}
	}

	return m_animationNames.size() > 0;
}
/*標準寸法算出*/
void CSfmlSpinePlayer::WorkOutDefualtSize()
{
	if (m_skeletonData.empty())return;

	float fMaxSize = 0.f;
	const auto CompareDimention = [this, &fMaxSize](float fWidth, float fHeight)
		-> bool
		{
			if (fWidth > 0.f && fHeight > 0.f && fWidth * fHeight > fMaxSize)
			{
				m_fBaseSize.x = fWidth;
				m_fBaseSize.y = fHeight;
				fMaxSize = fWidth * fHeight;
				return true;
			}

			return false;
		};

	for (const auto& pSkeletonData : m_skeletonData)
	{
		if (pSkeletonData->defaultSkin == nullptr)continue;

		const char* attachmentName = spSkin_getAttachmentName(pSkeletonData->defaultSkin, 0, 0);
		if (attachmentName == nullptr)continue;

		spAttachment* pAttachment = spSkin_getAttachment(pSkeletonData->defaultSkin, 0, attachmentName);
		if (pAttachment == nullptr)continue;

		/*Calculation based on the discussion of forum thread 2998 and 7662*/
		if (pAttachment->type == SP_ATTACHMENT_REGION)
		{
			spRegionAttachment* pRegionAttachment = (spRegionAttachment*)pAttachment;

			bool bRet = CompareDimention(pRegionAttachment->width * pRegionAttachment->scaleX, pRegionAttachment->height * pRegionAttachment->scaleY);
			if (bRet)
			{
				m_fDefaultOffset.x = pRegionAttachment->x * 2.f;
				m_fDefaultOffset.y = -pRegionAttachment->y * 2.f;

				if (pRegionAttachment->x != 0 || pRegionAttachment->y != 0)
				{
					spSlotData* pSlotData = spSkeletonData_findSlot(pSkeletonData.get(), attachmentName);
					if (pSlotData == nullptr)continue;

					for (int i = 0; i < pSkeletonData->bonesCount; ++i)
					{
						if (strcmp(pSlotData->boneData->name, pSkeletonData->bones[i]->name) == 0)
						{
							if (pRegionAttachment->y * pSkeletonData->bones[i]->y <= 0)
							{
								m_fDefaultOffset.y -= pSkeletonData->bones[i]->y * 2.f;
							}
							break;
						}
					}
				}
			}
		}
		else if (pAttachment->type == SP_ATTACHMENT_MESH)
		{
			spMeshAttachment* pMeshAttachment = (spMeshAttachment*)pAttachment;

			spSlotData* pSlotData = spSkeletonData_findSlot(pSkeletonData.get(), attachmentName);
			
			float fScaleX = pSlotData != nullptr ? pSlotData->boneData->scaleX : 1.f;
			float fScaleY = pSlotData != nullptr ? pSlotData->boneData->scaleY : 1.f;

			bool bRet = CompareDimention(pMeshAttachment->width * fScaleX, pMeshAttachment->height * fScaleY);
			if (bRet && pMeshAttachment->super.verticesCount >= pMeshAttachment->hullLength * 2)
			{
				const auto WorkoutCentroid = [&pMeshAttachment]()
					-> sf::Vector2f
					{
						sf::Vector2f fCentroid{};
						float fFilled = 0.f;

						sf::Vector2f fFore
						{
							pMeshAttachment->super.vertices[2 * (pMeshAttachment->hullLength - 1)],
							pMeshAttachment->super.vertices[2 * (pMeshAttachment->hullLength - 1) + 1]
						};
						for (int i = 0; i < pMeshAttachment->hullLength; ++i)
						{
							sf::Vector2f fNext
							{
								pMeshAttachment->super.vertices[2 * i],
								pMeshAttachment->super.vertices[2 * i + 1]
							};
							float fArea = fFore.x * fNext.y - fFore.y * fNext.x;
							fCentroid.x += (fFore.x + fNext.x) * fArea;
							fCentroid.y += (fFore.y + fNext.y) * fArea;
							fFilled += fArea;
							fFore = fNext;
						}
						fCentroid.x /= (6.f * fFilled * 0.5f);
						fCentroid.y /= (6.f * fFilled * 0.5f);

						return fCentroid;
					};

				sf::Vector2f fCentroid = WorkoutCentroid();

				m_fDefaultOffset.x = fCentroid.x * 2.f;
				m_fDefaultOffset.y = -fCentroid.y * 2.f;

				if (fCentroid.x != 0 || fCentroid.y != 0)
				{
					spSlotData* pSlotData = spSkeletonData_findSlot(pSkeletonData.get(), attachmentName);
					if (pSlotData == nullptr)continue;

					for (int i = 0; i < pSkeletonData->bonesCount; ++i)
					{
						if (strcmp(pSlotData->boneData->name, pSkeletonData->bones[i]->name) == 0)
						{
							m_fDefaultOffset.y -= pSkeletonData->bones[i]->y * 2.f;
							break;
						}
					}
				}
			}
		}
	}

	for (const auto& pSkeletonData : m_skeletonData)
	{
		CompareDimention(pSkeletonData->width, pSkeletonData->height);
	}
}
/*標準尺度算出*/
void CSfmlSpinePlayer::WorkOutDefaultScale()
{
	unsigned int uiSkeletonWidth = static_cast<unsigned int>(m_fBaseSize.x);
	unsigned int uiSkeletonHeight = static_cast<unsigned int>(m_fBaseSize.y);

	unsigned int uiDesktopWidth = sf::VideoMode::getDesktopMode().width;
	unsigned int uiDesktopHeight = sf::VideoMode::getDesktopMode().height;

	if (uiSkeletonWidth > uiDesktopWidth || uiSkeletonHeight > uiDesktopHeight)
	{
		if (uiDesktopWidth > uiDesktopHeight)
		{
			m_fDefaultScale = static_cast<float>(uiDesktopHeight) / uiSkeletonHeight;
			m_fThresholdScale = static_cast<float>(uiDesktopWidth) / uiSkeletonWidth;
		}
		else
		{
			m_fDefaultScale = static_cast<float>(uiDesktopWidth) / uiSkeletonWidth;
			m_fThresholdScale = static_cast<float>(uiDesktopHeight) / uiSkeletonHeight;
		}
		m_fSkeletonScale = m_fDefaultScale;
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
	m_fSkeletonScale = m_fDefaultScale;
	m_fOffset = m_bFDefaultOffSetEnabled ? m_fDefaultOffset : sf::Vector2f{};

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
		m_window->setSize(sf::Vector2u(static_cast<unsigned int>(m_fBaseSize.x * m_fSkeletonScale), static_cast<unsigned int>(m_fBaseSize.y * m_fSkeletonScale)));
	}
}
/*視点移動*/
void CSfmlSpinePlayer::MoveViewPoint(int iX, int iY)
{
	m_fOffset.x += iX;
	m_fOffset.y += iY;
	for (size_t i = 0; i < m_drawables.size(); ++i)
	{
		m_drawables.at(i)->skeleton->x = (m_fBaseSize.x - m_fOffset.x) / 2;
		m_drawables.at(i)->skeleton->y = (m_fBaseSize.y - m_fOffset.y) / 2;
	}
}
/*動作移行*/
void CSfmlSpinePlayer::ShiftAnimation()
{
	++m_nAnimationIndex;
	if (m_nAnimationIndex > m_animationNames.size() - 1)m_nAnimationIndex = 0;
	for (size_t i = 0; i < m_drawables.size(); ++i)
	{
		spAnimationState_setAnimationByName(m_drawables.at(i).get()->animationState, 0, m_animationNames.at(m_nAnimationIndex).c_str(), true);
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

		if (!m_bTrackHidden)
		{
			m_window->draw(m_trackText);
		}

		m_window->display();
	}
}
/*音声送り・戻し*/
void CSfmlSpinePlayer::StepOnTrack(bool bForward)
{
	if (bForward)
	{
		++m_nAudioIndex;
		if (m_nAudioIndex >= m_audioFilePaths.size())
		{
			m_nAudioIndex = 0;
		}
	}
	else
	{
		--m_nAudioIndex;
		if (m_nAudioIndex >= m_audioFilePaths.size())
		{
			m_nAudioIndex = m_audioFilePaths.size() - 1;
		}
	}

	UpdateTrack();
}
/*再生音声更新*/
void CSfmlSpinePlayer::UpdateTrack()
{
	if (m_nAudioIndex >= m_audioFilePaths.size() || m_pAudioPlayer.get() == nullptr)
	{
		m_trackText.setString("");
		return;
	}

	m_pAudioPlayer->Play(m_audioFilePaths[m_nAudioIndex].c_str());

	std::string str = std::to_string(m_nAudioIndex + 1) + "/" + std::to_string(m_audioFilePaths.size());
	m_trackText.setString(str);
}
/*音声再生速度変更*/
void CSfmlSpinePlayer::ChangePlaybackRate(bool bFaster)
{
	if (m_pAudioPlayer.get() == nullptr)return;

	constexpr double dbRatePortion = 0.1;
	constexpr double dbMaxRate = 2.5;
	constexpr double dbMinRate = 0.5;

	double dbPlaybackRate = m_pAudioPlayer->GetCurrentRate();
	if (bFaster)
	{
		dbPlaybackRate += dbRatePortion;
		if (dbPlaybackRate > dbMaxRate)
		{
			dbPlaybackRate = dbMaxRate;
		}
	}
	else
	{
		dbPlaybackRate -= dbPlaybackRate;
		if (dbPlaybackRate < dbMinRate)
		{
			dbPlaybackRate = dbMinRate;
		}
	}

	m_pAudioPlayer->SetCurrentRate(dbPlaybackRate);
}
/*文字色切り替え*/
void CSfmlSpinePlayer::ToggleTextColor()
{
	m_trackText.setFillColor(m_trackText.getFillColor() == sf::Color::Black ? sf::Color::White : sf::Color::Black);
	m_trackText.setOutlineColor(m_trackText.getFillColor() == sf::Color::Black ? sf::Color::White : sf::Color::Black);
}
