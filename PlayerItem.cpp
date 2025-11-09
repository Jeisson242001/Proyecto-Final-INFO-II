#include "PlayerItem.h"
#include <QPixmap>
#include <QtMath>

PlayerItem::PlayerItem(QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent)
{
    loadFrames();
    if (!idleFrames.isEmpty()) {
        setPixmap(idleFrames[0]);
        setOffset(-pixmap().width()/2, -pixmap().height()/2);
    }
    setZValue(10);
}

PlayerItem::~PlayerItem() {}

void PlayerItem::loadFrames()
{
    // Cargar frames originales (ajusta nombres según tu .qrc)
    idleFrames.clear();
    idleFrames.append(QPixmap(":/images/images/Soldado.png"));
    idleFrames.append(QPixmap(":/images/images/Soldado.png"));

    runFrames.clear();
    runFrames.append(QPixmap(":/images/images/Soldado1.png"));
    runFrames.append(QPixmap(":/images/images/Soldado2.png"));

    // --- Crear versiones volteadas (mirror) una sola vez para rendimiento ---
    idleFramesFlipped.clear();
    for (const QPixmap &p : idleFrames) {
        if (p.isNull()) {
            idleFramesFlipped.append(QPixmap());
            continue;
        }
        QTransform t;
        t.scale(-1, 1);
        idleFramesFlipped.append(p.transformed(t));
    }

    runFramesFlipped.clear();
    for (const QPixmap &p : runFrames) {
        if (p.isNull()) {
            runFramesFlipped.append(QPixmap());
            continue;
        }
        QTransform t;
        t.scale(-1, 1);
        runFramesFlipped.append(p.transformed(t));
    }
}

void PlayerItem::moveLeft()
{
    vx = qMax(vx - accel * (1.0/60.0), -maxSpeed);
}
void PlayerItem::moveRight()
{
    vx = qMin(vx + accel * (1.0/60.0), maxSpeed);
}
void PlayerItem::stopMoving()
{
    // dejamos que la fricción reduzca la velocidad en applyPhysics
}

void PlayerItem::jump()
{
    if (onGround) {
        vy = -jumpImpulse;
        onGround = false;
    }
}

void PlayerItem::applyPhysics(double dt)
{
    // fricción horizontal
    if (!qFuzzyIsNull(vx)) {
        double sign = vx > 0 ? 1.0 : -1.0;
        double dec = friction * dt;
        if (qAbs(vx) <= dec) vx = 0;
        else vx -= sign * dec;
    }

    // gravedad
    vy += gravity * dt;

    // actualizar posición
    QPointF p = pos();
    p.rx() += vx * dt;
    p.ry() += vy * dt;

    // impacto con suelo (ajusta groundY según tu escena)
    const double groundY = 480.0;
    if (p.y() >= groundY) {
        p.setY(groundY);
        vy = 0;
        onGround = true;
    }

    setPos(p);
}

void PlayerItem::updateAnimation(double dt)
{
    // determinar si el jugador se está moviendo horizontalmente
    bool isRunning = (qAbs(vx) > 1.0);

    // detectar nueva dirección (izquierda/derecha)
    bool newFacingLeft = (vx < -1.0);
    if (newFacingLeft != facingLeft) {
        // la dirección cambió: reiniciar animación para evitar saltos
        facingLeft = newFacingLeft;
        animTime = 0.0;
        currentFrame = 0;
    }

    animTime += dt;

    // seleccionar el conjunto de frames apropiado
    const QVector<QPixmap> *frames = nullptr;
    if (isRunning) {
        frames = facingLeft ? &runFramesFlipped : &runFrames;
    } else {
        frames = facingLeft ? &idleFramesFlipped : &idleFrames;
    }

    if (!frames || frames->isEmpty()) return;

    // velocidad de animación: más rápida al correr, más lenta en idle
    double speed = animSpeed;
    if (!isRunning) speed *= 4.0; // idle más lento

    if (animTime >= speed) {
        animTime = 0.0;
        currentFrame = (currentFrame + 1) % frames->size();
        setPixmap(frames->at(currentFrame));
        // centrar offset siempre después de setPixmap
        setOffset(-pixmap().width()/2, -pixmap().height()/2);
        // no usamos setScale ni transform en el item (ya están volcados los pixmaps)
    }
}


void PlayerItem::updateFrame(double dt)
{
    applyPhysics(dt);
    updateAnimation(dt);
}
