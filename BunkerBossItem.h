#ifndef BUNKERBOSSITEM_H
#define BUNKERBOSSITEM_H

#pragma once
#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPixmap> // <-- AÑADIR ESTE INCLUDE

class QTimer;
class QGraphicsScene;
class PlayerItem;

class BunkerBossItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    explicit BunkerBossItem(QGraphicsScene *scene, QGraphicsItem *parent = nullptr);
    ~BunkerBossItem() override;

    void takeDamage(int dmg);
    bool isAlive() const { return health_ > 0; }

    void startAttacking(PlayerItem *player);
    void stopAttacking();

signals:
    void bunkerDefeated();
    void bunkerFired();

private slots:
    void onShoot();
    void onShootAnimationFinished(); // <-- AÑADIR ESTE SLOT (para volver a 'idle')

private:
    QGraphicsScene *scene_ = nullptr;
    PlayerItem *playerTarget_ = nullptr;
    QTimer *shootTimer_ = nullptr;

    int health_ = 40;
    bool active_ = false;
    bool dying_ = false;

    // --- AÑADIR ESTOS MIEMBROS ---
    QPixmap idlePixmap_;
    QPixmap shootPixmap_;
    QPixmap destroyedPixmap_;
};

#endif // BUNKERBOSSITEM_H
