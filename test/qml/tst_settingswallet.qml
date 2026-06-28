// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Window 2.15
import QtTest 1.2
import "../../qml/pages/settings"

TestCase {
    name: "SettingsWallet"
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
        id: settingsWalletComponent

        SettingsWallet {
            width: 460
            height: 680
        }
    }

    function init() {
        optionsModel.walletSettingsDirty = false
    }

    function createSettingsWalletPage() {
        const page = createTemporaryObject(settingsWalletComponent, testWindow.contentItem)
        verify(page !== null)
        return page
    }

    function test_restart_notice_hidden_when_wallet_settings_unchanged() {
        const page = createSettingsWalletPage()
        const notice = findChild(page, "walletRestartNotice")
        verify(notice !== null)
        compare(notice.visible, false)
    }

    function test_restart_notice_visible_when_wallet_settings_changed() {
        optionsModel.walletSettingsDirty = true
        const page = createSettingsWalletPage()
        const notice = findChild(page, "walletRestartNotice")
        verify(notice !== null)
        compare(notice.visible, true)
    }
}
