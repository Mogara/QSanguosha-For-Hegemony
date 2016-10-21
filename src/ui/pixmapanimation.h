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

#ifndef _PIXMAP_ANIMATION_H
#define _PIXMAP_ANIMATION_H

#include <QGraphicsPixmapItem>

class PixmapAnimation : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    PixmapAnimation();

    QRectF boundingRect() const;
    void advance(int phase);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void timerEvent(QTimerEvent *e);

    void setPath(const QString &path, bool playback = false);
    void setSize(const QSize &size);
    void setHideonStop(bool hide);
    void setPlayTime(int msecs);
    bool valid();

    void start(bool permanent = true, int interval = 50);
    void stop();
    void reset();
    bool isFirstFrame();

    static PixmapAnimation *GetPixmapAnimation(QGraphicsItem *parent, const QString & emotion, bool playback = false, int duration = 0);
    static QPixmap GetFrameFromCache(const QString &filename);
    static int GetFrameCount(const QString &emotion);

    static const int S_DEFAULT_INTERVAL;

signals:
    void finished();
    void frame_loaded();

public slots:
    void preStart();
    void end();

private:
    int _m_timerId;
    QString path;
    QList<QPixmap> frames;
    int current, off_x, off_y, m_timer;
    bool m_fix_rect, hideonstop;
    QSize m_size;
};

#endif

