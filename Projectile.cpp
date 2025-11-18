#include "Projectile.h"
#include "EnemyItem.h"

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

#include <BunkerBossItem.h>

QSoundEffect* ProjectileItem::explosionSound_ = nullptr;
QPixmap* ProjectileItem::explosionPixmapPtr_ = nullptr;


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

    // carga perezosa del sprite de explosión (solo después de que exista QApplication)
    if (!explosionPixmapPtr_) {
        QPixmap px(":/images/images/granada_explosion.png"); // ajusta ruta si hace falta
        if (!px.isNull()) {
            // guardamos en heap un QPixmap ya escalado, listo para reutilizar
            explosionPixmapPtr_ = new QPixmap(px.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            qDebug() << "Projectile: explosion sprite NOT found at :/images/images/granada_explosion.png";
            // dejamos explosionPixmapPtr_ == nullptr si no existe
        }
    }


    if (!explosionSound_) {
        explosionSound_ = new QSoundEffect(qApp);
        explosionSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/granada.wav")));
        explosionSound_->setLoopCount(1);
        explosionSound_->setVolume(0.9f);
        //explosionSound_->play();
        //explosionSound_->stop();
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
    if (!exploded_ && p.y() >= 480) {
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
    // guard: si ya explotó, no volver a ejecutar
    if (exploded_) return;
    exploded_ = true;

    // parar timer para evitar que onStep vuelva a llamar a explode()
    if (timer) {
        timer->stop();
        disconnect(timer, &QTimer::timeout, this, &ProjectileItem::onStep);
    }

    if (explosionSound_) {
        explosionSound_->play();
    }

    // Si tenemos sprite de explosión válido, crear QGraphicsPixmapItem
    QGraphicsPixmapItem *expSprite = nullptr;
    if (explosionPixmapPtr_ && !explosionPixmapPtr_->isNull()) {
        expSprite = new QGraphicsPixmapItem(*explosionPixmapPtr_);
        expSprite->setOffset(-explosionPixmapPtr_->width()/2, -explosionPixmapPtr_->height()/2);
        expSprite->setPos(pos());
        expSprite->setZValue(60);
        if (scene_) scene_->addItem(expSprite);
    } else {
        // fallback visual: el círculo que ya tenías
        QGraphicsEllipseItem *e = new QGraphicsEllipseItem(-160/2, -160/2, 160, 160);
        e->setPos(pos());
        e->setBrush(QBrush(QColor(255,140,0,140)));
        e->setPen(QPen(Qt::NoPen));
        if (scene_) scene_->addItem(e);
        // programar la eliminación del círculo (igual que el sprite)
        QTimer::singleShot(300, [e]() {
            if (e->scene()) e->scene()->removeItem(e);
            delete e;
        });
    }

    // daño radial (igual que antes)
    const double R = 80.0; // radio de explosion
    const double baseDamage = 100.0;

    if (scene_) {
        QList<QGraphicsItem*> items = scene_->items(QRectF(pos().x()-R, pos().y()-R, R*2, R*2));
        for (QGraphicsItem *it : items) {
            if (!it || it == this) continue;
            EnemyItem *enemy = dynamic_cast<EnemyItem*>(it);
            if (enemy && enemy->isAlive()) {
                QPointF center = enemy->pos();
                double d = QLineF(center, pos()).length();
                double factor = 1.0 - (d / R);
                if (factor <= 0.0) continue;
                int damage = qMax(1, static_cast<int>(std::round(baseDamage * factor)));
                enemy->takeDamage(damage, true);
            }

            // --- AÑADIR ESTE BLOQUE NUEVO ---
            BunkerBossItem *bunker = dynamic_cast<BunkerBossItem*>(it);
            if (bunker && bunker->isAlive()) {
                QPointF center = bunker->pos(); // (O ajusta al centro real)
                double d = QLineF(center, pos()).length();
                double factor = 1.0 - (d / R);
                if (factor <= 0.0) continue;

                int damage = qMax(1, static_cast<int>(std::round(baseDamage * factor)));
                bunker->takeDamage(damage); // Aplicar daño de explosión
            }
            // --- FIN DEL BLOQUE NUEVO ---
        }
    }

    // eliminar la granada (visual y objeto)
    if (scene_) scene_->removeItem(this);
    deleteLater();

    // eliminar sprite de explosión tras un corto tiempo
    if (expSprite) {
        QTimer::singleShot(300, [expSprite]() {
            if (expSprite->scene()) expSprite->scene()->removeItem(expSprite);
            delete expSprite;
        });
    }
}



