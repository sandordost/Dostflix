#pragma once

#include <QAbstractListModel>
#include <vector>

struct TorrentVideoFile final
{
    int torrentIndex = -1;
    QString path;
    qint64 sizeBytes = 0;
};

class TorrentFileModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role { TorrentIndexRole = Qt::UserRole + 1, PathRole, SizeBytesRole };
    Q_ENUM(Role)

    explicit TorrentFileModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replace(std::vector<TorrentVideoFile> files);
    [[nodiscard]] const TorrentVideoFile *at(int row) const;

private:
    std::vector<TorrentVideoFile> m_files;
};
