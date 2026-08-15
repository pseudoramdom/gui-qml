// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "../../../controls"
import "../../../components"

CardSurface {
    id: root
    objectName: "sendTransactionSummaryCard"

    property var wallet: null
    property string errorText: ""
    readonly property bool previewAvailable: wallet ? wallet.sendPreviewAvailable : false

    signal reviewRequested()

    implicitHeight: Math.max(460, summaryColumn.implicitHeight + 48)

    contentItem: ColumnLayout {
        id: summaryColumn
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        SectionLabel {
            objectName: "sendTransactionSectionHeader"
            Layout.fillWidth: true
            text: qsTr("Transaction summary")
        }

        Item { Layout.preferredHeight: 4 }

        ColumnLayout {
            Layout.fillWidth: true
            visible: root.previewAvailable
            spacing: 14

            SummaryRow {
                Layout.fillWidth: true
                label: qsTr("Sending")
                value: root.wallet ? root.wallet.sendPreviewSending : ""
            }

            SummaryRow {
                Layout.fillWidth: true
                label: qsTr("Network fee")
                value: root.wallet ? root.wallet.sendPreviewFee : ""
            }

            SummaryRow {
                Layout.fillWidth: true
                label: qsTr("Fee rate")
                value: root.wallet ? root.wallet.estimatedFeeRate : ""
            }

            Separator {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            SummaryRow {
                objectName: "sendTotalAmountRow"
                Layout.fillWidth: true
                emphasized: true
                label: qsTr("Total")
                value: root.wallet ? root.wallet.sendPreviewTotal : ""
                valueObjectName: "sendTotalAmountValue"
            }

            SummaryRow {
                Layout.fillWidth: true
                label: qsTr("Remaining balance")
                value: root.wallet ? root.wallet.sendPreviewRemainingBalance : ""
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            visible: !root.previewAvailable
            spacing: 12

            Item { Layout.fillHeight: true }

            SpinningIndicator {
                Layout.alignment: Qt.AlignHCenter
                visible: root.wallet && root.wallet.feeEstimatePending
                running: visible
                color: Theme.color.orange
            }

            CoreText {
                Layout.fillWidth: true
                text: root.wallet && root.wallet.feeEstimatePending
                    ? qsTr("Calculating transaction details…")
                    : qsTr("Enter a valid recipient and amount to see the transaction summary.")
                color: Theme.color.neutral6
                font: Theme.text.description.font
                horizontalAlignment: Text.AlignHCenter
                wrap: true
            }

            Item { Layout.fillHeight: true }
        }

        RowLayout {
            objectName: "sendPrepareTransactionError"
            Layout.fillWidth: true
            visible: root.errorText.length > 0
            spacing: 8

            Icon {
                source: "image://images/alert-filled"
                color: Theme.color.red
                size: 20
            }

            CoreText {
                objectName: "sendPrepareTransactionErrorText"
                Layout.fillWidth: true
                text: root.errorText
                color: Theme.color.red
                font: Theme.text.description.font
                horizontalAlignment: Text.AlignLeft
                wrap: true
            }
        }

        Item { Layout.fillHeight: true }

        ContinueButton {
            objectName: "sendReviewButton"
            Layout.fillWidth: true
            text: qsTr("Review transaction")
            enabled: root.previewAvailable
                && root.wallet
                && (!root.wallet.customFeeEnabled || root.wallet.customFeeRateValid)
            onClicked: root.reviewRequested()
        }
    }
}
