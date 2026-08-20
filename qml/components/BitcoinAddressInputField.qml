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
    property bool embedded: false
    property bool showLabel: true
    property alias inputObjectName: addressInput.objectName
    property alias inputText: addressInput.text
    property alias placeholderText: addressInput.placeholderText
    property alias readOnly: addressInput.readOnly
    readonly property alias field: addressInput

    signal textChanged()
    signal editingFinished()

    Layout.fillWidth: true
    spacing: 4

    Item {
        id: inputRow
        Layout.fillWidth: true
        implicitHeight: root.embedded ? Math.max(52, addressInput.height) : Math.max(56, addressInput.height + 16)

        CoreText {
            id: label
            visible: root.showLabel
            width: visible ? 128 : 0
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
            anchors.left: label.right
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            enabled: root.enabled
            placeholderText: qsTr("Enter address...")
            text: root.address ? root.address.formattedAddress : ""
            wrapMode: Text.WrapAnywhere
            leftPadding: root.embedded ? 16 : 0
            topPadding: root.embedded ? 14 : 0
            rightPadding: root.embedded ? 16 : 0
            bottomPadding: root.embedded ? 14 : 0
            height: root.embedded ? Math.max(52, contentHeight + topPadding + bottomPadding) : Math.max(contentHeight, 32)
            font: Theme.text.monoBody.font
            color: Theme.color.neutral9
            placeholderTextColor: enabled ? Theme.color.neutral7 : Theme.color.neutral4
            background: Rectangle {
                color: root.embedded ? Theme.color.neutral2 : "transparent"
                radius: root.embedded ? 10 : 0
                border.width: root.embedded && addressInput.activeFocus ? 2 : 0
                border.color: Theme.color.orange
            }
            selectByMouse: true

            onTextChanged: {
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
