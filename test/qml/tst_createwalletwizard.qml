// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtTest 1.2
import "../../qml/pages/wallet"

TestCase {
    name: "CreateWalletWizard"
    when: windowShown
    width: 500
    height: 700

    readonly property string validXpub:
        "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8"

    Component {
        id: wizardComponent
        CreateWalletWizard {}
    }

    function init() {
        walletController.reset()
        walletListModel.reset()
        walletController.initialized = true
        walletController.isWalletLoaded = true
        walletController.walletLoadError = ""
        walletController.walletLoadInProgress = false
    }

    function test_wait_for_wallet_discovery_disables_actions_until_loaded() {
        walletListModel.walletDirLoaded = false

        const wizard = createTemporaryObject(wizardComponent, this)
        verify(wizard !== null)
        wizard.waitForWalletDiscovery = true

        const createButton = findChild(wizard, "createWalletButton")
        const importButton = findChild(wizard, "importWalletButton")
        verify(createButton !== null)
        verify(importButton !== null)

        compare(createButton.enabled, false)
        compare(importButton.enabled, false)

        walletListModel.walletDirLoaded = true

        tryCompare(createButton, "enabled", true)
        tryCompare(importButton, "enabled", true)
    }
}
