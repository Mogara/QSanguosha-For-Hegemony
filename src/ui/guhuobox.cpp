#include "guhuobox.h"
#include "button.h"
#include "engine.h"
#include "standard.h"
#include "clientplayer.h"
#include "skinbank.h"
#include "roomscene.h"

#include <QPropertyAnimation>
#include <QGraphicsSceneMouseEvent>

static qreal initialOpacity = 0.8;
static int optionButtonHeight = 40;

GuhuoButton::GuhuoButton(QGraphicsObject *parent, const QString &card, const int width, const QColor &color)
{
    this->setParentItem(parent);
    this->setObjectName(card);
    this->width = width;
    this->color = color;

    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setOpacity(initialOpacity);
}

void GuhuoButton::needDisabled(bool disabled)
{
    if (disabled) {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
        animation->setEndValue(0.2);
        animation->setDuration(100);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
        animation->setEndValue(initialOpacity);
        animation->setDuration(100);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void GuhuoButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
    painter->save();
    painter->setBrush(Qt::black);
    painter->setPen(this->isEnabled() ? color : "gray");
    QRectF rect = boundingRect();
    painter->drawRoundedRect(rect, 5, 5);
    painter->restore();


    QPixmap pixmap = G_ROOM_SKIN.getCardTinyPixmap(objectName());
    const Card *c = Sanguosha->cloneCard(objectName());
    if (c->isKindOf("DelayedTrick")) {
        QTransform transform;
        transform.rotate(90);
        pixmap = pixmap.transformed(transform);
    }
    pixmap = pixmap.scaledToHeight(optionButtonHeight, Qt::SmoothTransformation);
    QRect pixmapRect(QPoint(0, (rect.height() - pixmap.height()) / 2), pixmap.size());
    painter->setBrush(pixmap);
    painter->drawRoundedRect(pixmapRect, 5, 5);

    QRect textArea(optionButtonHeight, 0, width - optionButtonHeight,
        optionButtonHeight);

    G_COMMON_LAYOUT.optionButtonText.paintText(painter, textArea,
        Qt::AlignCenter,Sanguosha->translate(this->objectName()));
}

QRectF GuhuoButton::boundingRect() const
{
    return QRectF(0, 0, width, optionButtonHeight);
}


void GuhuoButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void GuhuoButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    emit clicked();
}

void GuhuoButton::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(1.0);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    emit hovered(true);
}

void GuhuoButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(initialOpacity);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    emit hovered(false);
}

const int GuhuoBox::minButtonWidth = 100;
const int GuhuoBox::defaultButtonHeight = 40;
const int GuhuoBox::topBlankWidth = 30; //42
const int GuhuoBox::bottomBlankWidth = 55; //85
const int GuhuoBox::interval = 10; //15
const int GuhuoBox::outerBlankWidth = 25; //37

const int GuhuoBox::titleWidth = 15; // 20

GuhuoBox::GuhuoBox(const QString &skillname, const QString &flag, bool playonly)
{
    this->skill_name = skillname;
    this->flags = flag;
    this->play_only = playonly;
    title = QString("%1 %2").arg(Sanguosha->translate(skill_name)).arg(tr("Please choose:"));;
    //collect Cards' objectNames
    if (flags.contains("b")) {
        QList<const BasicCard*> basics = Sanguosha->findChildren<const BasicCard*>();
        foreach (const BasicCard *card, basics) {
            if (!card_list["BasicCard"].contains(card->objectName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage()))
                card_list["BasicCard"].append(card->objectName());
        }
    }
    if (flags.contains("t")) {
        QList<const TrickCard*> tricks = Sanguosha->findChildren<const TrickCard*>();
        foreach (const TrickCard *card, tricks) {
            if (!ServerInfo.Extensions.contains("!" + card->getPackage()) && card->isNDTrick()) {
                if (card_list["SingleTargetTrick"].contains(card->objectName()) || card_list["MultiTarget"].contains(card->objectName()))
                    continue;
                if (card->inherits("SingleTargetTrick") && !card_list["SingleTargetTrick"].contains(card->objectName()))
                    card_list["SingleTargetTrick"].append(card->objectName());
                else
                    card_list["MultiTarget"].append(card->objectName());

            }
        }
    }
    if (flags.contains("d")) {
        QList<const DelayedTrick*> delays = Sanguosha->findChildren<const DelayedTrick*>();
        foreach (const DelayedTrick *card, delays) {
            if (!card_list["DelayedTrick"].contains(card->objectName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage()))
                card_list["DelayedTrick"].append(card->objectName());
        }
    }
}


QRectF GuhuoBox::boundingRect() const
{
    const int width = getButtonWidth() * 4 + outerBlankWidth * 5; // 4 buttons each line

    int height = topBlankWidth
        + ((card_list["BasicCard"].length() + 3) / 4) * defaultButtonHeight
        + (((card_list["BasicCard"].length() + 3) / 4) - 1) * interval
        + ((card_list["SingleTargetTrick"].length() + 3) / 4) * defaultButtonHeight
        + (((card_list["SingleTargetTrick"].length() + 3) / 4) - 1) * interval
        + ((card_list["MultiTarget"].length() + 3) / 4) * defaultButtonHeight
        + (((card_list["MultiTarget"].length() + 3) / 4) - 1) * interval
        + ((card_list["DelayedTrick"].length() + 3) / 4) * defaultButtonHeight
        + (((card_list["DelayedTrick"].length() + 3) / 4) - 1) * interval
        + card_list.keys().length()*titleWidth * 2 //add some titles......
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
        foreach (const QString choice, value) {
            const int width = fontMetrics.width(translate(choice));
            if (width > biggest)
                biggest = width;
        }
    }

    // Otherwise it would look compact
    biggest += 60;

    int width = minButtonWidth;
    return qMax(biggest, width);
}

bool GuhuoBox::isButtonEnable(const QString &card) const
{
    QString allowed_list = Self->property("guhuo_box_allowed_elemet").toString();
    if (!allowed_list.isEmpty())
        return allowed_list.split("+").contains(card);
    else {
        Card *ca = Sanguosha->cloneCard(card);
        ca->deleteLater();
        return ca->isAvailable(Self);
    }
}

QColor GuhuoBox::getColor(const QString &card) const
{
    const Card *c = Sanguosha->cloneCard(card);
    if (c) {
        if (c->isKindOf("FireSlash"))
            return QColor("red");
        else if (c->isKindOf("ThunderSlash"))
            return QColor(20,10,90);
        else if (c->isKindOf("Peach"))
            return QColor(250,123,123);
        else if (c->isKindOf("Analeptic"))
            return QColor(90,10,60);

    }
    return QColor("gold");
}

void GuhuoBox::popup()
{
    if (play_only && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        emit onButtonClick();
        return;
    }
    RoomSceneInstance->getDasboard()->disableAllCards();
    RoomSceneInstance->current_guhuo_box = this;
    const int buttonWidth = getButtonWidth();
    foreach (const QString &key, card_list.keys()) {
        foreach (const QString &card_name, card_list.value(key)) {
            //Button *button = new Button(translate(card_name), QSizeF(buttonWidth,
              //  defaultButtonHeight));
            GuhuoButton *button = new GuhuoButton(this,card_name,getButtonWidth(),getColor(card_name));
            buttons[card_name] = button;

            button->setEnabled(isButtonEnable(card_name));

            button->setParentItem(this);

            QString original_tooltip = QString(":%1").arg(title);
            QString tooltip = Sanguosha->translate(original_tooltip);
            if (tooltip == original_tooltip) {
                original_tooltip = QString(":%1").arg(card_name);
                tooltip = Sanguosha->translate(original_tooltip);
            }
            connect(button, &GuhuoButton::clicked, this, &GuhuoBox::reply);
            if (tooltip != original_tooltip)
                button->setToolTip(QString("<font color=%1>%2</font>")
                .arg(Config.SkillDescriptionInToolTipColor.name())
                .arg(tooltip));
        }

        titles[key] = new Title(this, translate(key), IQSanComponentSkin::QSanSimpleTextFont::_m_fontBank.key(G_COMMON_LAYOUT.graphicsBoxTitleFont.m_fontFace), Config.TinyFont.pixelSize()); //undefined reference to "GuhuoBox::titleWidth" 666666
        titles[key]->setParentItem(this);
    }
    moveToCenter();
    show();
    int x = 0;
    int y = 0;
    int titles_num = 0;
    foreach (const QString &key, card_list.keys()) {
        QPointF titlepos;
        titlepos.setX(interval);
        titlepos.setY(topBlankWidth + defaultButtonHeight*y + interval*(y - 1) + titleWidth*titles_num * 2 - 2 * titles[key]->y());
        titles[key]->setPos(titlepos);
        ++titles_num;
        foreach (const QString &card_name, card_list.value(key)) {
            QPointF apos;
            apos.setX((x + 1)*outerBlankWidth + x*buttonWidth);
            apos.setY(topBlankWidth + defaultButtonHeight*y + interval*(y - 1) + titleWidth*titles_num * 2);
            ++x;
            if (x == 4) {
                ++y;
                x = 0;
            }
            buttons[card_name]->setPos(apos);
        }
        ++y;
        x = 0;
    }
}
void GuhuoBox::reply()
{
    Self->tag.remove(skill_name);
    const QString &answer = sender()->objectName();
    Self->tag[skill_name] = answer;
    emit onButtonClick();
    clear();
}
void GuhuoBox::clear()
{
    RoomSceneInstance->current_guhuo_box = NULL;

    if (sender() != NULL && Self->tag[skill_name] == sender()->objectName() && Sanguosha->getViewAsSkill(skill_name) != NULL)
        RoomSceneInstance->getDasboard()->updatePending();
    else if (Self->getPhase() == Player::Play)
        RoomSceneInstance->getDasboard()->enableCards();

    if (!isVisible())
        return;

    foreach(GuhuoButton *button, buttons.values())
        button->deleteLater();

    buttons.values().clear();

    foreach (Title *title, titles.values())
        title->deleteLater();

    titles.values().clear();

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
