#include "favoritecitiesmodel.h"
#include "../locationsmodel_roles.h"
#include "types/locationid.h"

namespace gui {

FavoriteCitiesModel::FavoriteCitiesModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool FavoriteCitiesModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, gui::LOCATION_ID);
    LocationID lid = qvariant_cast<LocationID>(v);
    if (lid.isCustomConfigsLocation() || lid.isStaticIpsLocation())
    {
        return false;
    }
    else
    {
        return sourceModel()->data(mi, gui::IS_FAVORITE).toBool();
    }
}



} //namespace gui
