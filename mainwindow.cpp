#include "mainwindow.h"
#include "cellbutton.h"
#include "gameboard.h"
#include "statistics.h"
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QComboBox>
#include <QFont>
#include <QIcon>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_gameBoard(nullptr),
      m_timer(new QTimer(this)),
      m_timeElapsed(0),
      wins(0),
      losses(0),
      bestTime(INT_MAX)
{
    setWindowTitle("Сапер");
    setStyleSheet("background-color: silver;");

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    m_mainLayout = new QVBoxLayout(central);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);

    createMenu();
    createTopPanel();

    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::dataReceived, this, &MainWindow::onNetworkDataReceived);
    connect(m_networkManager, &NetworkManager::connected, this, &MainWindow::onNetworkConnected);
    connect(m_networkManager, &NetworkManager::disconnected, this, &MainWindow::onNetworkDisconnected);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &MainWindow::onNetworkError);

    setupNetworkMenu();

    m_boardLayout = new QGridLayout;
    m_boardLayout->setSpacing(1);
    m_boardLayout->setContentsMargins(5, 5, 5, 5);

    m_boardFrame = new QWidget(this);
    m_boardFrame->setStyleSheet("background-color: gray; border: 2px inset lightgray;");
    m_boardFrame->setLayout(m_boardLayout);
    m_mainLayout->addWidget(m_boardFrame);

    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateTimer);

    statsDialog = new Statistics(this);
    loadStats();
    updateStats();

    changeMode(0); // Начинаем с уровня "Новичок"
}

void MainWindow::setupNetworkMenu()
{
    QMenu *networkMenu = QMainWindow::menuBar()->addMenu("Сетевая игра");


    QAction *hostAction = networkMenu->addAction("Создать игру (Сервер)");
    QAction *joinAction = networkMenu->addAction("Присоединиться к игре (Клиент)");

    connect(hostAction, &QAction::triggered, this, &MainWindow::startServer);
    connect(joinAction, &QAction::triggered, this, &MainWindow::connectToServer);
}

void MainWindow::sendMove(int row, int col)
{
    if (m_networkManager->isConnected()) {
        QString msg = QString("MOVE %1 %2").arg(row).arg(col);
        m_networkManager->sendData(msg.toUtf8());
    }
}

void MainWindow::onNetworkConnected()
{
    QMessageBox::information(this, "Подключено", "Вы успешно подключились к серверу.");
    m_isNetworkGame = true;
    m_isMyTurn = false; // Клиент ходит вторым
    restartGame();
}

void MainWindow::onNetworkDisconnected()
{
    QMessageBox::warning(this, "Отключено", "Соединение потеряно.");
    m_isNetworkGame = false;
    m_isMyTurn = false;
    m_networkManager->stopServer();
}

void MainWindow::onNetworkError(const QString &error)
{
    QMessageBox::critical(this, "Ошибка сети", error);
}

void MainWindow::cellLeftClick(int row, int col)
{
    if (m_isNetworkGame) {
        if (!m_isMyTurn) {
            QMessageBox::information(this, "Подождите", "Сейчас не ваш ход!");
            return;
        }
        if (m_timeElapsed == 0)
            m_timer->start(1000);

        m_gameBoard->revealCell(row, col);

        // Отправляем ход другому игроку
        sendMove(row, col);

        m_isMyTurn = false;
        m_faceButton->setText("⏸"); // Индикация хода соперника
    } else {
        if (m_timeElapsed == 0)
            m_timer->start(1000);

        m_gameBoard->revealCell(row, col);
    }
}

void MainWindow::onNetworkDataReceived(const QByteArray &data)
{
    // Ожидаем формат: "MOVE row col"
    QString msg = QString::fromUtf8(data);
    QStringList parts = msg.split(' ');
    if (parts.size() == 3 && parts[0] == "MOVE") {
        int row = parts[1].toInt();
        int col = parts[2].toInt();

        // Применяем ход другого игрока
        m_gameBoard->revealCell(row, col);

        m_isMyTurn = true; // теперь мой ход
        m_faceButton->setText("▶"); // Можно обновить UI для индикации хода
    }
}

void MainWindow::startServer()
{
    bool ok;
    quint16 port = QInputDialog::getInt(this, "Порт сервера", "Введите порт для сервера:", 12345, 1024, 65535, 1, &ok);
    if (!ok) return;

    m_networkManager->startServer(port);
    m_isNetworkGame = true;
    m_isMyTurn = true; // Хост ходит первым

    QMessageBox::information(this, "Сервер запущен", QString("Сервер запущен на порту %1").arg(port));

    restartGame();
}

void MainWindow::connectToServer()
{
    bool ok;
    QString host = QInputDialog::getText(this, "Подключение к серверу", "Введите IP сервера:", QLineEdit::Normal, "127.0.0.1", &ok);
    if (!ok || host.isEmpty()) return;

    quint16 port = QInputDialog::getInt(this, "Порт сервера", "Введите порт сервера:", 12345, 1024, 65535, 1, &ok);
    if (!ok) return;

    m_networkManager->connectToHost(host, port);
}

void MainWindow::createMenu()
{
    QMenuBar *menuBar = new QMenuBar(this);

    // Меню "Игра"
    QMenu *gameMenu = menuBar->addMenu("Игра");

    QAction *newGameAction = gameMenu->addAction("Новая игра");
    connect(newGameAction, &QAction::triggered, this, &MainWindow::restartGame);

    gameMenu->addSeparator();

    QAction *beginnerAction = gameMenu->addAction("Новичок (9x9, 10 мин)");
    QAction *intermediateAction = gameMenu->addAction("Любитель (16x16, 40 мин)");
    QAction *expertAction = gameMenu->addAction("Профессионал (30x16, 99 мин)");

    connect(beginnerAction, &QAction::triggered, [this]() { changeMode(0); });
    connect(intermediateAction, &QAction::triggered, [this]() { changeMode(1); });
    connect(expertAction, &QAction::triggered, [this]() { changeMode(2); });

    // Меню "Дополнительно"
    QMenu *extraMenu = menuBar->addMenu("Дополнительно");

    QAction *statsAction = extraMenu->addAction("Статистика");
    connect(statsAction, &QAction::triggered, [this]() { statsDialog->exec(); });

    QAction *comingSoonAction = extraMenu->addAction("Coming soon...");
    comingSoonAction->setEnabled(false);
    connect(comingSoonAction, &QAction::triggered, []() {
        QMessageBox::information(nullptr, "Coming soon", "Эта функция появится в будущих обновлениях!");
    });

    setMenuBar(menuBar);
}

void MainWindow::createTopPanel()
{
    m_modeSelector = new QComboBox(this);
    m_modeSelector->addItem("Лёгкий (9x9)");
    m_modeSelector->addItem("Средний (16x16)");
    m_modeSelector->addItem("Сложный (30x16)");
    m_modeSelector->setFixedWidth(160);
    m_modeSelector->setVisible(false); // Скрываем, так как теперь выбор в меню

    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->addStretch();
    modeLayout->addWidget(m_modeSelector);
    modeLayout->addStretch();
    m_mainLayout->addLayout(modeLayout);

    QFont lcdFont("Courier", 18, QFont::Bold);
    QWidget *frame = new QWidget();
    frame->setStyleSheet("background-color: gray; border: 2px inset lightgray;");
    frame->setFixedHeight(50);

    QHBoxLayout *frameLayout = new QHBoxLayout(frame);
    frameLayout->setContentsMargins(5, 5, 5, 5);

    m_mineCounter = new QLabel("010");
    m_mineCounter->setAlignment(Qt::AlignCenter);
    m_mineCounter->setFont(lcdFont);
    m_mineCounter->setStyleSheet("background-color: black; color: red; border: 2px solid darkgray;");
    m_mineCounter->setFixedSize(60, 35);
    frameLayout->addWidget(m_mineCounter);

    m_faceButton = new QPushButton(this);
    m_faceButton->setIcon(QIcon(":/images/face.png"));
    m_faceButton->setIconSize(QSize(32, 32));
    m_faceButton->setFixedSize(40, 40);
    m_faceButton->setStyleSheet(R"(
        QPushButton {
            background-color: lightgray;
            border: 2px outset white;
        }
        QPushButton:pressed {
            border: 2px inset gray;
        })");
    connect(m_faceButton, &QPushButton::clicked, this, &MainWindow::restartGame);

    frameLayout->addStretch();
    frameLayout->addWidget(m_faceButton);
    frameLayout->addStretch();

    m_timerLabel = new QLabel("000");
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setFont(lcdFont);
    m_timerLabel->setStyleSheet("background-color: black; color: red; border: 2px solid darkgray;");
    m_timerLabel->setFixedSize(60, 35);
    frameLayout->addWidget(m_timerLabel);

    m_mainLayout->addWidget(frame);
}

// Остальные методы
//   ) остаются без изменений,
// как в предыдущих версиях, но с добавленными улучшениями по цветам клеток


void MainWindow::changeMode(int index)
{
    int rows = 9, cols = 9, mines = 10;
    switch (index) {
        case 0: rows = 9;  cols = 9;  mines = 10; break;
        case 1: rows = 16; cols = 16; mines = 40; break;
        case 2: rows = 16; cols = 30; mines = 99; break;
    }

    if (m_gameBoard)
        delete m_gameBoard;

    m_gameBoard = new GameBoard(rows, cols, mines, this);
    connect(m_gameBoard, &GameBoard::cellUpdated, this, &MainWindow::updateCell);
    connect(m_gameBoard, &GameBoard::gameOver, this, &MainWindow::handleGameOver);
    connect(m_gameBoard, &GameBoard::flagsChanged, this, &MainWindow::updateMineCounter);

    m_rows = rows;
    m_cols = cols;
    m_mines = mines;

    createBoard();
    restartGame();

    int width = cols * 32 + 40;
    int height = rows * 32 + 150;
    setMinimumSize(width, height);
    resize(width, height);
}

void MainWindow::createBoard()
{
    QLayoutItem *item;
    while ((item = m_boardLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    m_buttons.resize(m_rows);
    for (int row = 0; row < m_rows; ++row) {
        m_buttons[row].resize(m_cols);
        for (int col = 0; col < m_cols; ++col) {
            CellButton *btn = new CellButton(row, col, this);
            btn->setFixedSize(32, 32);
            btn->setStyleSheet(R"(
                QPushButton {
                    background-color: lightgray;
                    border: 2px outset white;
                }
                QPushButton:pressed {
                    border: 2px inset gray;
                })");
            connect(btn, &CellButton::leftClicked, this, &MainWindow::cellLeftClick);
            connect(btn, &CellButton::rightClicked, this, &MainWindow::cellRightClick);
            m_buttons[row][col] = btn;
            m_boardLayout->addWidget(btn, row, col);
        }
    }
}

//void MainWindow::cellLeftClick(int row, int col)
//    if (m_timeElapsed == 0)
//    m_gameBoard->revealCell(row, col);
//}

void MainWindow::cellRightClick(int row, int col)
{
    m_gameBoard->toggleFlag(row, col);
}

void MainWindow::updateTimer()
{
    ++m_timeElapsed;
    m_timerLabel->setText(QString("%1").arg(m_timeElapsed, 3, 10, QChar('0')));
}

void MainWindow::updateMineCounter(int flagsLeft)
{
    m_mineCounter->setText(QString("%1").arg(flagsLeft, 3, 10, QChar('0')));
}


void MainWindow::restartGame()
{
    m_gameBoard->reset(m_rows, m_cols, m_mines);
    m_timer->stop();
    m_timeElapsed = 0;
    m_timerLabel->setText("000");
    m_mineCounter->setText(QString("%1").arg(m_mines, 3, 10, QChar('0')));

    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            m_buttons[row][col]->setEnabled(true);
            m_buttons[row][col]->setIcon(QIcon());
            m_buttons[row][col]->setText("");
            // Устанавливаем темный цвет для закрытых клеток
            m_buttons[row][col]->setStyleSheet(R"(
                QPushButton {
                    background-color: #c0c0c0;
                    border: 2px outset white;
                }
                QPushButton:pressed {
                    border: 2px inset gray;
                })");
            // Сбрасываем шрифт к стандартному
            QFont font = m_buttons[row][col]->font();
            font.setBold(false);
            font.setPointSize(8);
            m_buttons[row][col]->setFont(font);
        }
    }

    m_faceButton->setStyleSheet(R"(
        QPushButton {
            background-color: lightgray;
            border: 2px outset white;
        }
        QPushButton:pressed {
            border: 2px inset gray;
        })");
}

void MainWindow::updateCell(int row, int col)
{
    CellButton *btn = m_buttons[row][col];
    const Cell &cell = m_gameBoard->getCell(row, col);
    btn->setFlat(false);

    // Базовая стилизация для всех кнопок
    QString baseStyle = R"(
        QPushButton {
            border: 2px outset white;
        }
        QPushButton:pressed {
            border: 2px inset gray;
        })";

    if (cell.isRevealed) {
        btn->setEnabled(false);
        btn->setIcon(QIcon());

        // Устанавливаем цвет фона для открытых клеток
        if (cell.adjacentMines == 0) {
            btn->setStyleSheet("background-color: #e0e0e0;" + baseStyle); // Светло-серый для пустых
        } else {
            btn->setStyleSheet("background-color: lightgray;" + baseStyle); // Обычный для клеток с цифрами
        }

        if (cell.hasMine) {
            btn->setIcon(QIcon(":/images/mine.png"));
            btn->setText("");
        } else if (cell.adjacentMines > 0) {
            btn->setText(QString::number(cell.adjacentMines));
            QFont font = btn->font();
            font.setBold(true);
            font.setPointSize(16);
            btn->setFont(font);

            QColor colors[9] = {
                Qt::transparent,
                QColor(0, 0, 255),
                QColor(0, 128, 0),
                QColor(255, 0, 0),
                QColor(0, 0, 128),
                QColor(128, 0, 0),
                QColor(0, 128, 128),
                QColor(0, 0, 0),
                QColor(128, 128, 128)
            };

            QPalette pal = btn->palette();
            pal.setColor(QPalette::ButtonText, colors[cell.adjacentMines]);
            btn->setPalette(pal);
        } else {
            btn->setText("");
        }
    } else {
        btn->setEnabled(true);
        btn->setText("");
        btn->setIcon(cell.isFlagged ? QIcon(":/images/flag.png") : QIcon());
        btn->setStyleSheet("background-color: #c0c0c0;" + baseStyle); // Темно-серый для закрытых
    }
}

void MainWindow::handleGameOver(bool won)
{
    m_timer->stop();

    // Показываем все мины и блокируем поле
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            const Cell &cell = m_gameBoard->getCell(row, col);
            if (cell.hasMine && !cell.isFlagged) {
                m_buttons[row][col]->setIcon(QIcon(":/images/mine.png"));
            }
            m_buttons[row][col]->setEnabled(false);
        }
    }

    // Обновляем статистику и интерфейс
    if (won) {
        m_faceButton->setStyleSheet("background-color: lightgreen;");
        wins++;
        if (m_timeElapsed < bestTime) {
            bestTime = m_timeElapsed;
        }
        QMessageBox::information(this, "Победа!",
                               QString("Вы выиграли за %1 секунд!").arg(m_timeElapsed));
    } else {
        m_faceButton->setStyleSheet("background-color: red;");
        losses++;
        QMessageBox::information(this, "Поражение", "Вы наступили на мину!");
    }

    saveStats();
    updateStats();
}

void MainWindow::saveStats()
{
    QSettings settings("Minesweeper", "Stats");
    settings.setValue("wins", wins);
    settings.setValue("losses", losses);
    settings.setValue("bestTime", bestTime);
}

void MainWindow::loadStats()
{
    QSettings settings("Minesweeper", "Stats");
    wins = settings.value("wins", 0).toInt();
    losses = settings.value("losses", 0).toInt();
    bestTime = settings.value("bestTime", INT_MAX).toInt();
}

void MainWindow::updateStats()
{
    statsDialog->updateStats(wins, losses, bestTime);
}



