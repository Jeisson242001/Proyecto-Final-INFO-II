#ifndef ENEMYITEM_H
#define ENEMYITEM_H

#pragma once
#include <QGraphicsPixmapItem>
#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPixmap>

class QSoundEffect; // forward declaration

class EnemyItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    // frames = animación normal; deathFrames = animación de muerte
    explicit EnemyItem(const QStringList &frames = QStringList(),
                       const QStringList &deathFrames = QStringList(),
                       bool movable = false,
                       QGraphicsItem *parent = nullptr);
    ~EnemyItem() override;

    // infligir daño (por ejemplo 1 por bala)
    void takeDamage(int dmg, bool explosive = false);

    bool isCrouching() const { return crouching_; }

    int crouchProtectionHeight() const {
        if (!pausePixmap_.isNull()) return pausePixmap_.height();
        if (!pixmap().isNull()) return static_cast<int>(pixmap().height() * 0.6); // fallback ~60% del sprite
        return 90; // último recurso: valor seguro
    }

    bool isAlive() const { return !dying_; }

    // API añadida para crouch
    void startCrouch();
    void stopCrouch();

    // permitir asignar pixmap de agachado desde fuera
    void setCrouchPixmap(const QPixmap &pix);

    // asignar secuencia de animación de muerte por explosión (sprites)
    void setExplosiveDeathFrames(const QStringList &paths);

signals:
    void enemyDefeated(EnemyItem *enemy);

private slots:
    void onAnimTick();
    void onMoveTick();
    void onDeathTick();

private:
    // stats / flags
    int health_;
    bool movable_;
    bool dying_; // true mientras reproduce anim de muerte

    // movimiento
    int direction_;
    double speed_;
    QTimer *moveTimer_;

    // animación normal
    QVector<QPixmap> frames_;
    QTimer *animTimer_;
    int currentFrame_;
    int animIntervalMs_;

    // muerte
    QVector<QPixmap> deathFrames_;
    QTimer *deathTimer_;
    int currentDeathFrame_;
    int deathIntervalMs_;

    QVector<QPixmap> explosiveDeathFrames_;
    int explosiveDeathIntervalMs_ = 140; // puedes ajustar distinto si quieres

    // pause-frame (opcional) — si no lo usas, queda vacío
    QPixmap pausePixmap_;

    // sonido de muerte
    QSoundEffect *deathSound_;

    // estado de crouch
    bool crouching_ = false;

    // offset dedicado cuando se aplica el sprite de crouch
    QPointF crouchOffset_;

    void loadFramesFromList(const QStringList &paths, QVector<QPixmap> &out, const QSize &targetSize = QSize());
    void applyFramePixmap(const QPixmap &pix); // helper para aplicar pixmap y offset
};

#endif // ENEMYITEM_H
