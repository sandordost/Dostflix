#pragma once

#include "vpn/VpnBackend.h"

#include <QAbstractListModel>

class VpnProfileModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role { UuidRole = Qt::UserRole + 1, NameRole };
    Q_ENUM(Role)

    explicit VpnProfileModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replaceProfiles(QList<VpnProfile> profiles);

private:
    QList<VpnProfile> m_profiles;
};
