#pragma once

#include <QScrollArea>
#include <QAbstractItemModel>

#include "expandableitemswidget.h"
#include "countryitemdelegate.h"
#include "cityitemdelegate.h"
#include "scrollbar.h"
#include "types/locationid.h"

namespace gui_locations {

class LocationsView : public QScrollArea
{
    Q_OBJECT

public:
    explicit LocationsView(QWidget *parent, QAbstractItemModel *model);
    ~LocationsView() override;

    void setModel(QAbstractItemModel *model);
    void setShowLatencyInMs(bool isShowLatencyInMs);
    void setShowLocationLoad(bool isShowLocationLoad);
    void updateScaling();
    bool isEmptyList() const { return widget_->isEmptyList(); }

    void expandAll() { widget_->expandAll(); }
    void collapseAll() { widget_->collapseAll(); }
    void scrollToTop();

    bool eventFilter(QObject *object, QEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
     void selected(const LocationID &lid);
     void emptyListStateChanged(bool isEmptyList);

private slots:
    void onScrollBarActionTriggered(int action);
    void onExpandingAnimationStarted(int top, int height);

private:
    static constexpr int kScrollBarWidth = 8;

    ExpandableItemsWidget *widget_;
    CountryItemDelegate *countryItemDelegate_;  // todo move outside class
    CityItemDelegate *cityItemDelegate_;        // todo move outside class
    ScrollBar *scrollBar_;

    void ensureVisible(int top, int bottom);
};

} // namespace gui_locations

