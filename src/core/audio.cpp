/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#include "audio.h"
#include "settings.h"
#include <QString>
#include <QCache>
#include <QMediaPlayer>
#include <QMediaPlaylist>
QMediaPlayer *Audio::BGMPlayer = nullptr;
QCache<QString, QMediaPlayer> *Audio::SoundCache = nullptr;

void Audio::init()
{
    SoundCache = new QCache<QString,QMediaPlayer>(20);
}

void Audio::quit()
{
    if(BGMPlayer)
        delete BGMPlayer;
    if(SoundCache)
    {
        SoundCache->clear();
        delete SoundCache;
    }
}

void Audio::play(const QString &filename, const bool doubleVolume)
{
    QMediaPlayer *sound = SoundCache->object(filename);
    if (sound == nullptr)
    {
        sound = new QMediaPlayer;
        sound->setMedia(QUrl(filename));
        SoundCache->insert(filename, sound);
    }
    else if (sound->state() == QMediaPlayer::PlayingState)
    {
        return;
    }
    sound->setVolume((doubleVolume ? 2 : 1) * Config.EffectVolume);
    sound->play();
}

void Audio::playAudioOfMoxuan()
{
    play("audio/system/moxuan.ogg", true);
}

void Audio::stop()
{
    if (!SoundCache)
        return;
    SoundCache->clear();
    stopBGM();
}

void Audio::playBGM(const QString &filename)
{
    if(!BGMPlayer)
        BGMPlayer = new QMediaPlayer;
    QMediaPlaylist *BGMList = new QMediaPlaylist(BGMPlayer);
    BGMList->addMedia(QUrl(filename));
    BGMList->setPlaybackMode(QMediaPlaylist::Loop);

    BGMPlayer->setPlaylist(BGMList);
    BGMPlayer->setVolume(Config.BGMVolume);
    BGMPlayer->play();
}

void Audio::setBGMVolume(int volume)
{
    if(BGMPlayer)
        BGMPlayer->setVolume(volume);
}

void Audio::stopBGM()
{
    if(BGMPlayer)
        BGMPlayer->stop();
}

