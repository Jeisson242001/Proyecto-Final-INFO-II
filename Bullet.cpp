#include "Bullet.h"
#include <QTimer>
#include <QGraphicsScene>
#include <QPixmap>
#include <QtMath>
#include "PlayerItem.h"
#include <QSoundEffect>
#include <QCoreApplication>
#include <QUrl>
#include <EnemyItem.h>

QSoundEffect* BulletItem::shotSound_ = nullptr;

BulletItem::BulletItem(const QPointF &direction, double speed, QGraphicsScene *scene, QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent), dir_(direction), speed_(speed), scene_(scene)
{
    // normalizar dirección si no es unit vector
    double len = std::hypot(dir_.x(), dir_.y());
    if (len <= 0.0001) {
        dir_ = QPointF(1,0);
    } else {
        dir_ /= len;
    }

    // intentar cargar pixmap desde varias rutas posibles


    // Sustituye la parte que carga pixmap / fallback por esta (pegar en Bullet.cpp)
    // intentar cargar pixmap desde varias rutas posibles

    QPixmap pix(":/images/images/Bala.png");


    if (!pix.isNull()) {
        // usa un tamaño un poco mayor para que se vea bien
        setPixmap(pix.scaled(24, 12, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        setOffset(-pixmap().width()/2, -pixmap().height()/2);
    }
    setZValue(50);        // encima de la mayorÃ­a de cosas
    setVisible(true);

    // Inicializar efecto de disparo la primera vez
    if (!shotSound_) {
        shotSound_ = new QSoundEffect(qApp); // qApp es el parent, incluir <QCoreApplication>
        shotSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/arma_player.wav")));
        shotSound_->setLoopCount(1);
        shotSound_->setVolume(0.9f); // 0.0 - 1.0
        // No llamamos play() aún: queremos precargar
        // forcing load: a veces se necesita tocar setLoopCount o play+stop rápido:
        shotSound_->play();
        shotSound_->stop();
    }

    // reproducir efecto al disparar
    if (shotSound_) {
        shotSound_->play();
    }

    // Iniciar timer (pero no mover antes de que el caller ponga la bala en la escena)
    // (timer solo actualiza posición según pos() — start ok)
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BulletItem::onStep);
    timer->start(1000/60);
    qDebug() << "Bullet: constructed. dir=" << dir_ << " speed=" << speed_;
}

BulletItem::~BulletItem()
{
    if (timer) timer->stop();
}

void BulletItem::onStep()
{
    double dt = 1.0/60.0;
    elapsed += dt;

    QPointF p = pos();
    p += dir_ * speed_ * dt;
    setPos(p);

    // quitar si sale del borde
    if (p.x() < -200 || p.x() > 3000 || p.y() < -200 || p.y() > 2000 || elapsed > lifeTime) {
        if (scene_) scene_->removeItem(this);
        deleteLater();
        return;
    }

    // comprobar colisiones simples (items en la zona)
    // comprobar colisiones simples (items en la zona)
    QList<QGraphicsItem*> coll = collidingItems();
    for (QGraphicsItem *it : coll) {

        // Ignorar colisión con el Player
        if (dynamic_cast<PlayerItem*>(it))
            continue;

        // Si colisiona con un enemigo -> infligir daño y destruir la bala
        EnemyItem *enemy = dynamic_cast<EnemyItem*>(it);
        if (enemy) {
            if (!enemy->isAlive()) {
                // si ya está en death sequence, ignorar
                continue;
            }
            enemy->takeDamage(1);
            if (scene_) scene_->removeItem(this);
            deleteLater();
            return;
        }
    }
}
