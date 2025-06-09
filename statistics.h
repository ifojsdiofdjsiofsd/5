#ifndef STATISTICS_H
#define STATISTICS_H

#include <QDialog>
#include <QLabel>  // Добавляем этот include

class Statistics : public QDialog
{
    Q_OBJECT
public:
    explicit Statistics(QWidget *parent = nullptr);
    void updateStats(int wins, int losses, int bestTime);

private:
    QLabel *winsLabel;
    QLabel *lossesLabel;
    QLabel *bestTimeLabel;
};

#endif // STATISTICS_H
