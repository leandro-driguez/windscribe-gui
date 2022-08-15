#ifndef GUI_LOCATIONS_LOCATIONSVIEW_H
#define GUI_LOCATIONS_LOCATIONSVIEW_H

#include <QScrollArea>
#include <QAbstractItemModel>
#include "expandableitemswidget.h"
#include "countryitemdelegate.h"
#include "cityitemdelegate.h"

namespace gui_location {

class LocationsView : public QScrollArea
{
    Q_OBJECT

public:
    explicit LocationsView(QWidget *parent);
    ~LocationsView() override;

    void setModel(QAbstractItemModel *model);
    void setCountViewportItems(int cnt);
    void setShowLatencyInMs(bool isShowLatencyInMs);
    void setShowLocationLoad(bool isShowLocationLoad);
    void updateScaling();

protected:
    void paintEvent(QPaintEvent *event) override;

signals:

private slots:


private:
    ExpandableItemsWidget *widget_;
    CountryItemDelegate *countryItemDelegate_;  // todo move outside class
    CityItemDelegate *cityItemDelegate_;        // todo move outside class

};

} // namespace gui_location


#endif // GUI_LOCATIONS_LOCATIONSVIEW_H
