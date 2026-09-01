#include <QtQuickTest>
#include <QQuickWindow>
#include <QSGRendererInterface>

class QuickTestSetup final : public QObject
{
    Q_OBJECT

public slots:
    void applicationAvailable()
    {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(dostflix_qml, QuickTestSetup)
#include "tst_qml.moc"
