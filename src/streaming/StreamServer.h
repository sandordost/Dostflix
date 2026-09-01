#pragma once

#include <QObject>
#include <QTcpServer>
#include <functional>

class StreamServer final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(QString url READ url NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)

public:
    using AvailabilityCheck = std::function<bool(qint64, qint64)>;
    using RangeRequested = std::function<void(qint64, qint64)>;

    explicit StreamServer(QObject *parent = nullptr);
    [[nodiscard]] bool running() const;
    [[nodiscard]] QString url() const;
    [[nodiscard]] QString errorMessage() const;

    bool start(QString filePath, qint64 fileSize, AvailabilityCheck availability,
               RangeRequested requested = {});
    void stop();

signals:
    void stateChanged();

private:
    void acceptConnection();
    void processRequest(QTcpSocket *socket);

    QTcpServer m_server;
    QString m_filePath;
    QString m_token;
    QString m_error;
    qint64 m_fileSize = 0;
    AvailabilityCheck m_availability;
    RangeRequested m_requested;
};
