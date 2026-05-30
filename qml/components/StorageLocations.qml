// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

import org.bitcoincore.qt 1.0

import "../controls"

ColumnLayout {
    id: root
    property string validationError: ""
    readonly property bool validSelection: validationError.length === 0

    ButtonGroup {
        id: group
    }
    spacing: 15
    OptionButton {
        id: defaultDirOption
        Layout.fillWidth: true
        ButtonGroup.group: group
        text: qsTr("Default")
        description: qsTr("Your application directory.")
        customDir: optionsModel.getDefaultDataDirString
        checked: optionsModel.dataDir === optionsModel.getDefaultDataDirString
        onClicked: {
            root.validationError = ""
            optionsModel.useDefaultDataDir()
        }
    }
    OptionButton {
        id: customDirOption
        Layout.fillWidth: true
        ButtonGroup.group: group
        text: qsTr("Custom")
        description: qsTr("Choose the directory and storage device.")
        customDir: checked && optionsModel.dataDir !== optionsModel.getDefaultDataDirString ? optionsModel.dataDir : ""
        checked: optionsModel.dataDir !== optionsModel.getDefaultDataDirString
        onClicked: fileDialog.open()
    }
    FileDialog {
        id: fileDialog
        onAccepted: {
            var customDataDir = fileDialog.selectedFile.toString();
            if (customDataDir !== "") {
                root.validationError = optionsModel.validateCustomDataDir(customDataDir)
                if (root.validationError === "" && optionsModel.selectCustomDataDir(customDataDir)) {
                } else if (root.validationError === "") {
                    root.validationError = qsTr("The selected data directory could not be created.")
                }
            }
        }
        onRejected: {
            console.log("Custom datadir selection canceled")
        }
    }
    CoreText {
        Layout.fillWidth: true
        visible: root.validationError.length > 0
        text: root.validationError
        color: Theme.color.blue
        horizontalAlignment: Text.AlignLeft
        font.pixelSize: 15
    }
}
