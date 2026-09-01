#include "player/MpvPlayer.h"

#include <QGuiApplication>
#include <QDir>
#include <QFile>
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
        QVERIFY(player.subtitleTracks().isEmpty());
        QCOMPARE(player.selectedSubtitleId(), QStringLiteral("no"));
        QCOMPARE(player.subtitleDelay(), 0.0);
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

    void boundsSubtitleDelay()
    {
        MpvPlayer player;
        player.setSubtitleDelay(-75.0);
        QCOMPARE(player.subtitleDelay(), -60.0);
        player.setSubtitleDelay(90.0);
        QCOMPARE(player.subtitleDelay(), 60.0);
    }

    void rejectsUnsupportedSubtitleFiles()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = QDir(directory.path()).filePath(QStringLiteral("subtitle.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("not a supported subtitle");
        file.close();

        MpvPlayer player;
        player.play(QStringLiteral("file:///unused.mp4"), QStringLiteral("Fixture"));
        player.addSubtitleFile(QUrl::fromLocalFile(path));
        QVERIFY(player.errorMessage().contains(QStringLiteral(".srt")));
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
             QStringLiteral("-i"), QStringLiteral("color=c=red:s=320x180:d=5"),
             QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"), videoPath}), 0);
        player.play(QUrl::fromLocalFile(videoPath).toString(), QStringLiteral("Fixture"), 2.0);
        QTRY_VERIFY_WITH_TIMEOUT(!player.buffering(), 3'000);
        QTRY_VERIFY_WITH_TIMEOUT(player.position() >= 1.5, 3'000);
        QVERIFY(player.hasActivePlayback());
        QCOMPARE(player.activeTitle(), QStringLiteral("Fixture"));
        QVERIFY2(player.errorMessage().isEmpty(), qPrintable(player.errorMessage()));

        const QString subtitlePath = QDir(mediaDirectory.path()).filePath(QStringLiteral("fixture.srt"));
        QFile subtitle(subtitlePath);
        QVERIFY(subtitle.open(QIODevice::WriteOnly));
        subtitle.write("1\n00:00:00,000 --> 00:00:04,000\nDostflix subtitle test\n");
        subtitle.close();
        player.addSubtitleFile(QUrl::fromLocalFile(subtitlePath));
        QTRY_VERIFY_WITH_TIMEOUT(!player.subtitleTracks().isEmpty(), 3'000);
        QVERIFY(player.selectedSubtitleId() != QStringLiteral("no"));
        QVERIFY2(player.errorMessage().isEmpty(), qPrintable(player.errorMessage()));
        QSignalSpy stoppingSpy(&player, &MpvPlayer::playbackStopping);
        player.stop();
        QCOMPARE(stoppingSpy.size(), 1);
        QVERIFY(stoppingSpy.first().first().toDouble() >= 1.5);
        window.releaseResources();
    }
};

QTEST_MAIN(MpvPlayerTest)
#include "tst_mpv_player.moc"
