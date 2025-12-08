#pragma once
#include <QPainter>
#include <QTimer>
#include <QWidget>

class ProgressPopup : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressPopup(QWidget *parent = nullptr);
    ~ProgressPopup();
    void showCentered();
    void setBlurEffect(bool enable);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateRotation();

private:
    QTimer *rotationTimer;
    int rotationAngle;
};
