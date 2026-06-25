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
        // Display order and grouping follow the desktop settings design
        // (BitcoinDesign/Bitcoin-Core-App#163). Row order is kept in lockstep
        // with the contentStack page order below, so a row's index is its page
        // index. Peers and Console live on the main nav bar, not in settings.
        sidebarModel.append({ label: qsTr("Wallet"), section: "wallet", group: "wallet", alwaysVisible: false })
        sidebarModel.append({ label: qsTr("External Signer"), section: "externalsigner", group: "wallet", alwaysVisible: false })
        sidebarModel.append({ label: qsTr("Display"), section: "display", group: "display", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Window Behavior"), section: "windowbehavior", group: "display", alwaysVisible: false })
        sidebarModel.append({ label: qsTr("Storage"), section: "storage", group: "display", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Connection"), section: "connection", group: "network", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Network Traffic"), section: "networktraffic", group: "network", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("Mempool Information"), section: "mempool", group: "network", alwaysVisible: false })
        sidebarModel.append({ label: qsTr("Debug Log"), section: "debuglog", group: "developer", alwaysVisible: true })
        sidebarModel.append({ label: qsTr("About"), section: "about", group: "about", alwaysVisible: true })
        root.selectFirstVisibleSection()
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

    // Land on the first visible row so node-only mode (where Wallet/External
    // Signer are hidden) never opens on a hidden section.
    function selectFirstVisibleSection() {
        for (var i = 0; i < sidebarModel.count; i++) {
            if (isSectionVisible(i)) { root.currentSection = i; return }
        }
    }

    // True when this row begins a new group relative to the previous *visible*
    // row, so the delegate can add leading space between groups while skipping
    // hidden rows.
    function isFirstVisibleInGroup(index) {
        var group = sidebarModel.get(index).group
        for (var j = index - 1; j >= 0; j--) {
            if (!isSectionVisible(j)) continue
            return sidebarModel.get(j).group !== group
        }
        return false
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
            // Keep the sidebar off the window edge for breathing room, but drop
            // the margin near the minimum window width so content is not squeezed.
            Layout.leftMargin: root.width > 700 ? 20 : 0
            spacing: 0

            Repeater {
                model: sidebarModel
                delegate: AbstractButton {
                    objectName: "settings_" + model.section
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    Layout.topMargin: root.isFirstVisibleInGroup(index) ? 16 : 0
                    visible: root.isSectionVisible(index)
                    hoverEnabled: AppMode.isDesktop
                    focusPolicy: Qt.TabFocus
                    leftPadding: 16
                    rightPadding: 16
                    Accessible.name: model.label
                    Accessible.role: Accessible.ListItem

                    onClicked: root.currentSection = index

                    background: Item {
                        Rectangle {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            anchors.topMargin: 2
                            anchors.bottomMargin: 2
                            radius: 8
                            // Highlight the selected row with a filled pill rather
                            // than an accent bar; a fainter fill marks hover.
                            color: root.currentSection === index
                                ? Theme.color.neutral3
                                : parent.parent.hovered
                                    ? Theme.color.neutral2
                                    : "transparent"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        FocusBorder {
                            visible: parent.parent.visualFocus
                        }
                    }

                    contentItem: CoreText {
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
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
                // Content order is kept in lockstep with the sidebar row order
                // so the selected row maps directly to its page.
                currentIndex: root.currentSection

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
                SettingsWallet { showBackButton: false }
                SettingsDisplay { showBackButton: false }
                SettingsWindowBehavior { showBackButton: false }
                SettingsStorage { showBackButton: false }
                SettingsConnection { showBackButton: false }
                NetworkTraffic { showBackButton: false; showHeader: false }
                MempoolInformationSettings { showBackButton: false }
                SettingsDebugLog { showBackButton: false }
                PageStack {
                    id: aboutStack
                    initialItem: SettingsAbout {
                        showBackButton: false
                    }
                }
            }
        }
    }
}
