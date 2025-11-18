#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QSoundEffect>
#include <QPropertyAnimation>
#include <QGraphicsPixmapItem>
#include <QVector>
#include <QPushButton>

class QGraphicsView;
class QGraphicsScene;
class PlayerItem;
class QTimer;
class EnemyItem;
class BunkerBossItem;

enum class Weapon { Grenade, Gun };

class GameWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GameWindow(int nivel = 1, QWidget *parent = nullptr);
    ~GameWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onTick();

    // secuenciador de disparo de enemigos
    void beginEnemyShootingSequence();
    void shootCycleForEnemy();

    void checkLevelCompletion();

private:
    int nivel_;
    QGraphicsView *view_;
    QGraphicsScene *scene_;
    QGraphicsPixmapItem *bgItem_ = nullptr; // <-- fondo
    PlayerItem *player_;
    QTimer *timer_;
    bool moveLeftPressed = false;
    bool moveRightPressed = false;

    // NEW: armas
    Weapon currentWeapon = Weapon::Grenade;
    QLabel *weaponLabel = nullptr; // HUD
    void updateWeaponLabel();

    void setupLevel1();
    void spawnEnemiesForLevel1();

    // NEW: disparar según arma
    void fireCurrentWeapon();

    // --- sonido de fondo del nivel (QSoundEffect) ---
    QSoundEffect *bgSound_ = nullptr;
    QPropertyAnimation *bgFadeAnim_ = nullptr;
    void startLevelMusic();            // inicia loop con fade-in
    void fadeOutAndStopLevelMusic(int ms = 600); // fade-out y stop
    void stopLevelMusic();             // parada inmediata

    // lista de enemigos y secuenciador
    QVector<EnemyItem*> enemies_;
    int currentShooterIndex = 0;

    // sonido reutilizable de disparo enemigo (opcional, inicializar en constructor)
    QSoundEffect *enemyShotSound_ = nullptr;

    QLabel *healthBar_ = nullptr;
    void updateHealthBar(int lives);

    // Game Over / estado
    QLabel *gameOverLabel_ = nullptr;
    QSoundEffect *deathMusic_ = nullptr;    // canción que suena cuando el player muere
    QPushButton *retryButton_ = nullptr;  // ⬅️ nuevo
    bool gameOver_ = false;

    // mostrar pantalla / lógica de Game Over
    void showGameOver();
    void restartLevel();

    BunkerBossItem *bunkerBoss_ = nullptr; // <-- AÑADIR ESTA LÍNEA

    // en la sección private:
    bool enemyShootingActive_ = true;   // controla si la secuencia de disparo de enemigos está activa

    // añadir método para parar la secuencia de disparo
    void stopEnemyShootingSequence();


};

#endif // GAMEWINDOW_H
