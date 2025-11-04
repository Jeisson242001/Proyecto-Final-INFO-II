#ifndef INTERFAZ_H
#define INTERFAZ_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QSoundEffect>

QT_BEGIN_NAMESPACE
namespace Ui {
class Interfaz;
}
QT_END_NAMESPACE

class Interfaz : public QMainWindow
{
    Q_OBJECT

public:
    Interfaz(QWidget *parent = nullptr);
    ~Interfaz();

private slots:
    void onActionNivelesTriggered();
    void onActionJugarTriggered();
    void onBtnNivelesClicked();
    void onBtnJugarClicked();

private:
    Ui::Interfaz *ui;
    QGraphicsScene *scene;

    QSoundEffect *menuSound;

    void setupScene();
    void setupMenuAndButtons();
};



#endif // INTERFAZ_H
