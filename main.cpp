#include <appbundle.h>
#include <ui/mainwindow.h>
#include <QApplication>
#include <QFile>
#include <QLocale>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QTranslator>

class ApplicationStyle : public QProxyStyle
{
public:
    int styleHint(StyleHint hint,
                  const QStyleOption *option = nullptr, //
                  const QWidget *widget = nullptr,      //
                  QStyleHintReturn *returnData = nullptr) const override
    {
        switch (hint) {
            case QStyle::SH_ComboBox_Popup: {
                return 0;
            }
            case QStyle::SH_MessageBox_CenterButtons: {
                return 0;
            }
            case QStyle::SH_FocusFrame_AboveWidget: {
                return 1;
            }
            default: {
                break;
            }
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

int main(int argc, char *argv[])
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    /* configure custom GUI style hinter */
    QStyle *style;
    if (!(style = QStyleFactory::create("Fusion"))) {
        return -1;
    }
    ApplicationStyle *appStyle = new ApplicationStyle();
    appStyle->setBaseStyle(style);

    QString css;
    QFile r(":/eofaichat.css");
    if (r.open(QFile::ReadOnly)) {
        css = r.readAll();
        r.close();
    }

    QApplication::setOrganizationName(QStringLiteral("EoF Software Labs"));
    QApplication::setOrganizationDomain(QStringLiteral("org.eof.tools.smartaichat"));
    QApplication::setApplicationName(QStringLiteral("eofaichat"));
    QApplication::setApplicationDisplayName(QStringLiteral("EoF Smart AI Chat"));
    QApplication::setApplicationVersion(QStringLiteral("%1.%2").arg(getBundleVersion(), getBuildNumber()));

    QApplication a(argc, argv);
    a.setPalette(darkPalette);
    a.setStyleSheet(css);
    a.setStyle(appStyle);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "eofaichat_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();

    return a.exec();
}
