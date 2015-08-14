#ifndef GUHUOBOX
#define GUHUOBOX

#include "graphicsbox.h"
#include "title.h"

class GuhuoButton : public QGraphicsObject
{
    Q_OBJECT

public:
    static QFont defaultFont();

    GuhuoButton(QGraphicsObject *parent,const QString &card, const int width, const QColor &color);

signals:
    void clicked();
    void hovered(bool entering);

public slots:
    void needDisabled(bool disabled);
protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual QRectF boundingRect() const;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    int width;
    QColor color;
};

class GuhuoBox : public GraphicsBox
{
    Q_OBJECT

public:
    GuhuoBox(const QString& skill_name, const QString& flags, bool playonly = true);
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

    int getButtonWidth() const;

    bool isButtonEnable(const QString &card) const;

    QColor getColor(const QString &card) const;

    QString translate(const QString &option) const;

    bool play_only;
    QString flags;
    QString skill_name;

    QMap<QString, QStringList> card_list;

    QMap<QString, GuhuoButton *> buttons;

    QMap<QString, Title*> titles;

    static const int minButtonWidth;
    static const int defaultButtonHeight;
    static const int topBlankWidth;
    static const int bottomBlankWidth;
    static const int interval;
    static const int outerBlankWidth;

    static const int titleWidth;
};

#endif // GUHUOBOX

