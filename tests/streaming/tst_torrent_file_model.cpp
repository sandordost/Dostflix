#include "streaming/TorrentFileModel.h"

#include <QtTest>

class TorrentFileModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesVideoChoices()
    {
        TorrentFileModel model;
        model.replace({{3, QStringLiteral("Movie/main.mkv"), 42'000},
                       {7, QStringLiteral("Movie/extra.mp4"), 12'000}});
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), TorrentFileModel::TorrentIndexRole), 3);
        QCOMPARE(model.data(model.index(1), TorrentFileModel::PathRole),
                 QStringLiteral("Movie/extra.mp4"));
    }
};

QTEST_GUILESS_MAIN(TorrentFileModelTest)
#include "tst_torrent_file_model.moc"
