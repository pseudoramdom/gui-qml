// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../../controls"
import "../../components"

Page {
    id: root
    background: null

    property WalletQmlModel wallet: walletController.selectedWallet
    property WalletQmlModelTransaction transaction: walletController.selectedWallet.currentTransaction

    signal finished()
    signal back()
    signal transactionSent()

    header: NavigationBar2 {
        id: navbar
        leftItem: NavButton {
            iconSource: "image://images/caret-left"
            text: qsTr("Back")
            onClicked: {
                root.back()
            }
        }
    }

    ScrollView {
        clip: true
        width: parent.width
        height: parent.height
        contentWidth: width

        ColumnLayout {
            id: columnLayout
            width: 450
            anchors.horizontalCenter: parent.horizontalCenter

            spacing: 20

            CoreText {
                id: title
                Layout.topMargin: 30
                Layout.bottomMargin: 20
                text: qsTr("Transaction details")
                font.pixelSize: 21
                bold: true
            }

            RowLayout {
                CoreText {
                    text: qsTr("Send to")
                    font.pixelSize: 15
                    Layout.preferredWidth: 110
                    color: Theme.color.neutral7
                }
                CoreText {
                    text: root.transaction.address
                    font.pixelSize: 15
                    color: Theme.color.neutral9
                }
            }

            RowLayout {
                CoreText {
                    text: qsTr("Note")
                    font.pixelSize: 15
                    Layout.preferredWidth: 110
                    color: Theme.color.neutral7
                }
                CoreText {
                    text: root.transaction.label
                    font.pixelSize: 15
                    color: Theme.color.neutral9
                }
            }

            RowLayout {
                CoreText {
                    text: qsTr("Amount")
                    font.pixelSize: 15
                    Layout.preferredWidth: 110
                    color: Theme.color.neutral7
                }
                CoreText {
                    text: root.transaction.amount
                    font.pixelSize: 15
                    color: Theme.color.neutral9
                }
            }

            RowLayout {
                CoreText {
                    text: qsTr("Fee")
                    font.pixelSize: 15
                    Layout.preferredWidth: 110
                    color: Theme.color.neutral7
                }
                CoreText {
                    text: root.transaction.fee
                    font.pixelSize: 15
                    color: Theme.color.neutral9
                }
            }

            RowLayout {
                CoreText {
                    text: qsTr("Total")
                    font.pixelSize: 15
                    Layout.preferredWidth: 110
                    color: Theme.color.neutral7
                }
                CoreText {
                    text: root.transaction.total
                    font.pixelSize: 15
                    color: Theme.color.neutral9
                }
            }

            ContinueButton {
                id: confimationButton
                Layout.fillWidth: true
                Layout.topMargin: 30
                text: qsTr("Send")
                onClicked: {
                    if (root.wallet.isEncrypted && root.wallet.isLocked) {
                        sendPassphrasePopup.errorText = ""
                        sendPassphrasePopup.open()
                        return
                    }
                    if (root.wallet.sendTransaction()) {
                        root.transactionSent()
                    }
                }
            }

            CoreText {
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.wallet.transactionError
                color: Theme.color.red
                font.pixelSize: 15
                wrapMode: Text.WordWrap
            }
        }
    }

    WalletPassphrasePopup {
        id: sendPassphrasePopup
        parent: Overlay.overlay
        width: Math.min(420, root.width - 40)
        titleText: qsTr("Enter wallet password")
        descriptionText: qsTr("Enter your wallet password to sign and send this transaction.")
        confirmText: qsTr("Sign and send")
        busyConfirmText: qsTr("Signing...")
        onSubmitted: (passphrase) => {
            sendPassphrasePopup.busy = true
            if (root.wallet.sendTransactionWithPassphrase(passphrase)) {
                sendPassphrasePopup.busy = false
                sendPassphrasePopup.close()
                root.transactionSent()
                return
            }
            sendPassphrasePopup.busy = false
            sendPassphrasePopup.errorText = root.wallet.transactionError
        }
    }
}
