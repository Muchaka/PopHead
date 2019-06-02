#include "musicPlayer.hpp"

using PopHead::Audio::MusicPlayer;

MusicPlayer::MusicPlayer()
	:mVolume(70.f)
	,mMute(true)
{
}

MusicPlayer::~MusicPlayer()
{
	mMusic.stop();
}

void MusicPlayer::play(std::string filePath)
{
	MusicData currentThemeData = musicDataHolder.getMusicData(filePath);
	mMusic.openFromFile(filePath);
	mMusic.setVolume(mVolume * currentThemeData.volumeMultiplier);
	mMusic.setLoop(currentThemeData.loop);
	setMute(mMute);
	mMusic.play();
}

void MusicPlayer::stop()
{
	mMusic.stop();
}

void MusicPlayer::setPaused(bool pause)
{
	pause ? mMusic.pause() : mMusic.play();
}

void MusicPlayer::setMute(bool mute)
{
	mMute = mute;
	mute ? mMusic.setVolume(0.f) : setVolume(mVolume);
}

void MusicPlayer::setVolume(float volume)
{
	mVolume = volume;
	auto themeData = musicDataHolder.getCurrentThemeData();
	float volumeMultiplier = themeData.volumeMultiplier;
	mMusic.setVolume(volume * volumeMultiplier);
}
