// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick.Layouts 1.15

InfoBanner {
    id: root
    Layout.fillWidth: true
    Layout.leftMargin: 10
    Layout.rightMargin: 10
    iconSource: "image://images/info-filled"
    message: qsTr("Restart the application for these changes to take effect.")
    bannerLayout: InfoBanner.Layout.Horizontal
    contentMargin: 10
    contentSpacing: 8
}
