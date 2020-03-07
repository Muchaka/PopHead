#include "quadRenderer.hpp"
#include "Renderer/API/texture.hpp"
#include "Renderer/API/openglErrors.hpp"
#include "Renderer/Shaders/embeddedShaders.hpp"
#include "Utilities/cast.hpp"
#include "Utilities/profiling.hpp"
#include "Utilities/math.hpp"
#include "Utilities/memory.hpp"
#include "Logs/logs.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cstdlib>

namespace ph {

struct RenderGroupsHashMap
{
	BumpMemoryArena arena;
	unsigned* indices = nullptr;
	RenderGroupKey* keys = nullptr;
	QuadRenderGroup* renderGroups = nullptr;
	unsigned capacity;
	unsigned size;
	bool needsToBeSorted = false;
};

static BumpMemoryArena quadRendererArena;
static RenderGroupsHashMap renderGroupsHashMap;
static RenderGroupsHashMap notAffectedByLightRenderGroupsHashMap;
static QuadRendererDebugNumbers debugNumbers;

QuadRendererDebugNumbers getQuadRendererDebugNumbers()
{
	return debugNumbers;
}

void resetQuadRendererDebugNumbers()
{
	debugNumbers.renderGroupsSizes = {}; 
	debugNumbers.notAffectedByLightRenderGroupsSizes = {}; 
	debugNumbers.drawCalls = 0;
	debugNumbers.drawnSprites = 0;
	debugNumbers.drawnTextures = 0;
}

static unsigned bumpToNext4000(unsigned size)
{
	unsigned temp = size;
	while(temp > 4000)
		temp -= 4000;
	return size + 4000 - temp;	
}

bool operator==(const RenderGroupKey& lhs, const RenderGroupKey& rhs)
{
	return lhs.shader == rhs.shader && lhs.z == rhs.z;
}

static QuadRenderGroup* insertIfDoesNotExitstAndGetRenderGroup(RenderGroupsHashMap* hashMap, RenderGroupKey key, unsigned quadDataCount)
{
	// return render group of matching key if exists
	for(unsigned i = 0; i < hashMap->size; ++i)
		if(key == *(hashMap->keys + i))
			return hashMap->renderGroups + i;

	// bump debug numbers
	if(hashMap == &renderGroupsHashMap)
	{
		debugNumbers.renderGroups = hashMap->size;
		// TODO: Clean this up
		debugNumbers.renderGroupsZ.data[debugNumbers.renderGroupsZ.marker++] = (unsigned)(key.z * 255);
	}
	else if(hashMap == &notAffectedByLightRenderGroupsHashMap)
	{
		debugNumbers.renderGroupsNotAffectedByLight = hashMap->size; 
		// TODO: Clean this up
		debugNumbers.notAffectedByLightRenderGroupsZ.data[debugNumbers.notAffectedByLightRenderGroupsZ.marker++] = (unsigned)(key.z * 255);
	}

	// TODO: reallocate if there is no more space
	/*
	if(hashMap->capacity == hashMap->size)
	{
		hashMap->capacity *= 2;

		void* newIndices = realloc(hashMap->indices, hashMap->capacity * sizeof(unsigned));
		PH_ASSERT_CRITICAL(newIndices, "quad render group hash map indices realloc failed!");
		hashMap->indices = (unsigned*)newIndices;

		void* newKeys = realloc(hashMap->keys, hashMap->capacity * sizeof(RenderGroupKey));
		PH_ASSERT_CRITICAL(newKeys, "quad render group hash map keys realloc failed!");
		hashMap->keys = (RenderGroupKey*)newKeys;

		void* newRenderGroups = realloc(hashMap->renderGroups, hashMap->capacity * sizeof(QuadRenderGroup));
		PH_ASSERT_CRITICAL(newRenderGroups, "quad render group hash map render groups realloc failed!");
		hashMap->renderGroups = (QuadRenderGroup*)newRenderGroups;
	}*/

	// insert data to hash map and render group
	hashMap->indices[hashMap->size] = hashMap->size;

	hashMap->keys[hashMap->size] = key;

	quadDataCount = bumpToNext4000(quadDataCount);
	size_t arenaSize = sizeof(QuadData) * quadDataCount; 
	QuadRenderGroup& qrg = hashMap->renderGroups[hashMap->size];
	qrg.quadsDataArenaSize = (unsigned)arenaSize; 
	qrg.texturesSize = 0;
	qrg.texturesCapacity = 32;
	qrg.textures = (unsigned*)pushArray(&hashMap->arena, 32, unsigned);
	qrg.quadsDataSize = 0; 
	qrg.quadsDataCapacity = quadDataCount; 
	qrg.quadsData = (QuadData*)pushArray(&hashMap->arena, quadDataCount, QuadData);  

	++hashMap->size;

	hashMap->needsToBeSorted = true;

	return &qrg; 
}

static auto getTextureSlotToWhichThisTextureIsBound(const Texture* texture, QuadRenderGroup* rg) -> std::optional<float>
{
	unsigned* ptr = rg->textures;
	for(size_t i = 0; i < rg->texturesSize; ++i, ++ptr)
		if(*ptr == texture->getID())
			return static_cast<float>(i);
	return std::nullopt;
}

static void insertQuadDataToQuadRenderGroup(QuadData* quadData, unsigned count, QuadRenderGroup* quadRenderGroup)
{
	// TODO: reallocate quad render group arena
	/*
	bool thereIsNoPlaceForNewQuadData = quadRenderGroup->quadsDataSize + count > quadRenderGroup->quadsDataCapacity;
	if(thereIsNoPlaceForNewQuadData)
	{
		size_t oldArenaSize = (size_t)quadRenderGroup->quadsDataArenaSize; 
		void* oldArena = (void*)quadRenderGroup->quadsData; 
		unsigned newQuadsDataCapacity = quadRenderGroup->quadsDataCapacity + count * 2; 
		size_t newArenaSize = newQuadsDataCapacity * sizeof(QuadData); 
		quadRenderGroup->quadsDataArenaSize = newArenaSize;
		quadRenderGroup->quadsDataCapacity = newQuadsDataCapacity; 
		quadRenderGroup->quadsData = (QuadData*)allocateArena(newArenaSize);
		memcpy(quadRenderGroup->quadsData, oldArena, oldArenaSize);
		deallocateArena(oldArena);
	}
	*/
	void* writePtr = quadRenderGroup->quadsData + (quadRenderGroup->quadsDataSize);
	memcpy(writePtr, quadData, sizeof(QuadData) * count);
	quadRenderGroup->quadsDataSize += count;
}

void QuadRenderer::init()
{
	static bool shouldInitializeRenderGroups = true;
	if(shouldInitializeRenderGroups)
	{
		shouldInitializeRenderGroups = false;

		auto initRenderGroupsHashMap = [](RenderGroupsHashMap& hashMap, size_t subArenaSize)
		{
			constexpr size_t initGroupsCapacity = 150;
			hashMap.arena = subArena(&quadRendererArena, subArenaSize); 
			hashMap.indices = (unsigned*)pushArray(&hashMap.arena, initGroupsCapacity, unsigned);
			hashMap.keys = (RenderGroupKey*)pushArray(&hashMap.arena, initGroupsCapacity, RenderGroupKey); 
			hashMap.renderGroups = (QuadRenderGroup*)pushArray(&hashMap.arena, initGroupsCapacity, QuadRenderGroup); 
			hashMap.capacity = initGroupsCapacity;
			hashMap.size = 0;
		};
		allocateAndInitArena(&quadRendererArena, Megabytes(512));	
		initRenderGroupsHashMap(renderGroupsHashMap, Megabytes(512 / 4 * 3));
		initRenderGroupsHashMap(notAffectedByLightRenderGroupsHashMap, Megabytes(512 / 4));
	}

	mDefaultQuadShader.init(shader::quadSrc());
	mDefaultQuadShader.initUniformBlock("SharedData", 0);

	GLCheck( glGenVertexArrays(1, &mVAO) );
	GLCheck( glBindVertexArray(mVAO) );

	unsigned quadIndices[] = {0, 1, 3, 1, 2, 3};
	GLCheck( glGenBuffers(1, &mQuadIBO) );
	GLCheck( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadIBO) );
	GLCheck( glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW) ); 

	GLCheck( glGenBuffers(1, &mQuadsDataVBO) );
	GLCheck( glBindBuffer(GL_ARRAY_BUFFER, mQuadsDataVBO) );

	GLCheck( glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(QuadData), (void*) offsetof(QuadData, color)) );
	GLCheck( glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QuadData), (void*) offsetof(QuadData, textureRect)) );
	GLCheck( glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QuadData), (void*) offsetof(QuadData, position)) );
	GLCheck( glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(QuadData), (void*) offsetof(QuadData, size)) );
	GLCheck( glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(QuadData), (void*) offsetof(QuadData, rotationOrigin)) );
	GLCheck( glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(QuadData), (void*) offsetof(QuadData, rotation)) );
	GLCheck( glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(QuadData), (void*) offsetof(QuadData, textureSlotRef)) );

	for(int i = 0; i < 7; ++i) {
		GLCheck( glEnableVertexAttribArray(i) );
	}
	for(int i = 0; i < 7; ++i) {
		GLCheck( glVertexAttribDivisor(i, 1) );
	}

	mWhiteTexture = new Texture;
	unsigned white = 0xffffffff;
	mWhiteTexture->setData(&white, sizeof(unsigned), sf::Vector2i(1, 1));
}

void QuadRenderer::shutDown()
{
	delete mWhiteTexture;
	mDefaultQuadShader.remove();
	GLCheck( glDeleteBuffers(1, &mQuadIBO) );
	GLCheck( glDeleteBuffers(1, &mQuadsDataVBO) );
	GLCheck( glDeleteVertexArrays(1, &mVAO) );
}

void QuadRenderer::submitBunchOfQuadsWithTheSameTexture(std::vector<QuadData>& quadsData, Texture* texture,
                                                        const Shader* shader, float z, ProjectionType projectionType)
{
	if(!shader)
		shader = &mDefaultQuadShader;

	QuadRenderGroup* renderGroup = 
		insertIfDoesNotExitstAndGetRenderGroup(&renderGroupsHashMap, {shader, z, projectionType}, (unsigned)quadsData.size());


	if(!texture)
		texture = mWhiteTexture;
	auto textureSlotOfThisTexture = getTextureSlotToWhichThisTextureIsBound(texture, renderGroup);
	if(textureSlotOfThisTexture) 
	{
		for(auto& quad : quadsData)
			quad.textureSlotRef = *textureSlotOfThisTexture;
	}
	else 
	{
		for(auto& quad : quadsData)
			quad.textureSlotRef = (float)renderGroup->texturesSize;
		renderGroup->textures[renderGroup->texturesSize] = texture->getID();
		++renderGroup->texturesSize;
	}

	insertQuadDataToQuadRenderGroup(quadsData.data(), (unsigned)quadsData.size(), renderGroup);
}

void QuadRenderer::submitQuad(Texture* texture, const IntRect* textureRect, const sf::Color* color, const Shader* shader,
                              sf::Vector2f position, sf::Vector2f size, float z, float rotation, sf::Vector2f rotationOrigin,
                              ProjectionType projectionType, bool isAffectedByLight)
{
	// culling
	FloatRect bounds = projectionType == ProjectionType::gameWorld ? *mScreenBounds : FloatRect(0.f, 0.f, 1920.f, 1080.f);
	if(rotation == 0.f)
		if(!bounds.doPositiveRectsIntersect(FloatRect(position.x, position.y, size.x, size.y)))
			return;
	else if(!bounds.doPositiveRectsIntersect(FloatRect(position.x - size.x * 2, position.y - size.y * 2, size.x * 4, size.y * 4)))
		return;

	// if shader is not specified use default shader 
	if(!shader)
		shader = &mDefaultQuadShader;

	// find or add draw call group
	auto& hashMap = isAffectedByLight ? renderGroupsHashMap : notAffectedByLightRenderGroupsHashMap;
	QuadRenderGroup* renderGroup = insertIfDoesNotExitstAndGetRenderGroup(&hashMap, {shader, z, projectionType}, 1);

	// compute final texture rect
	FloatRect finalTextureRect;
	if(textureRect)
	{
		auto ts = static_cast<sf::Vector2f>(texture->getSize());
		finalTextureRect = FloatRect(
			textureRect->left / ts.x, (ts.y - textureRect->top - textureRect->height) / ts.y,
			textureRect->width / ts.x, textureRect->height / ts.y
		);
	}
	else
	{
		finalTextureRect = FloatRect(0.f, 0.f, 1.f, 1.f);	
	}

	// submit data
	QuadData quadData;
	
	quadData.color = color ? Cast::toNormalizedColorVector4f(*color) : Cast::toNormalizedColorVector4f(sf::Color::White);
	quadData.textureRect = finalTextureRect; 
	quadData.position = position;
	quadData.size = size;
	quadData.rotationOrigin = rotationOrigin;
	quadData.rotation = Math::degreesToRadians(rotation);
	
	if(!texture)
		texture = mWhiteTexture;
	if(auto textureSlotOfThisTexture = getTextureSlotToWhichThisTextureIsBound(texture, renderGroup))
	{
		quadData.textureSlotRef = *textureSlotOfThisTexture;
	}
	else 
	{
		unsigned textureSlotID = renderGroup->texturesSize;
		quadData.textureSlotRef = (float)textureSlotID;
		*(renderGroup->textures + textureSlotID) = texture->getID();
		++renderGroup->texturesSize;
	}

	insertQuadDataToQuadRenderGroup(&quadData, 1, renderGroup);
}

void QuadRenderer::flush(bool affectedByLight)
{
	PH_PROFILE_FUNCTION(0);

	debugNumbers.arenaUsedMemory = renderGroupsHashMap.arena.used / 1024 +
	                               notAffectedByLightRenderGroupsHashMap.arena.used / 1024; 

	mCurrentlyBoundQuadShader = nullptr;
	auto& hashMap = affectedByLight ? renderGroupsHashMap : notAffectedByLightRenderGroupsHashMap;

	// sort hash map indices
	if(hashMap.needsToBeSorted)
	{
		PH_PROFILE_SCOPE("sorting indices", 0);
		hashMap.needsToBeSorted = false;
		for(unsigned i = 0; i < hashMap.size - 1; ++i)
		{
			for(unsigned j = 0; j < hashMap.size - 1; ++j)
			{
				unsigned index1 = hashMap.indices[j];
				unsigned index2 = hashMap.indices[j + 1];
				if(hashMap.keys[index1].z < hashMap.keys[index2].z)
				{
					unsigned temp = hashMap.indices[j];
					hashMap.indices[j] = hashMap.indices[j + 1];
					hashMap.indices[j + 1] = temp; 
				}
			}
		}
		
		debugNumbers.renderGroupsIndices.marker = 0;
		auto& debugIndices = affectedByLight ? debugNumbers.renderGroupsIndices : debugNumbers.notAffectedByLightRenderGroupsIndices;
		for(unsigned i = 0; i < hashMap.size; ++i)
		{
			debugNumbers.renderGroupsIndices.data[i] = hashMap.indices[i];
			++debugNumbers.renderGroupsIndices.marker;
		}
	}

	for(unsigned i = 0; i < hashMap.size; ++i)
	{
		unsigned renderGroupIndex = hashMap.indices[i];
		auto& key = hashMap.keys[renderGroupIndex];
		auto& rg = hashMap.renderGroups[renderGroupIndex];

		// update debug info
		if(mIsDebugCountingActive) {
			debugNumbers.drawnSprites += rg.quadsDataSize;
			debugNumbers.drawnTextures += rg.texturesSize;
			if(affectedByLight)
				debugNumbers.renderGroupsSizes.data[debugNumbers.renderGroupsSizes.marker++] = rg.quadsDataSize;
			else
				debugNumbers.notAffectedByLightRenderGroupsSizes.data[debugNumbers.notAffectedByLightRenderGroupsSizes.marker++] = rg.quadsDataSize;
		}

		// set up shader
		if(key.shader != mCurrentlyBoundQuadShader) 
		{
			key.shader->bind();
			mCurrentlyBoundQuadShader = key.shader;

			int textures[32];
			for(int i = 0; i < 32; ++i)
				textures[i] = i;
			key.shader->setUniformIntArray("textures", 32, textures);
		}
		key.shader->setUniformFloat("z", key.z);
		key.shader->setUniformBool("isGameWorldProjection", key.projectionType == ProjectionType::gameWorld);

		// TODO: sort quads by texture slot ref if there are more then 32 
		// std::sort(rg.quadsData.begin(), rg.quadsData.end(), [](const QuadData& a, const QuadData& b) { return a.textureSlotRef < b.textureSlotRef; });

		// draw render group
		unsigned quadsDataSize = rg.quadsDataSize;
		unsigned texturesSize = rg.texturesSize;
		QuadData* quadsData = rg.quadsData;
		unsigned* textures = rg.textures;

		auto bindTexturesForNextDrawCall = [textures, texturesSize]
		{
			for(unsigned textureSlot = 0; textureSlot < (texturesSize > 32 ? 32 : texturesSize); ++textureSlot)
			{
				glActiveTexture(GL_TEXTURE0 + textureSlot);
				glBindTexture(GL_TEXTURE_2D, *(textures + textureSlot));
			}
		};

		auto drawCall = [this, quadsData](size_t nrOfInstances)
		{
			char log[50];
			sprintf(log, "draw call instances: %zu", nrOfInstances);
			PH_PROFILE_SCOPE(log, 0);

			GLCheck( glBindBuffer(GL_ARRAY_BUFFER, mQuadsDataVBO) );
			GLCheck( glBufferData(GL_ARRAY_BUFFER, nrOfInstances * sizeof(QuadData), quadsData, GL_STATIC_DRAW) );

			GLCheck( glBindVertexArray(mVAO) );
			GLCheck( glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (GLsizei)nrOfInstances) );

			if(mIsDebugCountingActive)
				++debugNumbers.drawCalls;
		};

		for(size_t i = 0; i < rg.quadsDataSize; ++i)
		{
			if(i == rg.quadsDataSize - 1)
			{
				bindTexturesForNextDrawCall();
				drawCall(i + 1);
				break;
			}
			else if(rg.quadsData[i + 1].textureSlotRef == 32)
			{
				bindTexturesForNextDrawCall();
				drawCall(i + 1);

				quadsData += i;

				QuadData* ptr = quadsData;
				for(unsigned i = 0; i < quadsDataSize; ++i)
				{
					ptr->textureSlotRef -= 32;
					++ptr;
				}

				if(texturesSize > 32)
				{
					textures += 32;
					texturesSize -= 32;
				}
				else
				{
					texturesSize = 0;
				}

				i = 0;
			}
		}

		rg.quadsDataSize = 0;
		rg.texturesSize = 0;
	}
}


}
