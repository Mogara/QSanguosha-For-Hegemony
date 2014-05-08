#include "jsonutils.h"

Json::Value QSanProtocol::Utils::toJsonArray(const QString &s1, const QString &s2) {
    Json::Value val(Json::arrayValue);
    val[0] = s1.toUtf8().constData();
    val[1] = s2.toUtf8().constData();
    return val;
}

Json::Value QSanProtocol::Utils::toJsonArray(const QString &s1, const QString &s2, const QString &s3) {
    Json::Value val(Json::arrayValue);
    val[0] = s1.toUtf8().constData();
    val[1] = s2.toUtf8().constData();
    val[2] = s3.toUtf8().constData();
    return val;
}

Json::Value QSanProtocol::Utils::toJsonArray(const QString &s1, const Json::Value &s2) {
    Json::Value val(Json::arrayValue);
    val[0] = s1.toUtf8().constData();
    val[1] = s2;
    return val;
}

Json::Value QSanProtocol::Utils::toJsonArray(const QList<int> &arg) {
    Json::Value val(Json::arrayValue);
    foreach (int i, arg)
        val.append(i);
    return val;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, QList<int> &result) {
    if (!arg.isArray()) return false;
    for (unsigned int i = 0; i < arg.size(); i++)
        if (!arg[i].isInt()) return false;
    for (unsigned int i = 0; i < arg.size(); i++)
        result.append(arg[i].asInt());
    return true;
}

Json::Value QSanProtocol::Utils::toJsonArray(const QList<QString> &arg) {
    Json::Value val(Json::arrayValue);
    foreach (QString s, arg)
        val.append(toJsonString(s));
    return val;
}

Json::Value QSanProtocol::Utils::toJsonArray(const QStringList &arg) {
    Json::Value val(Json::arrayValue);
    foreach (QString s, arg)
        val.append(toJsonString(s));
    return val;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, int &result) {
    if (!arg.isInt()) return false;
    result = arg.asInt();
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, double &result) {
    if (arg.isDouble())
        result = arg.asDouble();
    else if (arg.isInt())
        result = arg.asInt();
    else
        return false;
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, bool &result) {
    if (!arg.isBool()) return false;
    result = arg.asBool();
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, Qt::Alignment &align) {
    if (!arg.isString()) return false;
    QString alignStr = toQString(arg).toLower();
    if (alignStr.contains("left"))
        align = Qt::AlignLeft;
    else if (alignStr.contains("right"))
        align = Qt::AlignRight;
    else if (alignStr.contains("center"))
        align = Qt::AlignHCenter;

    if (alignStr.contains("top"))
        align |= Qt::AlignTop;
    else if (alignStr.contains("bottom"))
        align |= Qt::AlignBottom;
    else if (alignStr.contains("center"))
        align |= Qt::AlignVCenter;

    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, QString &result) {
    if (!arg.isString()) return false;
    result = toQString(arg);
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, QStringList &result) {
    if (!arg.isArray()) return false;
    for (unsigned int i = 0; i < arg.size(); i++)
        if (!arg[i].isString()) return false;
    for (unsigned int i = 0; i < arg.size(); i++)
        result.append(arg[i].asCString());
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, QRect &result) {
    if (!arg.isArray() || arg.size() != 4) return false;
    result.setLeft(arg[0].asInt());
    result.setTop(arg[1].asInt());
    result.setWidth(arg[2].asInt());
    result.setHeight(arg[3].asInt());
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, QSize &result) {
    if (!arg.isArray() || arg.size() != 2) return false;
    result.setWidth(arg[0].asInt());
    result.setHeight(arg[1].asInt());
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, QPoint &result) {
    if (!arg.isArray() || arg.size() != 2) return false;
    result.setX(arg[0].asInt());
    result.setY(arg[1].asInt());
    return true;
}

bool QSanProtocol::Utils::tryParse(const Json::Value &arg, QColor &color) {
    if (!arg.isArray() && arg.size() < 3) return false;
    color.setRed(arg[0].asInt());
    color.setGreen(arg[1].asInt());
    color.setBlue(arg[2].asInt());
    if (arg.size() > 3)
        color.setAlpha(arg[3].asInt());
    else
        color.setAlpha(255);
    return true;
}

