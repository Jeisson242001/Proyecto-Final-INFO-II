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

    if (nivel_ == 1) {
        setupLevel1();
        // iniciar musica del nivel 1
        startLevelMusic();
    }

    // HUD: etiqueta de arma (icono + texto)
    weaponLabel = new QLabel(this);
    weaponLabel->setObjectName("weaponLabel");
    weaponLabel->setFixedSize(200, 64); // ancho x alto del HUD, ajusta si quieres
    weaponLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    weaponLabel->setStyleSheet(
        "QLabel#weaponLabel {"
        "  color: white;"
        "  background: rgba(0,0,0,120);"
        "  padding:6px;"
        "  border-radius:6px;"
        "  font-weight: 600;"
        "}"
        );

    weaponLabel->move(10, 10);
    weaponLabel->show();
    updateWeaponLabel();


    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &GameWindow::onTick);
    timer_->start(1000/60);
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
}

void GameWindow::updateWeaponLabel()
{
    QString weaponName;
    QString weaponIcon;

    switch (currentWeapon) {
    case Weapon::Grenade:
        weaponName = "Granada";
        weaponIcon = ":/images/images/granade.png"; // ajusta ruta si tu qrc es otra
        break;

    default:
        weaponName = "Rifle";
        weaponIcon = ":/images/images/rifle.png";
        break;
    }

    // HTML simple con <img> para mostrar icono + texto alineado
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
    spawnEnemiesForLevel1();
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

        connect(e, &EnemyItem::enemyDefeated, this, [this](EnemyItem *enemy){
            Q_UNUSED(enemy);
            qDebug() << "Enemy defeated - you can add score here";
        });
    }
}


void GameWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        moveLeftPressed = true;
    } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        moveRightPressed = true;
    } else if (event->key() == Qt::Key_Space) {
        // disparar el arma actual
        fireCurrentWeapon();
    } else if (event->key() == Qt::Key_Q) {
        // cambiar arma
        currentWeapon = (currentWeapon == Weapon::Grenade) ? Weapon::Gun : Weapon::Grenade;
        updateWeaponLabel();
    } else if (event->key() == Qt::Key_S) {
        // ejemplo: tecla S para lanzar granada alternativa
        fireCurrentWeapon();
    }
    QMainWindow::keyPressEvent(event);
}

void GameWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        moveLeftPressed = false;
    } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        moveRightPressed = false;
    }
    QMainWindow::keyReleaseEvent(event);
}

void GameWindow::fireCurrentWeapon()
{
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
        return;
    }

    // RIFLE
    bool facingLeft = player_->isFacingLeft();
    QPointF dir = facingLeft ? QPointF(-1, 0) : QPointF(1, 0);

    // Crea bullet, sitúala fuera del cuerpo del player (más separada)
    QPointF start = player_->pos() + QPointF(facingLeft ? 0 : 0, -15);
    qDebug() << "Firing rifle bullet at" << start << "dir" << dir << "facingLeft=" << facingLeft;

    BulletItem *b = new BulletItem(dir, 500.0, scene_);
    b->setPos(start);
    scene_->addItem(b);
}


void GameWindow::onTick()
{
    double dt = 1.0/60.0;

    if (moveLeftPressed) player_->moveLeft();
    if (moveRightPressed) player_->moveRight();

    player_->updateFrame(dt);

    // centrar cámara en el jugador (suavizado simple)
    view_->centerOn(player_->pos());
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
