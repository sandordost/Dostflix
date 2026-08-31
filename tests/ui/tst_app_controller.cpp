#include "ui/AppController.h"

#include <QSignalSpy>
#include <QtTest>

class AppControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void nowWatchingIsConditional()
    {
        AppController controller;
        QVERIFY(!controller.hasActivePlayback());
        QSignalSpy spy(&controller, &AppController::activePlaybackChanged);

        controller.setActivePlayback(QStringLiteral("m1"), QStringLiteral("Arrival"), 42);
        QVERIFY(controller.hasActivePlayback());
        QCOMPARE(controller.activeTitle(), QStringLiteral("Arrival"));
        QCOMPARE(controller.watchedSeconds(), 42);
        QCOMPARE(spy.count(), 1);

        controller.clearActivePlayback();
        QVERIFY(!controller.hasActivePlayback());
        QCOMPARE(spy.count(), 2);
    }

    void clearingAnEmptySessionDoesNotEmit()
    {
        AppController controller;
        QSignalSpy spy(&controller, &AppController::activePlaybackChanged);
        controller.clearActivePlayback();
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_GUILESS_MAIN(AppControllerTest)
#include "tst_app_controller.moc"
