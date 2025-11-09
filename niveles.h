#ifndef NIVELES_H
#define NIVELES_H

#include <QDialog>

class QListWidget;
class QListWidgetItem;

class Niveles : public QDialog
{
    Q_OBJECT
public:
    explicit Niveles(QWidget *parent = nullptr);

private slots:
    // Firma correcta: QListWidget emite signals con QListWidgetItem*
    void onLevelActivated(QListWidgetItem *item);

private:
    QListWidget *list_;
};

#endif // NIVELES_H
