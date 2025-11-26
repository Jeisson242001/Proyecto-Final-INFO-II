#include "TopDownPlayerItem.h"
#include <QBrush>
#include <QGraphicsScene>
#include <QtMath>
#include <QDebug>
#include <QTimer>

// 1. CONSTRUCTOR: Carga de Imágenes
TopDownPlayerItem::TopDownPlayerItem(QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent) // <--- CAMBIO: QGraphicsPixmapItem
{
    // 1. CARGAR SPRITES
    QPixmap rawAlive(":/images/images/jugador_vivo.png");
    QPixmap rawDead(":/images/images/jugador_muerto.png");

    int sizeAlive = 70; // Tamaño normal (vivo)
    int sizeDead = 200;  // <--- CAMBIO: Hacemos al muerto MÁS GRANDE

    // 2. ESCALAR Y GUARDAR (VIVO) - Se queda en 40
    if (!rawAlive.isNull()) {
        alivePixmap_ = rawAlive.scaled(sizeAlive, sizeAlive, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        alivePixmap_ = QPixmap(sizeAlive, sizeAlive);
        alivePixmap_.fill(QColor(60, 140, 220));
    }

    // 3. ESCALAR Y GUARDAR (MUERTO) - Usamos el tamaño grande
    if (!rawDead.isNull()) {
        // Usamos sizeDead (70) para que se vea bien el cuerpo tirado
        deadPixmap_ = rawDead.scaled(sizeDead, sizeDead, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        deadPixmap_ = QPixmap(sizeDead, sizeDead);
        deadPixmap_.fill(Qt::darkGray);
    }

    // Configuración Inicial
    setPixmap(alivePixmap_);

    // Centrar el Pivote (Importante para que rote sobre su eje)
    setOffset(-sizeAlive/2.0, -sizeAlive/2.0);

    setTransformationMode(Qt::SmoothTransformation);
    setZValue(20); // Capa superior

    lives_ = 6;
}

void TopDownPlayerItem::resetLives(int lives)
{
    lives_ = lives;
    // Al reiniciar, volvemos a poner la imagen de vivo
    setPixmap(alivePixmap_);
    setOffset(-alivePixmap_.width()/2.0, -alivePixmap_.height()/2.0);

    emit playerHealthChanged(lives_);
}

void TopDownPlayerItem::notifyFired()
{
    emit playerFired();
}

QPointF TopDownPlayerItem::facingDirection() const
{
    if (qFuzzyIsNull(facing_.x()) && qFuzzyIsNull(facing_.y())) return QPointF(1,0);
    double len = std::hypot(facing_.x(), facing_.y());
    if (len < 1e-6) return QPointF(1,0);
    return QPointF(facing_.x()/len, facing_.y()/len);
}

// 2. UPDATE FRAME: Movimiento, Colisiones y Rotación
void TopDownPlayerItem::updateFrame(double dt)
{
    if (lives_ <= 0) return;

    // --- Calcular movimiento deseado ---
    double mx = vx_;
    double my = vy_;

    // Normalizar diagonal
    if (!qFuzzyIsNull(mx) && !qFuzzyIsNull(my)) {
        double norm = std::hypot(mx, my);
        if (norm > 0.0) {
            mx = (mx / norm) * speed_;
            my = (my / norm) * speed_;
        }
    }

    // --- Mover en X y checar colisión ---
    qreal dx = mx * dt;
    if (!qFuzzyIsNull(dx)) {
        setPos(x() + dx, y());

        QList<QGraphicsItem*> hits = collidingItems();
        for (auto* item : hits) {
            if (item->data(0).toString() == "cover") {
                setPos(x() - dx, y()); // Revertir X
                break;
            }
        }
    }

    // --- Mover en Y y checar colisión ---
    qreal dy = my * dt;
    if (!qFuzzyIsNull(dy)) {
        setPos(x(), y() + dy);

        QList<QGraphicsItem*> hits = collidingItems();
        for (auto* item : hits) {
            if (item->data(0).toString() == "cover") {
                setPos(x(), y() - dy); // Revertir Y
                break;
            }
        }
    }

    // --- Actualizar Facing y ROTAR Sprite ---
    if (!qFuzzyIsNull(vx_) || !qFuzzyIsNull(vy_)) {
        double fx = vx_;
        double fy = vy_;
        double len = std::hypot(fx, fy);
        if (len > 1e-6) {
            facing_ = QPointF(fx/len, fy/len);
        }

        // NUEVO: Rotar la imagen hacia donde camina
        qreal angle = qRadiansToDegrees(qAtan2(facing_.y(), facing_.x()));
        setRotation(angle + 90);
    }

    // --- Límites del mapa ---
    if (scene()) {
        QRectF s = scene()->sceneRect();

        // CAMBIO: Usamos pixmap().width() en lugar de rect().width()
        qreal halfW = 0;
        qreal halfH = 0;
        if (!pixmap().isNull()) {
            halfW = pixmap().width() / 2.0;
            halfH = pixmap().height() / 2.0;
        }

        qreal newX = qBound(s.left()+halfW, pos().x(), s.right()-halfW);
        qreal newY = qBound(s.top()+halfH, pos().y(), s.bottom()-halfH);
        setPos(newX, newY);
    }
}

// 3. TAKE DAMAGE: Cambio de Sprite al morir
void TopDownPlayerItem::takeDamage(int amount)
{
    if (lives_ <= 0) return;
    lives_ -= amount;
    if (lives_ < 0) lives_ = 0;

    // Parpadeo visual
    setOpacity(0.5);
    QTimer::singleShot(150, this, [this]() { setOpacity(1.0); });

    emit playerHealthChanged(lives_);

    if (lives_ == 0) {
        // --- NUEVO: Poner sprite de muerto ---
        setPixmap(deadPixmap_);
        setOffset(-deadPixmap_.width()/2.0, -deadPixmap_.height()/2.0);

        emit playerDied();
    }
}

// Agrega esto al final de TopDownPlayerItem.cpp

QPainterPath TopDownPlayerItem::shape() const
{
    QPainterPath path;
    // Creamos un círculo de radio 15 (30x30 total) centrado.
    // Al ser un poco más pequeño que la imagen (40x40), evitas roces molestos.
    path.addEllipse(-15, -15, 30, 30);
    return path;
}
