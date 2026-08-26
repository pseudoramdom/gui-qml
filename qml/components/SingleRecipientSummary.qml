// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../controls"

ColumnLayout {
    id: root
    objectName: "singleRecipientSummary"

    property WalletQmlModel wallet: walletController.selectedWallet
    property SendRecipient recipient: wallet ? wallet.recipients.current : null
    property WalletQmlModelTransaction transaction: wallet ? wallet.currentTransaction : null

    spacing: 10

    BitcoinAddressDisplayField {
        objectName: "sendReviewAddressField"
        labelPixelSize: Theme.text.description.pixelSize
        labelColor: Theme.color.neutral7
        text: root.recipient ? root.recipient.address.ellipsesAddress : ""
        fullText: root.recipient ? root.recipient.address.formattedAddress : ""
    }

    LabeledValueField {
        objectName: "sendReviewNoteField"
        visible: text.length > 0
        labelText: qsTr("Note")
        labelPixelSize: Theme.text.description.pixelSize
        labelColor: Theme.color.neutral7
        valueHorizontalAlignment: Text.AlignRight
        text: root.recipient ? root.recipient.label : ""
    }

    BitcoinAmountDisplayField {
        objectName: "sendReviewAmountField"
        labelText: qsTr("Amount")
        labelPixelSize: Theme.text.description.pixelSize
        labelColor: Theme.color.neutral7
        amountText: root.transaction ? root.transaction.amountAmount.display : ""
        unitText: root.transaction ? root.transaction.amountAmount.unitLabel : ""
    }

    BitcoinAmountDisplayField {
        objectName: "sendReviewFeeField"
        labelText: qsTr("Fee")
        labelPixelSize: Theme.text.description.pixelSize
        labelColor: Theme.color.neutral7
        amountText: root.transaction ? root.transaction.feeAmount.display : ""
        unitText: root.transaction ? root.transaction.feeAmount.unitLabel : ""
    }

    Separator {
        Layout.fillWidth: true
    }

    BitcoinAmountDisplayField {
        objectName: "sendReviewTotalField"
        labelWidth: 130
        labelText: qsTr("Total amount")
        amountText: root.transaction ? root.transaction.totalAmount.display : ""
        unitText: root.transaction ? root.transaction.totalAmount.unitLabel : ""
    }
}
