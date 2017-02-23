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

#ifndef _AUDIO_H
#define _AUDIO_H

#ifdef AUDIO_SUPPORT

class QString;
class QMediaPlayer;
template<typename T, typename K>
class QCache;
class Audio
{
    static QMediaPlayer *BGMPlayer;
    static QCache<QString, QMediaPlayer> *SoundCache;
public:
    static void init();
    static void quit();

    static void play(const QString &filename, const bool doubleVolume = false);
    static void playAudioOfMoxuan();
    static void stop();

    static void playBGM(const QString &filename);
    static void setBGMVolume(int volume);
    static void stopBGM();
};
#endif

#endif

