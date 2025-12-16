#include <settingsmanager.h>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QScreen>
#include <QStandardPaths>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    // Create settings object with proper path in AppConfigLocation
    QString orgName = qApp->organizationName();
    QString appName = qApp->applicationName();

    // server tools configuration
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

    m_settings = new QSettings(          //
        QSettings::Format::NativeFormat, //
        QSettings::Scope::UserScope,     //
        orgName,
        appName,
        this);
    m_settings->setDefaultFormat(QSettings::Format::NativeFormat);
    m_settings->setFallbacksEnabled(true);
    m_settings->setPath( //
        m_settings->format(),
        m_settings->scope(),
        dir.absoluteFilePath("eofaichat.settings"));
    flushSettings();
}

SettingsManager::~SettingsManager()
{
    if (m_settings && m_settings->isWritable()) {
        m_settings->sync();
    }
}

inline void SettingsManager::flushSettings()
{
    if (m_settings->isWritable()) {
        m_settings->sync();
    } else {
        qCritical("[SettingsManager] Unable to write file %s", //
                  qPrintable(m_settings->fileName()));
    }
}

void SettingsManager::saveWindowSize(const QWidget *window)
{
    if (!window)
        return;

    m_settings->setValue("window_width", window->geometry().width());
    m_settings->setValue("window_height", window->geometry().height());
    m_settings->setValue("window_x", window->geometry().x());
    m_settings->setValue("window_y", window->geometry().y());
    flushSettings();
}

void SettingsManager::loadWindowSize(QWidget *window)
{
    if (!window)
        return;

    // Get screen size for validation
    QScreen *screen = qApp->primaryScreen();

    // Load window size and position
    int width = m_settings->value("window_width", 1024).toInt();
    int height = m_settings->value("window_height", 720).toInt();
    int x = m_settings->value("window_x", -1).toInt();
    int y = m_settings->value("window_y", -1).toInt();

    // Validate size and position
    if (width > 0 && width < screen->size().width() && height > 0 && height < screen->size().height()) {
        // If position is not set or invalid, center the window
        if (x < 0 || y < 0) {
            x = screen->size().width() / 2 - width / 2;
            y = screen->size().height() / 2 - height / 2;
        }

        window->setGeometry(x, y, width, height);
    }
}

void SettingsManager::saveSplitterPosition(const QString prefix, QSplitter *splitter)
{
    if (!splitter || prefix.isEmpty())
        return;

    // Save splitter sizes
    const QList<int> sizes = splitter->sizes();
    if (!sizes.isEmpty()) {
        m_settings->setValue( //
            QStringLiteral("%1_splitter_left").arg(prefix),
            sizes.at(0));
        m_settings->setValue( //
            QStringLiteral("%1_splitter_right").arg(prefix),
            sizes.at(1));
        flushSettings();
    }
}

void SettingsManager::loadSplitterPosition(const QString prefix, QSplitter *splitter, int default1, int default2)
{
    if (!splitter)
        return;

    // Load splitter sizes
    int leftSize = m_settings
                       ->value( //
                           QStringLiteral("%1_splitter_left").arg(prefix),
                           default1)
                       .toInt();
    int rightSize = m_settings
                        ->value( //
                            QStringLiteral("%1_splitter_right").arg(prefix),
                            default2)
                        .toInt(); // Default to remaining space

    splitter->setSizes(QList<int>() << leftSize << rightSize);
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
    flushSettings();
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}
