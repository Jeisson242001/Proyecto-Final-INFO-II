#include "PlayerItem.h"
#include <QPixmap>
#include <QtMath>
#include <QTransform>
#include <QTimer>

PlayerItem::PlayerItem(QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent)
{
    loadFrames();
    if (!idleFrames.isEmpty()) {
        setFrameAndOffset(idleFrames[0], true);
    }
    currentMaxSpeed = maxSpeed;
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
    runFrames.append(QPixmap(":/images/images/Soldado4.png"));
    runFrames.append(QPixmap(":/images/images/Soldado2.png"));

    // intentar cargar frames de agachado (si existen en tu qrc)
    crouchFrames.clear();
    QPixmap c1(":/images/images/Soldado_abajo.png");
    if (!c1.isNull()) {
        crouchFrames.append(c1);
    } else {
        // fallback: crear versión escalada del idle para simular agachado
        if (!idleFrames.isEmpty() && !idleFrames[0].isNull()) {
            QPixmap scaled = idleFrames[0].scaled(idleFrames[0].width(), qMax(1, idleFrames[0].height() * 60 / 100),
                                                  Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            crouchFrames.append(scaled);
        }
    }

    // --- Crear versiones volteadas (mirror) una sola vez para rendimiento ---
    idleFramesFlipped.clear();
    for (const QPixmap &p : idleFrames) {
        if (p.isNull()) {
            idleFramesFlipped.append(QPixmap());
            continue;
        }
        QTransform t; t.scale(-1, 1);
        idleFramesFlipped.append(p.transformed(t));
    }

    runFramesFlipped.clear();
    for (const QPixmap &p : runFrames) {
        if (p.isNull()) {
            runFramesFlipped.append(QPixmap());
            continue;
        }
        QTransform t; t.scale(-1, 1);
        runFramesFlipped.append(p.transformed(t));
    }

    crouchFramesFlipped.clear();
    for (const QPixmap &p : crouchFrames) {
        if (p.isNull()) {
            crouchFramesFlipped.append(QPixmap());
            continue;
        }
        QTransform t; t.scale(-1, 1);
        crouchFramesFlipped.append(p.transformed(t));
    }

    // === Sprite de salto ===
    jumpFrame_ = QPixmap(":/images/images/Soldado_arriba.png"); // ajusta el nombre a tu archivo real
    if (jumpFrame_.isNull()) {
        qDebug() << "⚠️ No se encontró el sprite de salto";
    } else {
        QTransform t; t.scale(-1, 1);
        jumpFrameFlipped_ = jumpFrame_.transformed(t);
    }

    // === Sprite de muerte (tumbado) ===
    deadPixmap_ = QPixmap(":/images/images/Soldado_muerto.png");
    if (deadPixmap_.isNull()) {
        qDebug() << "⚠️ No se encontró el sprite de jugador muerto (Soldado_muerto.png)";
    } else {
        // opcional: escalarlo al tamaño de los demás (ajusta si hace falta)
        const QSize targetSize(100, 106);
        deadPixmap_ = deadPixmap_.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

}

void PlayerItem::setFrameAndOffset(const QPixmap &pix, bool alignFeet)
{
    if (pix.isNull()) return;
    setPixmap(pix);
    if (alignFeet) {
        // pies en el suelo: offset Y = -height para que pos().y sea la línea de los pies
        setOffset(-pixmap().width()/2, -pixmap().height() + 50);
    } else {
        // centrar verticalmente (tu implementación anterior)
        setOffset(-pixmap().width()/2, -pixmap().height()/2);
    }
}

void PlayerItem::moveLeft()
{
    if (!isAlive()) return; // bloquear movimiento si está muerto
    vx = qMax(vx - accel * (1.0/60.0), -currentMaxSpeed);
}
void PlayerItem::moveRight()
{
    if (!isAlive()) return;
    vx = qMin(vx + accel * (1.0/60.0), currentMaxSpeed);
}
void PlayerItem::stopMoving()
{
    if (!isAlive()) return;
    // dejamos que la fricción reduzca la velocidad en applyPhysics
}

void PlayerItem::jump()
{
    if (!isAlive()) return;
    if (onGround && !crouching) { // no puede saltar agachado
        vy = -jumpImpulse;
        onGround = false;

        // aplicar frame de salto
        const QPixmap &frame = facingLeft ? jumpFrameFlipped_ : jumpFrame_;
        if (!frame.isNull())
            setFrameAndOffset(frame, true);
    }
}

// AGACHARSE
void PlayerItem::startCrouch()
{
    if (!isAlive()) return;
    if (crouching) return;
    crouching = true;
    currentMaxSpeed = crouchMaxSpeed; // reducir velocidad al agacharse
    // aplicar primer frame de crouch (alineado a los pies)
    const QVector<QPixmap> *frames = facingLeft ? &crouchFramesFlipped : &crouchFrames;
    if (frames && !frames->isEmpty()) {
        setFrameAndOffset(frames->at(0), true);
    }
    // reset anim counters para la animación de crouch si la hubiera
    animTime = 0.0;
    currentFrame = 0;
}

void PlayerItem::stopCrouch()
{
    if (!crouching) return;
    crouching = false;
    currentMaxSpeed = maxSpeed;
    // restaurar frame idle (centrado)
    const QVector<QPixmap> *frames = facingLeft ? &idleFramesFlipped : &idleFrames;
    if (frames && !frames->isEmpty()) {
        setFrameAndOffset(frames->at(0), true);
    }
    animTime = 0.0;
    currentFrame = 0;
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

    // guardamos la posición anterior COMPLETA (antes de mover nada)
    QPointF prevPos = pos();

    // -------------------------
    // 1) MOVIMIENTO HORIZONTAL
    // -------------------------
    QPointF tryH = prevPos;
    tryH.rx() += vx * dt;
    setPos(tryH);

    QList<QGraphicsItem*> collH = collidingItems();
    bool horizontalCollision = false;
    QGraphicsRectItem *hitRectH = nullptr;

    for (QGraphicsItem *it : collH) {
        QVariant tag = it->data(0);
        if (tag.isValid() && tag.toString() == QLatin1String("cover")) {
            hitRectH = dynamic_cast<QGraphicsRectItem*>(it);
            if (hitRectH) { horizontalCollision = true; break; }
        }
    }

    if (horizontalCollision && hitRectH) {
        // colocamos al jugador justo al borde del rectángulo para que pueda retroceder.
        const double halfW = static_cast<double>(pixmap().width()) / 2.0;
        QRectF rScene = hitRectH->sceneBoundingRect();
        // Si te movías a la derecha (vx > 0), te posicionamos a la izquierda del cover
        if (vx > 0) {
            double newX = rScene.left() - halfW - 0.5; // 0.5 pixel de margen para evitar solapamiento
            setPos(QPointF(newX, pos().y()));
        }
        // Si te movías a la izquierda (vx < 0), te posicionamos a la derecha del cover
        else if (vx < 0) {
            double newX = rScene.right() + halfW + 0.5;
            setPos(QPointF(newX, pos().y()));
        } else {
            // caso raro: si vx == 0, revertimos a prev X para seguridad
            setPos(QPointF(prevPos.x(), pos().y()));
        }

        // cancelamos la componente hacia el obstáculo, pero permitimos inversión inmediata:
        // si venías hacia la derecha dejamos vx = qMin(vx, 0.0) (efectivamente 0 o negativo es permitido)
        if (vx > 0) vx = 0;
        else if (vx < 0) vx = 0; // si venías hacia la izquierda también lo cancelamos
        // no dejamos vx bloqueado en sentido contrario — futuros inputs cambiarán vx.
    }

    // -------------------------
    // 2) MOVIMIENTO VERTICAL
    // -------------------------
    // Partimos de la posición actual tras resolver X (permite "pararse encima")
    QPointF beforeV = pos();
    QPointF tryV = beforeV;
    tryV.ry() += vy * dt;
    setPos(tryV);

    QList<QGraphicsItem*> collV = collidingItems();
    bool landed = false;
    for (QGraphicsItem *it : collV) {
        QVariant tag = it->data(0);
        if (tag.isValid() && tag.toString() == QLatin1String("cover")) {
            QGraphicsRectItem *r = dynamic_cast<QGraphicsRectItem*>(it);
            if (!r) continue;

            QRectF rScene = r->sceneBoundingRect();
            double topY = rScene.top();

            // línea de pies del jugador: tu convención es pos().y = pies
            // usamos prevPos (antes de mover nada) para ver si venías desde arriba
            double prevFeetY = prevPos.y();
            double tryFeetY = tryV.y();

            // Ajuste visual usado para alinear pies con superficie:
            const double playerFeetOffset = static_cast<double>(pixmap().height()) - 50.0;

            // Si venías cayendo y cruzaste la línea superior del cover -> aterrizaje válido
            if (vy > 0 && prevFeetY <= topY && tryFeetY >= topY) {
                double targetY = topY - playerFeetOffset;
                setPos(QPointF(pos().x(), targetY));
                vy = 0;
                onGround = true;
                landed = true;
                break;
            }

            // Si venías desde abajo (saliendo hacia arriba) y golpeas el techo del cover:
            if (vy < 0 && prevFeetY >= topY && tryFeetY <= topY) {
                // revertir Y (mantenerse) y cancelar vy — evita "pegarse" al techo
                setPos(QPointF(pos().x(), prevPos.y()));
                vy = 0;
                landed = false;
                break;
            }
        }
    }

    // Si no aterrizamos sobre un cover, comprobamos suelo global
    if (!landed) {
        const double groundY = 480.0;
        if (pos().y() >= groundY) {
            setPos(QPointF(pos().x(), groundY));
            vy = 0;
            if (!onGround) {
                onGround = true;
                const QVector<QPixmap> *frames = facingLeft ? &idleFramesFlipped : &idleFrames;
                if (frames && !frames->isEmpty()) {
                    setFrameAndOffset(frames->at(0), true);
                }
            }
        } else {
            onGround = false;
        }
    }

    // NOTA: no forzamos salto en ningún momento; salto solo ocurre al llamar a jump()
}

void PlayerItem::updateAnimation(double dt)
{
    // Primero: actualizar orientación (facing) según velocidad horizontal
    bool newFacingLeft = (vx < -1.0);
    if (newFacingLeft != facingLeft) {
        facingLeft = newFacingLeft;
        animTime = 0.0;
        currentFrame = 0;
    }

    // Si está en el aire -> mantener siempre el frame de salto
    if (!onGround) {
        const QPixmap &frame = facingLeft ? jumpFrameFlipped_ : jumpFrame_;
        if (!frame.isNull()) {
            // usamos alignFeet = true para que los pies sigan en la misma línea de suelo visual
            setFrameAndOffset(frame, true);
        }
        return; // salir: no ejecutar lógica de run/idle/crouch
    }

    // prioridad: si está agachado usamos set de frames de crouch
    if (crouching) {
        const QVector<QPixmap> *frames = facingLeft ? &crouchFramesFlipped : &crouchFrames;
        if (!frames || frames->isEmpty()) return;

        // animación simple: si hay varios frames, ciclarlos lento
        animTime += dt;
        double speed = animSpeed * 3.0;
        if (animTime >= speed) {
            animTime = 0.0;
            currentFrame = (currentFrame + 1) % frames->size();
            setFrameAndOffset(frames->at(currentFrame), true);
        }
        return;
    }

    // Si llegamos aquí, el jugador está en tierra y no agachado -> run / idle normal
    bool isRunning = (qAbs(vx) > 1.0);

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
    if (!isRunning) speed *= 6.0; // idle más lento

    if (animTime >= speed) {
        animTime = 0.0;
        currentFrame = (currentFrame + 1) % frames->size();
        setFrameAndOffset(frames->at(currentFrame), true);
    }
}


void PlayerItem::updateFrame(double dt)
{
    applyPhysics(dt);
    updateAnimation(dt);
}

void PlayerItem::notifyFired()
{
    emit playerFired();
}

void PlayerItem::takeDamage(int amount)
{
    if (lives_ <= 0) return;      // ya muerto, ignorar
    if (invulnerable_) return;    // evita daño repetido durante el parpadeo (opcional)

    // aplicar daño
    lives_ -= amount;
    if (lives_ < 0) lives_ = 0;

    // emitir HUD
    emit playerHealthChanged(lives_);

    // feedback visual: parpadeo corto (igual que EnemyItem)
    // ponemos opacidad baja y la restablecemos rápidamente
    setOpacity(0.6);
    QTimer::singleShot(120, this, [this]() {
        setOpacity(1.0);
    });

    // si quieres prevenir daño inmediato sucesivo, puedes activar invulnerable_ por un corto tiempo:
    invulnerable_ = true;
    QTimer::singleShot(350, this, [this]() { invulnerable_ = false; });

    // si llegó a 0 -> mostrar sprite muerto y emitir playerDied
    if (lives_ == 0) {
        // bloqueamos comportamiento del jugador
        qDebug() << "💀 Player died (PlayerItem)";

        // aplicar sprite muerto (alinear pies)
        if (!deadPixmap_.isNull()) {
            setPixmap(deadPixmap_);
            // offset para que quede "tirado en el piso": puedes ajustar el +10 si quieres bajarlo/ subirlo
            setOffset(-deadPixmap_.width()/2, -deadPixmap_.height() + 100);
        }

        // detener cualquier movimiento físico
        vx = 0;
        vy = 0;

        // notificar al mundo (GameWindow se encargará de gameOver y UI)
        emit playerDied();
    }
}


// en PlayerItem.cpp
void PlayerItem::resetLives(int lives)
{
    lives_ = lives;
    emit playerHealthChanged(lives_);
}

