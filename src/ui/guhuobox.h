#ifndef GUHUO_BOX_H
#define GUHUO_BOX_H

#include "graphicsbox.h"
#include "title.h"

class CardButton : public QGraphicsObject
{
    Q_OBJECT

public:
    CardButton(QGraphicsObject *parent, const QString &card, int scale, const QString &skill = QString());

signals:
    void clicked();

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual QRectF boundingRect() const;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    QString cardName;
    int Scale;
};

class GuhuoBox : public GraphicsBox
{
    Q_OBJECT

public:
    GuhuoBox(const QString &skill_name, const QString &flags);
    QString getSkillName()const
    {
        return skill_name;
    }

signals:
    void onButtonClick();

public slots:
    void popup();
    void reply();
    void clear();

protected:
    virtual QRectF boundingRect() const;
    QString translate(const QString &option) const;
    QString flags;
    QString skill_name;
    QList<CardButton *> buttons;
    QList<Title*> titles;

    static const int topBlankWidth;
    static const int bottomBlankWidth;
    static const int interval;
    static const int outerBlankWidth;
    static const int titleWidth;

private:
    int maxcardcount, maxrow, scale;
};

#endif
