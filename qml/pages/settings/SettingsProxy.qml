// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../controls"
import "../../components"

Page {
    signal back

    id: root
    objectName: "settingsProxy"

    readonly property bool proxySettingsDirty: optionsModel.proxySettingsDirty

    background: null

    header: SettingsHeader {
        title: qsTr("Proxy settings")
        backButtonObjectName: "settingsProxyBack"
        onBack: root.back()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: width
        clip: true

        ColumnLayout {
            width: Math.min(parent.width, 450)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 0

            SettingsRestartNotice {
                Layout.fillWidth: true
                Layout.topMargin: 10
                Layout.bottomMargin: 20
                visible: optionsModel.proxySettingsDirty
            }

            ProxySettings {
                Layout.fillWidth: true
            }
        }
    }
}
