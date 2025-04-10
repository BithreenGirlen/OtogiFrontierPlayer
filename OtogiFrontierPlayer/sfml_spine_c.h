#ifndef SFML_SPINE_C_H_
#define SFML_SPINE_C_H_

#include <spine/spine.h>
#include <SFML/Graphics.hpp>

class CSfmlSpineDrawableC : public sf::Drawable
{
public:
	CSfmlSpineDrawableC(spSkeletonData* pSkeletonData, spAnimationStateData* pAnimationStateData = nullptr);
	~CSfmlSpineDrawableC();

	spSkeleton* skeleton = nullptr;
	spAnimationState* animationState = nullptr;
	float timeScale = 1.f;

	void Update(float fDelta);
	virtual void draw(sf::RenderTarget& renderTarget, sf::RenderStates renderStates) const;

	/*<string> and <vector> are included somewhere in SFML headers*/
	void SetSelectivePmaList(const std::vector<std::string>& list) { m_PmaSelectiveList = list; }

	void SetSelectivePma(bool bEnabled) { m_bSelectivePma = bEnabled; }
	bool GetSelectivePma()const { return m_bSelectivePma; }

	sf::FloatRect GetBoundingBox();
private:
	bool m_bHasOwnAnimationStateData = false;
	mutable bool m_bAlphaPremultiplied = true;
	bool m_bSelectivePma = false;

	spSkeletonClipping* m_clipper = nullptr;

	spFloatArray* m_worldVertices = nullptr;
	mutable sf::VertexArray m_sfmlVertices;

	std::vector<std::string> m_PmaSelectiveList;

	bool IsPmaForciblyDisabled(const char* const szSlotName) const;
};

#endif // SFML_SPINE_C_H_
