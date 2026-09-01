#include "player/MpvPlayer.h"

#include <QGuiApplication>
#include <QDir>
#include <QProcess>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

class MpvPlayerTest final : public QObject
{
    Q_OBJECT

private slots:
    void startsWithoutPlayback()
    {
        MpvPlayer player;
        QVERIFY(!player.hasActivePlayback());
        QVERIFY(player.activeTitle().isEmpty());
        QCOMPARE(player.position(), 0.0);
        QCOMPARE(player.duration(), 0.0);
        QVERIFY(player.errorMessage().isEmpty());
    }

    void boundsVolume()
    {
        MpvPlayer player;
        player.setVolume(-12.0);
        QCOMPARE(player.volume(), 0.0);
        player.setVolume(140.0);
        QCOMPARE(player.volume(), 100.0);
    }

    void stopIsIdempotent()
    {
        MpvPlayer player;
        player.stop();
        player.stop();
        QVERIFY(!player.hasActivePlayback());
    }

    void initializesOpenGlRenderContext()
    {
        if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
            QSKIP("The offscreen Qt platform does not expose a window framebuffer");
        }
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
        QQuickWindow window;
        MpvPlayer player(window.contentItem());
        player.setSize(QSizeF(320, 180));
        window.resize(320, 180);
        window.show();
        QTRY_VERIFY_WITH_TIMEOUT(window.isExposed(), 3'000);
        QTRY_VERIFY_WITH_TIMEOUT(player.renderReady(), 3'000);
        QVERIFY2(player.errorMessage().isEmpty(), qPrintable(player.errorMessage()));

        QTemporaryDir mediaDirectory;
        QVERIFY(mediaDirectory.isValid());
        const QString videoPath = QDir(mediaDirectory.path()).filePath(QStringLiteral("fixture.mp4"));
        QCOMPARE(QProcess::execute(
            QStringLiteral("/usr/bin/ffmpeg"),
            {QStringLiteral("-loglevel"), QStringLiteral("error"), QStringLiteral("-y"),
             QStringLiteral("-f"), QStringLiteral("lavfi"),
             QStringLiteral("-i"), QStringLiteral("color=c=red:s=320x180:d=1"),
             QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"), videoPath}), 0);
        player.play(QUrl::fromLocalFile(videoPath).toString(), QStringLiteral("Fixture"));
        QTRY_VERIFY_WITH_TIMEOUT(!player.buffering(), 3'000);
        QVERIFY(player.hasActivePlayback());
        QCOMPARE(player.activeTitle(), QStringLiteral("Fixture"));
        QVERIFY2(player.errorMessage().isEmpty(), qPrintable(player.errorMessage()));
        player.stop();
        window.releaseResources();
    }
};

QTEST_MAIN(MpvPlayerTest)
#include "tst_mpv_player.moc"
