// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root

    property string label: ""
    property string value: ""
    property string labelObjectName: ""
    property string valueObjectName: ""
    property bool emphasized: false
    property color labelColor: Theme.color.neutral7
    property color valueColor: Theme.color.neutral9

    spacing: 16

    CoreText {
        objectName: root.labelObjectName
        Layout.fillWidth: true
        text: root.label
        color: root.labelColor
        font: root.emphasized ? Theme.text.subheading.font : Theme.text.description.font
        lineHeight: root.emphasized ? Theme.text.subheading.lineHeight : Theme.text.description.lineHeight
        lineHeightMode: Text.FixedHeight
        horizontalAlignment: Text.AlignLeft
        wrap: false
    }

    CoreText {
        objectName: root.valueObjectName
        text: root.value
        color: root.valueColor
        font: Theme.text.monoDescription.font
        lineHeight: Theme.text.monoDescription.lineHeight
        lineHeightMode: Text.FixedHeight
        horizontalAlignment: Text.AlignRight
        wrap: false
    }
}
