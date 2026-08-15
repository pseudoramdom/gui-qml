// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

AbstractButton {
    id: root

    property string title: ""
    property string subtitle: ""
    property string detail: ""
    property color selectedColor: Theme.color.orange

    checkable: true
    hoverEnabled: AppMode.isDesktop
    focusPolicy: Qt.StrongFocus
    implicitHeight: 100
    leftPadding: 16
    rightPadding: 16
    topPadding: 14
    bottomPadding: 14

    Accessible.name: title + (subtitle.length > 0 ? ", " + subtitle : "")
    Accessible.checked: checked

    HoverHandler { cursorShape: Qt.PointingHandCursor }

    contentItem: ColumnLayout {
        spacing: 3

        CoreText {
            Layout.fillWidth: true
            text: root.title
            color: root.checked ? root.selectedColor : Theme.color.neutral9
            font: Theme.text.subheading.font
            lineHeight: Theme.text.subheading.lineHeight
            lineHeightMode: Text.FixedHeight
            horizontalAlignment: Text.AlignLeft
            wrap: false
        }

        CoreText {
            Layout.fillWidth: true
            text: root.subtitle
            color: Theme.color.neutral7
            font: Theme.text.caption.font
            lineHeight: Theme.text.caption.lineHeight
            lineHeightMode: Text.FixedHeight
            horizontalAlignment: Text.AlignLeft
            wrap: false
        }

        Item { Layout.fillHeight: true }

        CoreText {
            Layout.fillWidth: true
            text: root.detail
            color: Theme.color.neutral7
            font: Theme.text.caption.font
            lineHeight: Theme.text.caption.lineHeight
            lineHeightMode: Text.FixedHeight
            horizontalAlignment: Text.AlignLeft
            wrap: false
        }
    }

    background: Rectangle {
        id: background
        color: root.checked
            ? Qt.rgba(
                root.selectedColor.r,
                root.selectedColor.g,
                root.selectedColor.b,
                Theme.dark ? 0.14 : 0.10)
            : Theme.color.neutral1
        border.width: root.checked || root.visualFocus ? 2 : 1
        border.color: root.checked
            ? root.selectedColor
            : (root.hovered ? Theme.color.neutral6 : Theme.color.neutral2)
        radius: 12

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }
}
