#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include "statistics.h"
#include "networkmanager.h"
class QVBoxLayout;
class QGridLayout;
class QLabel;
class QPushButton;
class QTimer;
class QComboBox;
class GameBoard;
class CellButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void createTopPanel();
    void changeMode(int index);
    void createBoard();
    void restartGame();
    void cellLeftClick(int row, int col);
    void cellRightClick(int row, int col);
    void updateCell(int row, int col);
    void handleGameOver(bool won);
    void updateTimer();
    void updateMineCounter(int flagsLeft);
    void startServer();
    void connectToServer();
    void onNetworkDataReceived(const QByteArray &data);
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString &error);
    void sendMove(int row, int col);

private:
    QMenuBar *menuBar;
    QMenu *gameMenu;
    QMenu *statsMenu;
    QAction *newGameAction;
    QAction *beginnerAction;
    QAction *intermediateAction;
    QAction *expertAction;
    QAction *statsAction;
    QAction *comingSoonAction;
    Statistics *statsDialog;
    NetworkManager *m_networkManager = nullptr;
    bool m_isNetworkGame = false;
    bool m_isMyTurn = false; // чей ход
    void setupNetworkMenu();
    // Статистика
    int wins;
    int losses;
    int bestTime;

    void createMenu();
    void saveStats();
    void loadStats();
    void updateStats();
    QVBoxLayout *m_mainLayout = nullptr;
    QGridLayout *m_boardLayout = nullptr;
    QWidget *m_boardFrame = nullptr;

    QComboBox *m_modeSelector = nullptr;
    QLabel *m_mineCounter = nullptr;
    QLabel *m_timerLabel = nullptr;
    QPushButton *m_faceButton = nullptr;

    QTimer *m_timer = nullptr;
    int m_timeElapsed = 0;

    GameBoard *m_gameBoard = nullptr;

    int m_rows = 9;
    int m_cols = 9;
    int m_mines = 10;

    QVector<QVector<CellButton*>> m_buttons;
};

#endif // MAINWINDOW_H
