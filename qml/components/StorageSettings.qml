// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../controls"

ColumnLayout {
    id: root
    property bool customStorage: false
    property int customStorageAmount
    property bool showRestartNotice: false
    spacing: 4
    SettingsRestartNotice {
        visible: root.showRestartNotice
        Layout.fillWidth: true
        Layout.bottomMargin: visible ? 12 : 0
    }
    Setting {
        Layout.fillWidth: true
        header: qsTr("Store recent blocks only")
        actionItem: OptionSwitch {
            checked: optionsModel.prune
            onToggled: optionsModel.prune = checked
        }
        onClicked: {
          loadedItem.toggle()
          loadedItem.toggled()
        }
    }
    Separator { Layout.fillWidth: true }
    Setting {
        id: pruneTargetSetting
        Layout.fillWidth: true
        header: qsTr("Block Storage limit (GB)")
        errorText: qsTr("This is not a valid prune target. Please choose a value that is equal to or larger than 1GB")
        state: optionsModel.prune ? "FILLED" : "DISABLED"
        showErrorText: false
        actionItem: ValueInput {
            parentState: pruneTargetSetting.state
            description: optionsModel.pruneSizeGB
            onEditingFinished: {
                const parsed = parseInt(text)
                if (isNaN(parsed) || parsed < 1) {
                    pruneTargetSetting.showErrorText = true
                } else {
                    root.customStorage = true
                    root.customStorageAmount = parsed
                    optionsModel.pruneSizeGB = parsed
                    pruneTargetSetting.forceActiveFocus()
                    pruneTargetSetting.showErrorText = false
                }
            }
        }
        onClicked: {
            loadedItem.filled = true
            loadedItem.forceActiveFocus()
        }
    }
    Separator { Layout.fillWidth: true }
    Setting {
        id: customDataDirSetting
        Layout.fillWidth: true
        header: qsTr("Data Directory")
    }
    CoreText {
        Layout.fillWidth: true
        text: optionsModel.dataDir
        color: Theme.color.neutral7
        font.pixelSize: 15
        horizontalAlignment: Text.AlignLeft
    }
}
