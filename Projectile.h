#ifndef PROJECTILE_H
#define PROJECTILE_H

#pragma once
#include <QGraphicsPixmapItem>
#include <QObject>
#include <QSoundEffect>

class QTimer;
class QGraphicsScene;

class ProjectileItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    ProjectileItem(double v0, double angleDegrees, QGraphicsScene *scene, QGraphicsItem *parent = nullptr);
    ~ProjectileItem() override;

public slots:
    void onStep();

private:
    double vx; // px/s
    double vy; // px/s (positivo hacia abajo)
    const double gravity = 980.0; // px/s^2
    QTimer *timer = nullptr;
    QGraphicsScene *scene_ = nullptr;
    double lifeTime = 10.0;
    double elapsed = 0.0;

    // evita explosiones duplicadas
    bool exploded_ = false;

    void explode();
    static QSoundEffect* explosionSound_;

    // puntero estático: se inicializa nulo en tiempo de carga del módulo (no construye QPixmap)
    static QPixmap *explosionPixmapPtr_;

};


#endif // PROJECTILE_H
