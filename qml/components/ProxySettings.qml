// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../controls"

import org.bitcoincore.qt 1.0

ColumnLayout {
    property string proxyLocationHeader: qsTr("Proxy location")

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
        actionItem: OptionSwitch {
            objectName: "proxyEnableSwitch"
            checked: optionsModel.proxyEnabled
            onToggled: {
                optionsModel.proxyEnabled = checked
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
        state: optionsModel.proxyEnabled ? "FILLED" : "DISABLED"
        showErrorText: loadedItem && !loadedItem.validInput && optionsModel.proxyEnabled
        actionItem: ProxyLocationInput {
            objectName: "proxyAddressInput"
            parentState: defaultProxy.state
            accessibleName: qsTr("Default proxy location")
            description: optionsModel.proxyAddress.length > 0
                         ? optionsModel.proxyAddress
                         : optionsModel.defaultProxyAddress()
            Component.onCompleted: {
                if (text !== "") filled = true
                validationError = optionsModel.validateProxyLocation(text)
                validInput = validationError === ""
            }
            onTextChanged: {
                validationError = optionsModel.validateProxyLocation(text)
                validInput = validationError === ""
            }
            onEditingFinished: {
                if (validInput) {
                    optionsModel.commitProxyLocation(text)
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
        actionItem: OptionSwitch {
            objectName: "torEnableSwitch"
            checked: optionsModel.torEnabled
            onToggled: {
                optionsModel.torEnabled = checked
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
        state: optionsModel.torEnabled ? "FILLED" : "DISABLED"
        showErrorText: loadedItem && !loadedItem.validInput && optionsModel.torEnabled
        actionItem: ProxyLocationInput {
            objectName: "torAddressInput"
            parentState: torProxy.state
            accessibleName: qsTr("Tor proxy location")
            description: optionsModel.torAddress.length > 0
                         ? optionsModel.torAddress
                         : optionsModel.defaultProxyAddress()
            Component.onCompleted: {
                if (text !== "") filled = true
                validationError = optionsModel.validateProxyLocation(text)
                validInput = validationError === ""
            }
            onTextChanged: {
                validationError = optionsModel.validateProxyLocation(text)
                validInput = validationError === ""
            }
            onEditingFinished: {
                if (validInput) {
                    optionsModel.commitTorLocation(text)
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
