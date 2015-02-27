#ifndef GUHUOBOX
#define GUHUOBOX

#include "graphicsbox.h"
#include "button.h"
#include "title.h"
class GuhuoBox : public GraphicsBox{
    Q_OBJECT
public:
    //static GuhuoBox *getInstance(const QString &skill_name,const QString &flags,bool play_only);
    GuhuoBox(const QString& skill_name, const QString& flags, bool playonly = true);
    QString getSkillName()const {return skill_name;}
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

    QMap<QString,QStringList> card_list;

    QMap<QString,Button *> buttons;

    QMap<QString,Title*> titles;

    static const int minButtonWidth = 100;
    static const int defaultButtonHeight = 30;
    static const int topBlankWidth = 42;
    static const int bottomBlankWidth = 85;
    static const int interval = 15;
    static const int outerBlankWidth = 37;

    static const int titleWidth = 20;

};

#endif // GUHUOBOX

