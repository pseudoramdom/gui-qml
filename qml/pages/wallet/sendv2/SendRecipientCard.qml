// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../../../controls"

ColumnLayout {
    id: root
    objectName: "sendRecipientCard"

    property var wallet: null
    property var recipient: null
    property int recipientIndex: 0
    property int recipientCount: 1
    property bool expanded: true
    readonly property bool multiple: recipientCount > 1

    signal selected()
    signal removeRequested()
    signal previewChanged()

    function amountInputPattern(unit) {
        if (unit === BitcoinAmount.SAT) return /^0*\d{0,16}$/
        if (unit === BitcoinAmount.uBTC) return /^0*\d{0,13}(\.\d{0,2})?$/
        if (unit === BitcoinAmount.mBTC) return /^0*\d{0,10}(\.\d{0,5})?$/
        return /^0*\d{0,7}(\.\d{0,8})?$/
    }

    function flippedDisplayUnit(unit) {
        return unit === BitcoinAmount.SAT ? BitcoinAmount.BTC : BitcoinAmount.SAT
    }

    Layout.fillWidth: true
    spacing: 12

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: root.multiple ? recipientHeader.implicitHeight + 24 : 0
        visible: root.multiple
        color: root.expanded ? Theme.color.neutral1 : "transparent"
        border.width: 1
        border.color: root.expanded ? Theme.color.orange : Theme.color.neutral2
        radius: 10

        RowLayout {
            id: recipientHeader
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 10
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                CoreText {
                    Layout.fillWidth: true
                    text: qsTr("Recipient %1").arg(root.recipientIndex + 1)
                    color: Theme.color.neutral9
                    font: Theme.text.subheading.font
                    horizontalAlignment: Text.AlignLeft
                }

                CoreText {
                    Layout.fillWidth: true
                    visible: !root.expanded
                    text: root.recipient
                        ? (root.recipient.label || root.recipient.address.ellipsesAddress || qsTr("Not completed"))
                        : qsTr("Not completed")
                    color: Theme.color.neutral7
                    font: Theme.text.caption.font
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideMiddle
                }
            }

            IconButton {
                objectName: "sendRecipientRemoveButton"
                visible: root.recipientCount > 1
                iconSource: "image://images/cross"
                size: 28
                onClicked: root.removeRequested()
            }

            Icon {
                source: root.expanded ? "image://images/caret-down-medium-filled" : "image://images/caret-right"
                color: Theme.color.neutral7
                size: 22
            }
        }

        TapHandler { onTapped: root.selected() }
        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }

    ColumnLayout {
        Layout.fillWidth: true
        visible: root.expanded
        spacing: 12

        SectionLabel {
            Layout.fillWidth: true
            text: qsTr("Recipient address")
        }

        FieldSurface {
            objectName: "sendAddressField"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(56, addressInput.contentHeight + 24)
            active: addressInput.activeFocus
            error: root.recipient && root.recipient.addressError.length > 0

            TextArea {
                id: addressInput
                objectName: "sendAddressInput"
                anchors.fill: parent
                text: root.recipient ? root.recipient.address.formattedAddress : ""
                placeholderText: qsTr("bc1q…")
                wrapMode: Text.WrapAnywhere
                selectByMouse: true
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0
                color: Theme.color.neutral9
                placeholderTextColor: Theme.color.neutral6
                font: Theme.text.monoDescription.font
                background: Item {}

                onTextChanged: {
                    if (root.recipient && root.recipient.address.formattedAddress !== text) {
                        cursorPosition = root.recipient.address.setAddress(text, cursorPosition)
                        root.previewChanged()
                    }
                }
                onEditingFinished: root.previewChanged()
            }
        }

        CoreText {
            Layout.fillWidth: true
            visible: root.recipient && root.recipient.addressError.length > 0
            text: root.recipient ? root.recipient.addressError : ""
            color: Theme.color.red
            font: Theme.text.description.font
            horizontalAlignment: Text.AlignLeft
        }

        SectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Amount")
        }

        FieldSurface {
            objectName: "sendNoteField"
            Layout.fillWidth: true
            active: amountInput.activeFocus
            error: root.recipient && root.recipient.amountError.length > 0

            TextField {
                id: amountInput
                objectName: "sendAmountInput"
                anchors.left: parent.left
                anchors.right: unitButton.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: root.recipient ? root.recipient.amount.display : ""
                placeholderText: root.recipient && root.recipient.amount.unit === BitcoinAmount.SAT ? "0" : "0.00000000"
                selectByMouse: true
                leftPadding: 0
                rightPadding: 0
                color: Theme.color.neutral9
                placeholderTextColor: Theme.color.neutral6
                font: Theme.text.monoDescription.font
                background: Item {}
                validator: RegularExpressionValidator {
                    regularExpression: root.recipient
                        ? root.amountInputPattern(root.recipient.amount.unit)
                        : /^0*\d{0,7}(\.\d{0,8})?$/
                }

                onTextEdited: {
                    if (!root.recipient) return
                    root.recipient.amount.display = text
                    root.previewChanged()
                }
                onEditingFinished: {
                    if (!root.recipient) return
                    root.recipient.amount.format()
                    root.previewChanged()
                }
            }

            Button {
                id: unitButton
                objectName: "sendAmountUnitToggle"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: root.recipient ? root.recipient.amount.unitLabel : ""
                hoverEnabled: AppMode.isDesktop
                onClicked: {
                    if (!root.recipient) return
                    optionsModel.displayUnit = root.flippedDisplayUnit(root.recipient.amount.unit)
                }
                contentItem: CoreText {
                    text: unitButton.text
                    color: Theme.color.neutral7
                    font: Theme.text.description.font
                }
                background: Item {}
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CoreText {
                Layout.fillWidth: true
                text: root.wallet
                    ? qsTr("Available · %1 %2").arg(root.wallet.balance).arg(optionsModel.displayUnitLabelForAmount(root.wallet.balanceSatoshi))
                    : ""
                color: Theme.color.neutral6
                font: Theme.text.description.font
                horizontalAlignment: Text.AlignLeft
            }
        }

        CoreText {
            Layout.fillWidth: true
            visible: root.recipient && root.recipient.amountError.length > 0
            text: root.recipient ? root.recipient.amountError : ""
            color: Theme.color.red
            font: Theme.text.description.font
            horizontalAlignment: Text.AlignLeft
        }

        SectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Note")
            detail: qsTr("· stored locally only")
        }

        FieldSurface {
            Layout.fillWidth: true
            active: noteInput.activeFocus

            TextField {
                id: noteInput
                objectName: "sendNoteInput"
                anchors.fill: parent
                text: root.recipient ? root.recipient.label : ""
                placeholderText: qsTr("Add a note")
                selectByMouse: true
                leftPadding: 0
                rightPadding: 0
                color: Theme.color.neutral9
                placeholderTextColor: Theme.color.neutral6
                font: Theme.text.description.font
                background: Item {}
                onTextEdited: if (root.recipient) root.recipient.label = text
            }
        }
    }
}
