// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root

    property string title: ""
    property int minMenuWidth: 240
    property int itemSpacing: 0
    property int menuPadding: 6

    default property alias menuItems: _column.data

    padding: menuPadding
    focus: true

    implicitWidth: Math.max(
        root.minMenuWidth,
        _column.implicitWidth + 2 * root.menuPadding)
    implicitHeight: _column.implicitHeight + 2 * root.menuPadding

    background: Rectangle {
        color: Theme.color.neutral1
        border.color: Theme.dark ? Theme.color.neutral2 : Theme.color.neutral3
        border.width: 1
        radius: 5
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 120; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100; easing.type: Easing.InCubic }
    }

    contentItem: ColumnLayout {
        id: _column
        spacing: root.itemSpacing
        readonly property bool _contextMenuMarker: true
        function _closeMenu() { root.close() }

        CoreText {
            visible: root.title !== ""
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 8
            Layout.topMargin: visible ? 8 : 0
            Layout.bottomMargin: visible ? 4 : 0
            text: root.title
            horizontalAlignment: Text.AlignLeft
            font: Theme.text.heading.font
            lineHeight: Theme.text.heading.lineHeight
            lineHeightMode: Text.FixedHeight
            wrap: false
            color: Theme.color.neutral6
        }
    }
}
