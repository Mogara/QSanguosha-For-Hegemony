#include "guhuobox.h"
#include "button.h"
#include "engine.h"
#include "standard.h"
#include "clientplayer.h"
#include "skinbank.h"

GuhuoBox::GuhuoBox(const QString &skillname, const QString &flag, bool playonly){
    this->skill_name = skillname;
    this->flags = flag;
    this->play_only = playonly;
    title  = QString("%1 %2").arg(Sanguosha->translate(skill_name)).arg(tr("Please choose:"));;
    //collect Cards' objectNames
    if(flags.contains("b")){
        QList<const BasicCard*> basics = Sanguosha->findChildren<const BasicCard*>();
        foreach(const BasicCard *card,basics){
            if(!card_list["BasicCard"].contains(card->objectName())
                    && !ServerInfo.Extensions.contains("!" + card->getPackage()))
                card_list["BasicCard"].append(card->objectName());
        }
    }
    if(flags.contains("t")){
        QList<const TrickCard*> tricks = Sanguosha->findChildren<const TrickCard*>();
        foreach(const TrickCard *card,tricks){
            if(!ServerInfo.Extensions.contains("!" + card->getPackage()) && card->isNDTrick()){
                if(card->inherits("SingleTargetTrick") && !card_list["SingleTarget"].contains(card->objectName())){
                    card_list["SingleTarget"].append(card->objectName());
                    continue;
                }
                if(!card_list["MultiTarget"].contains(card->objectName())){
                    card_list["MultiTarget"].append(card->objectName());
                }
            }
        }
    }
    if(flags.contains("d")){
        QList<const DelayedTrick*> delays = Sanguosha->findChildren<const DelayedTrick*>();
        foreach(const DelayedTrick *card,delays){
            if(!card_list["DelayedTrick"].contains(card->objectName())
                    && !ServerInfo.Extensions.contains("!" + card->getPackage()))
                card_list["DelayedTrick"].append(card->objectName());
        }
    }
    if(flags.contains("e")){
        QList<const EquipCard*> equips = Sanguosha->findChildren<const EquipCard*>();
        foreach(const EquipCard *card,equips){
            if(!card_list["EquipCard"].contains(card->objectName())
                    && !ServerInfo.Extensions.contains("!" + card->getPackage()))
                card_list["EquipCard"].append(card->objectName());
        }
    }
}


QRectF GuhuoBox::boundingRect() const {
    const int width = getButtonWidth()*4 + outerBlankWidth * 5; // 4 buttons each line

    int height = topBlankWidth +
            ((card_list["BasicCard"].length()+3)/4) * defaultButtonHeight
            + (((card_list["BasicCard"].length()+3)/4) - 1) * interval
            +((card_list["SingleTarget"].length()+3)/4) * defaultButtonHeight
            + (((card_list["SingleTarget"].length()+3)/4) - 1) * interval
            +((card_list["MultiTarget"].length()+3)/4) * defaultButtonHeight
            + (((card_list["MultiTarget"].length()+3)/4) - 1) * interval
            +((card_list["DelayedTrick"].length()+3)/4) * defaultButtonHeight
            + (((card_list["DelayedTrick"].length()+3)/4) - 1) * interval
            +((card_list["EquipCard"].length()+3)/4) * defaultButtonHeight
            + (((card_list["EquipCard"].length()+3)/4) - 1) * interval
            +card_list.keys().length()*bottomBlankWidth //add some titles……
            +defaultButtonHeight+interval //for cancel button
            + bottomBlankWidth;

    return QRectF(0, 0, width, height);
}
int GuhuoBox::getButtonWidth() const
{
    if (card_list.values().isEmpty())
        return minButtonWidth;

    QFontMetrics fontMetrics(Button::defaultFont());
    int biggest = 0;
    foreach (const QStringList &value, card_list.values()) {
        foreach(const QString choice,value){
            const int width = fontMetrics.width(translate(choice));
            if (width > biggest)
                biggest = width;
        }
    }

    // Otherwise it would look compact
    biggest += 20;

    int width = minButtonWidth;
    return qMax(biggest, width);
}

void GuhuoBox::popup(){
    if (play_only && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        emit onButtonClick();
        return;
    }
    const int buttonWidth = getButtonWidth();
    foreach (const QString &key, card_list.keys()) {
        foreach(const QString &card_name,card_list.value(key)){
            Button *button = new Button(translate(card_name), QSizeF(buttonWidth,
                                                          defaultButtonHeight));
            button->setObjectName(card_name);
            buttons[card_name] = button;

            Card *ca = Sanguosha->cloneCard(card_name);
            button->setEnabled(ca->isAvailable(Self));
            ca->deleteLater();

            button->setParentItem(this);

            QString original_tooltip = QString(":%1").arg(title);
            QString tooltip = Sanguosha->translate(original_tooltip);
            if (tooltip == original_tooltip) {
                original_tooltip = QString(":%1").arg(card_name);
                tooltip = Sanguosha->translate(original_tooltip);
            }
            connect(button, &Button::clicked, this, &GuhuoBox::reply);
            connect(button,&Button::clicked,this,&GuhuoBox::onButtonClick);
            if (tooltip != original_tooltip)
                button->setToolTip(QString("<font color=%1>%2</font>")
                                   .arg(Config.SkillDescriptionInToolTipColor.name())
                                   .arg(tooltip));
        }
        titles[key] = new Title(this,translate(key),Button::defaultFont().toString(),bottomBlankWidth-5);
    }
    moveToCenter();
    show();
    int x = 1;
    int y = 1;
    foreach(const QString &key, card_list.keys()){
        titles[key]->setPos(interval,topBlankWidth + defaultButtonHeight * y + (y - 1) * interval + defaultButtonHeight / 2 + bottomBlankWidth);
        foreach(const QString &card_name,card_list.value(key)){
            QPointF apos;
            apos.setX(x*outerBlankWidth + (x-1)*buttonWidth);
            ++x;
            if (x == 4){
                ++y;
                x = 0;
            }
            apos.setY(topBlankWidth + defaultButtonHeight * y + (y - 1) * interval + defaultButtonHeight / 2);
            buttons[card_name]->setPos(apos);
        }
        ++y;
    }
    cancel = new Button(translate("cancel"), QSizeF(buttonWidth,
                                               defaultButtonHeight));
    cancel->setObjectName("cancel");
    cancel->setPos(boundingRect().x()/2,boundingRect().y()-interval);

    connect(cancel, &Button::clicked, this, &GuhuoBox::reply);
    connect(cancel,&Button::clicked,this,&GuhuoBox::onButtonClick);
}
void GuhuoBox::reply(){
    const QString &answer = sender()->objectName();
    if(answer == "cancel" || !sender()->inherits("Button")){
        clear();
        return;
    }
    Self->tag[skill_name] = answer;
    clear();
}
void GuhuoBox::clear(){
    foreach (Button *button, buttons.values())
        button->deleteLater();

    buttons.values().clear();

    foreach(Title *title,titles.values()){
        title->deleteLater();
    }

    titles.values().clear();

    cancel->deleteLater();

    disappear();
}
QString GuhuoBox::translate(const QString &option) const
{
    QString title = QString("%1:%2").arg(skill_name).arg(option);
    QString translated = Sanguosha->translate(title);
    if (translated == title)
        translated = Sanguosha->translate(option);
    return translated;
}

//GuhuoBox *GuhuoBox::getInstance(const QString &skill_name, const QString &flags, bool play_only){
//    static GuhuoBox *instance;
//    if(instance == NULL || instance->getSkillName() != skill_name){
//        instance = new GuhuoBox(skill_name,flags,play_only);
//    }
//    return instance;
//}
