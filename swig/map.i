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
template <class Key, class T>
class QMap {
public:
    QMap();
    ~QMap();
    int size() const;
    bool isEmpty() const;
    void clear();
    int remove(const Key &key);
    T take(const Key &key);
    bool contains(const Key &key) const;
    const Key key(const T &value, const Key &defaultKey = Key()) const;
    const T value(const Key &key, const T &defaultValue = T()) const;
    const T &first() const;
    const T &last() const;
    
    QList<Key> uniqueKeys() const;
    QList<Key> keys() const;
    QList<Key> keys(const T &value) const;
    QList<T> values() const;
    QList<T> values(const Key &key) const;
    int count(const Key &key) const;
};

%extend QMap {
    T at(Key i) const
    {
        return $self->value(i);
    }
}

%template(QVariantMap) QMap<QString, QVariant>;
%template(CardMoveMap) QMap<QString, QList<int> >;
