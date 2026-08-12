// Copyright (c) 2025-2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../controls"

ColumnLayout {
    id: root

    property var address
    property string errorText: ""
    property string labelText: qsTr("Send to")
    property bool enabled: true
    property alias inputObjectName: addressInput.objectName
    property bool interceptPaste: false

    signal textChanged()
    signal editingFinished()
    signal pasteRequested()

    function paymentUri(text) {
        const trimmedText = String(text).trim()
        return root.interceptPaste && trimmedText.toLowerCase().startsWith("bitcoin:")
            ? trimmedText
            : ""
    }

    function pastedPaymentUri(previousText, currentText) {
        const clipboardText = Clipboard.text()
        const uri = paymentUri(clipboardText)
        if (uri.length === 0 || previousText === currentText) return ""
        return String(currentText).indexOf(clipboardText) === -1 ? "" : uri
    }

    function paste() {
        addressInput.paste()
    }

    function syncFromAddress() {
        addressInput.syncFromAddress()
    }

    onAddressChanged: syncFromAddress()

    Layout.fillWidth: true
    spacing: 4

    Item {
        id: inputRow
        Layout.fillWidth: true
        implicitHeight: Math.max(56, addressInput.height + 16)

        CoreText {
            id: label
            width: 128
            anchors.left: parent.left
            anchors.verticalCenter: addressInput.verticalCenter
            horizontalAlignment: Text.AlignLeft
            text: root.labelText
            font: Theme.text.body.font
            lineHeight: Theme.text.body.lineHeight
            lineHeightMode: Text.FixedHeight
        }

        TextArea {
            id: addressInput
            property bool syncingFromAddress: false

            function syncFromAddress() {
                const formattedAddress = root.address ? root.address.formattedAddress : ""
                if (text === formattedAddress) return
                syncingFromAddress = true
                text = formattedAddress
                syncingFromAddress = false
            }

            function handlePaymentUriInput() {
                const previousText = root.address ? root.address.formattedAddress : ""
                const uri = root.pastedPaymentUri(previousText, text)
                if (uri.length === 0) return false
                syncFromAddress()
                root.pasteRequested()
                syncFromAddress()
                return true
            }

            objectName: root.inputObjectName
            anchors.left: label.right
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            enabled: root.enabled
            placeholderText: qsTr("Enter address...")
            text: ""
            Component.onCompleted: syncFromAddress()
            wrapMode: Text.WrapAnywhere
            leftPadding: 0
            topPadding: 0
            rightPadding: 0
            bottomPadding: 0
            height: Math.max(contentHeight, 32)
            font: Theme.text.monoBody.font
            color: Theme.color.neutral9
            placeholderTextColor: enabled ? Theme.color.neutral7 : Theme.color.neutral4
            background: Item {}
            selectByMouse: true

            Keys.onPressed: (event) => {
                if (root.interceptPaste && event.matches(StandardKey.Paste)) {
                    root.pasteRequested()
                    event.accepted = true
                }
            }

            onTextChanged: {
                if (syncingFromAddress) {
                    root.textChanged()
                    return
                }
                if (handlePaymentUriInput()) return
                if (root.address) {
                    cursorPosition = root.address.setAddress(text, cursorPosition)
                }
                root.textChanged()
            }

            onEditingFinished: {
                if (root.address) {
                    root.address.setAddress(text, cursorPosition)
                    root.editingFinished()
                }
            }

            onActiveFocusChanged: {
                if (!activeFocus && root.address) {
                    root.address.setAddress(text)
                }
            }

            Connections {
                target: root.address ? root.address : null
                function onFormattedAddressChanged() {
                    addressInput.syncFromAddress()
                }
            }
        }
    }


    RowLayout {
        id: addressIssue
        Layout.fillWidth: true
        visible: root.errorText.length > 0
        spacing: 8
        Layout.topMargin: 4

        Icon {
            source: "image://images/alert-filled"
            size: 22
            color: Theme.color.red
        }

        CoreText {
            text: root.errorText
            font: Theme.text.description.font
            lineHeight: Theme.text.description.lineHeight
            lineHeightMode: Text.FixedHeight
            color: Theme.color.red
            horizontalAlignment: Text.AlignLeft
            Layout.fillWidth: true
        }
    }
}
