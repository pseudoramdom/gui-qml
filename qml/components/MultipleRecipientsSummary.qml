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
    objectName: "multipleRecipientsSummary"

    property WalletQmlModel wallet: walletController.selectedWallet
    property WalletQmlModelTransaction transaction: wallet ? wallet.currentTransaction : null
    property var recipients: wallet ? wallet.recipients : null

    spacing: 15

    Separator {
        Layout.fillWidth: true
    }

    ListView {
        id: inputsList
        objectName: "multipleSendReviewRecipientsList"
        Layout.fillWidth: true
        Layout.preferredHeight: contentHeight
        model: root.recipients
        clip: true
        interactive: false

        delegate: Item {
            id: delegate
            implicitHeight: delegateColumn.implicitHeight
            height: implicitHeight
            width: ListView.view.width

            required property string address;
            required property string label;
            required property string amount;
            required property string formattedAddress;
            required property string amountUnitLabel;
            required property int index;
            property bool expanded: false
            readonly property bool expandable: formattedAddress.length > 0
            readonly property string amountText: amountUnitLabel.length > 0 ? amount + " " + amountUnitLabel : amount
            readonly property string addressText: expanded ? formattedAddress : address

            activeFocusOnTab: expandable
            Accessible.role: Accessible.Button
            Accessible.name: label.length > 0
                ? qsTr("%1, address %2, amount %3").arg(label).arg(formattedAddress).arg(amountText)
                : qsTr("Address %1, amount %2").arg(formattedAddress).arg(amountText)
            Accessible.description: expanded ? qsTr("Hide full address") : qsTr("Show full address")
            Accessible.onPressAction: click()

            function click() {
                if (expandable) {
                    expanded = !expanded
                }
            }

            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                    delegate.click()
                    event.accepted = true
                }
            }

            onExpandableChanged: {
                if (!expandable) {
                    expanded = false
                }
            }

            Column {
                id: delegateColumn
                width: parent.width
                spacing: 0

                Item {
                    width: 1
                    height: 15
                }

                CoreText {
                    objectName: "multipleSendReviewRecipient" + index + "PrimaryText"
                    visible: delegate.label.length > 0
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    wrap: false
                    elide: Text.ElideRight
                    text: delegate.label
                    font: Theme.text.description.font
                    lineHeight: Theme.text.description.lineHeight
                    lineHeightMode: Text.FixedHeight
                    color: Theme.color.neutral9
                }

                Item {
                    width: 1
                    height: 10
                    visible: delegate.label.length > 0
                }

                Item {
                    width: parent.width
                    height: Math.max(addressTextItem.implicitHeight, amountDisplay.implicitHeight)

                    CoreText {
                        id: addressTextItem
                        objectName: "multipleSendReviewRecipient" + index + "AddressText"
                        anchors.left: parent.left
                        anchors.right: amountDisplay.left
                        anchors.rightMargin: 10
                        anchors.top: parent.top
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignTop
                        wrap: delegate.expanded
                        elide: delegate.expanded ? Text.ElideNone : Text.ElideRight
                        text: delegate.addressText
                        font: Theme.text.monoBody.font
                        lineHeight: Theme.text.monoBody.lineHeight
                        lineHeightMode: Text.FixedHeight
                        color: Theme.color.neutral9

                        HoverHandler {
                            enabled: delegate.expandable
                            cursorShape: Qt.PointingHandCursor
                        }

                        TapHandler {
                            enabled: delegate.expandable
                            onTapped: delegate.click()
                        }
                    }

                    Item {
                        id: amountDisplay
                        objectName: "multipleSendReviewRecipient" + index + "Amount"
                        property string text: delegate.amountText
                        implicitWidth: amountRow.implicitWidth
                        implicitHeight: amountRow.implicitHeight
                        anchors.right: parent.right
                        anchors.top: addressTextItem.top
                        width: implicitWidth
                        height: implicitHeight

                        RowLayout {
                            id: amountRow
                            anchors.fill: parent
                            spacing: 5

                            CoreText {
                                objectName: "multipleSendReviewRecipient" + index + "AmountValue"
                                text: delegate.amount
                                font.family: optionsModel.moneyFont.family
                                font.weight: optionsModel.moneyFont.weight
                                font.pixelSize: Theme.text.body.pixelSize
                                wrap: false
                                color: Theme.color.neutral9
                            }

                            CoreText {
                                objectName: "multipleSendReviewRecipient" + index + "AmountUnit"
                                visible: delegate.amountUnitLabel.length > 0
                                text: delegate.amountUnitLabel
                                font: Theme.text.body.font
                                wrap: false
                                color: Theme.color.neutral9
                            }
                        }
                    }
                }

                Item {
                    width: 1
                    height: 15
                }

                Separator {
                    objectName: "multipleSendReviewRecipient" + index + "Separator"
                    width: parent.width
                }
            }

            FocusBorder {
                visible: delegate.activeFocus
                topMargin: 0
                bottomMargin: 0
                leftMargin: -4
                rightMargin: -4
            }
        }
    }

    BitcoinAmountDisplayField {
        objectName: "multipleSendReviewFeeField"
        Layout.topMargin: 10
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
        objectName: "multipleSendReviewTotalField"
        labelWidth: 130
        labelText: qsTr("Total amount")
        amountText: root.transaction ? root.transaction.totalAmount.display : ""
        unitText: root.transaction ? root.transaction.totalAmount.unitLabel : ""
    }
}
