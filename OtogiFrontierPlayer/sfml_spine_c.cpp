﻿
#include <spine/extension.h>

#include "sfml_spine_c.h"

/*Implementations for <extension.h>*/

void _spAtlasPage_createTexture(spAtlasPage* pAtlasPage, const char* path)
{
	sf::Texture* texture = new sf::Texture();
	if (!texture->loadFromFile(path))
	{
		delete texture;
		return;
	}

	if (pAtlasPage->magFilter == SP_ATLAS_LINEAR)
	{
		texture->setSmooth(true);
	}
	if (pAtlasPage->uWrap == SP_ATLAS_REPEAT && pAtlasPage->vWrap == SP_ATLAS_REPEAT)
	{
		texture->setRepeated(true);
	}

	if (pAtlasPage->width == 0 || pAtlasPage->height == 0)
	{
		sf::Vector2u size = texture->getSize();
		pAtlasPage->width = size.x;
		pAtlasPage->height = size.y;
	}

	pAtlasPage->rendererObject = texture;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self)
{
	delete (sf::Texture*)self->rendererObject;
}

char* _spUtil_readFile(const char* path, int* length)
{
	return _spReadFile(path, length);
}
/* end of implementations for <extension.h> */

static sf::BlendMode g_sfmlBlendModeNormalPma = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcAlpha);
static sf::BlendMode g_sfmlBlendModeAddPma = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::One);
static sf::BlendMode g_sfmlBlendModeScreen = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcColor);
static sf::BlendMode g_sfmlBlendModeMultiply = sf::BlendMode
(
	sf::BlendMode::Factor::DstColor,
	sf::BlendMode::Factor::OneMinusSrcAlpha,
	sf::BlendMode::Equation::Add,
	sf::BlendMode::Factor::Zero,
	sf::BlendMode::Factor::One,
	sf::BlendMode::Equation::Add
);

CSfmlSpineDrawableC::CSfmlSpineDrawableC(spSkeletonData* pSkeletonData, spAnimationStateData* pAnimationStateData)
{
	spBone_setYDown(true);

	m_worldVertices = spFloatArray_create(128);

	m_sfmlVertices.setPrimitiveType(sf::PrimitiveType::Triangles);

	skeleton = spSkeleton_create(pSkeletonData);
	if (pAnimationStateData == nullptr)
	{
		pAnimationStateData = spAnimationStateData_create(pSkeletonData);
		m_bHasOwnAnimationStateData = true;
	}

	animationState = spAnimationState_create(pAnimationStateData);

	m_clipper = spSkeletonClipping_create();
}

CSfmlSpineDrawableC::~CSfmlSpineDrawableC()
{
	if (m_worldVertices != nullptr)
	{
		spFloatArray_dispose(m_worldVertices);
	}
	if (animationState != nullptr)
	{
		if (m_bHasOwnAnimationStateData)
		{
			spAnimationStateData_dispose(animationState->data);
		}

		spAnimationState_dispose(animationState);
	}
	if (skeleton != nullptr)
	{
		spSkeleton_dispose(skeleton);
	}
	if (m_clipper != nullptr)
	{
		spSkeletonClipping_dispose(m_clipper);
	}
}

void CSfmlSpineDrawableC::Update(float fDelta)
{
	if (skeleton == nullptr || animationState == nullptr)return;

	spSkeleton_update(skeleton, fDelta);
	spAnimationState_update(animationState, fDelta * timeScale);
	spAnimationState_apply(animationState, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}

void CSfmlSpineDrawableC::draw(sf::RenderTarget& renderTarget, sf::RenderStates renderStates) const
{
	static unsigned short quadIndices[] = { 0, 1, 2, 2, 3, 0 };

	if (m_worldVertices == nullptr || m_clipper == nullptr || skeleton == nullptr || animationState == nullptr)return;

	if (skeleton->color.a == 0) return;

	for (int i = 0; i < skeleton->slotsCount; ++i)
	{
		spSlot* pSlot = skeleton->drawOrder[i];
		spAttachment* pAttachment = pSlot->attachment;
		/*spine-c 3.6 lacks slot->bone->active*/
		if (pAttachment == nullptr || (pSlot->color.a == 0 && pAttachment->type != SP_ATTACHMENT_CLIPPING))
		{
			spSkeletonClipping_clipEnd(m_clipper, pSlot);
			continue;
		}

		spFloatArray* pVertices = m_worldVertices;
		float* pAttachmentUvs = nullptr;
		unsigned short* pIndices = nullptr;
		int indicesCount = 0;

		spColor* pAttachmentColor = nullptr;

		sf::Texture* pSfmlTexture = nullptr;

		if (pAttachment->type == SP_ATTACHMENT_REGION)
		{
			spRegionAttachment* pRegionAttachment = (spRegionAttachment*)pAttachment;
			pAttachmentColor = &pRegionAttachment->color;

			if (pAttachmentColor->a == 0)
			{
				spSkeletonClipping_clipEnd(m_clipper, pSlot);
				continue;
			}

			spFloatArray_setSize(pVertices, 8);
			spRegionAttachment_computeWorldVertices(pRegionAttachment, pSlot->bone, pVertices->items, 0, 2);
			pAttachmentUvs = pRegionAttachment->uvs;
			pIndices = quadIndices;
			indicesCount = sizeof(quadIndices) / sizeof(unsigned short);

			pSfmlTexture = (sf::Texture*)((spAtlasRegion*)pRegionAttachment->rendererObject)->page->rendererObject;
		}
		else if (pAttachment->type == SP_ATTACHMENT_MESH)
		{
			spMeshAttachment* pMeshAttachment = (spMeshAttachment*)pAttachment;
			pAttachmentColor = &pMeshAttachment->color;

			if (pAttachmentColor->a == 0)
			{
				spSkeletonClipping_clipEnd(m_clipper, pSlot);
				continue;
			}
			spFloatArray_setSize(pVertices, pMeshAttachment->super.worldVerticesLength);
			spVertexAttachment_computeWorldVertices(SUPER(pMeshAttachment), pSlot, 0, pMeshAttachment->super.worldVerticesLength, pVertices->items, 0, 2);
			pAttachmentUvs = pMeshAttachment->uvs;
			pIndices = pMeshAttachment->triangles;
			indicesCount = pMeshAttachment->trianglesCount;

			pSfmlTexture = (sf::Texture*)((spAtlasRegion*)pMeshAttachment->rendererObject)->page->rendererObject;

		}
		else if (pAttachment->type == SP_ATTACHMENT_CLIPPING)
		{
			spClippingAttachment* clip = (spClippingAttachment*)pSlot->attachment;
			spSkeletonClipping_clipStart(m_clipper, pSlot, clip);
			continue;
		}
		else
		{
			spSkeletonClipping_clipEnd(m_clipper, pSlot);
			continue;
		}

		if (spSkeletonClipping_isClipping(m_clipper))
		{
			/*The third argumnet `verticesLength` is not unsed inside the function.*/
			spSkeletonClipping_clipTriangles(m_clipper, pVertices->items, pVertices->size, pIndices, indicesCount, pAttachmentUvs, 2);
			if (m_clipper->clippedTriangles->size == 0)
			{
				spSkeletonClipping_clipEnd(m_clipper, pSlot);
				continue;
			}
			pVertices = m_clipper->clippedVertices;
			pAttachmentUvs = m_clipper->clippedUVs->items;
			pIndices = m_clipper->clippedTriangles->items;
			indicesCount = m_clipper->clippedTriangles->size;
		}

		spColor tint;
		tint.r = skeleton->color.r * pSlot->color.r * pAttachmentColor->r;
		tint.g = skeleton->color.g * pSlot->color.g * pAttachmentColor->g;
		tint.b = skeleton->color.b * pSlot->color.b * pAttachmentColor->b;
		tint.a = skeleton->color.a * pSlot->color.a * pAttachmentColor->a;

		sf::Vector2u sfmlSize = pSfmlTexture->getSize();
		m_bAlphaPremultiplied = true ^ IsPmaForciblyDisabled(pSlot->attachment->name);

		m_sfmlVertices.clear();
		for (int ii = 0; ii < indicesCount; ++ii)
		{
			sf::Vertex sfmlVertex;
			sfmlVertex.position.x = pVertices->items[pIndices[ii] * 2LL];
			sfmlVertex.position.y = pVertices->items[pIndices[ii] * 2LL + 1];
			sfmlVertex.color.r = (sf::Uint8)(tint.r * 255.f * (m_bAlphaPremultiplied ? tint.a : 1.f));
			sfmlVertex.color.g = (sf::Uint8)(tint.g * 255.f * (m_bAlphaPremultiplied ? tint.a : 1.f));
			sfmlVertex.color.b = (sf::Uint8)(tint.b * 255.f * (m_bAlphaPremultiplied ? tint.a : 1.f));
			sfmlVertex.color.a = (sf::Uint8)(tint.a * 255.f);
			sfmlVertex.texCoords.x = pAttachmentUvs[pIndices[ii] * 2LL] * sfmlSize.x;
			sfmlVertex.texCoords.y = pAttachmentUvs[pIndices[ii] * 2LL + 1] * sfmlSize.y;
			m_sfmlVertices.append(sfmlVertex);
		}

		sf::BlendMode sfmlBlendMode;
		switch (pSlot->data->blendMode)
		{
		case spBlendMode::SP_BLEND_MODE_ADDITIVE:
			sfmlBlendMode = m_bAlphaPremultiplied ? g_sfmlBlendModeAddPma : sf::BlendAdd;
			break;
		case spBlendMode::SP_BLEND_MODE_MULTIPLY:
			sfmlBlendMode = g_sfmlBlendModeMultiply;
			break;
		case spBlendMode::SP_BLEND_MODE_SCREEN:
			sfmlBlendMode = g_sfmlBlendModeScreen;
			break;
		default:
			sfmlBlendMode = m_bAlphaPremultiplied ? g_sfmlBlendModeNormalPma : sf::BlendAlpha;
			break;
		}

		renderStates.blendMode = sfmlBlendMode;
		renderStates.texture = pSfmlTexture;
		renderTarget.draw(m_sfmlVertices, renderStates);

		spSkeletonClipping_clipEnd(m_clipper, pSlot);
	}
	spSkeletonClipping_clipEnd2(m_clipper);
}

sf::FloatRect CSfmlSpineDrawableC::GetBoundingBox()
{
	float fMinX = FLT_MAX;
	float fMinY = FLT_MAX;
	float fMaxX = -FLT_MAX;
	float fMaxY = -FLT_MAX;

	spFloatArray* pTempVertices = spFloatArray_create(128);

	for (int i = 0; i < skeleton->slotsCount; ++i)
	{
		spSlot* pSlot = skeleton->drawOrder[i];
		spAttachment* pAttachment = pSlot->attachment;

		if (pAttachment == nullptr)continue;

		if (pAttachment->type == SP_ATTACHMENT_REGION)
		{
			spRegionAttachment* pRegionAttachment = (spRegionAttachment*)pAttachment;

			spFloatArray_setSize(pTempVertices, 8);
			spRegionAttachment_computeWorldVertices(pRegionAttachment, pSlot->bone, pTempVertices->items, 0, 2);

		}
		else if (pAttachment->type == SP_ATTACHMENT_MESH)
		{
			spMeshAttachment* pMeshAttachment = (spMeshAttachment*)pAttachment;

			spFloatArray_setSize(pTempVertices, pMeshAttachment->super.worldVerticesLength);
			spVertexAttachment_computeWorldVertices(SUPER(pMeshAttachment), pSlot, 0, pMeshAttachment->super.worldVerticesLength, pTempVertices->items, 0, 2);
		}
		else
		{
			continue;
		}

		for (size_t i = 0; i < pTempVertices->size; i += 2)
		{
			float fX = pTempVertices->items[i];
			float fY = pTempVertices->items[i + 1LL];

			fMinX = fMinX < fX ? fMinX : fX;
			fMinY = fMinY < fY ? fMinY : fY;
			fMaxX = fMaxX > fX ? fMaxX : fX;
			fMaxY = fMaxY > fY ? fMaxY : fY;
		}
	}

	if(pTempVertices != nullptr)spFloatArray_dispose(pTempVertices);

	return sf::FloatRect{ fMinX, fMinY, fMaxX, fMaxY };
}

bool CSfmlSpineDrawableC::IsPmaForciblyDisabled(const char* const szSlotName) const
{
	if (m_bSelectivePma)
	{
		for (size_t ii = 0; ii < m_PmaSelectiveList.size(); ++ii)
		{
			if (strstr(szSlotName, m_PmaSelectiveList.at(ii).c_str()))
			{
				return true;
			}
		}
	}
	return false;
}
