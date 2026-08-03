// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../controls"

AbstractButton {
    id: root

    property string address: ""
    property color primaryColor: Theme.color.neutral9
    property color secondaryColor: Theme.color.neutral7
    property var clipboard: Clipboard
    readonly property string formattedText: formatAddressRichText(address)
    readonly property bool showCopiedStatus: copiedResetTimer.running

    signal copied()

    function formatAddressRichText(value) {
        if (!value) return ""

        var html = ""
        for (var i = 0; i < value.length; i += 4) {
            var chunk = value.substring(i, Math.min(i + 4, value.length))
            var color = (Math.floor(i / 4) % 2 === 0)
                ? root.primaryColor
                : root.secondaryColor
            if (i > 0) html += " "
            html += "<nobr><font color=\"" + color + "\">" + chunk + "</font></nobr>"
        }
        return html
    }

    function copy() {
        if (root.address === "" || !root.clipboard) return
        root.clipboard.setText(root.address)
        copiedResetTimer.restart()
        root.copied()
    }

    hoverEnabled: AppMode.isDesktop
    enabled: address !== ""
    leftPadding: 8
    rightPadding: 8
    topPadding: 4
    bottomPadding: 4
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    Accessible.name: showCopiedStatus ? qsTr("Copied") : qsTr("Copy address")

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    onClicked: copy()

    contentItem: Item {
        id: content
        implicitHeight: Math.max(addressText.paintedHeight, copiedRow.implicitHeight)

        CoreText {
            id: addressText
            objectName: root.objectName + "Value"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: paintedHeight
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignTop
            font: Theme.text.monoBody.font
            lineHeight: Theme.text.monoBody.lineHeight
            lineHeightMode: Text.FixedHeight
            textFormat: Text.RichText
            text: root.formattedText
            wrapMode: Text.WordWrap
            opacity: root.showCopiedStatus ? 0 : 1

            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }

        RowLayout {
            id: copiedRow
            objectName: root.objectName + "CopiedStatus"
            anchors.centerIn: parent
            spacing: 6
            opacity: root.showCopiedStatus ? 1 : 0
            scale: root.showCopiedStatus ? 1 : 0.8

            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }

            Behavior on scale {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.InOutCubic
                }
            }

            Icon {
                Layout.alignment: Qt.AlignVCenter
                source: "qrc:/icons/copy"
                color: Theme.color.neutral9
                size: 20
            }

            CoreText {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Copied")
                color: Theme.color.neutral9
                font: Theme.text.body.font
                lineHeight: Theme.text.body.lineHeight
                lineHeightMode: Text.FixedHeight
            }
        }
    }

    background: Rectangle {
        radius: 5
        color: root.hovered || root.down ? Theme.color.neutral2 : "transparent"

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    Timer {
        id: copiedResetTimer
        interval: 1000
    }
}
