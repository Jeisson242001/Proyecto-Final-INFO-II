#ifndef PLAYERITEM_H
#define PLAYERITEM_H

#pragma once
#include <QGraphicsPixmapItem>
#include <QObject>
#include <QVector>

class PlayerItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    explicit PlayerItem(QGraphicsItem *parent = nullptr);
    ~PlayerItem() override;

    bool isFacingLeft() const { return facingLeft; }

    void moveLeft();
    void moveRight();
    void stopMoving();
    void jump();

    // Llamar desde el loop principal cada frame
    void updateFrame(double dt);

private:
    // física
    double vx = 0.0;        // px/s
    double vy = 0.0;        // px/s (positivo hacia abajo)
    const double maxSpeed = 300.0;
    const double accel = 1200.0;
    const double friction = 800.0;
    const double gravity = 1400.0;
    const double jumpImpulse = 520.0;

    bool onGround = false;

    // animación
    QVector<QPixmap> idleFrames;
    QVector<QPixmap> runFrames;

    // en PlayerItem.h, dentro de la sección privada:
    QVector<QPixmap> idleFramesFlipped;
    QVector<QPixmap> runFramesFlipped;
    bool facingLeft = false;

    int currentFrame = 0;
    double animTime = 0.0;
    double animSpeed = 0.12; // segundos por frame

    void loadFrames();
    void applyPhysics(double dt);
    void updateAnimation(double dt);
};



#endif // PLAYERITEM_H
