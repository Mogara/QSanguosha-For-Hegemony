#ifndef GUHUOBOX
#define GUHUOBOX

#include "graphicsbox.h"
#include "button.h"
#include "title.h"

class GuhuoBox : public GraphicsBox
{
    Q_OBJECT

public:
    //static GuhuoBox *getInstance(const QString &skill_name,const QString &flags,bool play_only);
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

    QString translate(const QString &option) const;

    bool play_only;
    QString flags;
    QString skill_name;

    QMap<QString, QStringList> card_list;

    QMap<QString, Button *> buttons;

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

