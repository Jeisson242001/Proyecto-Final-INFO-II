#include "Bullet.h"
#include <QTimer>
#include <QGraphicsScene>
#include <QPixmap>
#include <QtMath>
#include <QDebug>
#include <QGraphicsRectItem>
#include <QVariant>

#include <QSoundEffect>
#include <QCoreApplication>
#include <QUrl>

#include "PlayerItem.h"
#include "EnemyItem.h"

#include <BunkerBossItem.h>

QSoundEffect* BulletItem::shotSound_ = nullptr;

BulletItem::BulletItem(const QPointF &direction, double speed, QGraphicsScene *scene, Owner owner, QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent), dir_(direction), speed_(speed), scene_(scene), owner_(owner)
{
    // normalizar dirección si no es unit vector
    double len = std::hypot(dir_.x(), dir_.y());
    if (len <= 0.0001) {
        dir_ = QPointF(1,0);
    } else {
        dir_ /= len;
    }

    // Cargar pixmap por defecto (puedes sobreescribir desde el caller si quieres)
    QPixmap pix(":/images/images/Bala.png");
    if (!pix.isNull()) {
        setPixmap(pix.scaled(24, 12, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        setOffset(-pixmap().width()/2, -pixmap().height()/2);
    }
    setZValue(50);
    setVisible(true);

    // sonido de disparo (precarga)
    if (!shotSound_) {
        shotSound_ = new QSoundEffect(qApp);
        shotSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/arma_player.wav")));
        shotSound_->setLoopCount(1);
        shotSound_->setVolume(0.9f);
        // precarga: play+stop
        shotSound_->play();
        shotSound_->stop();
    }

    // Si la bala es de enemigo quizá quieras otro sonido; dejamos el control al caller
    if (shotSound_ && owner_ == Owner::Player) {
        shotSound_->play();
    }

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BulletItem::onStep);
    timer->start(1000/60);
    qDebug() << "Bullet: constructed. dir=" << dir_ << " speed=" << speed_ << " owner=" << (owner_==Owner::Player ? "Player" : "Enemy");
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

    // fuera de escena o tiempo
    if (p.x() < -200 || p.x() > 3000 || p.y() < -200 || p.y() > 2000 || elapsed > lifeTime) {
        if (scene_) scene_->removeItem(this);
        deleteLater();
        return;
    }

    QList<QGraphicsItem*> coll = collidingItems();
    for (QGraphicsItem *it : coll) {

        // CONTROL DE "friendly fire" según owner_
        // Si la bala es del jugador -> ignorar colisión con PlayerItem
        if (owner_ == Owner::Player) {
            if (dynamic_cast<PlayerItem*>(it)) continue;
        }
        // Si la bala es de enemigo -> ignorar colisión con EnemyItem
        if (owner_ == Owner::Enemy) {
            if (dynamic_cast<EnemyItem*>(it)) continue;
        }

        // === Caso cobertura ("cover") ===
        QVariant tag = it->data(0);
        if (tag.isValid() && tag.toString() == QLatin1String("cover")) {
            QGraphicsRectItem *r = dynamic_cast<QGraphicsRectItem*>(it);
            if (r) {
                qreal topY = it->pos().y();
                qreal bottomY = topY + r->rect().height();
                qreal bulletY = pos().y();

                if (bulletY >= topY && bulletY <= bottomY) {
                    if (scene_) scene_->removeItem(this);
                    deleteLater();
                    qDebug() << "Bullet blocked by bunker at y" << bulletY;
                    return;
                }
            }
        }

        // === Caso: enemigo ===
        EnemyItem *enemy = dynamic_cast<EnemyItem*>(it);
        if (enemy) {
            // sólo el jugador puede dañar enemigos
            if (owner_ == Owner::Player) {
                if (!enemy->isAlive()) continue;

                // Si el enemigo está agachado, comprobamos si la bala impacta por encima
                if (enemy->isCrouching()) {
                    // calculamos línea superior de la "protección" del enemigo
                    // pos().y() del enemy es la línea de los pies; la parte protegida va desde (feetY - protectionHeight) hasta feetY
                    double feetY = enemy->pos().y();
                    double protectionH = static_cast<double>(enemy->crouchProtectionHeight());
                    double protectionTopY = feetY - protectionH;
                    double bulletY = pos().y();

                    // Si la bala está por debajo o dentro de la zona protegida -> se bloquea (no daño)
                    // recordá: en coordenadas Qt, y aumenta hacia abajo
                    if (bulletY >= protectionTopY && bulletY <= feetY) {
                        // opcional: efecto de ricochet (qDebug o sonido)
                        qDebug() << "Bullet hit crouched enemy (blocked) at y" << bulletY;
                        if (scene_) scene_->removeItem(this);
                        deleteLater();
                        return;
                    }
                    // si la bala vino por encima de protectionTopY, sigue y aplica daño abajo
                }

                // Si no está agachado (o la bala impactó por encima de la protección), aplicar daño
                enemy->takeDamage(1);
                if (scene_) scene_->removeItem(this);
                deleteLater();
                return;
            }
            // owner == Enemy -> ignorar daño a otros enemigos
            continue;
        }

        // --- AÑADIR ESTE BLOQUE NUEVO ---
        BunkerBossItem *bunker = dynamic_cast<BunkerBossItem*>(it);
        if (bunker) {
            // sólo el jugador puede dañar al búnker
            if (owner_ == Owner::Player) {
                bunker->takeDamage(1); // Daño de la bala

                if (scene_) scene_->removeItem(this);
                deleteLater();
                return;
            }
            continue;
        }
        // --- FIN DEL BLOQUE NUEVO ---


        // === Caso: player ===
        PlayerItem *player = dynamic_cast<PlayerItem*>(it);
        if (player) {
            // sólo las balas de ENEMIGO deben dañar al jugador
            if (owner_ == Owner::Enemy) {
                qDebug() << "Player hit by enemy bullet!";
                player->takeDamage(1);  // ⚡️ aquí se aplica el daño real al jugador
                if (scene_) scene_->removeItem(this);
                deleteLater();
                return;
            }
            continue;
        }

    }
}
