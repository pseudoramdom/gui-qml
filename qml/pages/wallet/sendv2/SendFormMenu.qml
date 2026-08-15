// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15

import "../../../controls"

ContextMenu {
    id: root
    objectName: "sendOptionsPopup"

    property bool multipleRecipientsEnabled: false
    property bool customFeeEnabled: false
    property bool includeFeeInAmount: false

    signal multipleRecipientsToggled(bool enabled)
    signal customFeeToggled(bool enabled)
    signal includeFeeInAmountToggled(bool enabled)
    signal editCustomFeeRequested()
    signal openPaymentRequestRequested()
    signal importPsbtRequested()
    signal clearFormRequested()

    modal: true
    dim: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    ContextMenuButton {
        objectName: "sendOptionsOpenPaymentRequestButton"
        text: qsTr("Open payment request")
        onTriggered: root.openPaymentRequestRequested()
    }

    ContextMenuButton {
        objectName: "sendImportPsbtFromFileButton"
        text: qsTr("Import PSBT from file…")
        iconSource: "qrc:/icons/file"
        onTriggered: root.importPsbtRequested()
    }

    ContextMenuDivider {}

    ContextMenuToggle {
        objectName: "sendOptionsMultipleRecipientsToggle"
        text: qsTr("Multiple recipients")
        checked: root.multipleRecipientsEnabled
        onClicked: root.multipleRecipientsToggled(!root.multipleRecipientsEnabled)
    }

    ContextMenuToggle {
        objectName: "feeSelectionCustomToggle"
        text: qsTr("Use custom fee rate")
        checked: root.customFeeEnabled
        onClicked: root.customFeeToggled(!root.customFeeEnabled)
    }

    ContextMenuButton {
        objectName: "feeSelectionEditCustomRateButton"
        text: qsTr("Edit custom fee rate…")
        onTriggered: root.editCustomFeeRequested()
    }

    ContextMenuToggle {
        objectName: "feeSelectionIncludeFeeToggle"
        text: qsTr("Include fee in amount")
        checked: root.includeFeeInAmount
        onClicked: root.includeFeeInAmountToggled(!root.includeFeeInAmount)
    }

    ContextMenuDivider {}

    ContextMenuButton {
        objectName: "sendClearFormButton"
        text: qsTr("Clear form")
        iconSource: "qrc:/icons/cross"
        role: ContextMenuButton.Destructive
        onTriggered: root.clearFormRequested()
    }
}
