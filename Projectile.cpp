#include "Projectile.h"

#include <QTimer>
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QPen>
#include <QGraphicsScene>
#include <QDebug>
#include <QtMath>

#include <QSoundEffect>
#include <QCoreApplication>
#include <QUrl>

QSoundEffect* ProjectileItem::explosionSound_ = nullptr;


// constructor...
ProjectileItem::ProjectileItem(double v0, double angleDegrees, QGraphicsScene *scene, QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent), scene_(scene)
{
    // usa imagen de granada si existe, sino no pinta nada
    QPixmap pix(":/images/images/granade.png");
    if (!pix.isNull()) {
        setPixmap(pix.scaled(24,24,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        setOffset(-pixmap().width()/2, -pixmap().height()/2);
    }

    double ang = qDegreesToRadians(angleDegrees);
    vx = v0 * qCos(ang);
    vy = -v0 * qSin(ang); // y hacia abajo es positivo

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ProjectileItem::onStep);
    timer->start(1000/60);

    if (!explosionSound_) {
        explosionSound_ = new QSoundEffect(qApp);
        explosionSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/granada.wav")));
        explosionSound_->setLoopCount(1);
        explosionSound_->setVolume(0.9f);
        explosionSound_->play();
        explosionSound_->stop();
    }

}

ProjectileItem::~ProjectileItem()
{
    if (timer) timer->stop();
}

void ProjectileItem::onStep()
{
    double dt = 1.0/60.0;
    elapsed += dt;

    vy += gravity * dt;

    QPointF p = pos();
    p.rx() += vx * dt;
    p.ry() += vy * dt;
    setPos(p);

    // colisión con suelo (ajusta)
    if (p.y() >= 480) {
        explode();
        return;
    }

    if (elapsed > lifeTime) {
        if (scene_) scene_->removeItem(this);
        deleteLater();
    }
}

void ProjectileItem::explode()
{
    if (explosionSound_) {
        explosionSound_->play();
    }

    // efecto visual (círculo)
    QGraphicsEllipseItem *e = new QGraphicsEllipseItem(-160/2, -160/2, 160, 160);
    e->setPos(pos());
    e->setBrush(QBrush(QColor(255,140,0,140)));
    e->setPen(QPen(Qt::NoPen));   // <-- usar QPen explicitamente
    if (scene_) scene_->addItem(e);

    // daño radial ejemplo
    double R = 80.0; // radio de explosion
    double baseDamage = 100.0;

    if (scene_) {
        QList<QGraphicsItem*> items = scene_->items(QRectF(pos().x()-R, pos().y()-R, R*2, R*2));
        for (QGraphicsItem *it : items) {
            if (it == this) continue;
            QPointF center = it->pos();
            double d = QLineF(center, pos()).length();
            double factor = 1.0 - (d / R);
            if (factor <= 0) continue;
            double damage = baseDamage * factor;
            qDebug() << "Damage to item" << it << "=" << damage;
            // si tu EnemyItem tiene método applyDamage, haz dynamic_cast y llama
        }
    }

    if (scene_) scene_->removeItem(this);
    deleteLater();

    // eliminar visual tras un rato (lambda que no usa receiver)
    QTimer::singleShot(300, [e]() {
        if (e->scene()) e->scene()->removeItem(e);
        delete e;
    });
}
