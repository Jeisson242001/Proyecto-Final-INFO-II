#include "FlameArea.h"
#include "TopDownEnemy.h" // Necesario para dañarlos
#include <QGraphicsScene>
#include <QPolygonF>
#include <QBrush>
#include <QPen>
#include <QtMath>
#include <QTimer>
#include <QDebug>

FlameArea::FlameArea(QPointF direction, QGraphicsScene* scene, QGraphicsItem *parent)
    : QObject(), QGraphicsPolygonItem(parent)
{
    // --- CONFIGURACIÓN ---
    qreal alcance = 180.0;
    qreal apertura = 40.0;
    // ---------------------

    // 1. DEFINIR LA FORMA (El Triángulo)
    // El punto (0,0) es el "cañón" del arma.
    QPolygonF coneShape;
    coneShape << QPointF(0, 0)                 // Origen (Pegado al player)
              << QPointF(alcance, -apertura)   // Punta arriba
              << QPointF(alcance, apertura);   // Punta abajo

    setPolygon(coneShape);
    setPen(Qt::NoPen);

    // 2. TEXTURA (Aquí está la corrección visual)
    QPixmap flamePix(":/images/images/flame.png");

    if (!flamePix.isNull()) {
        // A) Escalar la imagen para que sea del tamaño exacto del área
        // Usamos 'alcance' como ancho y 'apertura*2' como alto total
        QPixmap scaled = flamePix.scaled(static_cast<int>(alcance),
                                         static_cast<int>(apertura * 2),
                                         Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation);

        // B) Crear el Brush con la imagen
        QBrush flameBrush(scaled);

        // C) ¡TRUCO IMPORTANTE! Transformar el Brush
        // Por defecto, el brush empieza a pintar en el (0,0) de la ESCENA, no del objeto.
        // Tenemos que mover el origen de la pintura para que coincida con el objeto.
        // Como nuestro triangulo va de Y=-40 a Y=40, y la imagen va de 0 a 80,
        // necesitamos desplazar la imagen hacia arriba (en Y) la mitad de su altura.
        QTransform brushTransform;
        brushTransform.translate(0, -apertura);
        flameBrush.setTransform(brushTransform);

        setBrush(flameBrush);
    } else {
        // Fallback: Color sólido naranja brillante
        setBrush(QBrush(QColor(255, 69, 0, 200)));
    }

    // 3. Rotación y Posición
    qreal angle = qRadiansToDegrees(qAtan2(direction.y(), direction.x()));
    setRotation(angle);
    setZValue(30);

    // 4. Colisiones (Código igual al anterior)
    QTimer::singleShot(0, this, [this](){
        QList<QGraphicsItem*> overlaps = this->collidingItems(Qt::IntersectsItemShape);
        for (QGraphicsItem* it : overlaps) {
            TopDownEnemy* enemy = dynamic_cast<TopDownEnemy*>(it);
            if (enemy && enemy->isAlive()) {
                enemy->takeDamage(3);
            }
        }
    });

    // 5. Desaparecer
    QTimer::singleShot(150, this, &FlameArea::vanish);
}

void FlameArea::vanish()
{
    if (scene()) scene()->removeItem(this);
    deleteLater();
}
