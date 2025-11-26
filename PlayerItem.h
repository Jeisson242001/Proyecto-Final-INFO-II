#ifndef PLAYERITEM_H
#define PLAYERITEM_H

#pragma once
#include <QGraphicsPixmapItem>
#include <QObject>
#include <QVector>

class QTimer;

class PlayerItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    explicit PlayerItem(QGraphicsItem *parent = nullptr);
    ~PlayerItem() override;

    void resetLives(int lives = 6);

signals:
    void playerFired();   // se emite cuando el jugador dispara
    void playerHealthChanged(int lives);
    void playerDied();

public:

    // Método auxiliar que notifica a listeners que el jugador ha disparado
    void notifyFired();

    bool isFacingLeft() const { return facingLeft; }

    void moveLeft();
    void moveRight();
    void stopMoving();
    void jump();

    // AGACHARSE
    void startCrouch();
    void stopCrouch();
    bool isCrouching() const { return crouching; }

    // Llamar desde el loop principal cada frame
    void updateFrame(double dt);

    int getLives() const { return lives_; }
    void takeDamage(int amount = 1);
    bool isAlive() const { return lives_ > 0; }

private:
    // física
    double vx = 0.0;        // px/s
    double vy = 0.0;        // px/s (positivo hacia abajo)
    double currentMaxSpeed = 300.0;
    const double maxSpeed = 300.0;
    const double crouchMaxSpeed = 120.0; // velocidad al agacharse
    const double accel = 1200.0;
    const double friction = 800.0;
    const double gravity = 1400.0;
    const double jumpImpulse = 520.0;

    bool onGround = false;

    // animación
    QVector<QPixmap> idleFrames;
    QVector<QPixmap> runFrames;

    // versiones volteadas
    QVector<QPixmap> idleFramesFlipped;
    QVector<QPixmap> runFramesFlipped;

    // frames de agachado
    QVector<QPixmap> crouchFrames;
    QVector<QPixmap> crouchFramesFlipped;

    // frame de salto
    QPixmap jumpFrame_;
    QPixmap jumpFrameFlipped_;

    // sprite muerto/tirado
    QPixmap deadPixmap_;

    bool facingLeft = false;
    bool crouching = false;

    int currentFrame = 0;
    double animTime = 0.0;
    double animSpeed = 0.08; // segundos por frame

    void loadFrames();
    void applyPhysics(double dt);
    void updateAnimation(double dt);

    // helper para aplicar frame y mantener pies en el suelo cuando convenga
    void setFrameAndOffset(const QPixmap &pix, bool alignFeet = false);

    int lives_ = 6;  // Vida máxima del jugador

    // --- Control de feedback por daño ---
    bool invulnerable_ = false;      // evita recursividad durante parpadeo
};

#endif // PLAYERITEM_H
