#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);

    // Сервер
    void startServer(quint16 port = 12345);
    void stopServer();

    // Клиент
    void connectToHost(const QString &host, quint16 port = 12345);
    void disconnectFromHost();

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &errorString);

public slots:
    void sendData(const QByteArray &data);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpServer *m_server = nullptr;
    QTcpSocket *m_socket = nullptr;
};

#endif // NETWORKMANAGER_H
