

#include "spine_loader_c.h"

std::shared_ptr<spAtlas> spine_loader_c::CreateAtlasFromFile(const char* filePath, void* rendererObject)
{
	auto atlas = std::shared_ptr<spAtlas>
		(
			spAtlas_createFromFile(filePath, rendererObject),
			[](spAtlas *pAtlas)
			{
				if(pAtlas != nullptr)spAtlas_dispose(pAtlas);
			}
		);
	return atlas;
}

std::shared_ptr<spAtlas> spine_loader_c::CreateAtlasFromMemory(const char* atlasData, int atlasLength, const char* fileDirectory, void* rendererObject)
{
	auto atlas = std::shared_ptr<spAtlas>
		(
			spAtlas_create(atlasData, atlasLength, fileDirectory, rendererObject),
			[](spAtlas* pAtlas)
			{
				if (pAtlas != nullptr)spAtlas_dispose(pAtlas);
			}
		);
	return atlas;
}

std::shared_ptr<spSkeletonData> spine_loader_c::ReadTextSkeletonFromFile(const char* filePath, spAtlas* atlas, float scale)
{
	spSkeletonJson* json = spSkeletonJson_create(atlas);
	json->scale = scale;
	auto skeletonData = std::shared_ptr<spSkeletonData>
		(
			spSkeletonJson_readSkeletonDataFile(json, filePath),
			[](spSkeletonData* pSkeletonData)
			{
				if(pSkeletonData != nullptr)spSkeletonData_dispose(pSkeletonData);
			}
		);
	spSkeletonJson_dispose(json);
    return skeletonData;
}

std::shared_ptr<spSkeletonData> spine_loader_c::ReadBinarySkeletonFromFile(const char* filePath, spAtlas* atlas, float scale)
{
	spSkeletonBinary* binary = spSkeletonBinary_create(atlas);
	binary->scale = scale;
	auto skeletonData = std::shared_ptr<spSkeletonData>
		(
			spSkeletonBinary_readSkeletonDataFile(binary, filePath),
			[](spSkeletonData* pSkeletonData)
			{
				if (pSkeletonData != nullptr)spSkeletonData_dispose(pSkeletonData);
			}
		);
	spSkeletonBinary_dispose(binary);
	return skeletonData;
}

std::shared_ptr<spSkeletonData> spine_loader_c::ReadTextSkeletonFromMemory(const char* skeletonJson, spAtlas* atlas, float scale)
{
	spSkeletonJson* json = spSkeletonJson_create(atlas);
	json->scale = scale;
	auto skeletonData = std::shared_ptr<spSkeletonData>
		(
			spSkeletonJson_readSkeletonData(json, skeletonJson),
			[](spSkeletonData* pSkeletonData)
			{
				if (pSkeletonData != nullptr)spSkeletonData_dispose(pSkeletonData);
			}
		);
	spSkeletonJson_dispose(json);
	return skeletonData;
}

std::shared_ptr<spSkeletonData> spine_loader_c::ReadBinarySkeletonFromMemory(const unsigned char* skeletonBinary, int skeletonLength, spAtlas* atlas, float scale)
{
	spSkeletonBinary* binary = spSkeletonBinary_create(atlas);
	binary->scale = scale;
	auto skeletonData = std::shared_ptr<spSkeletonData>
		(
			spSkeletonBinary_readSkeletonData(binary, skeletonBinary, skeletonLength),
			[](spSkeletonData* pSkeletonData)
			{
				if (pSkeletonData != nullptr)spSkeletonData_dispose(pSkeletonData);
			}
		);
	spSkeletonBinary_dispose(binary);
	return skeletonData;
}
