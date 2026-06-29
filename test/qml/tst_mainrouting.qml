// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtTest 1.2
import "../../qml/pages"

TestCase {
    id: testCase
    name: "MainRouting"
    when: windowShown
    width: 900
    height: 600

    property bool walletAvailable: true
    property bool preInitOnboardingRan: false
    property var windowUnderTest: null

    Component {
        id: mainWindowComponent
        MainWindow {
            walletAvailableForUi: testCase.walletAvailable
            preInitOnboardingRanForUi: testCase.preInitOnboardingRan
        }
    }

    function cleanup() {
        if (windowUnderTest) {
            windowUnderTest.close()
            windowUnderTest.destroy()
            windowUnderTest = null
        }
    }

    function createMain(wallet_enabled, preinit_onboarding_ran, no_wallets_found, wallet_dir_loaded, initialized) {
        cleanup()
        walletAvailable = wallet_enabled
        preInitOnboardingRan = preinit_onboarding_ran || false
        walletController.reset()
        walletListModel.reset()
        walletController.setInitialized(initialized === undefined ? true : initialized)
        walletController.setWalletLoaded(!no_wallets_found)
        walletController.setNoWalletsFound(no_wallets_found || false)
        walletListModel.setWalletDirLoaded(wallet_dir_loaded === undefined ? true : wallet_dir_loaded)
        windowUnderTest = mainWindowComponent.createObject(null)
        verify(windowUnderTest !== null)
        wait(0)
        return windowUnderTest
    }

    function test_wallet_available_routes_to_desktop_wallets() {
        const window = createMain(true)
        verify(findChild(window, "mainPageStack") !== null)
        verify(findChild(window, "desktopWalletsPage") !== null)
        verify(findChild(window, "walletBadge") !== null)
        verify(findChild(window, "nodeRunner") === null)
    }

    function test_wallet_unavailable_routes_to_node_runner() {
        const window = createMain(false)
        verify(findChild(window, "mainPageStack") !== null)
        verify(findChild(window, "nodeRunner") !== null)
        verify(findChild(window, "walletBadge") === null)
    }

    function test_preinit_onboarding_shows_startup_loading_while_wallet_scan_pending() {
        const window = createMain(true, true, false, false, false)
        verify(findChild(window, "postOnboardingStartupPage") !== null)
        verify(findChild(window, "postOnboardingStartupBusyIndicator") !== null)
        verify(findChild(window, "desktopWalletsPage") === null)
        verify(findChild(window, "createWalletWizard") === null)
        verify(findChild(window, "nodeRunner") === null)
    }

    function test_preinit_onboarding_no_wallets_opens_create_wallet_wizard_after_scan() {
        const window = createMain(true, true, true, false, false)
        verify(findChild(window, "postOnboardingStartupPage") !== null)
        verify(findChild(window, "createWalletWizard") === null)
        walletController.setInitialized(true)
        walletListModel.setWalletDirLoaded(true)
        tryVerify(function() {
            const createButton = findChild(window, "createWalletButton")
            return createButton && createButton.enabled
        })
        compare(findChild(window, "createWalletDiscoveryBusyIndicator").visible, false)
        verify(findChild(window, "desktopWalletsPage") !== null)
        verify(findChild(window, "walletBadge") !== null)
        verify(findChild(window, "nodeRunner") === null)
    }

    function test_preinit_onboarding_existing_wallets_opens_wallet_shell_after_scan() {
        const window = createMain(true, true, false, false, false)
        verify(findChild(window, "postOnboardingStartupPage") !== null)
        verify(findChild(window, "createWalletWizard") === null)
        walletController.setInitialized(true)
        walletListModel.setWalletDirLoaded(true)
        tryVerify(function() {
            return findChild(window, "desktopWalletsPage") !== null
        })
        verify(findChild(window, "desktopWalletsPage") !== null)
        verify(findChild(window, "createWalletWizard") === null)
        verify(findChild(window, "nodeRunner") === null)
    }

    function test_normal_restart_no_wallets_does_not_open_create_wallet_wizard() {
        const window = createMain(true, false, true)
        verify(findChild(window, "desktopWalletsPage") !== null)
        verify(findChild(window, "walletBadge") !== null)
        verify(findChild(window, "createWalletWizard") === null)
    }
}
