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
#include "TopDownEnemy.h"

class QGraphicsView;
class QGraphicsScene;
class PlayerItem;
class QTimer;
class EnemyItem;
class BunkerBossItem;
class TopDownPlayerItem;

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
    TopDownPlayerItem *tdPlayer_ = nullptr;
    QTimer *timer_;
    bool moveLeftPressed = false;
    bool moveRightPressed = false;

    bool isShooting_ = false;     // ¿Está apretando espacio?
    int fireCooldown_ = 0;        // Para controlar la velocidad de disparo

    // NEW: armas
    Weapon currentWeapon = Weapon::Grenade;
    QLabel *weaponLabel = nullptr; // HUD
    void updateWeaponLabel();

    void setupLevel1();
    void setupLevel2();

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
    QSoundEffect *flamethrowerSound_ = nullptr;

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

    void loadNextLevel();


    QList<TopDownEnemy*> survivalEnemies_; // Lista de enemigos rojos
    QTimer *survivalTimer_ = nullptr;      // Timer de los 20 segundos
    int survivalSecondsLeft_ = 20;         // Contador
    QLabel *messageLabel_ = nullptr;       // Texto en pantalla

    void spawnSurvivalWave();              // Función para generar 4 enemigos
    void updateSurvivalTimer();            // Función que resta segundos
};

#endif // GAMEWINDOW_H
