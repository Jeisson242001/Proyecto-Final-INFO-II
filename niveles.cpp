#include "niveles.h"
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QListWidgetItem>

Niveles::Niveles(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Selecciona nivel"));
    setMinimumSize(300, 250);

    QVBoxLayout *v = new QVBoxLayout(this);

    list_ = new QListWidget(this);
    list_->addItem("Nivel 1 - Fácil");
    list_->addItem("Nivel 2 - Intermedio");
    list_->addItem("Nivel 3 - Difícil");

    // Botones
    QPushButton *btnAceptar = new QPushButton(tr("Seleccionar"), this);
    QPushButton *btnCancelar = new QPushButton(tr("Cancelar"), this);

    connect(btnAceptar, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *it = list_->currentItem();
        if (!it) {
            QMessageBox::warning(this, tr("Atención"), tr("Seleccione un nivel primero."));
            return;
        }
        QMessageBox::information(this, tr("Nivel seleccionado"), tr("Has seleccionado: %1").arg(it->text()));
        accept();
    });

    connect(btnCancelar, &QPushButton::clicked, this, &QDialog::reject);

    // Conexión directa: doble click / enter en un item activa itemActivated
    connect(list_, &QListWidget::itemActivated, this, &Niveles::onLevelActivated);

    v->addWidget(list_);
    v->addStretch();
    v->addWidget(btnAceptar);
    v->addWidget(btnCancelar);
}

// Implementación del slot declarado en el header
void Niveles::onLevelActivated(QListWidgetItem *item)
{
    if (!item) return;
    QMessageBox::information(this, tr("Nivel activado"), tr("Has activado: %1").arg(item->text()));
    accept();
}
