#include "statistics.h"
#include <QVBoxLayout>
#include <QLabel>

Statistics::Statistics(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Статистика");
    setFixedSize(300, 200);

    QVBoxLayout *layout = new QVBoxLayout(this);

    winsLabel = new QLabel("Побед: 0", this);
    lossesLabel = new QLabel("Поражений: 0", this);
    bestTimeLabel = new QLabel("Лучшее время: --", this);

    layout->addWidget(winsLabel);
    layout->addWidget(lossesLabel);
    layout->addWidget(bestTimeLabel);
    layout->addStretch();

    setLayout(layout);
}

void Statistics::updateStats(int wins, int losses, int bestTime)
{
    winsLabel->setText("Побед: " + QString::number(wins));
    lossesLabel->setText("Поражений: " + QString::number(losses));
    bestTimeLabel->setText("Лучшее время: " +
                         (bestTime == INT_MAX ? "--" : QString::number(bestTime)));
}
