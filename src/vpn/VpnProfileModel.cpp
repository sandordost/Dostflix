#include "vpn/VpnProfileModel.h"

VpnProfileModel::VpnProfileModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int VpnProfileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_profiles.size());
}

QVariant VpnProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_profiles.size()) {
        return {};
    }
    const VpnProfile &profile = m_profiles.at(index.row());
    if (role == UuidRole) {
        return profile.uuid;
    }
    if (role == NameRole) {
        return profile.name;
    }
    return {};
}

QHash<int, QByteArray> VpnProfileModel::roleNames() const
{
    return {{UuidRole, "uuid"}, {NameRole, "name"}};
}

void VpnProfileModel::replaceProfiles(QList<VpnProfile> profiles)
{
    beginResetModel();
    m_profiles = std::move(profiles);
    endResetModel();
}
