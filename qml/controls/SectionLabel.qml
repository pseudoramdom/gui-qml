// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root

    property string text: ""
    property string detail: ""
    property color textColor: Theme.color.neutral7
    property color detailColor: Theme.color.neutral6

    spacing: 6

    CoreText {
        text: root.text.toUpperCase()
        color: root.textColor
        font: Qt.font({
            family: Theme.text.subheading.family,
            styleName: Theme.text.subheading.styleName,
            pixelSize: Theme.text.subheading.pixelSize,
            letterSpacing: 1.2
        })
        lineHeight: Theme.text.subheading.lineHeight
        lineHeightMode: Text.FixedHeight
        horizontalAlignment: Text.AlignLeft
        wrap: false
    }

    CoreText {
        Layout.fillWidth: true
        visible: root.detail.length > 0
        text: root.detail
        color: root.detailColor
        font: Theme.text.description.font
        lineHeight: Theme.text.description.lineHeight
        lineHeightMode: Text.FixedHeight
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideRight
    }
}
