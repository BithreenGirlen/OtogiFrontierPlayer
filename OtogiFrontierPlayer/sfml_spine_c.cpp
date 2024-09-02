
#include <spine/extension.h>

#include "sfml_spine_c.h"

/*Implementations for <extension.h>*/

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path)
{
	sf::Texture* texture = new sf::Texture();
	if (!texture->loadFromFile(path))
	{
		delete texture;
		return;
	}

	if (self->magFilter == SP_ATLAS_LINEAR)
	{
		texture->setSmooth(true);
	}
	if (self->uWrap == SP_ATLAS_REPEAT && self->vWrap == SP_ATLAS_REPEAT)
	{
		texture->setRepeated(true);
	}

	self->rendererObject = texture;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self)
{
	delete (sf::Texture*)self->rendererObject;
}

char* _spUtil_readFile(const char* path, int* length)
{
	return _spReadFile(path, length);
}
// end of implementations for <extension.h>

CSfmlSpineDrawableC::CSfmlSpineDrawableC(spSkeletonData* pSkeletonData, spAnimationStateData* pAnimationStateData)
{
	spBone_setYDown(true);

	m_worldVertices = spFloatArray_create(pSkeletonData->bonesCount * sizeof(spFloatArray) / sizeof(float));

	m_sfmlVertices.setPrimitiveType(sf::PrimitiveType::Triangles);

	skeleton = spSkeleton_create(pSkeletonData);
	if (pAnimationStateData == nullptr)
	{
		pAnimationStateData = spAnimationStateData_create(pSkeletonData);
		m_bHasOwnAnimationStateData = true;
	}

	state = spAnimationState_create(pAnimationStateData);

	m_clipper = spSkeletonClipping_create();
}

CSfmlSpineDrawableC::~CSfmlSpineDrawableC()
{
	if (m_worldVertices != nullptr)
	{
		spFloatArray_dispose(m_worldVertices);
	}
	if (state != nullptr)
	{
		if (m_bHasOwnAnimationStateData)
		{
			spAnimationStateData_dispose(state->data);
		}

		spAnimationState_dispose(state);
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
	if (skeleton == nullptr || state == nullptr)return;

	spSkeleton_update(skeleton, fDelta);
	spAnimationState_update(state, fDelta * timeScale);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}

void CSfmlSpineDrawableC::draw(sf::RenderTarget& renderTarget, sf::RenderStates renderStates) const
{
	static unsigned short quadIndices[] = { 0, 1, 2, 2, 3, 0 };

	if (m_worldVertices == nullptr || m_clipper == nullptr || skeleton == nullptr || state == nullptr)return;

	if (skeleton->color.a == 0) return;

	for (int i = 0; i < skeleton->slotsCount; ++i)
	{
		spSlot* slot = skeleton->drawOrder[i];
		spAttachment* pAttachment = slot->attachment;
		/*spine-c 3.6 lacks slot->bone->active*/
		if (pAttachment == nullptr || slot->color.a == 0 )
		{
			spSkeletonClipping_clipEnd(m_clipper, slot);
			continue;
		}

		spFloatArray* pVertices = m_worldVertices;
		int verticesCount = 0;
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
				spSkeletonClipping_clipEnd(m_clipper, slot);
				continue;
			}

			spFloatArray_setSize(pVertices, 8);
			spRegionAttachment_computeWorldVertices(pRegionAttachment, slot->bone, pVertices->items, 0, 2);
			verticesCount = 4;
			pAttachmentUvs = pRegionAttachment->uvs;
			pIndices = quadIndices;
			indicesCount = 6;
			pSfmlTexture = (sf::Texture*)((spAtlasRegion*)pRegionAttachment->rendererObject)->page->rendererObject;
		}
		else if (pAttachment->type == SP_ATTACHMENT_MESH)
		{
			spMeshAttachment* pMeshAttachment = (spMeshAttachment*)pAttachment;
			pAttachmentColor = &pMeshAttachment->color;

			if (pAttachmentColor->a == 0)
			{
				spSkeletonClipping_clipEnd(m_clipper, slot);
				continue;
			}
			spFloatArray_setSize(pVertices, pMeshAttachment->super.worldVerticesLength);
			spVertexAttachment_computeWorldVertices(SUPER(pMeshAttachment), slot, 0, pMeshAttachment->super.worldVerticesLength, pVertices->items, 0, 2);
			verticesCount = pMeshAttachment->super.worldVerticesLength / 2;
			pAttachmentUvs = pMeshAttachment->uvs;
			pIndices = pMeshAttachment->triangles;
			indicesCount = pMeshAttachment->trianglesCount;
			pSfmlTexture = (sf::Texture*)((spAtlasRegion*)pMeshAttachment->rendererObject)->page->rendererObject;

		}
		else if (pAttachment->type == SP_ATTACHMENT_CLIPPING)
		{
			spClippingAttachment* clip = (spClippingAttachment*)slot->attachment;
			spSkeletonClipping_clipStart(m_clipper, slot, clip);
			continue;
		}
		else
		{
			continue;
		}

		if (spSkeletonClipping_isClipping(m_clipper))
		{
			spSkeletonClipping_clipTriangles(m_clipper, pVertices->items, verticesCount / 2, pIndices, indicesCount, pAttachmentUvs, 2);
			pVertices = m_clipper->clippedVertices;
			verticesCount = m_clipper->clippedVertices->size / 2;
			pAttachmentUvs = m_clipper->clippedUVs->items;
			pIndices = m_clipper->clippedTriangles->items;
			indicesCount = m_clipper->clippedTriangles->size;
		}

		spColor tint;
		tint.r = skeleton->color.r * slot->color.r * pAttachmentColor->r;
		tint.g = skeleton->color.g * slot->color.g * pAttachmentColor->g;
		tint.b = skeleton->color.b * slot->color.b * pAttachmentColor->b;
		tint.a = skeleton->color.a * slot->color.a * pAttachmentColor->a;

		sf::Vector2u sfmlSize = pSfmlTexture->getSize();
		m_bAlphaPremultiplied = true ^ IsPmaForciblyDisabled(slot->attachment->name);

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
		switch (slot->data->blendMode)
		{
		case spBlendMode::SP_BLEND_MODE_ADDITIVE:
			sfmlBlendMode.colorSrcFactor = m_bAlphaPremultiplied ? sf::BlendMode::Factor::One : sf::BlendMode::Factor::SrcAlpha;
			sfmlBlendMode.colorDstFactor = sf::BlendMode::Factor::One;
			sfmlBlendMode.colorEquation = sf::BlendMode::Equation::Add;
			sfmlBlendMode.alphaSrcFactor = sf::BlendMode::Factor::One;
			sfmlBlendMode.alphaDstFactor = sf::BlendMode::Factor::One;
			sfmlBlendMode.alphaEquation = sf::BlendMode::Equation::Add;
			break;
		case spBlendMode::SP_BLEND_MODE_MULTIPLY:
			sfmlBlendMode.colorSrcFactor = sf::BlendMode::Factor::DstColor;
			sfmlBlendMode.colorDstFactor = sf::BlendMode::Factor::OneMinusSrcAlpha;
			sfmlBlendMode.colorEquation = sf::BlendMode::Equation::Add;
			sfmlBlendMode.alphaSrcFactor = sf::BlendMode::Factor::OneMinusSrcAlpha;
			sfmlBlendMode.alphaDstFactor = sf::BlendMode::Factor::OneMinusSrcAlpha;
			sfmlBlendMode.alphaEquation = sf::BlendMode::Equation::Add;
			break;
		case spBlendMode::SP_BLEND_MODE_SCREEN:
			sfmlBlendMode.colorSrcFactor = sf::BlendMode::Factor::One;
			sfmlBlendMode.colorDstFactor = sf::BlendMode::Factor::OneMinusSrcColor;
			sfmlBlendMode.colorEquation = sf::BlendMode::Equation::Add;
			sfmlBlendMode.alphaSrcFactor = sf::BlendMode::Factor::OneMinusSrcColor;
			sfmlBlendMode.alphaDstFactor = sf::BlendMode::Factor::OneMinusSrcColor;
			sfmlBlendMode.alphaEquation = sf::BlendMode::Equation::Add;
			break;
		default:
			sfmlBlendMode.colorSrcFactor = m_bAlphaPremultiplied ? sf::BlendMode::Factor::One : sf::BlendMode::SrcAlpha;
			sfmlBlendMode.colorDstFactor = sf::BlendMode::Factor::OneMinusSrcAlpha;
			sfmlBlendMode.colorEquation = sf::BlendMode::Equation::Add;
			sfmlBlendMode.alphaSrcFactor = sf::BlendMode::Factor::One;
			sfmlBlendMode.alphaDstFactor = sf::BlendMode::Factor::OneMinusSrcAlpha;
			sfmlBlendMode.alphaEquation = sf::BlendMode::Equation::Add;
			break;
		}

		renderStates.blendMode = sfmlBlendMode;
		renderStates.texture = pSfmlTexture;
		renderTarget.draw(m_sfmlVertices, renderStates);
		spSkeletonClipping_clipEnd(m_clipper, slot);
	}
	spSkeletonClipping_clipEnd2(m_clipper);
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
