#include "streaming/StreamServer.h"

#include <QHostAddress>
#include <QFile>
#include <QPointer>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>
#include <memory>
#include <algorithm>

namespace {
void sendError(QTcpSocket *socket, int status, const QByteArray &reason)
{
    socket->write("HTTP/1.1 " + QByteArray::number(status) + " " + reason
                  + "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    socket->disconnectFromHost();
}

struct Transfer final
{
    QPointer<QTcpSocket> socket;
    QFile file;
    QTimer *timer = nullptr;
    qint64 offset = 0;
    qint64 end = 0;
    StreamServer::AvailabilityCheck available;
    StreamServer::RangeRequested requested;
};
}

StreamServer::StreamServer(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &StreamServer::acceptConnection);
}

bool StreamServer::running() const { return m_server.isListening(); }
QString StreamServer::url() const
{
    return running() ? QStringLiteral("http://127.0.0.1:%1/stream/%2")
                           .arg(m_server.serverPort()).arg(m_token)
                     : QString();
}
QString StreamServer::errorMessage() const { return m_error; }

bool StreamServer::start(QString filePath, qint64 fileSize,
                         AvailabilityCheck availability, RangeRequested requested)
{
    stop();
    if (filePath.isEmpty() || fileSize <= 0 || !availability) {
        m_error = tr("The selected stream source is invalid");
        emit stateChanged();
        return false;
    }
    m_filePath = std::move(filePath);
    m_fileSize = fileSize;
    m_availability = std::move(availability);
    m_requested = std::move(requested);
    m_token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!m_server.listen(QHostAddress::LocalHost, 0)) {
        m_error = m_server.errorString();
        emit stateChanged();
        return false;
    }
    m_error.clear();
    emit stateChanged();
    return true;
}

void StreamServer::stop()
{
    if (m_server.isListening()) m_server.close();
    const auto sockets = findChildren<QTcpSocket *>();
    for (QTcpSocket *socket : sockets) socket->abort();
    m_filePath.clear();
    m_fileSize = 0;
    m_token.clear();
    m_availability = {};
    m_requested = {};
    emit stateChanged();
}

void StreamServer::acceptConnection()
{
    while (QTcpSocket *socket = m_server.nextPendingConnection()) {
        socket->setParent(this);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            processRequest(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void StreamServer::processRequest(QTcpSocket *socket)
{
    if (socket->property("requestHandled").toBool()) return;
    const QByteArray request = socket->peek(32 * 1024);
    const qsizetype headerEnd = request.indexOf("\r\n\r\n");
    if (headerEnd < 0) return;
    socket->read(headerEnd + 4);
    socket->setProperty("requestHandled", true);

    const QList<QByteArray> lines = request.left(headerEnd).split('\n');
    if (lines.isEmpty()) {
        sendError(socket, 400, "Bad Request");
        return;
    }
    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    const QByteArray expectedPath = QByteArray("/stream/") + m_token.toUtf8();
    if (requestLine.size() < 2 || requestLine.at(0) != "GET"
        || requestLine.at(1) != expectedPath) {
        sendError(socket, 404, "Not Found");
        return;
    }

    qint64 start = 0;
    qint64 end = m_fileSize - 1;
    bool partial = false;
    static const QRegularExpression rangePattern(QStringLiteral("^bytes=(\\d+)-(\\d*)$"));
    for (const QByteArray &rawLine : lines) {
        const QByteArray line = rawLine.trimmed();
        if (line.left(6).compare(QByteArrayLiteral("Range:"), Qt::CaseInsensitive) != 0) continue;
        const auto match = rangePattern.match(
            QString::fromLatin1(line.mid(line.indexOf(':') + 1).trimmed()));
        if (!match.hasMatch()) {
            sendError(socket, 416, "Range Not Satisfiable");
            return;
        }
        start = match.captured(1).toLongLong();
        if (!match.captured(2).isEmpty()) end = match.captured(2).toLongLong();
        partial = true;
        break;
    }
    if (start < 0 || start >= m_fileSize || end < start) {
        sendError(socket, 416, "Range Not Satisfiable");
        return;
    }
    end = std::min(end, m_fileSize - 1);

    auto transfer = std::make_shared<Transfer>();
    transfer->socket = socket;
    transfer->file.setFileName(m_filePath);
    transfer->offset = start;
    transfer->end = end;
    transfer->available = m_availability;
    transfer->requested = m_requested;
    if (!transfer->file.open(QIODevice::ReadOnly) || !transfer->file.seek(start)) {
        sendError(socket, 503, "Stream Not Ready");
        return;
    }

    QByteArray headers = partial ? "HTTP/1.1 206 Partial Content\r\n"
                                 : "HTTP/1.1 200 OK\r\n";
    headers += "Accept-Ranges: bytes\r\nContent-Type: application/octet-stream\r\n";
    headers += "Content-Length: " + QByteArray::number(end - start + 1) + "\r\n";
    if (partial) {
        headers += "Content-Range: bytes " + QByteArray::number(start) + "-"
            + QByteArray::number(end) + "/" + QByteArray::number(m_fileSize) + "\r\n";
    }
    headers += "Connection: close\r\n\r\n";
    socket->write(headers);

    transfer->timer = new QTimer(socket);
    transfer->timer->setInterval(50);
    const auto pump = [transfer] {
        if (!transfer->socket) return;
        if (transfer->socket->bytesToWrite() > 2 * 1024 * 1024) return;
        const qint64 remaining = transfer->end - transfer->offset + 1;
        if (remaining <= 0) {
            transfer->timer->stop();
            transfer->socket->disconnectFromHost();
            return;
        }
        const qint64 chunkSize = std::min<qint64>(remaining, 256 * 1024);
        if (!transfer->available(transfer->offset, chunkSize)) {
            if (transfer->requested) transfer->requested(transfer->offset, chunkSize);
            return;
        }
        const QByteArray chunk = transfer->file.read(chunkSize);
        if (chunk.isEmpty()) return;
        transfer->socket->write(chunk);
        transfer->offset += chunk.size();
    };
    connect(transfer->timer, &QTimer::timeout, socket, pump);
    connect(socket, &QTcpSocket::bytesWritten, socket, [pump](qint64) { pump(); });
    transfer->timer->start();
    pump();
}
