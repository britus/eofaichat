#pragma once
#include <QCoreApplication>
#include <QObject>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class DownloadManager;
static DownloadManager *m_instance;

class DownloadManager : public QObject
{
    Q_OBJECT

public:
    static DownloadManager *instance()
    {
        if (!m_instance) {
            m_instance = new DownloadManager(qApp);
        }
        return m_instance;
    }

    void startDownload(const QString &filename, int row = 0);
    void cancelDownload(const QString &filename, int row = 0);
    void download(const QString &filename, const QUrl &url, int row = 0);
    void cancel(const QString &filename, int row = 0);
    void signalDialogOpened();
    const QString &downloadPath() const;
    void setDownloadPath(const QString &path);

signals:
    void progressUpdated(int row, qint64 received, qint64 total, double speed);
    void downloadFinished(int row);
    void errorOccured(const QString &msg);

private:
    explicit DownloadManager(QObject *parent = nullptr);

private:
    struct ActiveDownload
    {
        QString filename;
        QNetworkReply *reply = nullptr;
        qint64 lastBytes = 0;
        qint64 lastTimestamp = 0;
        int row = -1;
    };

    QNetworkAccessManager m_mgr;
    QMap<QString, ActiveDownload> m_active;
    QString m_downloadPath;
};
