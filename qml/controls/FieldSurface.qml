// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15

Item {
    id: root

    default property alias fieldData: content.data
    property bool active: false
    property bool error: false
    property bool readOnly: false
    property real cornerRadius: 10
    property real contentPadding: 14

    implicitHeight: 56

    Rectangle {
        anchors.fill: parent
        color: Theme.color.neutral1
        border.width: root.active || root.error ? 2 : 1
        border.color: root.error
            ? Theme.color.red
            : (root.active ? Theme.color.orange : Theme.color.neutral2)
        radius: root.cornerRadius
        opacity: root.readOnly ? 0.65 : 1

        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    Item {
        id: content
        anchors.fill: parent
        anchors.margins: root.contentPadding
    }
}
