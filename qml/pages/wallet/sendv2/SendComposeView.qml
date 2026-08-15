// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

import "../../../controls"

Page {
    id: root
    objectName: "walletSendPage"

    property var wallet: null
    property string errorText: ""
    property bool showClipboardBanner: false
    property string paymentRequestStatus: ""
    property string paymentRequestMessage: ""
    property bool paymentRequestError: false

    readonly property bool compact:
        SizeClass.widthClassFor(Window.window ? Window.window.width : width) === SizeClass.compact
    readonly property real horizontalMargin: 20
    readonly property real horizontalGap: 10
    readonly property real summaryPreferredWidth: Math.min(520, Math.max(390, width * 0.38))
    readonly property real horizontalDetailsWidth:
        width - horizontalMargin * 2 - horizontalGap - summaryPreferredWidth
    readonly property bool stacked: compact || horizontalDetailsWidth <= summaryPreferredWidth

    signal reviewRequested()
    signal coinControlRequested()
    signal openPaymentRequestRequested()
    signal importPsbtRequested()
    signal fillClipboardRequested()
    signal dismissClipboardRequested()
    signal dropped(var drop)

    background: Rectangle { color: Theme.color.background }

    function schedulePreview() {
        if (root.wallet) root.wallet.scheduleFeeEstimates()
    }

    Loader {
        objectName: "sendComposeLayoutLoader"
        anchors.fill: parent
        sourceComponent: root.stacked ? stackedLayout : regularLayout
    }

    DropArea {
        objectName: "sendDropArea"
        anchors.fill: parent
        keys: ["text/uri-list", "text/plain"]
        onDropped: function(drop) { root.dropped(drop) }
    }

    Component {
        id: regularLayout

        Item {
            ScrollView {
                id: formScroll
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: summary.left
                anchors.margins: 20
                anchors.rightMargin: 10
                clip: true
                contentWidth: width

                SendFormCard {
                    width: formScroll.availableWidth
                    wallet: root.wallet
                    showClipboardBanner: root.showClipboardBanner
                    paymentRequestStatus: root.paymentRequestStatus
                    paymentRequestMessage: root.paymentRequestMessage
                    paymentRequestError: root.paymentRequestError
                    onPreviewChanged: root.schedulePreview()
                    onCoinControlRequested: root.coinControlRequested()
                    onOpenPaymentRequestRequested: root.openPaymentRequestRequested()
                    onImportPsbtRequested: root.importPsbtRequested()
                    onFillClipboardRequested: root.fillClipboardRequested()
                    onDismissClipboardRequested: root.dismissClipboardRequested()
                }
            }

            SendTransactionSummary {
                id: summary
                width: root.summaryPreferredWidth
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 20
                anchors.rightMargin: 20
                wallet: root.wallet
                errorText: root.errorText
                onReviewRequested: root.reviewRequested()
            }
        }
    }

    Component {
        id: stackedLayout

        ScrollView {
            clip: true
            contentWidth: width

            ColumnLayout {
                width: parent.width
                spacing: 16

                SendFormCard {
                    Layout.fillWidth: true
                    Layout.leftMargin: root.compact ? 16 : root.horizontalMargin
                    Layout.rightMargin: root.compact ? 16 : root.horizontalMargin
                    Layout.topMargin: root.compact ? 16 : root.horizontalMargin
                    wallet: root.wallet
                    showClipboardBanner: root.showClipboardBanner
                    paymentRequestStatus: root.paymentRequestStatus
                    paymentRequestMessage: root.paymentRequestMessage
                    paymentRequestError: root.paymentRequestError
                    onPreviewChanged: root.schedulePreview()
                    onCoinControlRequested: root.coinControlRequested()
                    onOpenPaymentRequestRequested: root.openPaymentRequestRequested()
                    onImportPsbtRequested: root.importPsbtRequested()
                    onFillClipboardRequested: root.fillClipboardRequested()
                    onDismissClipboardRequested: root.dismissClipboardRequested()
                }

                SendTransactionSummary {
                    Layout.fillWidth: true
                    Layout.leftMargin: root.compact ? 16 : root.horizontalMargin
                    Layout.rightMargin: root.compact ? 16 : root.horizontalMargin
                    Layout.bottomMargin: root.compact ? 16 : root.horizontalMargin
                    wallet: root.wallet
                    errorText: root.errorText
                    onReviewRequested: root.reviewRequested()
                }
            }
        }
    }
}
