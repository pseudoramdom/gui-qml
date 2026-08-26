// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls

Controls.MenuBar {
    id: root
    objectName: "desktopInWindowMenuBar"

    property DesktopMenuActions actions
    property bool active: false

    visible: active
    enabled: active && !actions.shuttingDown

    Controls.Menu {
        objectName: "fileMenu"
        title: root.actions.fileMenuTitle

        Controls.MenuItem {
            objectName: "menuCreateWallet"
            visible: root.actions.createWallet.visible
            action: Controls.Action {
                text: root.actions.createWallet.text
                enabled: root.actions.createWallet.enabled
                shortcut: root.actions.createWallet.shortcut
                onTriggered: root.actions.createWallet.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuCloseWallet"
            visible: root.actions.closeWallet.visible
            action: Controls.Action {
                text: root.actions.closeWallet.text
                enabled: root.actions.closeWallet.enabled
                shortcut: root.actions.closeWallet.shortcut
                onTriggered: root.actions.closeWallet.trigger()
            }
        }
        Controls.MenuSeparator { visible: root.actions.walletMode }
        Controls.MenuItem {
            objectName: "menuBackupWallet"
            visible: root.actions.backupWallet.visible
            action: Controls.Action {
                text: root.actions.backupWallet.text
                enabled: root.actions.backupWallet.enabled
                shortcut: root.actions.backupWallet.shortcut
                onTriggered: root.actions.backupWallet.trigger()
            }
        }
        Controls.MenuSeparator { visible: root.actions.walletMode }
        Controls.MenuItem {
            objectName: "menuOpenUri"
            visible: root.actions.openUri.visible
            action: Controls.Action {
                text: root.actions.openUri.text
                enabled: root.actions.openUri.enabled
                shortcut: root.actions.openUri.shortcut
                onTriggered: root.actions.openUri.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuSignMessage"
            visible: root.actions.signMessage.visible
            action: Controls.Action {
                text: root.actions.signMessage.text
                enabled: root.actions.signMessage.enabled
                shortcut: root.actions.signMessage.shortcut
                onTriggered: root.actions.signMessage.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuVerifyMessage"
            visible: root.actions.verifyMessage.visible
            action: Controls.Action {
                text: root.actions.verifyMessage.text
                enabled: root.actions.verifyMessage.enabled
                shortcut: root.actions.verifyMessage.shortcut
                onTriggered: root.actions.verifyMessage.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuLoadPsbt"
            visible: root.actions.loadPsbt.visible
            action: Controls.Action {
                text: root.actions.loadPsbt.text
                enabled: root.actions.loadPsbt.enabled
                shortcut: root.actions.loadPsbt.shortcut
                onTriggered: root.actions.loadPsbt.trigger()
            }
        }
        Controls.MenuSeparator { visible: root.actions.walletMode }
        Controls.MenuItem {
            objectName: "menuSettings"
            action: Controls.Action {
                text: root.actions.settings.text
                enabled: root.actions.settings.enabled
                shortcut: root.actions.settings.shortcut
                onTriggered: root.actions.settings.trigger()
            }
        }
        Controls.MenuSeparator {}
        Controls.MenuItem {
            objectName: "menuExit"
            action: Controls.Action {
                text: root.actions.exit.text
                enabled: root.actions.exit.enabled
                shortcut: root.actions.exit.shortcut
                onTriggered: root.actions.exit.trigger()
            }
        }
    }

    Controls.Menu {
        objectName: "editMenu"
        title: root.actions.editMenuTitle

        Controls.MenuItem {
            objectName: "menuUndo"
            action: Controls.Action {
                text: root.actions.undo.text
                enabled: root.actions.undo.enabled
                shortcut: root.actions.undo.shortcut
                onTriggered: root.actions.undo.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuRedo"
            action: Controls.Action {
                text: root.actions.redo.text
                enabled: root.actions.redo.enabled
                shortcut: root.actions.redo.shortcut
                onTriggered: root.actions.redo.trigger()
            }
        }
        Controls.MenuSeparator {}
        Controls.MenuItem {
            objectName: "menuCopy"
            action: Controls.Action {
                text: root.actions.copy.text
                enabled: root.actions.copy.enabled
                shortcut: root.actions.copy.shortcut
                onTriggered: root.actions.copy.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuPaste"
            action: Controls.Action {
                text: root.actions.paste.text
                enabled: root.actions.paste.enabled
                shortcut: root.actions.paste.shortcut
                onTriggered: root.actions.paste.trigger()
            }
        }
    }

    Controls.Menu {
        objectName: "viewMenu"
        title: root.actions.viewMenuTitle

        Controls.MenuItem {
            objectName: "menuNode"
            action: Controls.Action {
                text: root.actions.nodeView.text
                enabled: root.actions.nodeView.enabled
                shortcut: root.actions.nodeView.shortcut
                onTriggered: root.actions.nodeView.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuActivity"
            visible: root.actions.activityView.visible
            action: Controls.Action {
                text: root.actions.activityView.text
                enabled: root.actions.activityView.enabled
                shortcut: root.actions.activityView.shortcut
                onTriggered: root.actions.activityView.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuSend"
            visible: root.actions.sendView.visible
            action: Controls.Action {
                text: root.actions.sendView.text
                enabled: root.actions.sendView.enabled
                shortcut: root.actions.sendView.shortcut
                onTriggered: root.actions.sendView.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuReceive"
            visible: root.actions.receiveView.visible
            action: Controls.Action {
                text: root.actions.receiveView.text
                enabled: root.actions.receiveView.enabled
                shortcut: root.actions.receiveView.shortcut
                onTriggered: root.actions.receiveView.trigger()
            }
        }
        Controls.MenuSeparator { visible: root.actions.walletMode }
        Controls.MenuItem {
            objectName: "menuInformation"
            action: Controls.Action {
                text: root.actions.information.text
                enabled: root.actions.information.enabled
                shortcut: root.actions.information.shortcut
                onTriggered: root.actions.information.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuConsole"
            action: Controls.Action {
                text: root.actions.consoleView.text
                enabled: root.actions.consoleView.enabled
                shortcut: root.actions.consoleView.shortcut
                onTriggered: root.actions.consoleView.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuNetworkTraffic"
            action: Controls.Action {
                text: root.actions.networkTraffic.text
                enabled: root.actions.networkTraffic.enabled
                shortcut: root.actions.networkTraffic.shortcut
                onTriggered: root.actions.networkTraffic.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuPeers"
            action: Controls.Action {
                text: root.actions.peers.text
                enabled: root.actions.peers.enabled
                shortcut: root.actions.peers.shortcut
                onTriggered: root.actions.peers.trigger()
            }
        }
        Controls.MenuSeparator {
            objectName: "viewTrailingSeparator"
        }
    }

    Controls.Menu {
        objectName: "windowMenu"
        title: root.actions.windowMenuTitle

        Controls.MenuItem {
            objectName: "menuMinimize"
            action: Controls.Action {
                text: root.actions.minimize.text
                enabled: root.actions.minimize.enabled
                shortcut: root.actions.minimize.shortcut
                onTriggered: root.actions.minimize.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuZoom"
            visible: root.actions.zoom.visible
            action: Controls.Action {
                text: root.actions.zoom.text
                enabled: root.actions.zoom.enabled
                shortcut: root.actions.zoom.shortcut
                onTriggered: root.actions.zoom.trigger()
            }
        }
        Controls.MenuSeparator { visible: root.actions.mainWindow.visible }
        Controls.MenuItem {
            objectName: "menuMainWindow"
            visible: root.actions.mainWindow.visible
            action: Controls.Action {
                text: root.actions.mainWindow.text
                enabled: root.actions.mainWindow.enabled
                shortcut: root.actions.mainWindow.shortcut
                onTriggered: root.actions.mainWindow.trigger()
            }
        }
    }

    Controls.Menu {
        objectName: "helpMenu"
        title: root.actions.helpMenuTitle

        Controls.MenuItem {
            objectName: "menuRpcDocumentation"
            action: Controls.Action {
                text: root.actions.rpcDocumentation.text
                enabled: root.actions.rpcDocumentation.enabled
                shortcut: root.actions.rpcDocumentation.shortcut
                onTriggered: root.actions.rpcDocumentation.trigger()
            }
        }
        Controls.MenuItem {
            objectName: "menuAbout"
            action: Controls.Action {
                text: root.actions.about.text
                enabled: root.actions.about.enabled
                shortcut: root.actions.about.shortcut
                onTriggered: root.actions.about.trigger()
            }
        }
    }
}
