#pragma once
#include <toolmodel.h>
#include <QObject>

class ToolService : public QObject
{
    Q_OBJECT

public:
    explicit ToolService(QObject *parent = nullptr);

    bool execute(const ToolModel *model, const QString &function, const QString &arguments);

signals:
    void executeCompleted(const QByteArray &content);
};
