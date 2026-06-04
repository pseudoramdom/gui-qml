// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Window 2.15
import QtTest 1.2
import org.bitcoincore.qt 1.0
import "../../qml/pages/node"

TestCase {
    name: "NodeSettings"
    when: windowShown
    width: 520
    height: 720

    Window {
        id: testWindow
        width: 520
        height: 720
        visible: true
    }

    Component {
        id: nodeSettingsComponent

        NodeSettings {
            width: 460
            height: 680
        }
    }

    function init() {
        nodeModel.mempoolInformationAvailable = true
        AppMode.walletEnabled = true
        AppMode.isDesktop = true
    }

    function createNodeSettingsPage() {
        const page = createTemporaryObject(nodeSettingsComponent, testWindow.contentItem)
        verify(page !== null)
        wait(0)
        return page
    }

    function test_mempool_information_row_visible_when_available() {
        const page = createNodeSettingsPage()
        const row = findChild(page, "settings_mempool")
        verify(row !== null)
        compare(row.visible, true)
    }

    function test_mempool_information_row_hidden_when_unavailable() {
        nodeModel.mempoolInformationAvailable = false

        const page = createNodeSettingsPage()
        const row = findChild(page, "settings_mempool")
        verify(row !== null)
        compare(row.visible, false)
    }

    function test_sidebar_section_switching() {
        const page = createNodeSettingsPage()

        compare(page.currentSection, 0)

        const displayItem = findChild(page, "settings_display")
        verify(displayItem !== null)
        mouseClick(displayItem, displayItem.width / 2, displayItem.height / 2)
        compare(page.currentSection, 1)

        const connectionItem = findChild(page, "settings_connection")
        verify(connectionItem !== null)
        mouseClick(connectionItem, connectionItem.width / 2, connectionItem.height / 2)
        compare(page.currentSection, 5)
    }

    function test_wallet_section_hidden_when_disabled() {
        AppMode.walletEnabled = false

        const page = createNodeSettingsPage()
        const walletItem = findChild(page, "settings_wallet")
        verify(walletItem !== null)
        compare(walletItem.visible, false)

        const signerItem = findChild(page, "settings_externalsigner")
        verify(signerItem !== null)
        compare(signerItem.visible, false)
    }

    function test_window_behavior_hidden_on_non_desktop() {
        AppMode.isDesktop = false

        const page = createNodeSettingsPage()
        const windowItem = findChild(page, "settings_windowbehavior")
        verify(windowItem !== null)
        compare(windowItem.visible, false)
    }
}
