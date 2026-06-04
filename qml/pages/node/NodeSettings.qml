// Copyright (c) 2022-2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0
import "../../controls"
import "../../components"
import "../wallet"
import "../settings"

Page {
    signal doneClicked
    signal selectWalletRequested
    signal receiveRequested

    property alias showDoneButton: doneButton.visible

    id: root
    objectName: "nodeSettingsStack"
    background: null

    property int currentSection: 0

    function openWalletSettings() {
        for (var i = 0; i < sidebarModel.count; i++) {
            if (sidebarModel.get(i).section === "wallet") {
                root.currentSection = i
                return
            }
        }
    }

    function openAddressHistory() {
        if (!walletController.isWalletLoaded || !walletController.selectedWallet) {
            return
        }
        walletController.selectedWallet.addressListModel.refresh()
        openWalletSettings()
        walletStack.push(addressListComp)
    }

    function openWalletAddressHistory() {
        root.openAddressHistory()
    }

    Connections {
        target: typeof walletController !== "undefined" ? walletController : null
        function onOpenWalletSettingsRequested() {
            root.openWalletSettings()
        }
        function onSelectedWalletChanged() {
            if (walletStack.depth > 1) walletStack.pop(null)
        }
        function onIsWalletLoadedChanged() {
            if (!walletController.isWalletLoaded && walletStack.depth > 1) walletStack.pop(null)
        }
    }

    ListModel { id: sidebarModel }

    Component.onCompleted: {
        sidebarModel.append({ label: qsTr("About"), section: "about", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Display"), section: "display", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Wallet"), section: "wallet", alwaysVisible: false })
        sidebarModel.append({ label: qsTr("Storage"), section: "storage", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("External Signer"), section: "externalsigner", alwaysVisible: false })
        sidebarModel.append({ label: qsTr("Connection"), section: "connection", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Network Traffic"), section: "networktraffic", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Mempool Information"), section: "mempool", alwaysVisible: false })
        sidebarModel.append({ label: qsTr("Debug Log"), section: "debuglog", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Window Behavior"), section: "windowbehavior", alwaysVisible: false })
    }

    function isSectionVisible(index) {
        var item = sidebarModel.get(index)
        if (item.alwaysVisible) return true
        if (item.section === "wallet" || item.section === "externalsigner")
            return AppMode.walletEnabled
        if (item.section === "mempool")
            return nodeModel.mempoolInformationAvailable
        if (item.section === "windowbehavior")
            return AppMode.isDesktop
        return true
    }

    contentItem: RowLayout {
        spacing: 0

        ColumnLayout {
            Layout.preferredWidth: 200
            Layout.maximumWidth: 200
            Layout.minimumWidth: 200
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.topMargin: 20
            spacing: 0

            Repeater {
                model: sidebarModel
                delegate: AbstractButton {
                    objectName: "settings_" + model.section
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    visible: root.isSectionVisible(index)
                    hoverEnabled: AppMode.isDesktop
                    focusPolicy: Qt.TabFocus
                    rightPadding: 20
                    Accessible.name: model.label
                    Accessible.role: Accessible.ListItem

                    onClicked: root.currentSection = index

                    background: Item {
                        Rectangle {
                            id: indicator
                            width: 3
                            height: parent.height - 8
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            color: Theme.color.orange
                            visible: root.currentSection === index
                        }
                        FocusBorder {
                            visible: parent.parent.visualFocus
                        }
                    }

                    contentItem: CoreText {
                        horizontalAlignment: Text.AlignRight
                        text: model.label
                        font.pixelSize: 15
                        color: root.currentSection === index
                            ? Theme.color.orange
                            : parent.hovered
                                ? Theme.color.orangeLight1
                                : Theme.color.neutral7
                    }

                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }

            Item { Layout.fillHeight: true }

            NavButton {
                id: doneButton
                objectName: "nodeSettingsDoneButton"
                text: qsTr("Done")
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 20
                onClicked: root.doneClicked()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.rightMargin: 20
            color: "transparent"
            clip: true

            StackLayout {
                id: contentStack
                anchors.fill: parent
                currentIndex: root.currentSection

                PageStack {
                    id: aboutStack
                    initialItem: SettingsAbout {
                        showBackButton: false
                    }
                }
                SettingsDisplay { showBackButton: false }
                PageStack {
                    id: walletStack
                    objectName: "walletSettingsStack"
                    initialItem: WalletSettings {
                        objectName: "walletSettingsPage"
                        showBackButton: walletStack.depth > 1
                        onBack: walletStack.pop()
                        onSelectWalletRequested: root.selectWalletRequested()
                        onPasswordRequested: walletStack.push(walletPasswordComp, { "updating": walletController.selectedWallet.isEncrypted })
                        onSignVerifyMessageRequested: walletStack.push(signVerifyComp)
                        onAddressesRequested: {
                            if (walletController.isWalletLoaded && walletController.selectedWallet) {
                                walletController.selectedWallet.addressListModel.refresh()
                                walletStack.push(addressListComp)
                            }
                        }
                    }
                    Component {
                        id: walletPasswordComp
                        WalletPasswordSettings {
                            onBack: walletStack.pop()
                            onSaved: walletStack.pop()
                        }
                    }
                    Component {
                        id: signVerifyComp
                        SignVerifyMessage {
                            onBack: walletStack.pop()
                        }
                    }
                    Component {
                        id: addressListComp
                        AddressList {
                            onBack: walletStack.pop()
                            onReceiveRequested: {
                                walletStack.pop()
                                root.receiveRequested()
                            }
                        }
                    }
                }
                SettingsStorage { showBackButton: false }
                SettingsWallet { showBackButton: false }
                SettingsConnection { showBackButton: false }
                NetworkTraffic { showBackButton: false; showHeader: false }
                MempoolInformationSettings { showBackButton: false }
                SettingsDebugLog { showBackButton: false }
                SettingsWindowBehavior { showBackButton: false }
            }
        }
    }
}
