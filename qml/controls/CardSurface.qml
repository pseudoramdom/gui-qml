// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15

Pane {
    id: root

    property color surfaceColor: Theme.dark ? Theme.color.neutral0 : Theme.color.neutral1
    property color outlineColor: Theme.color.neutral2
    property real cornerRadius: 18

    padding: 0

    background: Rectangle {
        color: root.surfaceColor
        border.color: root.outlineColor
        border.width: 1
        radius: root.cornerRadius
    }
}
