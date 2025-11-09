#include "interfaz.h"
#include "ui_interfaz.h"

#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDialog>
#include <QListWidget>
#include <QLabel>

#include <QPropertyAnimation>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QEasingCurve>

#include "niveles.h" // dialogo de niveles (archivo nuevo)

#include "GameWindow.h"




class HoverAnimator : public QObject {
public:
    HoverAnimator(QPushButton *btn, QObject *parent = nullptr)
        : QObject(parent), button(btn)
    {
        // Sombra sutil
        auto *shadow = new QGraphicsDropShadowEffect(button);
        shadow->setBlurRadius(18);
        shadow->setOffset(0, 6);
        shadow->setColor(QColor(0,0,0,140));
        button->setGraphicsEffect(shadow);

        // Animación de geometry
        anim = new QPropertyAnimation(button, "geometry", this);
        anim->setDuration(180);
        anim->setEasingCurve(QEasingCurve::OutCubic);

        // Guardar geometría original (se actualizará con Resize events)
        originalGeom = button->geometry();

        // instalar filtro
        button->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *ev) override
    {
        if (watched != button) return QObject::eventFilter(watched, ev);

        if (ev->type() == QEvent::Enter) {
            QRect g = button->geometry();
            int dw = qMax(2, g.width() * 6 / 100);
            int dh = qMax(1, g.height() * 6 / 100);
            QRect target(g.x() - dw/2, g.y() - dh/2, g.width() + dw, g.height() + dh);
            anim->stop();
            anim->setStartValue(button->geometry());
            anim->setEndValue(target);
            anim->start();
        } else if (ev->type() == QEvent::Leave) {
            anim->stop();
            anim->setStartValue(button->geometry());
            anim->setEndValue(originalGeom);
            anim->start();
        } else if (ev->type() == QEvent::Resize) {
            // actualizar originalGeom si el botón cambia de tamaño por layout/responsive
            originalGeom = button->geometry();
        }
        return QObject::eventFilter(watched, ev);
    }

private:
    QPushButton *button;
    QRect originalGeom;
    QPropertyAnimation *anim;
};

Interfaz::Interfaz(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Interfaz)
{
    ui->setupUi(this);

    menuSound = new QSoundEffect(this);
    menuSound->setSource(QUrl("qrc:/sound/sounds/tema_menu.wav"));
    menuSound->setVolume(0.5); // 0.0 a 1.0
    menuSound->setLoopCount(QSoundEffect::Infinite);
    menuSound->play();

    // --- Scene (parent = this para que Qt la maneje) ---
    scene = new QGraphicsScene(this);
    setupScene();
    ui->graphicsView->setScene(scene);

    // --- Menu y botones centrales ---
    setupMenuAndButtons();
}

Interfaz::~Interfaz()
{
    delete ui; // scene será borrada por Qt (parent = this)
}

// Crear algunos items de ejemplo en la escena
void Interfaz::setupScene()
{
    QPen pen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QBrush brush(Qt::yellow);
    scene->addRect(0, 0, 100, 100, pen, brush);
    scene->addEllipse(0, 0, 100, 100, pen); // ahora se superpone al rectángulo

    // ajustar rectángulo visible de la escena
    scene->setSceneRect(-50, -50, 300, 300);
    ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

// Crea la barra de menú y dos botones centrales
void Interfaz::setupMenuAndButtons()
{
    // --- Barra de menú ---
    QMenu *menuJuego = menuBar()->addMenu(tr("Juego"));

    QAction *actionNiveles = new QAction(tr("Niveles"), this);
    QAction *actionJugar    = new QAction(tr("Jugar"), this);

    menuJuego->addAction(actionNiveles);
    menuJuego->addAction(actionJugar);

    connect(actionNiveles, &QAction::triggered, this, &Interfaz::onActionNivelesTriggered);
    connect(actionJugar, &QAction::triggered, this, &Interfaz::onActionJugarTriggered);

    // --- Widget central ---
    QWidget *central = new QWidget(this);
    central->setObjectName("centralWidget"); // para aplicar stylesheet selectivo

    // Layout principal del central (ocupa toda el área)
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(10, 350, 900, 10);
    mainLayout->setSpacing(0);

    // Título arriba centrado (puedes ajustarlo)
    QLabel *titulo = new QLabel(tr("MENÚ PRINCIPAL"), central);
    titulo->setAlignment(Qt::AlignCenter);
    QFont f = titulo->font();
    f.setPointSize(18);
    f.setBold(true);
    titulo->setFont(f);
    titulo->setStyleSheet("color: rgb(255,255,0); text-shadow: 1px 1px 2px rgba(0,0,0,160);");

    // Contenedor para botones (ancho fijo / responsive)
    QWidget *btnContainer = new QWidget(central);
    btnContainer->setObjectName("btnContainer");
    const int containerWidth = 420; // ancho del bloque de botones (ajusta si quieres)
    btnContainer->setMaximumWidth(containerWidth);
    btnContainer->setMinimumWidth(280);

    QVBoxLayout *vboxButtons = new QVBoxLayout(btnContainer);
    vboxButtons->setContentsMargins(0, 0, 0, 0);
    vboxButtons->setSpacing(18);

    // Crear botones

    QPushButton *btnJugar   = new QPushButton(tr("Jugar"), btnContainer);
    QPushButton *btnNiveles = new QPushButton(tr("Niveles"), btnContainer);

    // Tamaño y politicas
    btnNiveles->setFixedHeight(52);
    btnJugar->setFixedHeight(52);

    btnNiveles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnJugar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Estilo base (semi-oscuro, borde y tipografía grande)
    QString botonStyle =
        "QPushButton {"
        "  background-color: rgba(10, 10, 10, 170);" // semitransparente oscuro
        "  color: #FFFFFF;"
        "  border-radius: 10px;"
        "  padding: 8px 20px;"
        "  font-size: 18px;"
        "  font-weight: 600;"
        "  border: 1px solid rgba(255,255,255,10);"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255,255,255,12);" // ligera aclaración
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(0,0,0,200);"
        "}";
    btnNiveles->setStyleSheet(botonStyle);
    btnJugar->setStyleSheet(botonStyle);

    // Añadir botones al contenedor (expand para ocupar ancho del container)
    vboxButtons->addWidget(btnNiveles);
    vboxButtons->addWidget(btnJugar);

    // Añadir widgets al layout principal y centrar el contenedor
    mainLayout->addWidget(titulo);
    mainLayout->addStretch();
    mainLayout->addWidget(btnContainer, 0, Qt::AlignHCenter);
    mainLayout->addStretch();

    // Aplicar central
    setCentralWidget(central);

    // --- Background (tu imagen de fondo, mantenida igual) ---
    central->setStyleSheet(
        "QWidget#centralWidget {"
        "  border-image: url(:/images/images/Caratula.jpg) 0 0 0 0 stretch stretch;"
        "  background-position: center 120px;"
        "}"
        );

    // --- Hover animation y sombra (instanciar animadores con parent 'central' para manejo de memoria) ---
    new HoverAnimator(btnNiveles, central);
    new HoverAnimator(btnJugar, central);

    // Conexiones finales
    connect(btnNiveles, &QPushButton::clicked, this, &Interfaz::onBtnNivelesClicked);
    connect(btnJugar, &QPushButton::clicked, this, &Interfaz::onBtnJugarClicked);
}



// Slots del menú
void Interfaz::onActionNivelesTriggered()
{
    // Abrimos el diálogo de selección de niveles
    Niveles dlg(this);
    dlg.exec();
}

void Interfaz::onActionJugarTriggered()
{
    // Aquí lanzarías tu lógica de juego
    QMessageBox::information(this, tr("Jugar"), tr("Iniciar juego (aquí pondrás la lógica para empezar)"));
}

// Slots de los botones centrales (misma acción que el menú)
void Interfaz::onBtnNivelesClicked()
{
    onActionNivelesTriggered();
}

void Interfaz::onBtnJugarClicked()
{
    // Si tienes menuSound (QSoundEffect* menuSound miembro), hacemos fade-out y luego abrimos el nivel.
    if (menuSound && menuSound->isPlaying()) {
        QPropertyAnimation *anim = new QPropertyAnimation(menuSound, "volume", this);
        anim->setDuration(550);
        anim->setStartValue(menuSound->volume());
        anim->setEndValue(0.0);
        connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
            if (menuSound) menuSound->stop();
            anim->deleteLater();

            // abrir GameWindow (el constructor de GameWindow iniciará la música del nivel)
            GameWindow *gw = new GameWindow(1, this);
            gw->show();
            this->hide();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        // fallback: abrir inmediatamente
        if (menuSound) menuSound->stop();
        GameWindow *gw = new GameWindow(1, this);
        gw->show();
        this->hide();
    }
}



