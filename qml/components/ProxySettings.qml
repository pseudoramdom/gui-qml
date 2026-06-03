// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../controls"

import org.bitcoincore.qt 1.0

ColumnLayout {
    id: root
    property var settingsModel: optionsModel
    property var coreSettingsModel: settingsModel.coreSettings
    property string proxyLocationHeader: qsTr("Proxy location")
    readonly property var proxySetting: coreSettingsModel.entry("proxy")
    readonly property var onionSetting: coreSettingsModel.entry("onion")

    spacing: 4
    Header {
        headerBold: true
        center: false
        header: qsTr("Default Proxy")
        headerSize: 24
        description: qsTr("Run peer connections through a proxy (SOCKS5) for improved privacy. The default proxy supports connections via IPv4, IPv6 and Tor.")
        descriptionSize: 15
        Layout.bottomMargin: 10
    }
    Separator { Layout.fillWidth: true }
    Setting {
        id: defaultProxyEnable
        Layout.fillWidth: true
        header: qsTr("Enable")
        state: root.proxySetting.canEdit ? "FILLED" : "DISABLED"
        infoText: root.proxySetting.infoText
        showInfoText: infoText.length > 0
        actionItem: OptionSwitch {
            objectName: "proxyEnableSwitch"
            checked: root.proxySetting.enabled
            onToggled: {
                root.proxySetting.enabled = checked
            }
        }
        onClicked: {
            loadedItem.toggle()
            loadedItem.toggled()
        }
    }
    Separator { Layout.fillWidth: true }
    Setting {
        id: defaultProxy
        Layout.fillWidth: true
        header: proxyLocationHeader
        errorText: loadedItem ? loadedItem.validationError : ""
        state: root.proxySetting.enabled && root.proxySetting.canEdit ? "FILLED" : "DISABLED"
        showErrorText: loadedItem && !loadedItem.validInput && root.proxySetting.enabled
        infoText: root.proxySetting.infoText
        showInfoText: !showErrorText && infoText.length > 0
        actionItem: ProxyLocationInput {
            objectName: "proxyAddressInput"
            parentState: defaultProxy.visualState
            accessibleName: qsTr("Default proxy location")
            description: root.proxySetting.address.length > 0
                         ? root.proxySetting.address
                         : root.proxySetting.defaultAddress()
            Component.onCompleted: {
                if (text !== "") filled = true
                validationError = root.proxySetting.validate(text)
                validInput = validationError === ""
            }
            onTextChanged: {
                validationError = root.proxySetting.validate(text)
                validInput = validationError === ""
            }
            onEditingFinished: {
                if (validInput) {
                    root.proxySetting.commitAddress(text)
                    filled = true
                }
            }
        }
        onClicked: {
            loadedItem.beginEdit()
        }
    }
    Separator { Layout.fillWidth: true }
    Header {
        headerBold: true
        center: false
        header: qsTr("Tor Proxy")
        headerSize: 24
        description: qsTr("Run Tor connections through a dedicated proxy.")
        descriptionSize: 15
        Layout.topMargin: 35
        Layout.bottomMargin: 10
    }
    Separator { Layout.fillWidth: true }
    Setting {
        id: torProxyEnable
        Layout.fillWidth: true
        header: qsTr("Enable")
        state: root.onionSetting.canEdit ? "FILLED" : "DISABLED"
        infoText: root.onionSetting.infoText
        showInfoText: infoText.length > 0
        actionItem: OptionSwitch {
            objectName: "torEnableSwitch"
            checked: root.onionSetting.enabled
            onToggled: {
                root.onionSetting.enabled = checked
            }
        }
        onClicked: {
            loadedItem.toggle()
            loadedItem.toggled()
        }
    }
    Separator { Layout.fillWidth: true }
    Setting {
        id: torProxy
        Layout.fillWidth: true
        header: proxyLocationHeader
        errorText: loadedItem ? loadedItem.validationError : ""
        state: root.onionSetting.enabled && root.onionSetting.canEdit ? "FILLED" : "DISABLED"
        showErrorText: loadedItem && !loadedItem.validInput && root.onionSetting.enabled
        infoText: root.onionSetting.infoText
        showInfoText: !showErrorText && infoText.length > 0
        actionItem: ProxyLocationInput {
            objectName: "torAddressInput"
            parentState: torProxy.visualState
            accessibleName: qsTr("Tor proxy location")
            description: root.onionSetting.address.length > 0
                         ? root.onionSetting.address
                         : root.onionSetting.defaultAddress()
            Component.onCompleted: {
                if (text !== "") filled = true
                validationError = root.onionSetting.validate(text)
                validInput = validationError === ""
            }
            onTextChanged: {
                validationError = root.onionSetting.validate(text)
                validInput = validationError === ""
            }
            onEditingFinished: {
                if (validInput) {
                    root.onionSetting.commitAddress(text)
                    filled = true
                }
            }
        }
        onClicked: {
            loadedItem.beginEdit()
        }
    }
    Separator { Layout.fillWidth: true }
}
