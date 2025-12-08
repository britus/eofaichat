#include "syntaxcolormodel.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

SyntaxColorModel::SyntaxColorModel(QObject *parent)
    : QObject(parent)
{}

bool SyntaxColorModel::loadFromFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "SyntaxColorModel: cannot open file" << filePath;
        return false;
    }
    QByteArray data = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "SyntaxColorModel: json parse error" << err.errorString();
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
                qWarning() << "SyntaxColorModel: invalid color" << colorStr << "for" << lang << token;
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
