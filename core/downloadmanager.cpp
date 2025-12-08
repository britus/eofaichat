#include "core/downloadmanager.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
    , m_downloadPath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
{
    //
}

void DownloadManager::startDownload(const QString &filename, int row)
{
    download(filename, QUrl::fromLocalFile(filename), row);
}

void DownloadManager::cancelDownload(const QString &filename, int row)
{
    cancel(filename, row);
}

void DownloadManager::download(const QString &filename, const QUrl &url, int row)
{
    if (m_active.contains(filename))
        return;

    QNetworkRequest req(url);
    auto reply = m_mgr.get(req);

    ActiveDownload dl;
    dl.filename = filename;
    dl.reply = reply;
    dl.row = row;
    dl.lastTimestamp = QDateTime::currentMSecsSinceEpoch();

    m_active.insert(filename, dl);

    connect(reply, &QNetworkReply::downloadProgress, this, [this, filename](qint64 received, qint64 total) {
        auto &dl = m_active[filename];

        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 dt = now - dl.lastTimestamp;
        qint64 dbytes = received - dl.lastBytes;

        double speed = (dt > 0) ? (double) dbytes / ((double) dt / 1000.0) : 0.0;

        dl.lastTimestamp = now;
        dl.lastBytes = received;

        emit progressUpdated(dl.row, received, total, speed);
    });

    connect(reply, &QNetworkReply::finished, this, [this, filename]() {
        auto dl = m_active.take(filename);
        emit downloadFinished(dl.row);
        dl.reply->deleteLater();
    });
}

void DownloadManager::cancel(const QString &filename, int /*row*/)
{
    if (!m_active.contains(filename))
        return;

    auto dl = m_active.take(filename);
    dl.reply->abort();
    dl.reply->deleteLater();

    emit downloadFinished(dl.row);
}

void DownloadManager::signalDialogOpened()
{
    /* -- */
}

const QString &DownloadManager::downloadPath() const
{
    return m_downloadPath;
}

void DownloadManager::setDownloadPath(const QString &path)
{
    m_downloadPath = path;
}
