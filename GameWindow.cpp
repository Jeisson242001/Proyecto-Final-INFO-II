#include "GameWindow.h"
#include "PlayerItem.h"
#include "Projectile.h"
#include "Bullet.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QKeyEvent>
#include <QBrush>
#include <QDebug>
#include <QLabel>
#include "EnemyItem.h"

#include <QUrl>
#include <QCoreApplication>
#include <QPointer>
#include <algorithm>

#include <QRandomGenerator>
#include "BunkerBossItem.h"

#include "TopDownPlayerItem.h"
#include <QMessageBox>
#include "FlameArea.h"

GameWindow::GameWindow(int nivel, QWidget *parent)
    : QMainWindow(parent), nivel_(nivel)
{
    view_ = new QGraphicsView(this);
    scene_ = new QGraphicsScene(this);
    view_->setScene(scene_);
    setCentralWidget(view_);
    resize(1024, 600);

    view_->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    scene_->setSceneRect(0, 0, 2800, 600);
    view_->setRenderHint(QPainter::Antialiasing);

    // === Fondo del nivel ===
    QPixmap bgPixmap(":/images/images/fondo_playa.png");
    if (bgPixmap.isNull()) {
        qDebug() << "GameWindow: background image NOT found!";
    } else {
        QSize target(scene_->sceneRect().width(), scene_->sceneRect().height());
        QPixmap scaled = bgPixmap.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // aplicamos desplazamiento vertical (hacia arriba)
        QBrush bgBrush(scaled);
        QTransform transform;
        transform.translate(0, -80); //  mueve el fondo 80 px hacia arriba (ajusta este valor)
        bgBrush.setTransform(transform);

        scene_->setBackgroundBrush(bgBrush);
        qDebug() << "GameWindow: background set OK, size:" << scaled.size();
    }

    // suelo
    QGraphicsRectItem *ground = new QGraphicsRectItem(0, 520, 2800, 100);
    ground->setBrush(QBrush(QColor(200,180,120)));
    ground->setPen(QPen(Qt::NoPen));
    scene_->addItem(ground);

    // player
    player_ = new PlayerItem();
    player_->setPos(150, 480);
    scene_->addItem(player_);

    // --- Barra de vida del jugador ---
    healthBar_ = new QLabel(this);
    healthBar_->setPixmap(QPixmap(":/images/images/vida6.png")
                              .scaled(200, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    healthBar_->setFixedSize(200, 90);
    healthBar_->move(-10, -10);
    healthBar_->show();

    // Conectar eventos de cambio de vida y muerte
    connect(player_, &PlayerItem::playerHealthChanged, this, [this](int lives) {
        updateHealthBar(lives);
    });

    connect(player_, &PlayerItem::playerDied, this, &GameWindow::showGameOver);

    if (nivel_ == 1) {
        setupLevel1();
        // iniciar musica del nivel 1
        startLevelMusic();
    }

    // HUD: etiqueta de arma (icono + texto)
    weaponLabel = new QLabel(this);
    weaponLabel->setObjectName("weaponLabel");
    weaponLabel->setFixedSize(150, 64); // ancho x alto del HUD, ajusta si quieres
    weaponLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    weaponLabel->setStyleSheet(
        "QLabel#weaponLabel {"
        "  background: transparent;"
        "  color: white;"
        "  font-weight: 600;"
        "}"
        );


    weaponLabel->move(30, 50);
    weaponLabel->show();
    updateWeaponLabel();


    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &GameWindow::onTick);
    timer_->start(1000/60);

    // Inicializar sonido reutilizable de disparo enemigo (eficiente)
    enemyShotSound_ = new QSoundEffect(this);
    enemyShotSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/arma_enemigo.wav")));
    enemyShotSound_->setLoopCount(1);
    enemyShotSound_->setVolume(0.9f);
    // precarga:
    //enemyShotSound_->play();
    //enemyShotSound_->stop();

    // --- NUEVO: SONIDO LANZALLAMAS ---
    flamethrowerSound_ = new QSoundEffect(this);
    flamethrowerSound_->setSource(QUrl("qrc:/sound/sounds/lanzallamas.wav"));
    flamethrowerSound_->setVolume(1.0f);
    // ---------------------------------

    // --- Preparar assets / widgets de Game Over (no mostrarlos aún) ---
    gameOverLabel_ = new QLabel(this);
    gameOverLabel_->setObjectName("gameOverLabel");
    gameOverLabel_->setFixedSize(480, 140); // ajusta tamaño
    gameOverLabel_->setAlignment(Qt::AlignCenter);
    gameOverLabel_->setStyleSheet(
        "QLabel#gameOverLabel {"
        "  color: white;"
        "  background: rgba(0, 0, 0, 170);" // semitransparente negro
        "  border-radius: 10px;"
        "  font-size: 34px;"
        "  font-weight: 700;"
        "}"
        );
    gameOverLabel_->setText("GAME OVER");
    gameOverLabel_->hide();

    // --- Botón REINTENTAR ---
    retryButton_ = new QPushButton("Reintentar", this);
    retryButton_->setFixedSize(180, 50);
    retryButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(0, 0, 0, 160);"
        "  color: white;"
        "  font-size: 20px;"
        "  font-weight: bold;"
        "  border: 2px solid white;"
        "  border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 40);"
        "}"
        );
    retryButton_->hide();

    connect(retryButton_, &QPushButton::clicked, this, &GameWindow::restartLevel);

    // Preparar música/sonido de muerte (ruta: ajusta si hace falta)
    deathMusic_ = new QSoundEffect(this);
    deathMusic_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/derrota.wav")));
    deathMusic_->setLoopCount(1);
    deathMusic_->setVolume(1.0f);
    // no reproducir ahora; se reproducirá tras fade-out de bg music

}

GameWindow::~GameWindow()
{
    if (timer_) timer_->stop();

    // parar y eliminar sonido y animación si existen
    if (bgFadeAnim_) {
        bgFadeAnim_->stop();
        delete bgFadeAnim_;
        bgFadeAnim_ = nullptr;
    }
    if (bgSound_) {
        bgSound_->stop();
        delete bgSound_;
        bgSound_ = nullptr;
    }

    // enemyShotSound_ se libera por parent=this
}

void GameWindow::updateWeaponLabel()
{
    QString weaponName;
    QString weaponIcon;

    // --- NUEVA LÓGICA ---
    if (nivel_ == 2) {
        // En Nivel 2 forzamos el Lanzallamas
        weaponName = "Lanzallamas";
        weaponIcon = ":/images/images/lanzallamas.png";
    }
    else {
        // En Nivel 1 sigue la lógica normal (Rifle/Granada)
        switch (currentWeapon) {
        case Weapon::Grenade:
            weaponName = "Granada";
            weaponIcon = ":/images/images/granade.png";
            break;

        default:
            weaponName = "Rifle";
            weaponIcon = ":/images/images/rifle.png";
            break;
        }
    }
    // --------------------

    QString html = QString(
                       "<table><tr>"
                       "<td width='56' valign='middle'><img src='%1' width='48' height='48'></td>"
                       "<td valign='middle' style='padding-left:8px;'>"
                       "<div style='font-size:12px'>Arma en uso:</div>"
                       "<div style='font-size:14px; font-weight:bold;'>%2</div>"
                       "</td>"
                       "</tr></table>"
                       ).arg(weaponIcon).arg(weaponName);

    weaponLabel->setText(html);
}


void GameWindow::setupLevel1()
{
    // --- CAMBIO: Definir tamaño largo para el nivel de scroll ---
    scene_->setSceneRect(0, 0, 2800, 600);

    spawnEnemiesForLevel1();
}

void GameWindow::setupLevel2()
{
    // 1. Definimos el tamaño EXACTO de la ventana (1280x650)
    scene_->setSceneRect(0, 0, 1280, 650);

    // 2. Cargar la imagen
    QPixmap bgPixmap(":/images/images/fondo_2.png");

    if (!bgPixmap.isNull()) {
        // 3. La "Magia": Escalar la imagen al tamaño de la escena
        // Qt::IgnoreAspectRatio -> Estira la imagen para llenar todo (sin bordes negros)
        // Qt::SmoothTransformation -> Mantiene la calidad para que no se vea pixelada
        QPixmap scaled = bgPixmap.scaled(scene_->sceneRect().size().toSize(),
                                         Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation);

        // 4. Aplicar como fondo
        scene_->setBackgroundBrush(QBrush(scaled));
    } else {
        // Color de respaldo por si falla la imagen
        scene_->setBackgroundBrush(QBrush(QColor(30, 40, 60)));
    }

    // --- NOTA IMPORTANTE PARA NIVEL 2 (TOP-DOWN) ---
    // En el nivel 1 (plataformas) agregabas un "suelo" (rectángulo café abajo).
    // En el nivel 2 (vista aérea) NO deberías agregarlo, porque el personaje camina sobre el fondo.
    // Asegúrate de NO copiar el bloque del 'ground' aquí.

    // Crear obstaculos (paredes)
    QGraphicsRectItem *wall1 = new QGraphicsRectItem(0, 0, 20, 150);
    wall1->setBrush(Qt::NoBrush); // Relleno: NINGUNO (Transparente)
    wall1->setPen(Qt::NoPen);     // Borde: NINGUNO (Invisible)
    wall1->setPos(310, 0);

    wall1->setData(0, "cover");  // <--- Hace que retenga balas
    // -------------------------------

    scene_->addItem(wall1);

    QGraphicsRectItem *wall2 = new QGraphicsRectItem(0, 0, 20, 600);
    wall2->setBrush(Qt::NoBrush); // Relleno: NINGUNO (Transparente)
    wall2->setPen(Qt::NoPen);     // Borde: NINGUNO (Invisible)
    wall2->setPos(150, 0);

    wall2->setData(0, "cover");  // <--- Hace que retenga balas
    // -------------------------------

    scene_->addItem(wall2);

    QGraphicsRectItem *wall3 = new QGraphicsRectItem(0, 0, 20, 150);
    wall3->setBrush(Qt::NoBrush); // Relleno: NINGUNO (Transparente)
    wall3->setPen(Qt::NoPen);     // Borde: NINGUNO (Invisible)
    wall3->setPos(600, 30);

    wall3->setData(0, "cover");  // <--- Hace que retenga balas
    // -------------------------------

    scene_->addItem(wall3);

    QGraphicsRectItem *wall4 = new QGraphicsRectItem(0, 0, 20, 150);
    wall4->setBrush(Qt::NoBrush); // Relleno: NINGUNO (Transparente)
    wall4->setPen(Qt::NoPen);     // Borde: NINGUNO (Invisible)
    wall4->setPos(770, 400);

    wall4->setData(0, "cover");  // <--- Hace que retenga balas
    // -------------------------------

    scene_->addItem(wall4);

    QGraphicsRectItem *wall5 = new QGraphicsRectItem(0, 0, 20, 150);
    wall5->setBrush(Qt::NoBrush); // Relleno: NINGUNO (Transparente)
    wall5->setPen(Qt::NoPen);     // Borde: NINGUNO (Invisible)
    wall5->setPos(910, 400);

    wall5->setData(0, "cover");  // <--- Hace que retenga balas
    // -------------------------------

    scene_->addItem(wall5);

    QGraphicsRectItem *wall6 = new QGraphicsRectItem(0, 0, 20, 150);
    wall6->setBrush(Qt::NoBrush); // Relleno: NINGUNO (Transparente)
    wall6->setPen(Qt::NoPen);     // Borde: NINGUNO (Invisible)
    wall6->setPos(530, 400);

    wall6->setData(0, "cover");  // <--- Hace que retenga balas
    // -------------------------------

    scene_->addItem(wall6);

    // Crear jugador top-down
    tdPlayer_ = new TopDownPlayerItem();
    // Centrarlo de inicio
    tdPlayer_->setPos(250, 40);
    scene_->addItem(tdPlayer_);

    // Conexiones...
    connect(tdPlayer_, &TopDownPlayerItem::playerHealthChanged, this, &GameWindow::updateHealthBar);
    connect(tdPlayer_, &TopDownPlayerItem::playerDied, this, &GameWindow::showGameOver);

    updateHealthBar(tdPlayer_->getLives());

    // 1. Mensaje en pantalla
    messageLabel_ = new QLabel("¡AGUANTA LAS OLEADAS! Tiempo: 50", this);
    messageLabel_->setStyleSheet("color: white; font-size: 24px; font-weight: bold; background: rgba(0,0,0,100); padding: 10px;");
    messageLabel_->adjustSize();
    messageLabel_->move((width() - messageLabel_->width())/2, 50); // Centrado arriba
    messageLabel_->show();

    // 2. Resetear variables
    survivalEnemies_.clear();
    survivalSecondsLeft_ = 50;

    // 3. Crear Timer de 1 segundo para la cuenta regresiva
    survivalTimer_ = new QTimer(this);
    connect(survivalTimer_, &QTimer::timeout, this, &GameWindow::updateSurvivalTimer);
    survivalTimer_->start(1000); // Cada 1 segundo

    // 4. Generar la primera oleada (4 enemigos)
    spawnSurvivalWave();

    updateWeaponLabel();
}


void GameWindow::spawnEnemiesForLevel1()
{
    // Secuencia de animación normal
    QStringList enemyFrames = {
        ":/images/images/enemigo1.png",
        ":/images/images/enemigo2.png",
        ":/images/images/enemigo3.png",
        ":/images/images/enemigo4.png"
    };

    // Secuencia de animación de muerte
    QStringList deathFrames = {
        ":/images/images/muerte_enemigo1.png",
        ":/images/images/muerte_enemigo2.png",
        ":/images/images/muerte_enemigo3.png"
    };

    int count = 7;
    int startX = 700;
    int spacing = 250;
    for (int i = 0; i < count; ++i) {
        EnemyItem *e = new EnemyItem(enemyFrames, deathFrames, false); // ✅ usa el constructor correcto
        qreal playerY = player_->pos().y();
        e->setPos(startX + i*spacing, playerY + 50);
        scene_->addItem(e);

        // Guardar en la lista de enemigos para la secuencia
        enemies_.append(e);

        QStringList explosiveDeath = {
            ":/images/images/explosion_enemigo1.png",
            ":/images/images/explosion_enemigo2.png",
            ":/images/images/explosion_enemigo3.png",
            ":/images/images/muerte_enemigo3.png"
        };
        e->setExplosiveDeathFrames(explosiveDeath);

        connect(e, &EnemyItem::enemyDefeated, this, [this](EnemyItem *enemy){
            if (!enemy) return;

            int removedIndex = enemies_.indexOf(enemy);

            enemies_.removeAll(enemy);
            qDebug() << "Enemy defeated - removed from enemies_ list";

            // Ajustar currentShooterIndex para evitar out-of-range:
            if (enemies_.isEmpty()) {
                // no quedan enemigos -> reset index a 0
                currentShooterIndex = 0;
            } else {
                // Si el índice removido era válido y menor que el índice actual,
                // debemos reducir currentShooterIndex en 1 para mantener la rotación.
                if (removedIndex >= 0 && removedIndex < currentShooterIndex) {
                    --currentShooterIndex;
                }
                // Asegurarnos de que currentShooterIndex sigue dentro de los límites actuales
                if (currentShooterIndex >= enemies_.size()) {
                    currentShooterIndex = currentShooterIndex % enemies_.size();
                }
            }

            // --- ¡LÓGICA NUEVA! ---
            // Si la lista de enemigos está vacía Y el búnker existe Y está vivo...
            if (enemies_.isEmpty() && bunkerBoss_ && bunkerBoss_->isAlive()) {
                qDebug() << "All enemies defeated! Activating bunker boss.";
                // ¡Actívalo!
                bunkerBoss_->startAttacking(player_);
            }
        });


        // --- Crear bunker delante del enemigo ---
        QGraphicsRectItem *bunker = new QGraphicsRectItem(0, 0, 40, 60);
        bunker->setBrush(QBrush(QColor(100, 100, 100))); // color gris bunker
        bunker->setPen(QPen(Qt::black));
        bunker->setPos(e->pos().x() - 60, 480 - 20); // justo delante del enemigo
        bunker->setZValue(3); // detrás del jugador, pero delante del enemigo
        bunker->setData(0, QStringLiteral("cover"));
        bunker->setData(1, bunker->rect().height()); // altura útil del bunker
        scene_->addItem(bunker);

        // Si tienes sprite específico para agachado, asignarlo:
        QPixmap crouchPix(":/images/images/enemigo_agachado.png");
        if (!crouchPix.isNull()) {
            e->setCrouchPixmap(crouchPix);
        }
    }

    // -----------------------------
    // Reacción: algunos enemigos se agachan cuando el jugador dispara
    // -----------------------------
    connect(player_, &PlayerItem::playerFired, this, [this]() {
        for (EnemyItem *e : enemies_) {
            if (!e || !e->isAlive()) continue;

            // Probabilidad de agacharse (1 de cada 3)
            if ((QRandomGenerator::global()->bounded(2) == 0) && !e->isCrouching()) {
                e->startCrouch();
                // Se levantan después de 800 ms (ajusta si quieres)
                QTimer::singleShot(800, e, &EnemyItem::stopCrouch);
            }
        }
    });

    // iniciar secuencia de disparo de enemigos (tras un pequeño delay para que todo esté listo)
    QTimer::singleShot(800, this, [this]() {
        beginEnemyShootingSequence();
    });

    // 1. Crear el Búnker Jefe
    bunkerBoss_ = new BunkerBossItem(scene_);

    // 2. Posicionarlo al final del mapa
    // Tu escena mide 2800px de ancho (según scene_->setSceneRect(0, 0, 2800, 600))
    // Así que lo ponemos cerca del final, en X=2600.
    // La 'y' (480) es la misma del suelo del jugador.
    bunkerBoss_->setPos(2600, 575);
    scene_->addItem(bunkerBoss_);

    // 3. Conectar sus señales
    connect(bunkerBoss_, &BunkerBossItem::bunkerDefeated,
            this, &GameWindow::checkLevelCompletion);

    connect(bunkerBoss_, &BunkerBossItem::bunkerFired, this, [this]() {
        // Reutilizamos el sonido de disparo de enemigo que ya tenías
        if (enemyShotSound_) enemyShotSound_->play();
    });
}

void GameWindow::stopEnemyShootingSequence()
{
    // Desactiva la generación de disparos por parte de los enemigos.
    enemyShootingActive_ = false;

    // Si hay lógica adicional central de timers para la secuencia, parala aquí.
    // Además, pedir a los bunkers/enemigos que paren sus timers activos:
    if (bunkerBoss_) bunkerBoss_->stopAttacking();

    // Intentamos pedir a cada enemigo que pare acciones propias si existiera ese API:
    for (EnemyItem* e : enemies_) {
        if (e) {
            // si implementas un stopAttacking/stopTimers en EnemyItem,
            // llamalo aquí. Si no, esta llamada es segura de momento.
            // e->stopAttacking(); // <-- opcional si lo implementas
        }
    }
}


void GameWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    // --- Nivel 2: control top-down ---
    if (nivel_ == 2 && tdPlayer_) {
        // WASD / flechas para mover; espacio para disparar
        if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left) {
            // FIX: no acceder a members privados de TopDownPlayerItem; usar valor literal o getter.
            tdPlayer_->setMoveX(-260.0);
            return;
        } else if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right) {
            tdPlayer_->setMoveX(260.0);
            return;
        } else if (event->key() == Qt::Key_W || event->key() == Qt::Key_Up) {
            tdPlayer_->setMoveY(-260.0);
            return;
        } else if (event->key() == Qt::Key_S || event->key() == Qt::Key_Down) {
            tdPlayer_->setMoveY(260.0);
            return;
        } else if (event->key() == Qt::Key_Space) {
            isShooting_ = true; // <--- ¡Aquí está el cambio clave!
            return;
        }
    }

    // --- Nivel 1 / input global ---
    if (gameOver_) return; // no aceptar input tras morir

    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        moveLeftPressed = true;
    } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        moveRightPressed = true;
    } else if (event->key() == Qt::Key_W) {
        // SALTAR
        if (player_) player_->jump();
    } else if (event->key() == Qt::Key_Space) {
        fireCurrentWeapon();
    } else if (event->key() == Qt::Key_Q) {
        currentWeapon = (currentWeapon == Weapon::Grenade) ? Weapon::Gun : Weapon::Grenade;
        updateWeaponLabel();
    } else if (event->key() == Qt::Key_S) {
        if (player_) player_->startCrouch();
    }

    QMainWindow::keyPressEvent(event);
}



void GameWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    // Nivel 2: parar movimiento por eje
    if (nivel_ == 2 && tdPlayer_) {
        if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left) {
            tdPlayer_->stopX();
            return;
        } else if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right) {
            tdPlayer_->stopX();
            return;
        } else if (event->key() == Qt::Key_W || event->key() == Qt::Key_Up) {
            tdPlayer_->stopY();
            return;
        } else if (event->key() == Qt::Key_S || event->key() == Qt::Key_Down) {
            tdPlayer_->stopY();
            return;
        }
        // DETENER DISPARO (Lanzallamas)
        else if (event->key() == Qt::Key_Space) {
            isShooting_ = false; // <--- Apagamos la bandera

            // Importante: ¡Callar el sonido inmediatamente!
            if (flamethrowerSound_) flamethrowerSound_->stop();
            return;
        }
    }

    // Nivel 1 / global
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        moveLeftPressed = false;
    } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        moveRightPressed = false;
    } else if (event->key() == Qt::Key_S) {
        if (player_) player_->stopCrouch();
    }
    QMainWindow::keyReleaseEvent(event);
}


void GameWindow::fireCurrentWeapon()
{
    if (gameOver_) return; // bloquear disparo tras morir

    if (!player_ || !scene_) return;

    qDebug() << "GameWindow::fireCurrentWeapon() called; currentWeapon=" << (currentWeapon==Weapon::Grenade ? "G" : "R");

    if (currentWeapon == Weapon::Grenade) {
        double v0 = 700.0;
        double angle = 40.0;
        bool facingLeft = player_->isFacingLeft();
        QPointF start = player_->pos() + QPointF(facingLeft ? -0 : 0, -20); // <-- más separada DEL PLAYER
        qDebug() << "Launching grenade at" << start << "facingLeft=" << facingLeft;
        ProjectileItem *p = new ProjectileItem(v0, angle * (facingLeft ? -1.0 : 1.0), scene_);
        p->setPos(start);
        scene_->addItem(p);

        // Notificar a los listeners (enemigos) que el jugador disparó
        if (player_) player_->notifyFired();
        return;

    }

    // RIFLE
    bool facingLeft = player_->isFacingLeft();
    QPointF dir = facingLeft ? QPointF(-1, 0) : QPointF(1, 0);

    // Crea bullet, sitúala fuera del cuerpo del player (más separada)
    // Punto de salida según si está agachado o de pie
    QPointF start = player_->pos();
    if (player_->isCrouching())
        start += QPointF(facingLeft ? -10 : 10, -8);   // bala más baja
    else
        start += QPointF(facingLeft ? -10 : 10, -32);  // bala alta


    BulletItem *b = new BulletItem(dir, 500.0, scene_, BulletItem::Owner::Player);
    b->setPos(start);
    scene_->addItem(b);

    // Notificar a los listeners (enemigos) que el jugador disparó
    if (player_) player_->notifyFired();
}


void GameWindow::onTick()
{
    if (gameOver_) return;

    double dt = 1.0/60.0;

    // --- Nivel 1 (Plataformas) ---
    if (nivel_ == 1) {
        if (moveLeftPressed && player_) player_->moveLeft();
        if (moveRightPressed && player_) player_->moveRight();
        if (player_) player_->updateFrame(dt);
    }

    // --- Nivel 2 (Top-Down / Supervivencia) ---
    if (nivel_ == 2 && tdPlayer_) {

        // 1. ¡ESTA ES LA LÍNEA QUE FALTABA! (Mueve al jugador)
        tdPlayer_->updateFrame(dt);

        // 2. Mover a los enemigos (También faltaba esto)
        for (TopDownEnemy* e : survivalEnemies_) {
            if (e && e->isAlive()) {
                e->updateBehavior(dt);
            }
        }

        // 3. Lógica del Lanzallamas
        if (fireCooldown_ > 0) fireCooldown_--;

        if (isShooting_ && fireCooldown_ <= 0) {

            FlameArea* flame = new FlameArea(QPointF(0, -1), scene_, tdPlayer_);
            flame->setPos(0, -25);

            if (flamethrowerSound_ && !flamethrowerSound_->isPlaying()) {
                flamethrowerSound_->play();
            }

            tdPlayer_->notifyFired();
            fireCooldown_ = 5;
        }
    }

    // --- Lógica de Cámara ---
    if (nivel_ == 2) {
        // Cámara Fija
        view_->centerOn(scene_->sceneRect().width() / 2, scene_->sceneRect().height() / 2);
    }
    else {
        // Cámara Scroll (Nivel 1)
        QPointF centerPoint;
        if (player_) centerPoint = player_->pos();
        else centerPoint = QPointF(view_->width()/2.0, view_->height()/2.0);

        const double viewWidth = view_->width();
        const double sceneWidth = scene_->sceneRect().width();

        if (centerPoint.x() < viewWidth / 2) centerPoint.setX(viewWidth / 2);
        double maxCameraX = sceneWidth - (viewWidth / 2);
        if (centerPoint.x() > maxCameraX) centerPoint.setX(maxCameraX);

        view_->centerOn(centerPoint);
    }
}


void GameWindow::startLevelMusic()
{
    // ya existe y está sonando => no hacemos nada
    if (bgSound_ && bgSound_->isPlaying()) return;

    if (!bgSound_) {
        bgSound_ = new QSoundEffect(this);

        // ruta en resources (ajusta si tu qrc usa otro prefix)
        bgSound_->setSource(QUrl(QStringLiteral("qrc:/sound/sounds/ambiente_playa.wav")));

        // loop infinito
        bgSound_->setLoopCount(QSoundEffect::Infinite);

        // precarga silenciosa para evitar latencia (play+stop con volumen 0)
        bgSound_->setVolume(0.0f);
        bgSound_->play();
        bgSound_->stop();
    }

    // iniciar reproducción y hacer fade-in
    bgSound_->setLoopCount(QSoundEffect::Infinite);
    bgSound_->play();

    // fade-in del volumen (0.0 -> 0.45 en 800 ms)
    if (bgFadeAnim_) {
        bgFadeAnim_->stop();
        delete bgFadeAnim_;
        bgFadeAnim_ = nullptr;
    }
    bgFadeAnim_ = new QPropertyAnimation(bgSound_, "volume", this);
    bgFadeAnim_->setDuration(800);
    bgFadeAnim_->setStartValue(0.0);
    bgFadeAnim_->setEndValue(0.6);
    bgFadeAnim_->start(QAbstractAnimation::DeleteWhenStopped);
}

void GameWindow::fadeOutAndStopLevelMusic(int ms)
{
    if (!bgSound_) return;

    QPropertyAnimation *anim = new QPropertyAnimation(bgSound_, "volume", this);
    anim->setDuration(ms);
    anim->setStartValue(bgSound_->volume());
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
        if (bgSound_) bgSound_->stop();
        anim->deleteLater();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void GameWindow::stopLevelMusic()
{
    if (bgFadeAnim_) {
        bgFadeAnim_->stop();
        delete bgFadeAnim_;
        bgFadeAnim_ = nullptr;
    }
    if (bgSound_) {
        bgSound_->stop();
    }
}

// ------------------------
// SECUENCIA DE DISPARO ENEMIGOS
// ------------------------
void GameWindow::beginEnemyShootingSequence()
{
    if (gameOver_) return;              // si estamos en game over, no arrancar
    enemyShootingActive_ = true;        // permitir la secuencia
    if (enemies_.isEmpty()) return;
    currentShooterIndex = 0;
    shootCycleForEnemy();
}


void GameWindow::shootCycleForEnemy()
{
    // Si la secuencia fue desactivada (por Game Over o restart), salir.
    if (!enemyShootingActive_ || gameOver_) return;

    // limpiar nullptrs residuales
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                  [](EnemyItem* e){ return e == nullptr; }),
                   enemies_.end());
    if (enemies_.isEmpty()) return;

    // asegurar currentShooterIndex válido
    if (currentShooterIndex < 0) currentShooterIndex = 0;
    if (currentShooterIndex >= enemies_.size()) currentShooterIndex = currentShooterIndex % enemies_.size();

    // buscar siguiente enemigo vivo (en orden circular), con límites verificados
    int attempts = 0;
    while (attempts < static_cast<int>(enemies_.size())) {
        EnemyItem *cand = enemies_.at(currentShooterIndex);
        if (cand && cand->isAlive()) break;
        currentShooterIndex = (currentShooterIndex + 1) % enemies_.size();
        ++attempts;
    }
    if (attempts >= static_cast<int>(enemies_.size())) return; // todos muertos o inválidos

    EnemyItem *shooter = enemies_.at(currentShooterIndex);
    if (!shooter || !shooter->isAlive()) {
        // avanzar índice y reintentar pronto
        if (!enemies_.isEmpty()) currentShooterIndex = (currentShooterIndex + 1) % enemies_.size();
        QTimer::singleShot(150, this, [this]() { this->shootCycleForEnemy(); });
        return;
    }

    // Parámetros de disparo
    const int shots = 6;
    const int intervalMs = 400;

    // Programar los disparos (usar QPointer para evitar use-after-free)
    for (int i = 0; i < shots; ++i) {
        QPointer<EnemyItem> shooterPtr = shooter;
        QTimer::singleShot(i * intervalMs, this, [this, shooterPtr]() {
            // COMPROBACIONES AL INICIO: si la secuencia fue desactivada o game over -> salir
            if (!enemyShootingActive_ || gameOver_) return;
            if (!shooterPtr) return;
            if (!shooterPtr->isAlive()) return;
            if (!player_) return;

            QPointF from = shooterPtr->pos() - QPointF(0, 90);
            QPointF target = player_->pos() + QPointF(0, -20);
            QPointF dir = target - from;
            double len = std::hypot(dir.x(), dir.y());
            if (len > 0.0001) dir /= len;
            else dir = QPointF(-1, 0);

            BulletItem *b = new BulletItem(dir, 420.0, scene_, BulletItem::Owner::Enemy);

            QPixmap enemyBulletPix(":/images/images/bala_enemigo.png");
            if (!enemyBulletPix.isNull()) {
                b->setPixmap(enemyBulletPix.scaled(20, 10, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                b->setOffset(-b->pixmap().width()/2, -b->pixmap().height()/2);
            }

            QPointF spawnOffset = QPointF(dir.x()*36, dir.y()*8);
            b->setPos(from + spawnOffset);
            if (scene_) scene_->addItem(b);

            if (enemyShotSound_) enemyShotSound_->play();
        });
    }

    // Después de todos los disparos, el enemigo se agacha y esperamos hideMs antes de siguiente
    int totalShotsMs = shots * intervalMs;
    QPointer<EnemyItem> shooterPtr2 = shooter;
    QTimer::singleShot(totalShotsMs + 60, this, [this, shooterPtr2]() {
        // Si la secuencia fue desactivada o el enemigo ya fue destruido, avanzamos al siguiente
        if (!enemyShootingActive_ || gameOver_) return;

        if (!shooterPtr2) {
            if (!enemies_.isEmpty()) {
                currentShooterIndex = (currentShooterIndex + 1) % enemies_.size();
                QTimer::singleShot(150, this, [this]() { this->shootCycleForEnemy(); });
            }
            return;
        }
        if (!shooterPtr2->isAlive()) {
            if (!enemies_.isEmpty()) {
                currentShooterIndex = (currentShooterIndex + 1) % enemies_.size();
                QTimer::singleShot(150, this, [this]() { this->shootCycleForEnemy(); });
            }
            return;
        }

        shooterPtr2->startCrouch();

        const int hideMs = 3000;
        QPointer<EnemyItem> shooterPtr3 = shooterPtr2;
        QTimer::singleShot(hideMs, this, [this, shooterPtr3]() {
            if (!enemyShootingActive_ || gameOver_) return;
            if (shooterPtr3 && shooterPtr3->isAlive()) shooterPtr3->stopCrouch();
            if (!enemies_.isEmpty()) {
                currentShooterIndex = (currentShooterIndex + 1) % enemies_.size();
                QTimer::singleShot(200, this, [this]() { this->shootCycleForEnemy(); });
            }
        });
    });
}



void GameWindow::updateHealthBar(int lives)
{
    if (!healthBar_) return;

    QString spritePath = QString(":/images/images/vida%1.png").arg(lives);
    QPixmap pix(spritePath);
    if (pix.isNull()) {
        qWarning() << "⚠️ Sprite de vida no encontrado:" << spritePath;
        return;
    }

    healthBar_->setPixmap(pix.scaled(200, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void GameWindow::showGameOver()
{
    if (gameOver_) return;
    gameOver_ = true;

    qDebug() << "🏴 Showing Game Over screen";

    stopEnemyShootingSequence();

    if (timer_ && timer_->isActive()) timer_->stop();
    fadeOutAndStopLevelMusic(800);

    QTimer::singleShot(900, this, [this]() {
        if (deathMusic_) deathMusic_->play();
    });

    if (gameOverLabel_) {
        int w = gameOverLabel_->width();
        int h = gameOverLabel_->height();
        int cx = (this->width() - w) / 2;
        int cy = (this->height() - h) / 2 - 40;  // un poco más arriba para dejar espacio al botón
        gameOverLabel_->move(cx, cy);
        gameOverLabel_->show();
    }

    // --- Mostrar botón ---
    if (retryButton_) {
        int bw = retryButton_->width();
        int bx = (this->width() - bw) / 2;
        int by = (this->height() / 2) + 40;  // justo debajo del texto
        retryButton_->move(bx, by);
        retryButton_->show();
    }

    if (player_) player_->setEnabled(false);
}


void GameWindow::restartLevel()
{
    qDebug() << "🔁 Reiniciando nivel:" << nivel_;

    // 0) Desactivar inmediatamente la generación de disparos
    stopEnemyShootingSequence();
    gameOver_ = true;

    // 1) Ocultar UI de Game Over
    if (gameOverLabel_) gameOverLabel_->hide();
    if (retryButton_) retryButton_->hide();

    if (messageLabel_) {
        delete messageLabel_; // Borrar el objeto de la memoria
        messageLabel_ = nullptr; // Poner el puntero a cero por seguridad
    }

    // 2) Parar música/sonidos/timers
    if (deathMusic_ && deathMusic_->isPlaying()) deathMusic_->stop();
    if (bgSound_ && bgSound_->isPlaying()) bgSound_->stop();
    if (timer_ && timer_->isActive()) timer_->stop();

    // 3) Limpiar escena
    if (scene_) {
        QList<QGraphicsItem*> items = scene_->items();
        for (QGraphicsItem *it : items) {
            scene_->removeItem(it);
            delete it;
        }
    }

    // 4) Limpiar punteros
    enemies_.clear();
    tdPlayer_ = nullptr;
    player_ = nullptr;
    bunkerBoss_ = nullptr; // Importante limpiar el boss también

    gameOver_ = false;

    // 5) Crear el jugador SEGÚN EL NIVEL
    // (Nota: El fondo y el suelo se crean dentro de setupLevel1 o setupLevel2 ahora)

    if (nivel_ == 2) {
        // --- CONFIGURACIÓN NIVEL 2 (Top Down) ---
        setupLevel2(); // setupLevel2 ya crea el fondo, el jugador y las conexiones

        // NO creamos player_ aquí, solo tdPlayer_
    }
    else {
        // --- CONFIGURACIÓN NIVEL 1 (Plataforma) ---

        // Fondo Nivel 1
        QPixmap bgPixmap(":/images/images/fondo_playa.png");
        if (!bgPixmap.isNull()) {
            QSize target(2800, 600);
            QPixmap scaled = bgPixmap.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            QBrush bgBrush(scaled);
            QTransform transform;
            transform.translate(0, -80);
            bgBrush.setTransform(transform);
            scene_->setBackgroundBrush(bgBrush);
        }

        // Suelo Nivel 1
        QGraphicsRectItem *ground = new QGraphicsRectItem(0, 520, 2800, 100);
        ground->setBrush(QBrush(QColor(200,180,120)));
        ground->setPen(QPen(Qt::NoPen));
        scene_->addItem(ground);

        // Jugador Nivel 1
        player_ = new PlayerItem();
        player_->setPos(150, 480);
        scene_->addItem(player_);

        // Conexiones Nivel 1
        connect(player_, &PlayerItem::playerHealthChanged, this, [this](int lives) {
            updateHealthBar(lives);
        });
        connect(player_, &PlayerItem::playerDied, this, &GameWindow::showGameOver);

        // Cargar enemigos Nivel 1
        setupLevel1();
    }

    // ---------------------------------------------------------
    // BORRÉ EL BLOQUE 7 QUE CAUSABA EL CRASH
    // Ya conectamos las señales arriba dentro de cada if/else
    // ---------------------------------------------------------

    // 9) Resetear HUD y música
    if (tdPlayer_) updateHealthBar(tdPlayer_->getLives());
    else if (player_) updateHealthBar(player_->getLives());

    updateWeaponLabel();

    if (bgSound_) {
        bgSound_->setVolume(0.6f);
        bgSound_->play();
    }

    enemyShootingActive_ = true;

    // 10) Reiniciar el timer principal
    if (timer_) timer_->start(1000/60);

    qDebug() << "✅ Nivel reiniciado correctamente";
}

void GameWindow::checkLevelCompletion()
{
    // Esta función se llama cuando el búnker emite 'bunkerDefeated'

    // Verificamos AMBAS condiciones: no quedan enemigos Y el búnker no está vivo
    if (enemies_.isEmpty() && bunkerBoss_ && !bunkerBoss_->isAlive()) {

        qDebug() << "¡¡¡NIVEL COMPLETADO!!!";
        gameOver_ = true; // Evita que el jugador se mueva
        if (timer_) timer_->stop();

        fadeOutAndStopLevelMusic(800);

        // --- Pantalla de Victoria ---
        QLabel *victoryLabel = new QLabel(this);
        victoryLabel->setObjectName("victoryLabel");
        victoryLabel->setFixedSize(480, 140);
        victoryLabel->setAlignment(Qt::AlignCenter);
        victoryLabel->setStyleSheet(
            "QLabel#victoryLabel {"
            "  color: #FFFF00;" // Amarillo
            "  background: rgba(0, 80, 0, 170);" // Verde oscuro
            "  border-radius: 10px;"
            "  font-size: 34px;"
            "  font-weight: 700;"
            "}"
            );
        victoryLabel->setText("¡NIVEL COMPLETADO!");

        int w = victoryLabel->width();
        int h = victoryLabel->height();
        int cx = (this->width() - w) / 2;
        int cy = (this->height() - h) / 2;
        victoryLabel->move(cx, cy);
        victoryLabel->show();

        // Tras 3 segundos, cerramos la ventana
        QTimer::singleShot(3000, this, [this, victoryLabel](){
            victoryLabel->deleteLater(); // Limpiar la etiqueta
            loadNextLevel();
        });
    }
}

void GameWindow::loadNextLevel()
{
    nivel_++;  // Pasamos al siguiente nivel

    qDebug() << "Cargando nivel:" << nivel_;

    restartLevel();   // ✔ Usas tu mismo reinicio que ya funciona perfecto
}

void GameWindow::spawnSurvivalWave()
{
    if (gameOver_) return;

    // Cantidad de enemigos por oleada (puedes ajustarlo)
    int enemyCount = 10;

    for (int i = 0; i < enemyCount; i++) {
        // Crear enemigo
        TopDownEnemy* enemy = new TopDownEnemy(tdPlayer_, scene_);

        // Posición aleatoria a la DERECHA (entre X=1100 y X=1250)
        int randX = 1100 + QRandomGenerator::global()->bounded(150);
        // Altura aleatoria (dentro de los 650 de alto)
        int randY = QRandomGenerator::global()->bounded(50, 600);

        enemy->setPos(randX, randY);
        scene_->addItem(enemy);
        survivalEnemies_.append(enemy);

        // Detectar muerte
        connect(enemy, &TopDownEnemy::enemyDied, this, [this, enemy](){

            // Verificar si el enemigo está en la lista (para no procesarlo dos veces)
            if (survivalEnemies_.contains(enemy)) {

                // 1. Lo sacamos de la lista de enemigos "activos" (lógica del juego)
                survivalEnemies_.removeAll(enemy);

                // -------------------------------------------------------------
                // 2. ¡AQUÍ ESTABA EL ERROR!
                // scene_->removeItem(enemy);  <--- COMENTADA / BORRADA
                // -------------------------------------------------------------
                // Al quitar esta línea, dejamos que el objeto siga existiendo visualmente
                // para que pueda mostrar su sprite de muerto. Él mismo se borrará
                // con el 'deleteLater' que pusimos en su función takeDamage.

                // 3. Si se acabaron los enemigos de esta lista, lanzar nueva oleada
                if (survivalEnemies_.isEmpty()) {
                    spawnSurvivalWave();
                }
            }
        });
    }
}

void GameWindow::updateSurvivalTimer()
{
    if (gameOver_) {
        survivalTimer_->stop();
        return;
    }

    survivalSecondsLeft_--;
    messageLabel_->setText(QString("¡AGUANTA LAS OLEADAS! Tiempo: %1").arg(survivalSecondsLeft_));
    messageLabel_->adjustSize();

    // GANASTE
    if (survivalSecondsLeft_ <= 0) {
        survivalTimer_->stop();
        messageLabel_->hide();

        // Matar enemigos restantes visualmente (opcional)
        for(auto e : survivalEnemies_) if(e) delete e;
        survivalEnemies_.clear();

        // Mostrar Victoria y pasar de nivel (o terminar)
        // Reutilizamos tu logica de victoria
        gameOver_ = true;
        QLabel *victory = new QLabel("¡SOBREVIVISTE!", this);
        victory->setStyleSheet("color: lime; font-size: 40px; background: rgba(0,0,0,180); padding: 20px; border-radius: 10px;");
        victory->adjustSize();
        victory->move((width() - victory->width())/2, (height() - victory->height())/2);
        victory->show();

        // Cerrar o volver al menú tras 3 seg
        QTimer::singleShot(3000, this, [this, victory](){
            victory->deleteLater();
            // Aquí puedes llamar a loadNextLevel() o close()
            QMessageBox::information(this, "Fin", "Juego Completado");
            close();
        });
    }
}
