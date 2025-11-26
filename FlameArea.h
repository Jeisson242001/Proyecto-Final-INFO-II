#ifndef FLAMEAREA_H
#define FLAMEAREA_H

#include <QGraphicsPolygonItem>
#include <QObject>

class QGraphicsScene;

class FlameArea : public QObject, public QGraphicsPolygonItem {
    Q_OBJECT
public:
    // direction: vector unitario hacia donde mira el jugador
    FlameArea(QPointF direction, QGraphicsScene* scene, QGraphicsItem* parent = nullptr);

private slots:
    void vanish(); // Para que desaparezca rápido

private:
    int damage_ = 3; // Daño alto porque es un lanzallamas
};

#endif // FLAMEAREA_H
