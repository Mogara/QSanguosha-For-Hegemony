#include "guhuobox.h"
#include "roomscene.h"
#include "engine.h"
#include "standard.h"
#include "clientplayer.h"
#include "skinbank.h"

#include <QGraphicsSceneMouseEvent>
#include <QPropertyAnimation>
#include <QGraphicsProxyWidget>

static QSize cardButtonSize;

const int GuhuoBox::topBlankWidth = 38;
const int GuhuoBox::bottomBlankWidth = 15;
const int GuhuoBox::interval = 5;

const int GuhuoBox::titleWidth = 22;

GuhuoBox::GuhuoBox(const QString &skillname, const QString &flag, bool playonly)
{
    this->skill_name = skillname;
    this->flags = flag;
    this->play_only = playonly;
    title = QString("%1 %2").arg(Sanguosha->translate(skill_name)).arg(tr("Please choose:"));
    cardButtonSize = QSize(G_COMMON_LAYOUT.m_cardNormalWidth, G_COMMON_LAYOUT.m_cardNormalHeight);
}


QRectF GuhuoBox::boundingRect() const
{
    int count = qMax(maxcardcount, 2);
    int width = cardButtonSize.width() * count * scale / 10 + interval * 2;
    int defaultButtonHeight = cardButtonSize.height() * scale / 10;

    int height = topBlankWidth
        + maxrow * defaultButtonHeight + interval * (maxrow - 1)
        + titleWidth * maxrow + bottomBlankWidth;

    return QRectF(0, 0, width, height);
}

bool GuhuoBox::isButtonEnable(const QString &card) const
{
    QString allowed_list = Self->property("guhuo_box_allowed_elemet").toString();
    if (!allowed_list.isEmpty() && !allowed_list.split("+").contains(card))
        return false;

    Card *ca = Sanguosha->cloneCard(card);
    ca->setCanRecast(false);
    ca->deleteLater();
    return ca->isAvailable(Self);

}

void GuhuoBox::popup()
{
    if (RoomSceneInstance->current_guhuo_box != NULL) {
        RoomSceneInstance->current_guhuo_box->clear();
    }
    RoomSceneInstance->getDasboard()->unselectAll();
    RoomSceneInstance->getDasboard()->stopPending();
    if (play_only && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        emit onButtonClick();
        return;
    }

    RoomSceneInstance->getDasboard()->disableAllCards();
    RoomSceneInstance->current_guhuo_box = this;

    maxcardcount = 0;
    maxrow = 0;
    scale = 7;
    titles.clear();
    QStringList names1, names2, names3, names4;

    QList<Card *> cards = Sanguosha->getCards();
    if (flags.contains("b")) {
        foreach (const Card *card, cards) {
            if (card->isKindOf("BasicCard") && !names1.contains(card->objectName()) && isButtonEnable(card->objectName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                names1 << card->objectName();
            }
        }
        if (!names1.isEmpty()) {
            Title *newtitle = new Title(this, translate("BasicCard"), IQSanComponentSkin::QSanSimpleTextFont::_m_fontBank.key(G_COMMON_LAYOUT.graphicsBoxTitleFont.m_fontFace), Config.TinyFont.pixelSize());
            newtitle->setParentItem(this);
            titles << newtitle;
            maxcardcount = names1.length();
            ++maxrow;
        }
    }
    if (flags.contains("t")) {
        foreach (const Card *card, cards) {
            if (card->isNDTrick() && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                if (names2.contains(card->objectName()) || names3.contains(card->objectName()) || !isButtonEnable(card->objectName()))
                    continue;

                if (card->inherits("SingleTargetTrick"))
                    names2 << card->objectName();
                else
                    names3 << card->objectName();

            }
        }
        if (!names2.isEmpty()) {
            Title *newtitle = new Title(this, translate("SingleTargetTrick"), IQSanComponentSkin::QSanSimpleTextFont::_m_fontBank.key(G_COMMON_LAYOUT.graphicsBoxTitleFont.m_fontFace), Config.TinyFont.pixelSize());
            newtitle->setParentItem(this);
            titles << newtitle;
            maxcardcount = qMax(maxcardcount, names2.length());
            ++maxrow;
        }
        if (!names3.isEmpty()) {
            Title *newtitle = new Title(this, translate("MultiTarget"), IQSanComponentSkin::QSanSimpleTextFont::_m_fontBank.key(G_COMMON_LAYOUT.graphicsBoxTitleFont.m_fontFace), Config.TinyFont.pixelSize());
            newtitle->setParentItem(this);
            titles << newtitle;
            maxcardcount = qMax(maxcardcount, names3.length());
            ++maxrow;
        }
    }
    if (flags.contains("d")) {
        foreach (const Card *card, cards) {
            if (!card->isNDTrick() && card->isKindOf("TrickCard") && !names4.contains(card->objectName()) && isButtonEnable(card->objectName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                names4 << card->objectName();
            }
        }
        if (!names4.isEmpty()) {
            Title *newtitle = new Title(this, translate("DelayedTrick"), IQSanComponentSkin::QSanSimpleTextFont::_m_fontBank.key(G_COMMON_LAYOUT.graphicsBoxTitleFont.m_fontFace), Config.TinyFont.pixelSize());
            newtitle->setParentItem(this);
            titles << newtitle;
            maxcardcount = qMax(maxcardcount, names4.length());
            ++maxrow;
        }
    }

    if (maxcardcount == 0) {
        emit onButtonClick();
        return;
    }

    for (int i = 12; i > 7; i--) {
        if (cardButtonSize.width() * maxcardcount * i / 10 + interval * (maxcardcount + 1) < RoomSceneInstance->sceneRect().width() * 0.8
            && topBlankWidth + maxrow * cardButtonSize.height() * i / 10 + interval * (maxrow - 1)
            + titleWidth * maxrow + bottomBlankWidth < RoomSceneInstance->sceneRect().height() * 0.8) {
            scale = i;
            break;
        }
    }

    int buttonWidth = cardButtonSize.width() * scale / 10;
    int defaultButtonHeight = cardButtonSize.height() * scale / 10;

    moveToCenter();
    show();
    int x = 0;
    int y = 1;
    int app = 0;
    if (maxcardcount == 1)
        app = (buttonWidth + interval) / 2;
    for (int i = 0; i < titles.length(); ++i) {
        QPointF titlepos;
        titlepos.setX(interval + app);
        titlepos.setY(topBlankWidth + defaultButtonHeight * i + interval * i + titleWidth * i);
        titles.at(i)->setPos(titlepos);
    }

    if (!names1.isEmpty()) {
        foreach (const QString &cardname, names1) {
            CardButton *button = new CardButton(this, cardname, scale);
            connect(button, &CardButton::clicked, this, &GuhuoBox::reply);

            QPointF apos;
            apos.setX(interval + x * buttonWidth + app);
            apos.setY(topBlankWidth + (defaultButtonHeight + interval) * (y - 1) + titleWidth * y);
            ++x;

            button->setPos(apos);
            buttons << button;
        }
        ++y;
        x = 0;
    }
    if (!names2.isEmpty()) {
        foreach (const QString &cardname, names2) {
            CardButton *button = new CardButton(this, cardname, scale);
            connect(button, &CardButton::clicked, this, &GuhuoBox::reply);

            QPointF apos;
            apos.setX(interval + x * buttonWidth + app);
            apos.setY(topBlankWidth + (defaultButtonHeight + interval) * (y - 1) + titleWidth * y);
            ++x;

            button->setPos(apos);
            buttons << button;
        }
        ++y;
        x = 0;
    }
    if (!names3.isEmpty()) {
        foreach (const QString &cardname, names3) {
            CardButton *button = new CardButton(this, cardname, scale);
            connect(button, &CardButton::clicked, this, &GuhuoBox::reply);

            QPointF apos;
            apos.setX(interval + x * buttonWidth + app);
            apos.setY(topBlankWidth + (defaultButtonHeight + interval) * (y - 1) + titleWidth * y);
            ++x;

            button->setPos(apos);
            buttons << button;
        }
        ++y;
        x = 0;
    }
    if (!names4.isEmpty()) {
        foreach (const QString &cardname, names4) {
            CardButton *button = new CardButton(this, cardname, scale);
            connect(button, &CardButton::clicked, this, &GuhuoBox::reply);

            QPointF apos;
            apos.setX(interval + x * buttonWidth + app);
            apos.setY(topBlankWidth + (defaultButtonHeight + interval) * (y - 1) + titleWidth * y);
            ++x;

            button->setPos(apos);
            buttons << button;
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

    foreach(CardButton *button, buttons)
        button->deleteLater();

    buttons.clear();

    foreach (Title *title, titles)
        title->deleteLater();

    titles.clear();

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

CardButton::CardButton(QGraphicsObject *parent, const QString &card, int scale)
    : QGraphicsObject(parent), cardName(card), Scale(scale)
{
    setAcceptedMouseButtons(Qt::LeftButton);

    setAcceptHoverEvents(true);
    setFlag(ItemIsFocusable);

    setOpacity(0.7);
    setObjectName(card);

    QString original_tooltip = QString(":%1").arg(card);
    QString tooltip = "[" + Sanguosha->translate(card) + "] " + Sanguosha->translate(original_tooltip);
    setToolTip(QString("<font color=%1>%2</font>").arg(Config.SkillDescriptionInToolTipColor.name()).arg(tooltip));
}

void CardButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
    QPixmap generalImage = G_ROOM_SKIN.getCardMainPixmap(cardName);
    generalImage = generalImage.scaled(cardButtonSize * Scale / 10, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter->setBrush(generalImage);
    painter->drawRoundedRect(boundingRect(), 5, 5, Qt::RelativeSize);
}

QRectF CardButton::boundingRect() const
{
    return QRectF(QPoint(0, 0), cardButtonSize * Scale / 10);
}

void CardButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void CardButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    emit clicked();
}

void CardButton::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(1);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void CardButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(0.7);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
