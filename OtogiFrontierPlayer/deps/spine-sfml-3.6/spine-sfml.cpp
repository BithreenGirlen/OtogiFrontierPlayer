/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated July 28, 2023. Replaces all prior versions.
 *
 * Copyright (c) 2013-2023, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software or
 * otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THE
 * SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include "spine-sfml.h"

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

spColorArray* spColorArray_create(int initialCapacity) {
	spColorArray* array = ((spColorArray*)_spCalloc(1, sizeof(spColorArray), "_file_name_", 48));
	array->size = 0;
	array->capacity = initialCapacity;
	array->items = ((spColor*)_spCalloc(initialCapacity, sizeof(spColor), "_file_name_", 48));
	return array;
}
void spColorArray_dispose(spColorArray* self) {
	_spFree((void*)self->items);
	_spFree((void*)self);
}
void spColorArray_clear(spColorArray* self) { self->size = 0; }
spColorArray* spColorArray_setSize(spColorArray* self, int newSize) {
	self->size = newSize;
	if (self->capacity < newSize) {
		self->capacity = ((8) > ((int)(self->size * 1.75f)) ? (8) : ((int)(self->size * 1.75f)));
		self->items = ((spColor*)_spRealloc(self->items, sizeof(spColor) * (self->capacity)));
	}
	return self;
}
void spColorArray_ensureCapacity(spColorArray* self, int newCapacity) {
	if (self->capacity >= newCapacity) return;
	self->capacity = newCapacity;
	self->items = ((spColor*)_spRealloc(self->items, sizeof(spColor) * (self->capacity)));
}
void spColorArray_add(spColorArray* self, spColor value) {
	if (self->size == self->capacity) {
		self->capacity = ((8) > ((int)(self->size * 1.75f)) ? (8) : ((int)(self->size * 1.75f)));
		self->items = ((spColor*)_spRealloc(self->items, sizeof(spColor) * (self->capacity)));
	}
	self->items[self->size++] = value;
}
void spColorArray_addAll(spColorArray* self, spColorArray* other) {
	int i = 0;
	for (; i < other->size; i++) { spColorArray_add(self, other->items[i]); }
}
void spColorArray_addAllValues(spColorArray* self, spColor* values, int offset, int count) {
	int i = offset, n = offset + count;
	for (; i < n; i++) { spColorArray_add(self, values[i]); }
}
void spColorArray_removeAt(spColorArray* self, int index) {
	self->size--;
	memmove(self->items + index, self->items + index + 1, sizeof(spColor) * (self->size - index));
}

spColor spColorArray_pop(spColorArray* self) {
	spColor item = self->items[--self->size];
	return item;
}
spColor spColorArray_peek(spColorArray* self) { return self->items[self->size - 1]; }

/*Implementations for <extension.h>*/

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path) {
	sf::Texture* texture = new sf::Texture();
	if (!texture->loadFromFile(path)) return;

	if (self->magFilter == SP_ATLAS_LINEAR) texture->setSmooth(true);
	if (self->uWrap == SP_ATLAS_REPEAT && self->vWrap == SP_ATLAS_REPEAT) texture->setRepeated(true);

	self->rendererObject = texture;
	//sf::Vector2u size = texture->getSize();
	//self->width = size.x;
	//self->height = size.y;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self) {
	delete (sf::Texture*)self->rendererObject;
}

char* _spUtil_readFile(const char* path, int* length) {
	return _spReadFile(path, length);
}

namespace spine {

	SkeletonDrawable::SkeletonDrawable(spSkeletonData* skeletonData, spAnimationStateData* stateData) :
		timeScale(1),
		vertexArray(new sf::VertexArray(sf::Triangles, skeletonData->bonesCount * 4)),
		vertexEffect(0),
		worldVertices(0),
		clipper(0)
	{
		spBone_setYDown(true);
		worldVertices = MALLOC(float, SPINE_MESH_VERTEX_COUNT_MAX);
		skeleton = spSkeleton_create(skeletonData);
		tempUvs = spFloatArray_create(16);
		tempColors = spColorArray_create(16);

		ownsAnimationStateData = stateData == 0;
		if (ownsAnimationStateData) stateData = spAnimationStateData_create(skeletonData);

		state = spAnimationState_create(stateData);

		clipper = spSkeletonClipping_create();
	}

	SkeletonDrawable::~SkeletonDrawable() {
		delete vertexArray;
		FREE(worldVertices);
		if (ownsAnimationStateData) spAnimationStateData_dispose(state->data);
		spAnimationState_dispose(state);
		spSkeleton_dispose(skeleton);
		spSkeletonClipping_dispose(clipper);
		spFloatArray_dispose(tempUvs);
		spColorArray_dispose(tempColors);
	}

	void SkeletonDrawable::update(float deltaTime) {
		spSkeleton_update(skeleton, deltaTime);
		spAnimationState_update(state, deltaTime * timeScale);
		spAnimationState_apply(state, skeleton);
		spSkeleton_updateWorldTransform(skeleton);
	}

	void SkeletonDrawable::draw(sf::RenderTarget& target, sf::RenderStates states) const {
		vertexArray->clear();
		states.texture = NULL;
		unsigned short quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

		// Early out if skeleton is invisible
		if (skeleton->color.a == 0) return;

		if (vertexEffect != NULL) vertexEffect->begin(vertexEffect, skeleton);

		sf::Vertex vertex;
		sf::Texture* texture = NULL;
		for (int i = 0; i < skeleton->slotsCount; ++i) {
			spSlot* slot = skeleton->drawOrder[i];
			spAttachment* attachment = slot->attachment;
			if (!attachment) {
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}

			// Early out if the slot color is 0
			if (slot->color.a == 0) {
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}

			/*Leave out from drawing*/
			bool bFound = false;
			for (size_t ii = 0; ii < m_leaveOutList.size(); ++ii)
			{
				if (strstr(slot->data->name, m_leaveOutList.at(ii).c_str()))
				{
					bFound = true;
					break;
				}
			}
			if (bFound)
			{
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}

			float* vertices = worldVertices;
			int verticesCount = 0;
			float* uvs = 0;
			unsigned short* indices = 0;
			int indicesCount = 0;
			spColor* attachmentColor;

			if (attachment->type == SP_ATTACHMENT_REGION) {
				spRegionAttachment* regionAttachment = (spRegionAttachment*)attachment;
				attachmentColor = &regionAttachment->color;

				// Early out if slot is invisible
				if (attachmentColor->a == 0) {
					spSkeletonClipping_clipEnd(clipper, slot);
					continue;
				}

				spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, vertices, 0, 2);
				verticesCount = 4;
				uvs = regionAttachment->uvs;
				indices = quadIndices;
				indicesCount = 6;
				texture = (sf::Texture*)((spAtlasRegion*)regionAttachment->rendererObject)->page->rendererObject;

			}
			else if (attachment->type == SP_ATTACHMENT_MESH) {
				spMeshAttachment* mesh = (spMeshAttachment*)attachment;
				attachmentColor = &mesh->color;

				// Early out if slot is invisible
				if (attachmentColor->a == 0) {
					spSkeletonClipping_clipEnd(clipper, slot);
					continue;
				}

				if (mesh->super.worldVerticesLength > SPINE_MESH_VERTEX_COUNT_MAX) continue;
				spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, worldVertices, 0, 2);
				verticesCount = mesh->super.worldVerticesLength >> 1;
				uvs = mesh->uvs;
				indices = mesh->triangles;
				indicesCount = mesh->trianglesCount;
				texture = (sf::Texture*)((spAtlasRegion*)mesh->rendererObject)->page->rendererObject;

			}
			else if (attachment->type == SP_ATTACHMENT_CLIPPING) {
				spClippingAttachment* clip = (spClippingAttachment*)slot->attachment;
				spSkeletonClipping_clipStart(clipper, slot, clip);
				continue;
			}
			else
				continue;

			sf::Uint8 r = static_cast<sf::Uint8>(skeleton->color.r * slot->color.r * attachmentColor->r * 255);
			sf::Uint8 g = static_cast<sf::Uint8>(skeleton->color.g * slot->color.g * attachmentColor->g * 255);
			sf::Uint8 b = static_cast<sf::Uint8>(skeleton->color.b * slot->color.b * attachmentColor->b * 255);
			sf::Uint8 a = static_cast<sf::Uint8>(skeleton->color.a * slot->color.a * attachmentColor->a * 255);
			vertex.color.r = r;
			vertex.color.g = g;
			vertex.color.b = b;
			vertex.color.a = a;

			spColor light;
			light.r = r / 255.0f;
			light.g = g / 255.0f;
			light.b = b / 255.0f;
			light.a = a / 255.0f;

			usePremultipliedAlpha = r == 255 && g == 255 && b == 255 && a == 255;
			if (!usePremultipliedAlpha)
			{
				if (a > 96)
				{
					for (size_t ii = 0; ii < m_blendMultiplyList.size(); ++ii)
					{
						if (strcmp(slot->data->name, m_blendMultiplyList.at(ii).c_str()) == 0)
						{
							slot->data->blendMode = spBlendMode::SP_BLEND_MODE_MULTIPLY;
							break;
						}
					}
				}
			}
			if (m_bSelectivePma)
			{
				for (size_t ii = 0; ii < m_PmaSelectiveList.size(); ++ii)
				{
					if (strstr(slot->data->name, m_PmaSelectiveList.at(ii).c_str()))
					{
						usePremultipliedAlpha = false;
						break;
					}
				}
			}
			sf::BlendMode blend;
			switch (slot->data->blendMode)
			{
			case spBlendMode::SP_BLEND_MODE_ADDITIVE:
				blend = sf::BlendMode(usePremultipliedAlpha ? sf::BlendMode::One : sf::BlendMode::SrcAlpha, sf::BlendMode::One);
				break;
			case spBlendMode::SP_BLEND_MODE_MULTIPLY:
				blend = sf::BlendMode(sf::BlendMode::DstColor, sf::BlendMode::OneMinusSrcAlpha);
				break;
			case spBlendMode::SP_BLEND_MODE_SCREEN:
				blend = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcColor);
				break;
			default:
				blend = sf::BlendMode(usePremultipliedAlpha ? sf::BlendMode::One : sf::BlendMode::SrcAlpha, sf::BlendMode::OneMinusSrcAlpha);
				break;
			}

			if (states.texture == 0) states.texture = texture;

			if (states.blendMode != blend || states.texture != texture) {
				target.draw(*vertexArray, states);
				vertexArray->clear();
				states.blendMode = blend;
				states.texture = texture;
			}

			if (spSkeletonClipping_isClipping(clipper)) {
				spSkeletonClipping_clipTriangles(clipper, vertices, verticesCount << 1, indices, indicesCount, uvs, 2);
				vertices = clipper->clippedVertices->items;
				verticesCount = clipper->clippedVertices->size >> 1;
				uvs = clipper->clippedUVs->items;
				indices = clipper->clippedTriangles->items;
				indicesCount = clipper->clippedTriangles->size;
			}

			sf::Vector2u size = texture->getSize();

			if (vertexEffect != 0) {
				spFloatArray_clear(tempUvs);
				spColorArray_clear(tempColors);
				for (int ii = 0; ii < verticesCount; ii++) {
					spColor vertexColor = light;
					spColor dark;
					dark.r = dark.g = dark.b = dark.a = 0;
					int index = ii << 1;
					float x = vertices[index];
					float y = vertices[index + 1LL];
					float u = uvs[index];
					float v = uvs[index + 1LL];
					vertexEffect->transform(vertexEffect, &x, &y, &u, &v, &vertexColor, &dark);
					vertices[index] = x;
					vertices[index + 1LL] = y;
					spFloatArray_add(tempUvs, u);
					spFloatArray_add(tempUvs, v);
					spColorArray_add(tempColors, vertexColor);
				}

				for (int ii = 0; ii < indicesCount; ++ii) {
					int index = indices[ii] << 1;
					vertex.position.x = vertices[index];
					vertex.position.y = vertices[index + 1];
					vertex.texCoords.x = uvs[index] * size.x;
					vertex.texCoords.y = uvs[index + 1] * size.y;
					spColor vertexColor = tempColors->items[index >> 1];
					vertex.color.r = static_cast<sf::Uint8>(vertexColor.r * 255);
					vertex.color.g = static_cast<sf::Uint8>(vertexColor.g * 255);
					vertex.color.b = static_cast<sf::Uint8>(vertexColor.b * 255);
					vertex.color.a = static_cast<sf::Uint8>(vertexColor.a * 255);
					vertexArray->append(vertex);
				}
			}
			else {
				for (int ii = 0; ii < indicesCount; ++ii) {
					int index = indices[ii] << 1;
					vertex.position.x = vertices[index];
					vertex.position.y = vertices[index + 1];
					vertex.texCoords.x = uvs[index] * size.x;
					vertex.texCoords.y = uvs[index + 1] * size.y;
					vertexArray->append(vertex);
				}
			}
			spSkeletonClipping_clipEnd(clipper, slot);
		}
		target.draw(*vertexArray, states);
		spSkeletonClipping_clipEnd2(clipper);

		if (vertexEffect != 0) vertexEffect->end(vertexEffect);
	}

} /*namespace spine*/
