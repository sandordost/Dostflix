#include <QCoreApplication>
#include <QtTest>

class AppStartupTest final : public QObject
{
    Q_OBJECT

private slots:
    void applicationIdentityIsStable()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("SandorDost"));
        QCoreApplication::setApplicationName(QStringLiteral("Dostflix"));
        QCOMPARE(QCoreApplication::organizationName(), QStringLiteral("SandorDost"));
        QCOMPARE(QCoreApplication::applicationName(), QStringLiteral("Dostflix"));
    }
};

QTEST_GUILESS_MAIN(AppStartupTest)
#include "tst_app_startup.moc"
