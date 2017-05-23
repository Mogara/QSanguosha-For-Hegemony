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

#include "pixmapanimation.h"
#include "skinbank.h"

#include <QPainter>
#include <QPixmapCache>
#include <QDir>
#include <QTimer>
#include <QGraphicsScene>

const int PixmapAnimation::S_DEFAULT_INTERVAL = 50;

PixmapAnimation::PixmapAnimation()
    : QGraphicsItem(0), m_fix_rect(false), hideonstop(false), _m_timerId(0), m_timer(0), m_timeout(0)
{
}

void PixmapAnimation::advance(int phase)
{
    if (phase) {
        current++;
        m_timeout = m_timeout + m_interval;
    }

    if (current >= frame_list.size())
        current = 0;
    if (current == frame_list.size() - 1 && m_timer == 0)
        emit finished();
    if (m_timer > 0 && m_timeout >= m_timer)
        end();
    if (current == 0 || frame_list.at(current) != frame_list.at(current - 1))
        update();
}

void PixmapAnimation::setPath(const QString &path, bool playback)
{
    frames.clear();
    frame_list.clear();
    current = 0;

    QString config = QString("%1%2").arg(path).arg("config.txt");
    QFile file(config);
    if (QFile::exists(config) && file.open(QIODevice::ReadOnly)) {
        QRegExp rx("(\\w+)\\s+(\\d+)");
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (!rx.exactMatch(line))
                continue;

            QStringList texts = rx.capturedTexts();
            QString pic_name = texts.at(1);
            int count = texts.at(2).toInt();

            QString pic_path = QString("%1%2%3").arg(path).arg(pic_name).arg(".png");
            if (QFile::exists(pic_path)) {
                if (frames[pic_name].isNull()) frames[pic_name] = G_ROOM_SKIN.getPixmapFromFileName(pic_path);
                for (int i = 1; i <= count; i++)
                    frame_list << pic_name;
            }
        }
        file.close();
    } else {
        int i = 0;
        QString pic_path = QString("%1%2%3").arg(path).arg(i++).arg(".png");
        do {
            frames[QString::number(i)] = G_ROOM_SKIN.getPixmapFromFileName(pic_path);
            frame_list << QString::number(i);
            pic_path = QString("%1%2%3").arg(path).arg(i++).arg(".png");
        } while (QFile::exists(pic_path));
    }

    if (playback) {
        QStringList frames_copy = frame_list;
        for (int i = frames_copy.length() - 1; i >= 0; i--)
            frame_list << frames_copy[i];
    }
}

void PixmapAnimation::setAvatarAnimation(const QString &name, int size, int skin_id, const QRect &rect)
{
    frames.clear();
    current = 0;

    int i = 0;
    QPixmap pix = G_ROOM_SKIN.getAvatAnimPixmapPath(name, (QSanRoomSkin::GeneralIconSize)size, skin_id, i);
    while (!pix.isNull()) {
        pix = pix.scaled(rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        frames[QString::number(i)] = pix;
        frame_list << QString::number(i);
        i++;
        pix = G_ROOM_SKIN.getAvatAnimPixmapPath(name, (QSanRoomSkin::GeneralIconSize)size, skin_id, i);
    }
}

void PixmapAnimation::setSize(const QSize &size)
{
    m_fix_rect = true;
    m_size = size;
    update();
}

void PixmapAnimation::setHideonStop(bool hide)
{
    this->hideonstop = hide;
}

void PixmapAnimation::setPlayTime(int msecs)
{
    m_timer = msecs;
}

void PixmapAnimation::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QString pix_name = frame_list.at(current);
    QPixmap pic = frames[pix_name];
    if (m_fix_rect)
        painter->drawPixmap(0, 0, m_size.width(), m_size.height(), pic);
    else {
        double scale = G_ROOM_LAYOUT.scale;
        painter->drawPixmap(0, 0, (int)(pic.width() * scale), (int)(pic.height() * scale), pic);
    }
}

QRectF PixmapAnimation::boundingRect() const
{
    if (m_fix_rect) return QRect(0, 0, m_size.width(), m_size.height());
    double scale = G_ROOM_LAYOUT.scale;
    return QRect(0, 0, (int)(frames[frame_list.at(current)].width() * scale), (int)(frames[frame_list.at(current)].height() * scale));
}

bool PixmapAnimation::valid()
{
    return !frames.isEmpty();
}

void PixmapAnimation::timerEvent(QTimerEvent *)
{
    advance(1);
}

void PixmapAnimation::start(bool permanent, int interval)
{
    m_timeout = 0;
    if (_m_timerId > 0)
        return;

    m_interval = interval;
    _m_timerId = startTimer(interval);
    if (!permanent)
        connect(this, &PixmapAnimation::finished, this, &PixmapAnimation::deleteLater);
}

void PixmapAnimation::stop()
{
    killTimer(_m_timerId);
    _m_timerId = 0;
    if (hideonstop) this->hide();
}

void PixmapAnimation::reset()
{
    current = 0;
    update();
}

bool PixmapAnimation::isFirstFrame()
{
    return current == 0;
}

void PixmapAnimation::preStart()
{
    m_timeout = 0;
    if (_m_timerId > 0)
        return;

    this->show();
	m_interval = S_DEFAULT_INTERVAL;
    _m_timerId = startTimer(S_DEFAULT_INTERVAL);
}

void PixmapAnimation::end()
{
    stop();
    emit finished();
}

PixmapAnimation *PixmapAnimation::GetPixmapAnimation(QGraphicsItem *parent, const QString &emotion, bool playback, int duration)
{
    PixmapAnimation *pma = new PixmapAnimation();
    pma->setPath(QString("image/system/emotion/%1/").arg(emotion), playback);
    if (duration > 0) pma->setPlayTime(duration);
    if (pma->valid()) {
        if (emotion == "no-success") {
            pma->moveBy(pma->boundingRect().width() * 0.25,
                pma->boundingRect().height() * 0.25);
            pma->setScale(0.5);
        } else if (emotion == "success") {
            pma->moveBy(pma->boundingRect().width() * 0.1,
                pma->boundingRect().height() * 0.1);
            pma->setScale(0.8);
        } else if (emotion.contains("double_sword"))
            pma->moveBy(13, -20);
        else if (emotion.contains("fan") || emotion.contains("guding_blade"))
            pma->moveBy(0, -20);
        else if (emotion.contains("/spear"))
            pma->moveBy(-20, -20);

        pma->moveBy((parent->boundingRect().width() - pma->boundingRect().width()) / 2,
            (parent->boundingRect().height() - pma->boundingRect().height()) / 2);

        pma->setParentItem(parent);
        pma->setZValue(20002.0);
        if (emotion.contains("weapon")) {
            pma->hide();
            QTimer::singleShot(600, pma, SLOT(preStart()));
        } else
            pma->preStart();

        connect(pma, &PixmapAnimation::finished, pma, &PixmapAnimation::deleteLater);
        return pma;
    } else {
        delete pma;
        return NULL;
    }
}

QPixmap PixmapAnimation::GetFrameFromCache(const QString &filename)
{
    QPixmap pixmap;
    if (!QPixmapCache::find(filename, &pixmap)) {
        pixmap.load(filename);
        if (!pixmap.isNull()) QPixmapCache::insert(filename, pixmap);
    }
    return pixmap;
}

int PixmapAnimation::GetFrameCount(const QString &emotion)
{
    QString path = QString("image/system/emotion/%1/").arg(emotion);
    QDir dir(path);
    dir.setNameFilters(QStringList("*.png"));
    return dir.entryList(QDir::Files | QDir::NoDotAndDotDot).count();
}

