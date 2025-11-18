#include "BunkerBossItem.h"
#include "PlayerItem.h"
#include "Bullet.h"
#include <QGraphicsScene>
#include <QTimer>
#include <QDebug>
#include <QtMath>
#include <QSoundEffect>
#include <QUrl>
#include <QCoreApplication>

BunkerBossItem::BunkerBossItem(QGraphicsScene *scene, QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent), scene_(scene)
{
    // --- ¡IMPORTANTE! MODIFICADO ---
    // Carga los 3 sprites en lugar de uno solo.
    // ¡¡Asegúrate de que las rutas a tus 3 sprites sean correctas!!

    idlePixmap_ = QPixmap(":/images/images/bunker_quieto.png");     // <--- CAMBIA ESTA RUTA
    shootPixmap_ = QPixmap(":/images/images/bunker_disparando.png");   // <--- CAMBIA ESTA RUTA
    destroyedPixmap_ = QPixmap(":/images/images/bunker_destruido.png"); // <--- CAMBIA ESTA RUTA

    // Fallback por si no encuentra el idle
    if (idlePixmap_.isNull()) {
        qDebug() << "ERROR: No se encontró el sprite 'bunker_idle.png'.";
        idlePixmap_ = QPixmap(150, 100);
        idlePixmap_.fill(Qt::darkGray);
    }

    // Fallback (opcional) para los otros
    if (shootPixmap_.isNull()) {
        qDebug() << "ADVERTENCIA: No se encontró 'bunker_shoot.png', se usará 'idle'.";
        shootPixmap_ = idlePixmap_;
    }
    if (destroyedPixmap_.isNull()) {
        qDebug() << "ADVERTENCIA: No se encontró 'bunker_destroyed.png', se usará 'idle'.";
        destroyedPixmap_ = idlePixmap_;
    }

    // Establecer el sprite inicial
    setPixmap(idlePixmap_);

    // Alineamos los "pies" del búnker con su pos().y
    // (Asegúrate de que todos tus sprites tengan la misma altura)
    setOffset(-idlePixmap_.width() / 2, -idlePixmap_.height());
    setZValue(14);

    shootTimer_ = new QTimer(this);
    // Conectamos el timer de disparo al slot 'onShoot'
    connect(shootTimer_, &QTimer::timeout, this, &BunkerBossItem::onShoot);
}

BunkerBossItem::~BunkerBossItem()
{
    if (shootTimer_) shootTimer_->stop();
}

void BunkerBossItem::takeDamage(int dmg)
{
    if (dying_ || !active_ || !isAlive()) return;

    health_ -= dmg;
    setOpacity(0.6);
    QTimer::singleShot(120, this, [this]() { setOpacity(1.0); });

    if (health_ <= 0) {
        health_ = 0;
        dying_ = true;
        stopAttacking();

        qDebug() << "Búnker destruido!";

        // --- ¡NUEVO! Poner el sprite de destruido ---
        if (!destroyedPixmap_.isNull()) {
            setPixmap(destroyedPixmap_);
            // Ajusta el offset si el sprite destruido tiene otra altura
            setOffset(-destroyedPixmap_.width() / 2, -destroyedPixmap_.height());
        }

        // Sonido de explosión (como lo teníamos)
        static QSoundEffect* explosionSound_ = nullptr;
        if (!explosionSound_) {
            explosionSound_ = new QSoundEffect(qApp);
            explosionSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/granada.wav")));
            explosionSound_->setLoopCount(1);
            explosionSound_->setVolume(1.0f);
        }
        explosionSound_->play();

        // Emitimos la señal INMEDIATAMENTE para que GameWindow sepa que ganaste
        emit bunkerDefeated();

        // Retrasamos la eliminación 1.5 segundos para que se vea el sprite destruido
        QTimer::singleShot(1500, this, [this](){ // <--- TIEMPO AUMENTADO
            if (scene()) scene_->removeItem(this);
            deleteLater();
        });
    }
}

void BunkerBossItem::startAttacking(PlayerItem *player)
{
    // ... (Esta función se queda igual)
    if (active_ || dying_ || !isAlive()) return;
    qDebug() << "BunkerBoss: INICIANDO ATAQUE.";
    active_ = true;
    playerTarget_ = player;
    shootTimer_->start(450);
}

void BunkerBossItem::stopAttacking()
{
    // ... (Esta función se queda igual)
    active_ = false;
    if (shootTimer_) shootTimer_->stop();
}

void BunkerBossItem::onShoot()
{
    if (!active_ || !playerTarget_ || !scene_ || !playerTarget_->isAlive()) {
        stopAttacking();
        return;
    }

    // --- ¡NUEVO! Cambiar al sprite de disparo ---
    if (!shootPixmap_.isNull()) {
        setPixmap(shootPixmap_);
    }

    // --- ¡NUEVO! Programar la vuelta al sprite 'idle' ---
    // (200ms = 0.2 segundos. Ajusta este valor para un "fogonazo" más largo o corto)
    QTimer::singleShot(200, this, &BunkerBossItem::onShootAnimationFinished);


    // --- El resto de la lógica de disparo (se queda igual) ---
    QPointF from = pos() + QPointF(0, -150);
    QPointF target = playerTarget_->pos() + QPointF(0, -20);
    QPointF dir = target - from;
    double len = std::hypot(dir.x(), dir.y());
    if (len > 0.0001) dir /= len;
    else dir = QPointF(-1, 0);

    BulletItem *b = new BulletItem(dir, 450.0, scene_, BulletItem::Owner::Enemy);

    QPixmap enemyBulletPix(":/images/images/bala_enemigo.png");
    if (!enemyBulletPix.isNull()) {
        b->setPixmap(enemyBulletPix.scaled(20, 10, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        b->setOffset(-b->pixmap().width()/2, -b->pixmap().height()/2);
    }

    b->setPos(from);
    scene_->addItem(b);
    emit bunkerFired();
}

// --- ¡NUEVO! Slot para el fogonazo ---
void BunkerBossItem::onShootAnimationFinished()
{
    // Si no estamos muertos o muriendo, volvemos al sprite idle
    if (!dying_ && !idlePixmap_.isNull()) {
        setPixmap(idlePixmap_);
    }
}
