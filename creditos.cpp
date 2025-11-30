// creditos.cpp
#include "creditos.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit> // Usamos QTextEdit para facilitar el formato de los creditos

Creditos::Creditos(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Créditos del Juego"));
    setMinimumSize(400, 300);

    // 1. Contenido
    // Texto de los créditos
    QString creditText = R"(
        <h2>Proyecto Final: Infierno en Okinawa</h2>
        <hr>
        <b>Desarrolladores Principales:</b>
        <ul>
            <li>Rigoberto Berrio Berrio</li>
            <li>Jeisson Stevens Martinez Arevalo</li>
        </ul>
        <b>Asignatura:</b> Informática II - 2598521 (2025-2)
        <br>
        <b>Tecnologías Utilizadas:</b>
        <ul>
            <li>C++ 17</li>
            <li>Framework Qt (Core, Widgets, GraphicsView, Multimedia)</li>
        </ul>
        <br>
        Gracias por jugar.
    )";

    QTextEdit *textDisplay = new QTextEdit(this);
    textDisplay->setHtml(creditText); // Permite usar formato HTML
    textDisplay->setReadOnly(true);   // El usuario no puede editar el texto

    // 2. Botón de Cierre
    QPushButton *btnClose = new QPushButton(tr("Cerrar"), this);

    // 3. Conexión y Layout
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addWidget(textDisplay);
    v->addWidget(btnClose);

    setLayout(v);
}
