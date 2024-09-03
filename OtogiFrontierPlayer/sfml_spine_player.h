#ifndef SFML_SPINE_PLAYER_H_
#define SFML_SPINE_PLAYER_H_

#include <memory>

#include <SFML/Graphics.hpp>

#include "sfml_spine_c.h"

class CSfmlSpinePlayer
{
public:
	CSfmlSpinePlayer();
	~CSfmlSpinePlayer();
	bool SetSpine(const std::vector<std::string>& atlasPaths, const std::vector<std::string>& skelPaths, bool bIsBinary);
	void SetAudios(std::vector<std::wstring>& filePaths);
	int Display();
private:
	std::vector<spAtlas*> m_atlases;
	std::vector<spSkeletonData*> m_skeletonData;
	std::vector<std::shared_ptr<CSfmlSpineDrawableC>> m_drawables;

	std::unique_ptr<sf::RenderWindow> m_window;

	sf::Vector2f m_fBaseSize = sf::Vector2f{ 1920.f, 1080.f };
	float m_fDefaultScale = 1.f;
	float m_fThresholdScale = 1.f;
	sf::Vector2f m_fDefaultOffset{};
	bool m_bFDefaultOffSetEnabled= false;

	float m_fTimeScale = 1.f;
	float m_fSkeletonScale = 1.f;
	sf::Vector2f m_fOffset{};

	std::vector<std::string> m_animationNames;
	size_t m_nAnimationIndex = 0;

	std::vector<std::wstring> m_audio_files;
	size_t m_nAudioIndex = 0;

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
};

#endif // SFML_SPINE_PLAYER_H_
