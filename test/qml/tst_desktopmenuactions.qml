// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtTest 1.2
import "../../qml/components"

TestCase {
    id: testCase
    name: "DesktopMenuActions"

    property var actionsUnderTest: null
    property var menuUnderTest: null

    Component {
        id: actionsComponent
        DesktopMenuActions {}
    }

    Component {
        id: menuComponent
        DesktopInWindowMenuBar {
            actions: testCase.actionsUnderTest
            active: true
        }
    }

    SignalSpy {
        id: exitSpy
        target: testCase.actionsUnderTest
        signalName: "exitRequested"
    }

    SignalSpy {
        id: undoSpy
        target: testCase.actionsUnderTest
        signalName: "undoRequested"
    }

    SignalSpy {
        id: rpcDocumentationSpy
        target: testCase.actionsUnderTest
        signalName: "rpcDocumentationRequested"
    }

    function init() {
        actionsUnderTest = actionsComponent.createObject(testCase)
        verify(actionsUnderTest !== null)
        menuUnderTest = menuComponent.createObject(testCase)
        verify(menuUnderTest !== null)
    }

    function cleanup() {
        if (menuUnderTest) {
            menuUnderTest.destroy()
            menuUnderTest = null
        }
        if (actionsUnderTest) {
            actionsUnderTest.destroy()
            actionsUnderTest = null
        }
        exitSpy.clear()
        undoSpy.clear()
        rpcDocumentationSpy.clear()
    }

    function enableLoadedWallet() {
        actionsUnderTest.walletMode = true
        actionsUnderTest.walletInitialized = true
        actionsUnderTest.walletLoaded = true
    }

    function test_node_mode_omits_wallet_commands() {
        compare(actionsUnderTest.createWallet.visible, false)
        compare(actionsUnderTest.activityView.visible, false)
        compare(actionsUnderTest.sendView.visible, false)
        compare(actionsUnderTest.receiveView.visible, false)
        compare(actionsUnderTest.settings.visible, true)
        compare(actionsUnderTest.nodeView.enabled, true)
        compare(actionsUnderTest.consoleView.enabled, true)
        compare(actionsUnderTest.exit.enabled, true)
    }

    function test_menu_contains_supported_sections_and_commands() {
        const fileMenu = findChild(menuUnderTest, "fileMenu")
        const editMenu = findChild(menuUnderTest, "editMenu")
        const viewMenu = findChild(menuUnderTest, "viewMenu")
        const windowMenu = findChild(menuUnderTest, "windowMenu")

        verify(fileMenu !== null)
        verify(editMenu !== null)
        verify(viewMenu !== null)
        verify(windowMenu !== null)
        verify(findChild(menuUnderTest, "helpMenu") !== null)
        verify(findChild(menuUnderTest, "settingsMenu") === null)
        verify(findChild(menuUnderTest, "menuCreateWallet") !== null)
        verify(findChild(menuUnderTest, "menuRestoreWallet") === null)
        verify(findChild(menuUnderTest, "menuOpenWallet") === null)
        verify(findChild(fileMenu, "menuSettings") !== null)
        verify(findChild(editMenu, "menuUndo") !== null)
        verify(findChild(editMenu, "menuRedo") !== null)
        verify(findChild(editMenu, "menuCopy") !== null)
        verify(findChild(editMenu, "menuPaste") !== null)
        verify(findChild(editMenu, "menuPastePaymentRequest") === null)
        verify(findChild(viewMenu, "menuNode") !== null)
        verify(findChild(viewMenu, "menuActivity") !== null)
        verify(findChild(viewMenu, "menuSend") !== null)
        verify(findChild(viewMenu, "menuReceive") !== null)
        verify(findChild(viewMenu, "menuInformation") !== null)
        verify(findChild(viewMenu, "menuConsole") !== null)
        verify(findChild(viewMenu, "menuNetworkTraffic") !== null)
        verify(findChild(viewMenu, "menuPeers") !== null)
        verify(findChild(viewMenu, "viewTrailingSeparator") !== null)
        verify(findChild(windowMenu, "menuInformation") === null)
        verify(findChild(windowMenu, "menuConsole") === null)
        verify(findChild(windowMenu, "menuNetworkTraffic") === null)
        verify(findChild(windowMenu, "menuPeers") === null)
        verify(findChild(windowMenu, "menuReceivingAddresses") === null)
        verify(findChild(menuUnderTest, "menuRpcDocumentation") !== null)
        verify(findChild(menuUnderTest, "menuAbout") !== null)
    }

    function test_wallet_commands_follow_lifecycle_state() {
        actionsUnderTest.walletMode = true
        compare(actionsUnderTest.createWallet.enabled, false)
        compare(actionsUnderTest.closeWallet.enabled, false)

        actionsUnderTest.walletInitialized = true
        compare(actionsUnderTest.createWallet.enabled, true)
        compare(actionsUnderTest.closeWallet.enabled, false)
        compare(actionsUnderTest.backupWallet.enabled, false)
        compare(actionsUnderTest.openUri.enabled, false)
        compare(actionsUnderTest.signMessage.enabled, false)
        compare(actionsUnderTest.verifyMessage.enabled, false)
        compare(actionsUnderTest.loadPsbt.enabled, false)
        compare(actionsUnderTest.activityView.enabled, false)
        compare(actionsUnderTest.sendView.enabled, false)
        compare(actionsUnderTest.receiveView.enabled, false)

        actionsUnderTest.walletLoaded = true
        compare(actionsUnderTest.closeWallet.enabled, true)
        compare(actionsUnderTest.backupWallet.enabled, true)
        compare(actionsUnderTest.openUri.enabled, true)
        compare(actionsUnderTest.signMessage.enabled, true)
        compare(actionsUnderTest.verifyMessage.enabled, true)
        compare(actionsUnderTest.loadPsbt.enabled, true)
        compare(actionsUnderTest.activityView.enabled, true)
        compare(actionsUnderTest.sendView.enabled, true)
        compare(actionsUnderTest.receiveView.enabled, true)

        actionsUnderTest.walletBusy = true
        compare(actionsUnderTest.createWallet.enabled, false)
        compare(actionsUnderTest.closeWallet.enabled, false)
        compare(actionsUnderTest.backupWallet.enabled, false)
        compare(actionsUnderTest.openUri.enabled, false)
        compare(actionsUnderTest.signMessage.enabled, false)
        compare(actionsUnderTest.verifyMessage.enabled, false)
        compare(actionsUnderTest.loadPsbt.enabled, false)
        compare(actionsUnderTest.activityView.enabled, false)
        compare(actionsUnderTest.sendView.enabled, false)
        compare(actionsUnderTest.receiveView.enabled, false)
    }

    function test_edit_commands_follow_target_state_and_emit_requests() {
        compare(actionsUnderTest.undo.enabled, false)
        compare(actionsUnderTest.redo.enabled, false)
        compare(actionsUnderTest.copy.enabled, false)
        compare(actionsUnderTest.paste.enabled, false)

        actionsUnderTest.canUndo = true
        actionsUnderTest.canRedo = true
        actionsUnderTest.canCopy = true
        actionsUnderTest.canPaste = true
        compare(actionsUnderTest.undo.enabled, true)
        compare(actionsUnderTest.redo.enabled, true)
        compare(actionsUnderTest.copy.enabled, true)
        compare(actionsUnderTest.paste.enabled, true)

        actionsUnderTest.undo.trigger()
        compare(undoSpy.count, 1)
    }

    function test_shutdown_disables_commands_and_blocks_trigger() {
        actionsUnderTest.shuttingDown = true
        compare(actionsUnderTest.exit.enabled, false)
        compare(actionsUnderTest.settings.enabled, false)
        actionsUnderTest.canUndo = true
        compare(actionsUnderTest.undo.enabled, false)
        actionsUnderTest.exit.trigger()
        compare(exitSpy.count, 0)
    }

    function test_enabled_command_emits_request() {
        actionsUnderTest.exit.trigger()
        compare(exitSpy.count, 1)

        actionsUnderTest.rpcDocumentation.trigger()
        compare(rpcDocumentationSpy.count, 1)
    }
}
