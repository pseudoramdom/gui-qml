// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "../controls"

Item {
    id: root

    property string label: qsTr("Address")
    property string address: ""

    signal copied()

    Layout.fillWidth: true
    implicitHeight: addressCol.implicitHeight + 20

    ColumnLayout {
        id: addressCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 10
        spacing: 4

        CoreText {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            color: Theme.color.neutral7
            text: root.label
            font: Theme.text.description.font
            lineHeight: Theme.text.description.lineHeight
            lineHeightMode: Text.FixedHeight
        }

        AddressLabel {
            Layout.fillWidth: true
            address: root.address
            onCopied: root.copied()
        }
    }
}
