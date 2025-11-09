#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QSoundEffect>
#include <QPropertyAnimation>
#include <QGraphicsPixmapItem>

class QGraphicsView;
class QGraphicsScene;
class PlayerItem;
class QTimer;

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
};

#endif // GAMEWINDOW_H
