#ifndef SFML_SPINE_PLAYER_H_
#define SFML_SPINE_PLAYER_H_

#include <memory>

#include <SFML/Graphics.hpp>

#include "sfml_spine_c.h"
#include "mf_media_player.h"

class CSfmlSpinePlayer
{
public:
	CSfmlSpinePlayer();
	~CSfmlSpinePlayer();

	bool SetSpine(const std::vector<std::string>& atlasPaths, const std::vector<std::string>& skelPaths, bool bIsBinary);

	bool SetFont(const std::string& strFilePath, bool bBold, bool bItalic);
	void SetAudioFiles(const std::vector<std::wstring> &wstrAudioFilePaths);

	int Display();
private:
	std::vector<std::shared_ptr<spAtlas>> m_atlases;
	std::vector<std::shared_ptr<spSkeletonData>> m_skeletonData;
	std::vector<std::shared_ptr<CSfmlSpineDrawableC>> m_drawables;

	std::unique_ptr<sf::RenderWindow> m_window;

	sf::Vector2f m_fBaseSize = sf::Vector2f{ 1920.f, 1080.f };
	float m_fDefaultScale = 1.f;
	float m_fThresholdScale = 1.f;
	sf::Vector2f m_DefaultScaleOffset{};
	sf::Vector2f m_fDefaultPosOffset{};

	bool m_bSelectivePmaEnabled = false;

	float m_fTimeScale = 1.f;
	float m_fSkeletonScale = 1.f;
	sf::Vector2f m_fOffset{};

	std::vector<std::string> m_animationNames;
	size_t m_nAnimationIndex = 0;

	void ClearDrawables();
	bool SetupDrawer();

	void WorkOutDefualtSize();
	void WorkOutDefaultScale();

	void RescaleSkeleton();
	void RescaleTime();
	void ResetScale();
	void ResizeWindow();

	void MoveViewPoint(int iX, int iY);
	void ShiftAnimation();

	void Redraw(float fDelta);

	sf::FloatRect GetBoundingBox();

	std::vector<std::wstring> m_audioFilePaths;
	size_t m_nAudioIndex = 0;

	std::unique_ptr<CMfMediaPlayer> m_pAudioPlayer;

	void StepOnTrack(bool bForward = true);
	void UpdateTrack();
	void ChangePlaybackRate(bool bFaster);

	sf::Font m_trackFont;
	sf::Text m_trackText;
	bool m_bTrackHidden = false;

	void ToggleTextColor();
};

#endif // SFML_SPINE_PLAYER_H_
