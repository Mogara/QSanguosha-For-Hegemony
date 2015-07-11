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

#ifndef GENERALSELECTOR_H
#define GENERALSELECTOR_H

#include <QObject>
#include <QHash>
#include <QGenericMatrix>
#include <QStringList>

class ServerPlayer;

// singleton class
class GeneralSelector : public QObject
{
    Q_OBJECT

public:
    static GeneralSelector *getInstance();
    QStringList selectGenerals(ServerPlayer *player, const QStringList &candidates);
    inline void resetValues()
    {
        m_privatePairValueTable.clear();
    }

private:
    GeneralSelector();
    void loadGeneralTable();
    void loadPairTable();
    void calculatePairValues(const ServerPlayer *player, const QStringList &candidates);
    void calculateDeputyValue(const ServerPlayer *player, const QString &first, const QStringList &candidates, const QStringList &kingdom_list = QStringList());

    QHash<QString, int> m_singleGeneralTable;
    QHash<QString, int> m_pairTable;
    QHash<const ServerPlayer *, QHash<QString, int> > m_privatePairValueTable;
};

#endif // GENERALSELECTOR_H
