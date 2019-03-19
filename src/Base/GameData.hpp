#ifndef POPHEAD_BASE_GAMEDATA_H_
#define POPHEAD_BASE_GAMEDATA_H_

#include <memory>

#include "Audio/Music/MusicPlayer.hpp"
#include "Audio/Sound/SoundPlayer.hpp"
#include "Renderer/Renderer.hpp"
#include "States/StateMachine.hpp"
#include "Input/Input.hpp"
#include "Resources/ResourceHolder.hpp"

namespace PopHead {
namespace Base {

/// GameData is holder for observer pointers to Game Modules.
/** GameData is recivied by Game itself. */
class GameData
{
  public:
    inline GameData(
            Audio::SoundPlayer* const = nullptr,
            Audio::MusicPlayer* const = nullptr,
            Resources::TextureHolder* const = nullptr,
            Resources::FontHolder* const = nullptr,
            Resources::ShaderHolder* const = nullptr,
            States::StateMachie* const = nullptr,
            Input::Input* const = nullptr,
            Renderer::Renderer* const = nullptr );

    inline auto getSoundPlayer() const -> Audio::SoundPlayer&;
    inline auto getMusicPlayer() const -> Audio::MusicPlayer&;
    inline auto getTextures() const -> Resources::TextureHolder&;
    inline auto getFonts() const -> Resources::FontHolder&;
    inline auto getShaders() const -> Resources::ShaderHolder&;
    inline auto getStateMachie() const -> States::StateMachie&;
    inline auto getInput() const -> Input::Input&;
    inline auto getRenderer() const -> Renderer::Renderer&;

  private:
    Audio::SoundPlayer* const mSoundPlayer;
    Audio::MusicPlayer* const mMusicPlayer;
    Resources::TextureHolder* const mTextures;
    Resources::FontHolder* const mFonts;
    Resources::ShaderHolder* const mShaders;
    States::StateMachie* const mStateMachine;
    Input::Input* const mInput;
    Renderer::Renderer* const mRenderer;
};

inline GameData::GameData(
        Audio::SoundPlayer* const soundPlayer,
        Audio::MusicPlayer* const musicPlayer,
        Resources::TextureHolder* const textures,
        Resources::FontHolder* const fonts,
        Resources::ShaderHolder* const shaders,
        States::StateMachie* const stateMachine,
        Input::Input* const input,
        Renderer::Renderer* const renderer )
        : mSoundPlayer { soundPlayer }
        , mMusicPlayer { musicPlayer }
        , mTextures { textures }
        , mFonts { fonts }
        , mShaders { shaders }
        , mStateMachine { stateMachine }
        , mInput { input }
        , mRenderer { renderer }
        {
        }

inline auto GameData::getSoundPlayer() const -> Audio::SoundPlayer&
{ return *mSoundPlayer; }

inline auto GameData::getMusicPlayer() const -> Audio::MusicPlayer&
{ return *mMusicPlayer; }

inline auto GameData::getTextures() const -> Resources::TextureHolder&
{ return *mTextures; }

inline auto GameData::getFonts() const -> Resources::FontHolder&
{ return *mFonts; }

inline auto GameData::getShaders() const -> Resources::ShaderHolder&
{ return *mShaders; }

inline auto GameData::getStateMachie() const -> States::StateMachie&
{ return *mStateMachine; }

inline auto GameData::getInput() const -> Input::Input&
{ return *mInput; }

inline auto GameData::getRenderer() const -> Renderer::Renderer&
{ return *mRenderer; }

}}

#endif // !POPHEAD_BASE_GAMEDATA_H_
