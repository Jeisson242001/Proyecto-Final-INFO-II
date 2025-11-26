#include "TopDownEnemy.h"
#include "TopDownPlayerItem.h"
#include "Bullet.h"
#include <QGraphicsScene>
#include <QtMath>
#include <QRandomGenerator>
#include <QDebug>
#include <QSoundEffect>

TopDownEnemy::TopDownEnemy(TopDownPlayerItem* target, QGraphicsScene* scene, QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent), target_(target), scene_(scene)
{
    // 1. CARGAR IMÁGENES ORIGINALES
    QPixmap rawWalk(":/images/images/enemigo_camina.png");
    QPixmap rawShoot(":/images/images/enemigo_dispara.png");
    QPixmap rawDeath(":/images/images/enemigo_muerto_2.png");

    // 2. ESCALARLAS AL TAMAÑO DESEADO (40x40)
    // Qt::KeepAspectRatio -> Para que no se deforme (no se vea gordo o flaco)
    // Qt::SmoothTransformation -> Para que no se vea pixelado al reducirlo
    if (!rawWalk.isNull()) {
        walkPixmap_ = rawWalk.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        // Fallback: cuadro rojo si no hay imagen
        walkPixmap_ = QPixmap(60, 60);
        walkPixmap_.fill(Qt::red);
    }

    if (!rawShoot.isNull()) {
        shootPixmap_ = rawShoot.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        shootPixmap_ = walkPixmap_; // Si no hay shoot, usa walk
    }

    if (!rawDeath.isNull()) {
        deathPixmap_ = rawDeath.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        // Fallback: Si no tienes imagen, usamos la de caminar pero más oscura
        deathPixmap_ = walkPixmap_;
    }

    // 3. APLICAR LA IMAGEN INICIAL
    setPixmap(walkPixmap_);

    // 4. CENTRAR EL PIVOTE (Igual que antes, pero ahora usa el tamaño correcto)
    setOffset(-pixmap().width() / 2.0, -pixmap().height() / 2.0);

    setTransformationMode(Qt::SmoothTransformation);
    setZValue(15);

    // 2. CONFIGURAR COMPORTAMIENTO (Igual que antes)
    if (QRandomGenerator::global()->bounded(2) == 0) {
        behavior_ = Chaser;
        // El Chaser siempre usa el sprite de caminar porque no para
        setPixmap(walkPixmap_);
    } else {
        behavior_ = Tactical;
        // El Táctico empieza moviéndose
        setPixmap(walkPixmap_);
    }

    // 3. SONIDO Y TIMERS (Igual que antes)
    shotSound_ = new QSoundEffect(this);
    shotSound_->setSource(QUrl("qrc:/sound/sounds/arma_enemigo.wav"));
    shotSound_->setVolume(0.4f);

    deathSound_ = new QSoundEffect(this);
    deathSound_->setSource(QUrl("qrc:/sound/sounds/muerte-enemigo.wav"));
    deathSound_->setVolume(1.0f); // Volumen alto para que se escuche bien

    shootTimer_ = new QTimer(this);
    int shootInterval = 1500 + QRandomGenerator::global()->bounded(1000);
    connect(shootTimer_, &QTimer::timeout, this, &TopDownEnemy::shootAtPlayer);
    shootTimer_->start(shootInterval);

    if (behavior_ == Tactical) {
        stateTimer_ = new QTimer(this);
        connect(stateTimer_, &QTimer::timeout, this, &TopDownEnemy::toggleState);
        stateTimer_->start(2000);
    } else {
        stateTimer_ = nullptr;
    }
}

void TopDownEnemy::takeDamage(int damage)
{
    // Si ya está muerto, ignorar
    if (health_ <= 0) return;

    health_ -= damage;

    if (health_ <= 0) {
        // 1. Notificar muerte
        emit enemyDied(this);

        if (deathSound_) {
            deathSound_->play();
        }

        // 2. Cambiar al sprite de muerto
        setPixmap(deathPixmap_);

        // Ajustar el centro (pivote)
        setOffset(-deathPixmap_.width()/2.0, -deathPixmap_.height()/2.0);

        // 3. Mandar al fondo (pero visible)
        setZValue(10);

        // 4. Parar inteligencia
        if (shootTimer_) shootTimer_->stop();
        if (stateTimer_) stateTimer_->stop();

        // 5. Desactivar colisiones físicas
        setData(0, "dead");

        // 6. Borrar tras 3 segundos
        QTimer::singleShot(3000, this, &TopDownEnemy::deleteLater);

    } else {
        // Efecto de daño (Parpadeo)
        setOpacity(0.5);
        QTimer::singleShot(100, this, [this](){
            if(health_ > 0) setOpacity(1.0);
        });
    }
}

void TopDownEnemy::updateSpriteRotation()
{
    if (!target_) return;

    // Calcular ángulo hacia el jugador
    QLineF line(pos(), target_->pos());
    setRotation(-line.angle() + 180); // En Qt los grados van invertidos a veces dependiendo de la imagen
}

void TopDownEnemy::updateBehavior(double dt)
{
    if (!target_ || health_ <= 0) return;

    // ACTUALIZAR ROTACIÓN (Para que siempre mire al jugador)
    updateSpriteRotation();

    // --- Lógica de Movimiento (Tu código de evasión ya implementado) ---
    bool shouldMove = false;
    if (behavior_ == Chaser) shouldMove = true;
    else if (behavior_ == Tactical) shouldMove = isMoving_;

    if (shouldMove) {
        QPointF myPos = pos();
        QPointF targetPos = target_->pos();
        QPointF diff = targetPos - myPos;
        double len = std::hypot(diff.x(), diff.y());

        if (len > 5.0) {
            QPointF dir = diff / len;
            QPointF step = dir * speed_ * dt;

            setPos(myPos + step);

            if (isCollidingWithCover()) {
                setPos(myPos);
                // ... (Tu lógica de evasión lateral que ya funciona va aquí) ...
                // Copia el bloque de if(abs(x) > abs(y)) que te di en la respuesta anterior
                // para mantener la IA inteligente.
                if (std::abs(dir.x()) > std::abs(dir.y())) {
                    setPos(myPos + QPointF(0, speed_ * dt));
                    if (isCollidingWithCover()) {
                        setPos(myPos);
                        setPos(myPos + QPointF(0, -speed_ * dt));
                        if (isCollidingWithCover()) setPos(myPos);
                    }
                } else {
                    setPos(myPos + QPointF(speed_ * dt, 0));
                    if (isCollidingWithCover()) {
                        setPos(myPos);
                        setPos(myPos + QPointF(-speed_ * dt, 0));
                        if (isCollidingWithCover()) setPos(myPos);
                    }
                }
            }
        }
    }

    // Colisión cuerpo a cuerpo (Igual que antes)
    if (collidesWithItem(target_)) {
        target_->takeDamage(1);
        QPointF dir = pos() - target_->pos();
        double len = std::hypot(dir.x(), dir.y());
        if (len > 0) setPos(pos() + (dir/len) * 15);
    }
}

// Esta función cambia la imagen cuando el Táctico se detiene
void TopDownEnemy::toggleState()
{
    isMoving_ = !isMoving_;

    if (isMoving_) {
        // Si empieza a moverse -> Poner sprite de caminar
        setPixmap(walkPixmap_);
        // Ajustar offset de nuevo por si las imagenes tienen distinto tamaño
        setOffset(-walkPixmap_.width()/2.0, -walkPixmap_.height()/2.0);
    } else {
        // Si se detiene a disparar -> Poner sprite de disparo
        setPixmap(shootPixmap_);
        setOffset(-shootPixmap_.width()/2.0, -shootPixmap_.height()/2.0);
    }
}

void TopDownEnemy::shootAtPlayer()
{
    // ... (Tu lógica de disparo en ráfaga queda EXACTAMENTE IGUAL) ...
    // Solo asegúrate de copiar el contenido de shootAtPlayer que definimos antes.

    if (!target_ || !scene_ || health_ <= 0) return;
    if (!target_->isAlive()) return;
    if (behavior_ == Tactical && isMoving_) return;

    fireBullet();
    QTimer::singleShot(200, this, &TopDownEnemy::fireBullet);
}

void TopDownEnemy::fireBullet()
{
    // ... (Tu lógica de fireBullet queda EXACTAMENTE IGUAL) ...
    // Copia el código que tenías.

    if (!scene() || health_ <= 0 || !target_) return;
    QPointF startPos = pos();
    QPointF targetPos = target_->pos();
    QPointF dir = targetPos - startPos;
    double len = std::hypot(dir.x(), dir.y());
    if (len > 0.001) {
        dir /= len;
        BulletItem *b = new BulletItem(dir, 350.0, scene_, BulletItem::Owner::Enemy);
        QPixmap enemyBulletPix(":/images/images/bala_enemigo.png");
        if (!enemyBulletPix.isNull()) {
            b->setPixmap(enemyBulletPix.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            b->setOffset(-8, -8);
        }
        b->setPos(startPos + dir * 25);
        scene_->addItem(b);
        if (shotSound_) shotSound_->play();
    }
}

bool TopDownEnemy::isCollidingWithCover()
{
    QList<QGraphicsItem*> hits = collidingItems();
    for (QGraphicsItem* item : hits) {
        if (item->data(0).toString() == "cover") {
            return true;
        }
    }
    return false;
}
