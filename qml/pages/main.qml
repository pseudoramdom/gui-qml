// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.0
import org.bitcoincore.qt 1.0
import "../components"
import "../controls"
import "./onboarding"
import "./node"
import "./wallet"

ApplicationWindow {
    id: appWindow
    title: qsTr("Bitcoin Core App")
    minimumWidth: 640
    minimumHeight: 665
    color: Theme.color.background
    visible: true

    // Restore the saved size via initial bindings so it is applied during
    // construction, before the child Popups build their GridLayouts. Assigning
    // width/height after the scene is laid out (e.g. in onCompleted) forces a
    // live relayout that crashes Qt 6.4.x Quick Layouts. The bindings are
    // broken in Component.onCompleted (self-assign) so the user can still
    // resize the window freely afterwards.
    width: windowSettings.windowWidth > 0 ? windowSettings.windowWidth : minimumWidth
    height: windowSettings.windowHeight > 0 ? windowSettings.windowHeight : minimumHeight

    Settings {
        id: windowSettings
        property real windowX
        property real windowY
        property real windowWidth
        property real windowHeight
    }

    onXChanged: if (visibility === Window.Windowed) windowSettings.windowX = x
    onYChanged: if (visibility === Window.Windowed) windowSettings.windowY = y
    onWidthChanged: if (visibility === Window.Windowed) windowSettings.windowWidth = width
    onHeightChanged: if (visibility === Window.Windowed) windowSettings.windowHeight = height


    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    // Tracks the previous visibility state to distinguish a user-initiated minimize
    // (Windowed → Minimized) from a WM restore transition (Hidden → Minimized).
    // The latter occurs when showNormal() calls setVisible(true) before
    // setWindowState(NoState); without this guard the onVisibilityChanged handler
    // would immediately re-hide the window, making tray-icon restore a no-op.
    property int m_prevVisibility: Window.AutomaticVisibility

    onClosing: (close) => {
        close.accepted = false
        if (desktopWindowBehaviorModel.shouldMinimizeWindowOnClose()) {
            showMinimized()
        } else {
            nodeModel.requestShutdown()
        }
    }

    onVisibilityChanged: function(visibility) {
        const prev = m_prevVisibility
        m_prevVisibility = visibility

        // Capture geometry once the window reaches its normal (Windowed) state.
        // The onWidthChanged/onHeightChanged writes only fire on a *change*, and
        // the size is set before the window is Windowed (the onCompleted self-
        // assign that breaks the size binding doesn't re-trigger them), so without
        // this the initial size is never saved (windowWidth/Height stay 0).
        if (visibility === Window.Windowed) {
            windowSettings.windowX = x
            windowSettings.windowY = y
            windowSettings.windowWidth = width
            windowSettings.windowHeight = height
        }

        if (visibility === Window.Minimized &&
                prev !== Window.Hidden &&
                desktopWindowBehaviorModel.shouldHideToTrayOnMinimize()) {
            desktopTrayIconController.hideMainWindow()
        }
    }

    Connections {
        target: desktopWindowBehaviorModel
        function onShowTrayIconChanged(show) {
            desktopTrayIconController.visible = AppMode.isDesktop && show
        }
    }

    Binding {
        target: desktopTrayIconController
        property: "windowVisible"
        value: appWindow.visible &&
               appWindow.visibility !== Window.Hidden &&
               appWindow.visibility !== Window.Minimized
    }

    Connections {
        target: desktopTrayIconController
        function onShowRequested() {
            desktopTrayIconController.showMainWindow()
        }
        function onHideRequested() {
            desktopTrayIconController.hideMainWindow()
        }
        function onQuitRequested() {
            nodeModel.requestShutdown()
        }
    }

    PageStack {
        id: main
        objectName: "mainPageStack"
        initialItem: {
            if (needOnboarding) {
                onboardingWizard
            } else {
                if (AppMode.walletEnabled && AppMode.isDesktop) {
                    desktopWallets
                } else {
                    node
                }
            }
        }
        anchors.fill: parent
        focus: true
        Keys.onReleased: (event) => {
            if (event.key == Qt.Key_Back) {
                nodeModel.requestShutdown()
                event.accepted = true
            }
        }
    }

    Connections {
        target: nodeModel
        function onRequestedShutdown() {
            main.clear()
            main.push(shutdown)
        }
    }

    NodeRuntimeDialog {
        parent: Overlay.overlay
    }

    NodeFatalErrorPopup {
        parent: Overlay.overlay
    }

    Component {
        id: onboardingWizard
        OnboardingWizard {
            onFinished: {
                optionsModel.onboard()
                nodeModel.startNodeInitializionThread()
                if (AppMode.walletEnabled && AppMode.isDesktop) {
                    main.push([
                        desktopWallets, {},
                        createWalletWizard, { "launchContext": CreateWalletWizard.Context.Onboarding }
                    ])
                } else {
                    main.push(node)
                }
            }
        }
    }

    Component {
        id: desktopWallets
        DesktopWallets {
            onAddWallet: {
                main.push(createWalletWizard, { "launchContext": CreateWalletWizard.Context.Main })
            }
            onSendTransaction: {
                main.push(sendReviewPage)
            }
        }
    }

    Component {
        id: createWalletWizard
        CreateWalletWizard {
            onFinished: {
                main.pop()
            }
        }
    }

    Component {
        id: sendReviewPage
        SendReview {
            onBack: {
                main.pop()
            }
            onTransactionSent: {
                const externalSignerWallet = walletController.selectedWallet.hasExternalSigner
                const descriptionText = externalSignerWallet
                    ? qsTr("Approved on external signer. It should be confirmed within the next 10 minutes.")
                    : qsTr("Based on your selected fee, it should be confirmed within the next 10 minutes.")
                const actionText = externalSignerWallet ? qsTr("Done") : qsTr("Close window")
                walletController.selectedWallet.recipients.clear()
                main.push(sendResultPage, {
                    "descriptionText": descriptionText,
                    "actionText": actionText
                })
            }
        }
    }

    Component {
        id: sendResultPage
        SendResult {
            onDone: {
                main.pop(null)
            }
            onViewNewTransaction: {
                main.pop(null)
            }
        }
    }

    Component {
        id: shutdown
        Shutdown {}
    }

    Component.onCompleted: {
        // Break the size bindings without changing the value (no relayout), so
        // the user can resize freely while the one-way onWidthChanged/
        // onHeightChanged writes keep the saved size up to date.
        width = width
        height = height
        // Position can be restored imperatively: moving the window does not
        // relayout its contents, so it does not hit the layout crash.
        if (windowSettings.windowWidth > 0 && windowSettings.windowHeight > 0) {
            x = windowSettings.windowX
            y = windowSettings.windowY
        }
        if (!needOnboarding && AppMode.walletEnabled && AppMode.isDesktop) {
            nodeModel.startNodeInitializionThread()
        }
    }

    Component {
        id: node
        PageStack {
            id: nodeStack
            vertical: true
            initialItem: node
            Component {
                id: node
                NodeRunner {
                    onSettingsClicked: {
                        nodeStack.push(nodeSettings)
                    }
                    onPeersClicked: {
                        peerTableModel.startAutoRefresh()
                        nodeStack.push(peersPage)
                    }
                    onConsoleClicked: {
                        nodeStack.push(consolePage)
                    }
                }
            }
            Component {
                id: nodeSettings
                 NodeSettings {
                    onDoneClicked: {
                        nodeStack.pop()
                    }
                }
            }
            Component {
                id: peersPage
                Peers {
                    onBack: {
                        nodeStack.pop()
                        peerTableModel.stopAutoRefresh()
                    }
                    onPeerSelected: (peerDetails) => {
                        nodeStack.push(peerDetailsPage, {"details": peerDetails})
                    }
                    onBannedPeers: {
                        nodeStack.push(bannedPeersPage)
                    }
                }
            }
            Component {
                id: peerDetailsPage
                PeerDetails {
                    onBack: nodeStack.pop()
                }
            }
            Component {
                id: bannedPeersPage
                BannedPeers {
                    onBack: nodeStack.pop()
                }
            }
            Component {
                id: consolePage
                CommandConsole {
                    onBack: nodeStack.pop()
                }
            }
        }
    }
}
