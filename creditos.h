// creditos.h
#ifndef CREDITOS_H
#define CREDITOS_H

#include <QDialog>

class QLabel;
class QPushButton;

// La clase Créditos hereda de QDialog, al igual que Niveles.
class Creditos : public QDialog
{
    Q_OBJECT
public:
    // El constructor sigue recibiendo el padre (parent)
    explicit Creditos(QWidget *parent = nullptr);

    // No necesitamos slots para activar ítems, solo para cerrar.
};

#endif // CREDITOS_H
