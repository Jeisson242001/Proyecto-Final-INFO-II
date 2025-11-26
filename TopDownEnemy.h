#ifndef TOPDOWNENEMY_H
#define TOPDOWNENEMY_H

#include <QGraphicsPixmapItem> // <--- CAMBIO: Usamos PixmapItem en vez de RectItem
#include <QObject>
#include <QTimer>
#include <QPixmap> // Para guardar los sprites

class TopDownPlayerItem;
class QGraphicsScene;
class QSoundEffect;

// Heredamos de QGraphicsPixmapItem para manejar imagenes
class TopDownEnemy : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    explicit TopDownEnemy(TopDownPlayerItem* target, QGraphicsScene* scene, QGraphicsItem *parent = nullptr);

    void takeDamage(int damage);
    bool isAlive() { return health_ > 0; }
    void updateBehavior(double dt);

signals:
    void enemyDied(TopDownEnemy* enemy);

private slots:
    void shootAtPlayer();
    void toggleState();
    void fireBullet();

private:
    bool isCollidingWithCover(); // Helper de colisiones

    TopDownPlayerItem* target_;
    QGraphicsScene* scene_;

    int health_ = 3;
    double speed_ = 85.0;

    enum Behavior { Chaser, Tactical };
    Behavior behavior_;

    bool isMoving_ = true;

    QTimer *shootTimer_;
    QTimer *stateTimer_;
    QSoundEffect *shotSound_;
    QSoundEffect *deathSound_;

    // --- NUEVO: Sprites ---
    QPixmap walkPixmap_;  // Sprite caminando (para ambos)
    QPixmap shootPixmap_; // Sprite disparando (para cuando el táctico se para)
    QPixmap deathPixmap_;

    void updateSpriteRotation(); // Función para que mire al jugador
};

#endif // TOPDOWNENEMY_H
