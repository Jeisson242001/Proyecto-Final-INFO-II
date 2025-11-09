#ifndef BULLET_H
#define BULLET_H

#pragma once
#include <QGraphicsPixmapItem>
#include <QObject>

class QSoundEffect; // forward

class BulletItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    // direction: normalized unit vector; speed: px/s
    BulletItem(const QPointF &direction, double speed, QGraphicsScene *scene, QGraphicsItem *parent = nullptr);
    ~BulletItem() override;

public slots:
    void onStep();

private:
    QPointF dir_;
    double speed_ = 800.0;
    QTimer *timer = nullptr;
    QGraphicsScene *scene_ = nullptr;
    double lifeTime = 2.0;
    double elapsed = 0.0;

    static QSoundEffect* shotSound_;
};


#endif // BULLET_H
