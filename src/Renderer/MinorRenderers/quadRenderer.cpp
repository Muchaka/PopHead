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

namespace ph::QuadRenderer {

struct RenderGroupsHashMap
{
	LinkedMemoryArena arena;
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

static Shader defaultQuadShader;
static const FloatRect* screenBounds; 
static const Shader* currentlyBoundQuadShader;
static Texture* whiteTexture;
static unsigned quadIBO;
static unsigned quadsDataVBO;
static unsigned quadsDataVAO;

#ifndef PH_DISTRIBUTION
static DebugNumbers* debugNumbers;
static bool debugNumbersEnabled = false;

void setDebugNumbersEnabled(bool enabled)
{
	debugNumbersEnabled = enabled;
}

static void pushBack(DebugArray* arr, unsigned e)
{
	if(arr->marker > 100) __debugbreak();
	arr->data[arr->marker++] = e;
}

DebugNumbers getDebugNumbers()
{
	return *debugNumbers;
}

void resetDebugNumbers()
{
	debugNumbers->renderGroupsSizes = {}; 
	debugNumbers->notAffectedByLightRenderGroupsSizes = {}; 
	debugNumbers->drawCalls = 0;
	debugNumbers->drawnSprites = 0;
	debugNumbers->drawnTextures = 0;
}
#endif

static unsigned bumpToNext4000(unsigned size)
{
	unsigned temp = size;
	while(temp > 4000)
		temp -= 4000;
	return size + 4000 - temp;	
}

static bool operator==(const RenderGroupKey& lhs, const RenderGroupKey& rhs)
{
	return lhs.shader == rhs.shader && lhs.z == rhs.z;
}

static QuadRenderGroup* insertIfDoesNotExitstAndGetRenderGroup(RenderGroupsHashMap* hashMap, RenderGroupKey key, unsigned quadDataCount)
{
	// return render group of matching key if exists
	for(unsigned i = 0; i < hashMap->size; ++i)
		if(key == *(hashMap->keys + i))
			return hashMap->renderGroups + i;

	#ifndef PH_DISTRIBUTION
	// bump debug numbers
	if(hashMap == &renderGroupsHashMap)
	{
		debugNumbers->renderGroups = hashMap->size;
		pushBack(&debugNumbers->renderGroupsZ, (unsigned)(key.z * 255));
	}
	else if(hashMap == &notAffectedByLightRenderGroupsHashMap)
	{
		debugNumbers->renderGroupsNotAffectedByLight = hashMap->size; 
		pushBack(&debugNumbers->notAffectedByLightRenderGroupsZ, (unsigned)(key.z * 255));
	}
	#endif

	if(hashMap->capacity == hashMap->size)
	{
		hashMap->capacity *= 2;
		hashMap->indices = (unsigned*)rePushArray(&hashMap->arena, hashMap->indices, hashMap->capacity, unsigned);
		hashMap->keys = (RenderGroupKey*)rePushArray(&hashMap->arena, hashMap->keys, hashMap->capacity, RenderGroupKey);
		hashMap->renderGroups = (QuadRenderGroup*)rePushArray(&hashMap->arena, hashMap->renderGroups, hashMap->capacity, QuadRenderGroup);
	}

	// insert data to hash map and render group
	hashMap->indices[hashMap->size] = hashMap->size;

	hashMap->keys[hashMap->size] = key;

	quadDataCount = bumpToNext4000(quadDataCount);
	size_t arenaSize = sizeof(QuadData) * quadDataCount; 
	QuadRenderGroup& qrg = hashMap->renderGroups[hashMap->size];
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

static void insertQuadDataToQuadRenderGroup(QuadData* quadData, unsigned count, QuadRenderGroup* quadRenderGroup, RenderGroupsHashMap* hashMap)
{
	bool thereIsNoPlaceForNewQuadData = quadRenderGroup->quadsDataSize + count > quadRenderGroup->quadsDataCapacity;
	if(thereIsNoPlaceForNewQuadData)
	{
		// reallocate quads data 
		unsigned newQuadsDataCapacity = quadRenderGroup->quadsDataCapacity + count * 2; 
		quadRenderGroup->quadsDataCapacity = newQuadsDataCapacity;
		rePushArray(&hashMap->arena, quadRenderGroup->quadsData, newQuadsDataCapacity, QuadData);
	}
	void* writePtr = quadRenderGroup->quadsData + (quadRenderGroup->quadsDataSize);
	memcpy(writePtr, quadData, sizeof(QuadData) * count);
	quadRenderGroup->quadsDataSize += count;
}

void init()
{
	static bool shouldInitializeRenderGroups = true;
	if(shouldInitializeRenderGroups)
	{
		shouldInitializeRenderGroups = false;

		auto initRenderGroupsHashMap = [](RenderGroupsHashMap& hashMap, size_t subArenaSize)
		{
			constexpr size_t initGroupsCapacity = 150;
			hashMap.arena = subLinkedArena(&quadRendererArena, subArenaSize); 
			hashMap.indices = (unsigned*)pushArray(&hashMap.arena, initGroupsCapacity, unsigned);
			hashMap.keys = (RenderGroupKey*)pushArray(&hashMap.arena, initGroupsCapacity, RenderGroupKey); 
			hashMap.renderGroups = (QuadRenderGroup*)pushArray(&hashMap.arena, initGroupsCapacity, QuadRenderGroup); 
			hashMap.capacity = initGroupsCapacity;
			hashMap.size = 0;
		};
		allocateAndInitArena(&quadRendererArena, Megabytes(512));	
		initRenderGroupsHashMap(renderGroupsHashMap, Megabytes(512 / 4 * 3));
		initRenderGroupsHashMap(notAffectedByLightRenderGroupsHashMap, Megabytes(512 / 4));

		#ifndef PH_DISTRIBUTION
		debugNumbers = (DebugNumbers*)malloc(sizeof(DebugNumbers));
		memset(debugNumbers, 0, sizeof(DebugNumbers));
		#endif
	}

	defaultQuadShader.init(shader::quadSrc());
	defaultQuadShader.initUniformBlock("SharedData", 0);

	GLCheck( glGenVertexArrays(1, &quadsDataVAO) );
	GLCheck( glBindVertexArray(quadsDataVAO) );

	unsigned quadIndices[] = {0, 1, 3, 1, 2, 3};
	GLCheck( glGenBuffers(1, &quadIBO) );
	GLCheck( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO) );
	GLCheck( glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW) ); 

	GLCheck( glGenBuffers(1, &quadsDataVBO) );
	GLCheck( glBindBuffer(GL_ARRAY_BUFFER, quadsDataVBO) );

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

	whiteTexture = new Texture;
	unsigned white = 0xffffffff;
	whiteTexture->setData(&white, sizeof(unsigned), sf::Vector2i(1, 1));
}

void shutDown()
{
	delete whiteTexture;
	defaultQuadShader.remove();
	GLCheck( glDeleteBuffers(1, &quadIBO) );
	GLCheck( glDeleteBuffers(1, &quadsDataVBO) );
	GLCheck( glDeleteVertexArrays(1, &quadsDataVAO) );
}

void submitBunchOfQuadsWithTheSameTexture(std::vector<QuadData>& quadsData, Texture* texture,
                                          const Shader* shader, float z, ProjectionType projectionType)
{
	if(!shader)
		shader = &defaultQuadShader;

	QuadRenderGroup* renderGroup = 
		insertIfDoesNotExitstAndGetRenderGroup(&renderGroupsHashMap, {shader, z, projectionType}, (unsigned)quadsData.size());


	if(!texture)
		texture = whiteTexture;
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

	insertQuadDataToQuadRenderGroup(quadsData.data(), (unsigned)quadsData.size(), renderGroup, &renderGroupsHashMap);
}

void submitQuad(Texture* texture, const IntRect* textureRect, const sf::Color* color, const Shader* shader,
                sf::Vector2f position, sf::Vector2f size, float z, float rotation, sf::Vector2f rotationOrigin,
                ProjectionType projectionType, bool isAffectedByLight)
{
	// culling
	FloatRect bounds = projectionType == ProjectionType::gameWorld ? *screenBounds : FloatRect(0.f, 0.f, 1920.f, 1080.f);
	if(rotation == 0.f)
		if(!bounds.doPositiveRectsIntersect(FloatRect(position.x, position.y, size.x, size.y)))
			return;
	else if(!bounds.doPositiveRectsIntersect(FloatRect(position.x - size.x * 2, position.y - size.y * 2, size.x * 4, size.y * 4)))
		return;

	// if shader is not specified use default shader 
	if(!shader)
		shader = &defaultQuadShader;

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
		texture = whiteTexture;
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

	insertQuadDataToQuadRenderGroup(&quadData, 1, renderGroup, &hashMap);
}

void flush(bool affectedByLight)
{
	PH_PROFILE_FUNCTION(0);

	#ifndef PH_DISTRIBUTION
	debugNumbers->arenaUsedMemory = unsigned(renderGroupsHashMap.arena.used / 1024 +
	                                        notAffectedByLightRenderGroupsHashMap.arena.used / 1024); 
	#endif

	currentlyBoundQuadShader = nullptr;
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
		
		#ifndef PH_DISTRIBUTION
		debugNumbers->renderGroupsIndices.marker = 0;
		auto& debugIndices = affectedByLight ? debugNumbers->renderGroupsIndices : debugNumbers->notAffectedByLightRenderGroupsIndices;
		for(unsigned i = 0; i < hashMap.size; ++i)
		{
			pushBack(&debugNumbers->renderGroupsIndices, hashMap.indices[i]);
		}
		#endif
	}

	for(unsigned i = 0; i < hashMap.size; ++i)
	{
		unsigned renderGroupIndex = hashMap.indices[i];
		auto& key = hashMap.keys[renderGroupIndex];
		auto& rg = hashMap.renderGroups[renderGroupIndex];

		#ifndef PH_DISTRIBUTION
		// update debug info
		if(debugNumbersEnabled)
		{
			debugNumbers->drawnSprites += rg.quadsDataSize;
			debugNumbers->drawnTextures += rg.texturesSize;
			if(affectedByLight)
				pushBack(&debugNumbers->renderGroupsSizes, rg.quadsDataSize);
			else
				pushBack(&debugNumbers->notAffectedByLightRenderGroupsSizes, rg.quadsDataSize);
		}
		#endif

		// set up shader
		if(key.shader != currentlyBoundQuadShader) 
		{
			key.shader->bind();
			currentlyBoundQuadShader = key.shader;

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

		auto drawCall = [quadsData](size_t nrOfInstances)
		{
			char log[50];
			sprintf(log, "draw call instances: %zu", nrOfInstances);
			PH_PROFILE_SCOPE(log, 0);

			GLCheck( glBindBuffer(GL_ARRAY_BUFFER, quadsDataVBO) );
			GLCheck( glBufferData(GL_ARRAY_BUFFER, nrOfInstances * sizeof(QuadData), quadsData, GL_STATIC_DRAW) );

			GLCheck( glBindVertexArray(quadsDataVAO) );
			GLCheck( glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (GLsizei)nrOfInstances) );

			#ifndef PH_DISTRIBUTION
			++debugNumbers->drawCalls;
			#endif
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

void setScreenBoundsPtr(const FloatRect* bounds) 
{ 
	screenBounds = bounds; 
}

}
