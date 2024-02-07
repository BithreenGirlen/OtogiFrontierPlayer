#ifndef SFML_SPINE_PLAYER_H_
#define SFML_SPINE_PLAYER_H_

#include <memory>

#include <SFML/Graphics.hpp>

#include "deps/spine-sfml-3.6/spine-sfml.h"

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
	std::vector<std::shared_ptr<spine::SkeletonDrawable>> m_drawables;

	std::unique_ptr<sf::RenderWindow> m_window;

	float m_fMaxWidth = 1920.f;
	float m_fMaxHeight = 1080.f;
	float m_fDefaultWindowScale = 1.f;
	float m_fThresholdScale = 1.f;

	float m_fTimeScale = 1.f;
	float m_fSkeletonScale = 1.f;
	sf::Vector2i m_iOffset{};

	std::vector<std::string> m_animationNames;
	size_t m_nAnimationIndex = 0;

	std::vector<std::wstring> m_audio_files;
	size_t m_nAudioIndex = 0;

	bool SetupDrawer();
	void WorkOutDefaultScale();

	void RescaleSkeleton();
	void RescaleTime();
	void ResetScale();
	void ResizeWindow();

	void MoveViewPoint(int iX, int iY);
	void ShiftScene();

	void Redraw(float fDelta);
};

#endif // SFML_SPINE_PLAYER_H_
