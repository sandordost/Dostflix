#include "streaming/BufferController.h"

#include <cmath>
#include <QtTest>

class BufferControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void waitsForThirtyContiguousSecondsInitially()
    {
        const BufferEstimate estimate = BufferController::estimate(
            20'000'000, 1'000'000'000, 8'000'000, 5'000'000);
        QCOMPARE(estimate.playableSeconds, 20.0);
        QCOMPARE(estimate.targetSeconds, 30.0);
        QCOMPARE(estimate.estimatedWaitSeconds, 2.0);
        QVERIFY(!estimate.ready);
    }

    void resumesAtTenSecondsOnlyWithSustainableRate()
    {
        QVERIFY(!BufferController::estimate(
            12'000'000, 1'000'000'000, 8'000'000, 500'000, true).ready);
        QVERIFY(BufferController::estimate(
            12'000'000, 1'000'000'000, 8'000'000, 1'100'000, true).ready);
    }

    void completedFileStartsImmediately()
    {
        const BufferEstimate estimate = BufferController::estimate(
            100, 100, 0, 0);
        QVERIFY(estimate.ready);
        QVERIFY(std::isinf(estimate.playableSeconds));
    }
};

QTEST_GUILESS_MAIN(BufferControllerTest)
#include "tst_buffer_controller.moc"
