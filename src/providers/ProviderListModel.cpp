#include "providers/ProviderListModel.h"

ProviderListModel::ProviderListModel(QObject *parent) : QAbstractListModel(parent) {}

int ProviderListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_providers.size());
}

QVariant ProviderListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_providers.size()) return {};
    const ProviderConfig &provider = m_providers.at(index.row());
    switch (role) {
    case IdRole: return provider.id;
    case NameRole: return provider.name;
    case KindRole: return provider.kind == ProviderKind::Prowlarr ? QStringLiteral("Prowlarr")
                                                                  : QStringLiteral("Torznab");
    case EndpointRole: return provider.endpoint.toString();
    case EnabledRole: return provider.enabled;
    default: return {};
    }
}

QHash<int, QByteArray> ProviderListModel::roleNames() const
{
    return {{IdRole, "providerId"}, {NameRole, "name"}, {KindRole, "kind"},
            {EndpointRole, "endpoint"}, {EnabledRole, "enabled"}};
}

void ProviderListModel::replace(QList<ProviderConfig> providers)
{
    beginResetModel();
    m_providers = std::move(providers);
    endResetModel();
}

const QList<ProviderConfig> &ProviderListModel::providers() const { return m_providers; }
