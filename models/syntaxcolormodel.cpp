#include <syntaxcolormodel.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>

SyntaxColorModel::SyntaxColorModel(QObject *parent)
    : QObject(parent)
{}

bool SyntaxColorModel::loadFromFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[SyntaxColorModel] cannot open file" << filePath;
        return false;
    }
    QByteArray data = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[SyntaxColorModel] Json parse error" << err.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        QString lang = it.key();
        QJsonObject langObj = it.value().toObject();
        QMap<QString, QColor> tokenMap;
        for (auto jt = langObj.begin(); jt != langObj.end(); ++jt) {
            QString token = jt.key();
            QString colorStr = jt.value().toString();
            QColor col(colorStr);
            if (!col.isValid()) {
                qWarning() << "[SyntaxColorModel] Invalid color" << colorStr << "for" << lang << token;
                col = Qt::black;
            }
            tokenMap.insert(token, col);
        }
        m_map.insert(lang.toLower(), tokenMap);
    }
    return true;
}

QColor SyntaxColorModel::colorFor(const QString &language, const QString &token, const QColor &defaultColor) const
{
    QString langKey = language.toLower();
    if (!m_map.contains(langKey))
        return defaultColor;
    const QMap<QString, QColor> &tokenMap = m_map.value(langKey);
    if (!tokenMap.contains(token))
        return defaultColor;
    return tokenMap.value(token);
}

bool SyntaxColorModel::hasLanguage(const QString &language) const
{
    return m_map.contains(language.toLower());
}

// Try load color model from relative file; adjust path as needed.
void SyntaxColorModel::loadSyntaxModel()
{
    // if using resources
    QString colorFile = ":/syntaxcolors.json";
    if (!QFile::exists(colorFile)) {
        // fallback to local file in executable dir
        QString configPath;
        configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        configPath = configPath + QDir::separator() + "Syntax";
        QDir configDir(configPath);
        if (!configDir.exists()) {
            QFile::Permissions permissions;
            permissions.setFlag(QFile::Permission::ReadOwner, true);
            permissions.setFlag(QFile::Permission::ReadGroup, true);
            permissions.setFlag(QFile::Permission::WriteOwner, true);
            permissions.setFlag(QFile::Permission::WriteGroup, true);
            permissions.setFlag(QFile::Permission::ExeOwner, true);
            permissions.setFlag(QFile::Permission::ExeGroup, true);
            if (!configDir.mkpath(configDir.absolutePath(), permissions)) {
                qWarning("[SyntaxColorModel] Unable to create directory: %s", qPrintable(configDir.absolutePath()));
                return;
            }
        }
        colorFile = configDir.absoluteFilePath("syntaxcolors.json");
    }
    if (!loadFromFile(colorFile)) {
        qWarning() << "[SyntaxColorModel] Failed to load syntax color model from" << colorFile;
    }
}
