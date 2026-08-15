// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../../../controls"
import "../../../components"

CardSurface {
    id: root
    objectName: "sendContainerFormCard"

    property var wallet: null
    property bool showClipboardBanner: false
    property string paymentRequestStatus: ""
    property string paymentRequestMessage: ""
    property bool paymentRequestError: false

    readonly property var currentRecipient: wallet && wallet.recipients ? wallet.recipients.current : null

    signal previewChanged()
    signal coinControlRequested()
    signal openPaymentRequestRequested()
    signal importPsbtRequested()
    signal fillClipboardRequested()
    signal dismissClipboardRequested()

    implicitHeight: formColumn.implicitHeight + 48

    AppSettings {
        id: settings
        property bool multipleRecipientsEnabled: false
    }

    contentItem: ColumnLayout {
        id: formColumn
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            CoreText {
                Layout.fillWidth: true
                text: qsTr("Transaction details")
                color: Theme.color.neutral9
                font: Theme.text.heading.font
                lineHeight: Theme.text.heading.lineHeight
                lineHeightMode: Text.FixedHeight
                horizontalAlignment: Text.AlignLeft
            }

            IconButton {
                id: menuButton
                objectName: "sendOptionsButton"
                iconSource: "image://images/ellipsis"
                checked: formMenu.opened
                onClicked: formMenu.opened ? formMenu.close() : formMenu.open()
            }
        }

        InfoBanner {
            objectName: "clipboardUriBanner"
            Layout.fillWidth: true
            visible: root.showClipboardBanner
            title: qsTr("Payment request found on the clipboard")
            message: qsTr("Fill the recipient fields from this request?")
            showsCloseButton: true
            primaryButtonText: qsTr("Fill")
            onPrimaryClicked: root.fillClipboardRequested()
            onDismissClicked: root.dismissClipboardRequested()
        }

        InfoBanner {
            Layout.fillWidth: true
            visible: !root.paymentRequestError
                && (root.paymentRequestStatus.length > 0 || root.paymentRequestMessage.length > 0)
            title: root.paymentRequestStatus
            message: root.paymentRequestMessage
        }

        ToastBanner {
            Layout.fillWidth: true
            visible: root.paymentRequestError && root.paymentRequestStatus.length > 0
            backgroundColor: Theme.color.red
            iconSource: "image://images/alert-filled"
            text: root.paymentRequestStatus
        }

        CoreText {
            Layout.fillWidth: true
            visible: root.wallet && root.wallet.hasExternalSigner
            text: qsTr("Keep your external signer available to approve this transaction.")
            color: Theme.color.neutral7
            font: Theme.text.description.font
            horizontalAlignment: Text.AlignLeft
            wrap: true
        }

        Repeater {
            model: root.wallet && root.wallet.recipients ? root.wallet.recipients : null

            delegate: SendRecipientCard {
                required property int index

                Layout.fillWidth: true
                wallet: root.wallet
                recipient: root.wallet && root.wallet.recipients
                    ? root.wallet.recipients.recipientAt(index)
                    : null
                recipientIndex: index
                recipientCount: root.wallet && root.wallet.recipients ? root.wallet.recipients.count : 1
                expanded: recipientCount === 1
                    || (root.wallet && root.wallet.recipients.currentIndex === index + 1)
                onSelected: if (root.wallet) root.wallet.recipients.select(index)
                onRemoveRequested: {
                    if (!root.wallet) return
                    root.wallet.recipients.removeAt(index)
                    root.previewChanged()
                }
                onPreviewChanged: root.previewChanged()
            }
        }

        OutlineButton {
            objectName: "sendRecipientAddButton"
            Layout.fillWidth: true
            visible: settings.multipleRecipientsEnabled
            enabled: root.wallet && root.wallet.recipients.count < 25
            text: qsTr("Add recipient")
            iconSource: "image://images/plus"
            onClicked: {
                root.wallet.recipients.add()
                root.previewChanged()
            }
        }

        SectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Network fee")
        }

        SendFeeSelector {
            Layout.fillWidth: true
            wallet: root.wallet
            onFeeChanged: root.previewChanged()
        }

        OutlineButton {
            objectName: "sendCoinControlButton"
            Layout.fillWidth: true
            text: root.wallet && root.wallet.coinsListModel.selectedCoinsCount > 0
                ? qsTr("Coin control · %n input(s) selected", "", root.wallet.coinsListModel.selectedCoinsCount)
                : qsTr("Coin control · select inputs")
            enabled: root.wallet && root.wallet.coinsListModel.coinCount > 0
            onClicked: root.coinControlRequested()
        }
    }

    SendFormMenu {
        id: formMenu
        x: Math.max(0, menuButton.x + menuButton.width - width)
        y: menuButton.y + menuButton.height + 8
        multipleRecipientsEnabled: settings.multipleRecipientsEnabled
        customFeeEnabled: root.wallet ? root.wallet.customFeeEnabled : false
        includeFeeInAmount: root.currentRecipient ? root.currentRecipient.subtractFeeFromAmount : false

        onMultipleRecipientsToggled: function(enabled) {
            settings.multipleRecipientsEnabled = enabled
            if (!root.wallet) return
            if (enabled && root.wallet.recipients.count === 1) {
                root.wallet.recipients.add()
            } else if (!enabled) {
                root.wallet.recipients.clearToFront()
            }
            root.previewChanged()
        }
        onCustomFeeToggled: function(enabled) {
            if (!root.wallet) return
            root.wallet.customFeeEnabled = enabled
            if (enabled && !root.wallet.customFeeRateValid) customFeePopup.open()
            root.previewChanged()
        }
        onIncludeFeeInAmountToggled: function(enabled) {
            if (!root.currentRecipient) return
            root.currentRecipient.subtractFeeFromAmount = enabled
            root.previewChanged()
        }
        onEditCustomFeeRequested: customFeePopup.open()
        onOpenPaymentRequestRequested: root.openPaymentRequestRequested()
        onImportPsbtRequested: root.importPsbtRequested()
        onClearFormRequested: {
            if (!root.wallet) return
            root.wallet.recipients.clear()
            settings.multipleRecipientsEnabled = false
            root.previewChanged()
        }
    }

    Popup {
        id: customFeePopup
        objectName: "sendCustomFeePopup"
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        width: Math.min(420, Overlay.overlay.width - 40)
        modal: true
        focus: true
        padding: 24
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        onOpened: customFeeInput.text = root.wallet ? root.wallet.customFeeRate : ""

        background: Rectangle {
            color: Theme.color.neutral0
            border.color: Theme.color.neutral2
            border.width: 1
            radius: 14
        }

        contentItem: ColumnLayout {
            spacing: 16

            CoreText {
                Layout.fillWidth: true
                text: qsTr("Custom fee rate")
                color: Theme.color.neutral9
                font: Theme.text.heading.font
                horizontalAlignment: Text.AlignLeft
            }

            CoreTextField {
                id: customFeeInput
                objectName: "feeSelectionCustomRateInput"
                Layout.fillWidth: true
                placeholderText: qsTr("sat/vB")
                maximumLength: 12
                validator: RegularExpressionValidator {
                    regularExpression: /^(|[0-9]+(\.[0-9]{0,3})?)$/
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                OutlineButton {
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    onClicked: customFeePopup.close()
                }

                ContinueButton {
                    Layout.fillWidth: true
                    text: qsTr("Apply")
                    enabled: customFeeInput.text.trim().length > 0
                    onClicked: {
                        if (!root.wallet) return
                        root.wallet.customFeeRate = customFeeInput.text
                        root.wallet.customFeeEnabled = true
                        customFeePopup.close()
                        root.previewChanged()
                    }
                }
            }
        }
    }
}
