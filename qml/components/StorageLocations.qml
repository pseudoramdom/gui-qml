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
    property var settingsModel: optionsModel
    property string validationError: ""
    readonly property string storagePathMessage: root.settingsModel.storagePathMessage || ""
    readonly property string storageAvailableText: root.settingsModel.storageAvailableText || ""
    readonly property string storageErrorText: root.settingsModel.storageErrorText || ""
    readonly property bool storageCheckPending: root.settingsModel.storageCheckPending || false
    readonly property bool validSelection: validationError.length === 0 && storageErrorText.length === 0 && !storageCheckPending

    function updateValidation() {
        if (root.settingsModel.dataDir === root.settingsModel.getDefaultDataDirString) {
            root.validationError = ""
        } else {
            root.validationError = root.settingsModel.validateCustomDataDir(root.settingsModel.dataDir)
        }
    }

    Component.onCompleted: updateValidation()

    Connections {
        target: root.settingsModel
        function onDataDirChanged() {
            root.updateValidation()
        }
    }

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
        customDir: root.settingsModel.getDefaultDataDirString
        checked: root.settingsModel.dataDir === root.settingsModel.getDefaultDataDirString
        onClicked: {
            root.validationError = ""
            root.settingsModel.useDefaultDataDir()
        }
    }
    OptionButton {
        id: customDirOption
        Layout.fillWidth: true
        ButtonGroup.group: group
        text: qsTr("Custom")
        description: qsTr("Choose the directory and storage device.")
        customDir: checked && root.settingsModel.dataDir !== root.settingsModel.getDefaultDataDirString ? root.settingsModel.dataDir : ""
        checked: root.settingsModel.dataDir !== root.settingsModel.getDefaultDataDirString
        onClicked: folderDialog.open()
    }
    FolderDialog {
        id: folderDialog
        objectName: "customDataDirFolderDialog"
        onAccepted: {
            var customDataDir = folderDialog.selectedFolder.toString();
            if (customDataDir !== "") {
                root.validationError = root.settingsModel.validateCustomDataDir(customDataDir)
                if (root.validationError === "" && root.settingsModel.selectCustomDataDir(customDataDir)) {
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
    CoreText {
        Layout.fillWidth: true
        visible: root.validationError.length === 0 && root.storageErrorText.length > 0
        text: root.storageErrorText
        color: Theme.color.blue
        horizontalAlignment: Text.AlignLeft
        font.pixelSize: 15
    }
    CoreText {
        Layout.fillWidth: true
        visible: root.validationError.length === 0 && root.storageErrorText.length === 0 && (root.storageCheckPending || root.storageAvailableText.length > 0 || root.storagePathMessage.length > 0)
        text: root.storageCheckPending
            ? qsTr("Checking available storage...")
            : root.storageAvailableText.length > 0
                ? qsTr("%1. %2").arg(root.storageAvailableText).arg(root.storagePathMessage)
                : root.storagePathMessage
        color: Theme.color.neutral7
        horizontalAlignment: Text.AlignLeft
        font.pixelSize: 15
    }
}
