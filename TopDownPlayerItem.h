#ifndef TOPDOWNPLAYERITEM_H
#define TOPDOWNPLAYERITEM_H

#pragma once
#include <QObject>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPointF>

// Heredamos de QObject (para señales) y QGraphicsPixmapItem (para visuales)
class TopDownPlayerItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    explicit TopDownPlayerItem(QGraphicsItem *parent = nullptr);
    ~TopDownPlayerItem() override = default;

    // API compatible con la lógica del juego
    void resetLives(int lives = 6);
    void notifyFired();

    bool isAlive() const { return lives_ > 0; }
    int getLives() const { return lives_; }

    // Movimiento controlado externamente (desde GameWindow)
    void setMoveX(double vx) { vx_ = vx; }
    void setMoveY(double vy) { vy_ = vy; }
    void stopX() { vx_ = 0.0; }
    void stopY() { vy_ = 0.0; }

    // Actualizar cada frame desde GameWindow::onTick(dt)
    void updateFrame(double dt);
    QPainterPath shape() const override;

    // Dirección de disparo (unit vector). Por defecto mantiene la última dirección.
    QPointF facingDirection() const;

    // Recibir daño
    void takeDamage(int amount = 1);

    void setSpeed(double s) { speed_ = s; }
    double speed() const { return speed_; }

signals:
    void playerFired();
    void playerHealthChanged(int lives);
    void playerDied();

private:
    // Físicas
    double vx_ = 0.0;
    double vy_ = 0.0;
    double speed_ = 260.0; // px/s
    QPointF facing_ = QPointF(1.0, 0.0);

    int lives_ = 6;

    // --- NUEVO: Variables para guardar los sprites ---
    QPixmap alivePixmap_;
    QPixmap deadPixmap_;
};

#endif // TOPDOWNPLAYERITEM_H
