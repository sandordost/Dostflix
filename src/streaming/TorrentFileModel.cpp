#include "streaming/TorrentFileModel.h"

TorrentFileModel::TorrentFileModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TorrentFileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_files.size());
}

QVariant TorrentFileModel::data(const QModelIndex &index, int role) const
{
    const TorrentVideoFile *file = at(index.row());
    if (!index.isValid() || file == nullptr) return {};
    switch (role) {
    case TorrentIndexRole: return file->torrentIndex;
    case PathRole: return file->path;
    case SizeBytesRole: return file->sizeBytes;
    default: return {};
    }
}

QHash<int, QByteArray> TorrentFileModel::roleNames() const
{
    return {{TorrentIndexRole, "torrentIndex"}, {PathRole, "path"},
            {SizeBytesRole, "sizeBytes"}};
}

void TorrentFileModel::replace(std::vector<TorrentVideoFile> files)
{
    beginResetModel();
    m_files = std::move(files);
    endResetModel();
}

const TorrentVideoFile *TorrentFileModel::at(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_files.size())) return nullptr;
    return &m_files.at(static_cast<std::size_t>(row));
}
