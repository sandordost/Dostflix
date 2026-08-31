#pragma once

#include "providers/ProviderConfig.h"

#include <QAbstractListModel>

class ProviderListModel final : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role { IdRole = Qt::UserRole + 1, NameRole, KindRole, EndpointRole, EnabledRole };
    Q_ENUM(Role)

    explicit ProviderListModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replace(QList<ProviderConfig> providers);
    [[nodiscard]] const QList<ProviderConfig> &providers() const;

private:
    QList<ProviderConfig> m_providers;
};
