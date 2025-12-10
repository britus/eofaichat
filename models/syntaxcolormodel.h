#pragma once
#include <QColor>
#include <QMap>
#include <QObject>

class SyntaxColorModel : public QObject
{
    Q_OBJECT
public:
    explicit SyntaxColorModel(QObject *parent = nullptr);

    // Lade aus JSON-Datei; return true on success
    bool loadFromFile(const QString &filePath);

    // Gibt die Farbe für ein language/key zurück; falls nicht vorhanden, defaultColor
    QColor colorFor(const QString &language, const QString &token, const QColor &defaultColor = Qt::black) const;

    // Prüfe ob wir ein Modell für die Sprache haben
    bool hasLanguage(const QString &language) const;

    // load syntax color descriptor
    void loadSyntaxModel();

private:
    // language -> token -> QColor
    QMap<QString, QMap<QString, QColor>> m_map;
};
