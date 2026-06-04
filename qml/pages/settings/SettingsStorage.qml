// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../controls"
import "../../components"

InformationPage {
    id: root
    property bool customStorage: false
    property bool customStorageAmount
    property bool onboarding: false
    property bool showBackButton: true
    bannerActive: false
    bold: true
    showHeader: root.onboarding
    headerText: qsTr("Storage settings")
    headerMargin: 0
    detailActive: true
    detailItem: StorageSettings {
        id: storageSettings
        onCustomStorageChanged: {
            root.customStorage = storageSettings.customStorage
        }
        onCustomStorageAmountChanged: {
            root.customStorageAmount = storageSettings.customStorageAmount
        }
    }
    showNavBar: false
    header: SettingsHeader {
        title: root.onboarding ? "" : qsTr("Storage Settings")
        showBackButton: !root.onboarding && root.showBackButton
        backButtonObjectName: "settingsStorageBack"
        onBack: root.back()
        rightItem: NavButton {
            visible: root.onboarding
            text: qsTr("Done")
            onClicked: root.back()
        }
    }
}
