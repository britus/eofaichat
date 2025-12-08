#include <cmath>
#include <mainwindow.h>
#include <progresspopup.h>
#include <QApplication>
#include <QBrush>
#include <QDebug>
#include <QGraphicsBlurEffect>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QTimer>
#include <QWidget>

ProgressPopup::ProgressPopup(QWidget *parent)
    : QWidget(parent)
    , rotationAngle(0)
{
    setFixedSize(100, 100);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
    setEnabled(false);

    rotationTimer = new QTimer(this);
    connect(rotationTimer, &QTimer::timeout, this, &ProgressPopup::updateRotation);
}

ProgressPopup::~ProgressPopup()
{
    //qDebug() << __func__;
}

static inline MainWindow *mainWindow()
{
    // Finding the first QMainWindow in QApplication
    const QWidgetList allWidgets = QApplication::topLevelWidgets();
    for (QObject *obj : allWidgets) {
        if (MainWindow *mw = qobject_cast<MainWindow *>(obj)) {
            return mw;
        }
    }
    return nullptr;
}

void ProgressPopup::showCentered()
{
    if (MainWindow *w = mainWindow()) {
        // Center the popup on the main window
        QRect mainWindowRect = w->frameGeometry();
        QRect popupRect = frameGeometry();
        popupRect.moveCenter(mainWindowRect.center());
        move(popupRect.topLeft());
    }
    show();
}

void ProgressPopup::setBlurEffect(bool enable)
{
    if (MainWindow *mw = mainWindow()) {
        if (QWidget *cw = mw->getCentralWidget()) {
            cw->setEnabled(!enable);
            QGraphicsBlurEffect *bf = qobject_cast<QGraphicsBlurEffect *>(cw->graphicsEffect());
            if (enable && !bf) {
                bf = new QGraphicsBlurEffect();
                bf->setBlurRadius(5);
                bf->setBlurHints(QGraphicsBlurEffect::AnimationHint);
                cw->setGraphicsEffect(bf);
                // Update every 50ms for smooth animation
                rotationTimer->start(50);
            } else if (!enable && bf->blurHints()) {
                delete bf; // first remove effect
                cw->setGraphicsEffect(nullptr);
                rotationTimer->stop();
            }
        }
    }
}

void ProgressPopup::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw the circular background
    QRect circleRect(10, 10, 80, 80);
    QPen pen(Qt::white, 3);
    painter.setPen(pen);
    painter.drawEllipse(circleRect);

    // Draw the 8-corner star
    QPainterPath starPath;
    const int corners = 8;
    const double outerRadius = 35;
    const double innerRadius = 15;
    const double centerX = 50;
    const double centerY = 50;

    starPath.moveTo(centerX + outerRadius, centerY);

    for (int i = 0; i < corners; ++i) {
        double angle = M_PI / 2 + i * 2 * M_PI / corners;
        double x1 = centerX + outerRadius * cos(angle);
        double y1 = centerY - outerRadius * sin(angle);
        starPath.lineTo(x1, y1);

        angle += M_PI / corners;
        double x2 = centerX + innerRadius * cos(angle);
        double y2 = centerY - innerRadius * sin(angle);
        starPath.lineTo(x2, y2);
    }

    starPath.closeSubpath();

    // Apply rotation to the star
    QTransform transform;
    transform.translate(centerX, centerY);
    transform.rotate(rotationAngle);
    transform.translate(-centerX, -centerY);
    starPath = transform.map(starPath);

    // Draw the star with a gradient
    QLinearGradient gradient(centerX - outerRadius, centerY - outerRadius, centerX + outerRadius, centerY + outerRadius);
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(1, Qt::lightGray);

    painter.fillPath(starPath, QBrush(gradient));
    painter.setPen(QPen(Qt::white, 2));
    painter.drawPath(starPath);
}

void ProgressPopup::updateRotation()
{
    rotationAngle = (rotationAngle + 5) % 360;
    update(); // Trigger repaint
}
