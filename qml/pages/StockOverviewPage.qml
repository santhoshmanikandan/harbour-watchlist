import QtQuick 2.0
import QtGraphicalEffects 1.0
import QtMultimedia 5.0
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0

import "../components"

Page {
    id: overviewPage // TODO rename to stockOverviewPage

    property var stock
    property var theStock
    allowedOrientations: Orientation.All

    property int activeTabId: 0

    function openTab(tabId) {

        activeTabId = tabId

        switch (tabId) {
        case 0:
            detailsButtonPortrait.isActive = true
            chartsButtonPortrait.isActive = false
            newsButtonPortrait.isActive = false
            break
        case 1:
            detailsButtonPortrait.isActive = false
            chartsButtonPortrait.isActive = true
            newsButtonPortrait.isActive = false
            break
        case 2:
            detailsButtonPortrait.isActive = false
            chartsButtonPortrait.isActive = false
            newsButtonPortrait.isActive = true
            break
        default:
            console.log("Some strange navigation happened!")
        }
    }

    function getNavigationRowSize() {
        return Theme.iconSizeMedium + Theme.fontSizeMedium + Theme.paddingMedium
    }

    function handleDetailsClicked() {
        if (overviewPage.activeTabId === 0) {
            stockDetailsView.scrollToTop()
        } else {
            viewsSlideshow.opacity = 0
            slideshowVisibleTimer.goToTab(0)
            openTab(0)
        }
    }

    function handleChartsClicked() {
        if (overviewPage.activeTabId === 1) {
            stockChartsView.scrollToTop()
        } else {
            viewsSlideshow.opacity = 0
            slideshowVisibleTimer.goToTab(1)
            openTab(1)
        }
    }

    function handleNewsClicked() {
        if (overviewPage.activeTabId === 2) {
            stockDetailsView.scrollToTop()
        } else {
            viewsSlideshow.opacity = 0
            slideshowVisibleTimer.goToTab(2)
            openTab(2)
        }
    }

    SilicaFlickable {
        id: overviewContainer
        anchors.fill: parent
        visible: true
        contentHeight: parent.height
        contentWidth: parent.width

        PullDownMenu {
            MenuItem {
                text: qsTr("About Watchlist")
                onClicked: pageStack.push(aboutPage)
            }
        }

        Column {
            id: overviewColumn
            visible: true
            Behavior on opacity {
                NumberAnimation {
                }
            }
            width: parent.width
            height: parent.height

            Column {
                id: overviewColumnHeader
                width: parent.width
                height: overviewColumnPageHeader.height

                PageHeader {
                    id: overviewColumnPageHeader
                    title: ""
                }
            }

            Row {
                id: overviewRow
                width: parent.width
                height: parent.height - getNavigationRowSize() - overviewColumnHeader.height
                spacing: Theme.paddingSmall

                VisualItemModel {
                    id: viewsModel

                    Item {
                        id: detailsColumn
                        width: viewsSlideshow.width
                        height: viewsSlideshow.height

                        StockDetailsView {
                            id: stockDetailsView
                            width: parent.width
                            height: parent.height
                            stock: theStock
                        }
                    }

                    Item {
                        id: chartsColumn
                        width: viewsSlideshow.width
                        height: viewsSlideshow.height

                        StockChartsView {
                            id: stockChartsView
                            width: parent.width
                            height: parent.height
                            screenHeight: overviewPage.height
                            stock: theStock
                        }
                    }

                    Item {
                        id: newsColumn
                        width: viewsSlideshow.width
                        height: viewsSlideshow.height

                        StockNewsView {
                            id: stockNewsView
                            width: parent.width
                            height: parent.height
                        }
                    }
                }

                Timer {
                    id: slideshowVisibleTimer
                    property int tabId: 0
                    interval: 50
                    repeat: false
                    onTriggered: {
                        viewsSlideshow.positionViewAtIndex(
                                    tabId, PathView.SnapPosition)
                        viewsSlideshow.opacity = 1
                    }
                    function goToTab(newTabId) {
                        tabId = newTabId
                        start()
                    }
                }

                SlideshowView {
                    id: viewsSlideshow
                    width: parent.width - (overviewPage.isLandscape ? getNavigationRowSize(
                                                                          ) + (2 * Theme.horizontalPageMargin) : 0)
                    height: parent.height
                    itemWidth: width
                    clip: true
                    // interactive: accountModel.getUseSwipeNavigation()
                    model: viewsModel
                    onCurrentIndexChanged: {
                        openTab(currentIndex)
                    }
                    Behavior on opacity {
                        NumberAnimation {
                        }
                    }
                    onOpacityChanged: {
                        if (opacity === 0) {
                            slideshowVisibleTimer.start()
                        }
                    }
                }
            }

            Column {
                id: navigationRow
                width: parent.width
                height: overviewPage.isPortrait ? getNavigationRowSize() : 0
                visible: true // overviewPage.isPortrait
                Column {
                    id: navigationRowSeparatorColumn
                    width: parent.width
                    height: Theme.paddingMedium
                    Separator {
                        id: navigationRowSeparator
                        width: parent.width
                        color: Theme.primaryColor
                        horizontalAlignment: Qt.AlignHCenter
                    }
                }

                Row {
                    y: Theme.paddingSmall
                    width: parent.width
                    Item {
                        id: detailsButtonColumn
                        width: parent.width / 3
                        height: parent.height - Theme.paddingMedium
                        NavigationRowButton {
                            id: detailsButtonPortrait
                            anchors.top: parent.top
                            buttonText: qsTr("Details")
                            iconSource: "image://theme/icon-m-home"

                            function runOnClick() {
                                handleDetailsClicked()
                            }
                        }
                    }
                    Item {
                        id: chartsButtonColumn
                        width: parent.width / 3
                        height: parent.height - navigationRowSeparator.height
                        NavigationRowButton {
                            id: chartsButtonPortrait
                            anchors.top: parent.top
                            buttonText: qsTr("Charts")
                            iconSource: "image://theme/icon-m-note"

                            function runOnClick() {
                                handleChartsClicked()
                            }
                        }
                    }
                    Item {
                        id: newsButtonColumn
                        width: parent.width / 3
                        height: parent.height - navigationRowSeparator.height
                        NavigationRowButton {
                            id: newsButtonPortrait
                            anchors.top: parent.top
                            buttonText: qsTr("News")
                            iconSource: "image://theme/icon-m-note"

                            function runOnClick() {
                                handleNewsClicked()
                            }
                        }
                    }
                }
            }
        }

        Component.onCompleted: {
            openTab(0)
            theStock = stock
            overviewColumnPageHeader.title = (stock) ? stock.name : ''
        }
    }
}