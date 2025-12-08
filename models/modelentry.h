#pragma once
#include <QString>

struct ModelEntry
{
    QString filename;
    QString description;
    QString filesize;

    bool isDefault = false;
    bool bestGPTJ = false;
    bool bestLlama = false;
    bool bestMPT = false;

    bool installed = false;
    bool calcHash = false;

    // internal state
    bool downloading = false;
    qint64 bytesReceived = 0;
    qint64 bytesTotal = 0;
    double speed = 0.0;                 // bytes/sec
};
