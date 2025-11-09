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
    void takeDamage(int dmg);

    bool isAlive() const { return !dying_; }

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

    // movement
    int direction_;
    double speed_;
    QTimer *moveTimer_;

    // animation normal
    QVector<QPixmap> frames_;
    QTimer *animTimer_;
    int currentFrame_;
    int animIntervalMs_;

    // death animation
    QVector<QPixmap> deathFrames_;
    QTimer *deathTimer_;
    int currentDeathFrame_;
    int deathIntervalMs_;

    // pause-frame (opcional) — si no lo usas, queda vacío
    QPixmap pausePixmap_;

    // sonido de muerte
    QSoundEffect *deathSound_;

    void loadFramesFromList(const QStringList &paths, QVector<QPixmap> &out, const QSize &targetSize = QSize());
    void applyFramePixmap(const QPixmap &pix); // helper para aplicar pixmap y offset
};

#endif // ENEMYITEM_H
