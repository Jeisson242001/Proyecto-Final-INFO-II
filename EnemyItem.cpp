#include "EnemyItem.h"
#include <QPixmap>
#include <QGraphicsScene>
#include <QDebug>
#include <QPainter>
#include <QSoundEffect>
#include <QUrl>

EnemyItem::EnemyItem(const QStringList &frames, const QStringList &deathFrames, bool movable, QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent),
    health_(3),
    movable_(movable),
    dying_(false),
    direction_(1),
    speed_(1.6),
    moveTimer_(nullptr),
    animTimer_(nullptr),
    currentFrame_(0),
    animIntervalMs_(120),
    deathTimer_(nullptr),
    currentDeathFrame_(0),
    deathIntervalMs_(140),
    deathSound_(nullptr)
{
    const QSize targetSize(81, 106); // mantener tamaño igual que el player

    // cargar frames de animación normal y death
    loadFramesFromList(frames, frames_, targetSize);
    loadFramesFromList(deathFrames, deathFrames_, targetSize);

    // si no hay frames normales crear fallback
    if (frames_.isEmpty()) {
        QPixmap fallback(targetSize);
        fallback.fill(Qt::transparent);
        QPainter p(&fallback);
        p.setBrush(QBrush(QColor(120,120,120)));
        p.setPen(Qt::black);
        p.drawRect(0,0,targetSize.width()-1,targetSize.height()-1);
        p.end();
        frames_.append(fallback);
    }

    // aplicar primer frame (centro horizontal, base en suelo)
    applyFramePixmap(frames_.at(0));
    setZValue(15);

    // animación normal
    if (frames_.size() > 1) {
        animTimer_ = new QTimer(this);
        animTimer_->setInterval(animIntervalMs_);
        connect(animTimer_, &QTimer::timeout, this, &EnemyItem::onAnimTick);
        animTimer_->start();
    }

    // movimiento opcional
    if (movable_) {
        moveTimer_ = new QTimer(this);
        connect(moveTimer_, &QTimer::timeout, this, &EnemyItem::onMoveTick);
        moveTimer_->start(1000/60);
    }

    // Cargar pausePixmap_ si se quiere (no obligatorio aquí; puedes asignarlo externamente)
    // pausePixmap_ = QPixmap(":/images/images/enemigo_pause.png").scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Inicializar sonido de muerte (ajusta la ruta a tu .qrc)
    deathSound_ = new QSoundEffect(this);
    deathSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/muerte_enemigo.wav")));
    deathSound_->setLoopCount(1);
    deathSound_->setVolume(0.8f);

}

EnemyItem::~EnemyItem()
{
    if (moveTimer_) moveTimer_->stop();
    if (animTimer_) animTimer_->stop();
    if (deathTimer_) deathTimer_->stop();
    // deathSound_ se libera por parent=this
}

// helper: cargar QPixmaps desde lista de rutas
void EnemyItem::loadFramesFromList(const QStringList &paths, QVector<QPixmap> &out, const QSize &targetSize)
{
    out.clear();
    for (const QString &p : paths) {
        QPixmap pix(p);
        if (pix.isNull()) {
            qDebug() << "EnemyItem::loadFrames - frame not found:" << p;
            continue;
        }
        QPixmap scaled = pix;
        if (!targetSize.isEmpty()) {
            scaled = pix.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        out.append(scaled);
    }
}

// helper para aplicar pixmap y ajustar offset (pies alineados con pos().y())
void EnemyItem::applyFramePixmap(const QPixmap &pix)
{
    if (pix.isNull()) return;
    setPixmap(pix);
    // punto de referencia en la base del sprite (pies en el suelo)
    setOffset(-pixmap().width()/2, -pixmap().height());
}

// animación normal: cambia frame (ahora con pausa si completa ciclo)
void EnemyItem::onAnimTick()
{
    if (dying_) return; // no animar normal si está muriendo
    if (frames_.isEmpty() || !animTimer_) return;

    currentFrame_++;

    // si todavía hay frame válido, aplicar y salir
    if (currentFrame_ < frames_.size()) {
        applyFramePixmap(frames_.at(currentFrame_));
        return;
    }

    // llegamos al final del ciclo:
    // 1) si existe pausePixmap_ mostrarlo; si no, mantener último frame
    if (!pausePixmap_.isNull()) {
        applyFramePixmap(pausePixmap_);
    }

    // 2) parar animTimer_ y programar reanudación tras la pausa
    animTimer_->stop();
    currentFrame_ = 0; // reiniciamos contador para el próximo ciclo

    const int pauseMs = 1500; // duración de la pausa entre ciclos (ajusta)
    QTimer::singleShot(pauseMs, this, [this]() {
        if (dying_) return; // si murió durante la pausa, no reiniciamos
        if (!frames_.isEmpty()) applyFramePixmap(frames_.at(0));
        if (animTimer_) animTimer_->start(animIntervalMs_);
    });
}

// movimiento (si movable_)
void EnemyItem::onMoveTick()
{
    if (!scene() || dying_) return;
    QRectF s = scene()->sceneRect();
    QPointF p = pos();
    p.rx() += direction_ * speed_;
    setPos(p);

    // rebote sencillo en bordes
    if (p.x() < s.left() + 50) direction_ = 1;
    else if (p.x() > s.right() - 50) direction_ = -1;
}

// tomar daño; si life <= 0 iniciar animación de muerte
void EnemyItem::takeDamage(int dmg)
{
    if (dying_) return; // ya en death sequence, ignorar
    health_ -= dmg;

    // feedback visual corto
    setOpacity(0.6);
    QTimer::singleShot(120, this, [this]() { setOpacity(1.0); });

    if (health_ > 0) return;

    // iniciar animación de muerte
    dying_ = true;

    // reproducir sonido de muerte (si está cargado)
    if (deathSound_) {
        deathSound_->play();
    }

    // parar animación y movimiento normales
    if (animTimer_) { animTimer_->stop(); delete animTimer_; animTimer_ = nullptr; }
    if (moveTimer_) { moveTimer_->stop(); delete moveTimer_; moveTimer_ = nullptr; }

    // si no hay deathFrames, reproducir sonido y eliminar tras delay
    if (deathFrames_.isEmpty()) {
        if (deathSound_) deathSound_->play(); // aseguramos reproducción aquí
        QTimer::singleShot(1200, this, [this]() { // esperar duración del sonido
            emit enemyDefeated(this);
            if (scene()) scene()->removeItem(this);
            deleteLater();
        });
        return;
    }


    // arrancar timer de muerte
    currentDeathFrame_ = 0;
    applyFramePixmap(deathFrames_.at(0));
    deathTimer_ = new QTimer(this);
    deathTimer_->setInterval(deathIntervalMs_);
    connect(deathTimer_, &QTimer::timeout, this, &EnemyItem::onDeathTick);
    deathTimer_->start();
}

// llamado en cada tick de la animación de muerte
void EnemyItem::onDeathTick()
{
    if (deathFrames_.isEmpty()) return;
    currentDeathFrame_++;
    if (currentDeathFrame_ < deathFrames_.size()) {
        applyFramePixmap(deathFrames_.at(currentDeathFrame_));
        return;
    }

    // terminada la animación de muerte
    if (deathTimer_) { deathTimer_->stop(); delete deathTimer_; deathTimer_ = nullptr; }

    // emitir señal y eliminar
    emit enemyDefeated(this);
    if (scene()) scene()->removeItem(this);
    deleteLater();
}
