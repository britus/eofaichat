#pragma once

#include <QSettings>
#include <QSplitter>
#include <QWidget>

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager();

    // Window settings
    void saveWindowSize(const QWidget *window);
    void loadWindowSize(QWidget *window);

    // Splitter settings
    void saveSplitterPosition(QSplitter *splitter);
    void loadSplitterPosition(QSplitter *splitter);

    // Generic property setter/getter
    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

private:
    QSettings *m_settings;

private:
    inline void flushSettings();
};
