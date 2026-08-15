// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../../../controls"
import "../../../components"

Page {
    id: root
    objectName: "sendReviewPage"

    property var wallet: null
    property bool inspectionMode: false
    property bool sending: false
    property string saveStatus: ""
    property bool saveError: false

    readonly property bool multipleRecipients: wallet && wallet.recipients.count > 1
    readonly property bool watchOnly: wallet && wallet.keySchemeKind === WalletQmlModel.WatchOnly
    readonly property bool canSend: wallet ? wallet.currentTransactionCanSend : false
    readonly property bool canBroadcast: wallet ? wallet.currentTransactionCanBroadcast : false

    signal back()
    signal transactionSent()

    background: Rectangle { color: Theme.color.background }

    header: NavigationBar2 {
        leftItem: NavButton {
            objectName: "sendReviewBackButton"
            iconSource: "image://images/caret-left"
            text: root.inspectionMode ? qsTr("Done") : qsTr("Edit")
            onClicked: root.back()
        }
    }

    function commitSend() {
        if (!root.wallet || root.sending) return
        if (root.wallet.sendTransaction()) {
            root.sending = true
            root.transactionSent()
        } else if (root.wallet.transactionNeedsUnlock) {
            sendPassphrasePopup.errorText = ""
            sendPassphrasePopup.open()
        }
    }

    function commitBroadcast() {
        if (!root.wallet || root.sending) return
        if (root.wallet.broadcastCurrentTransaction()) {
            root.sending = true
            root.transactionSent()
        }
    }

    function defaultSaveUrl() {
        const stamp = Qt.formatDateTime(new Date(), "yyyy-MM-dd-HHmm")
        return "file://" + walletController.homePath() + "/transaction-" + stamp + ".psbt"
    }

    FileDialog {
        id: saveDialog
        objectName: "sendReviewSavePsbtDialog"
        title: qsTr("Save transaction as PSBT")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "psbt"
        nameFilters: [qsTr("Partially Signed Bitcoin Transactions (*.psbt)"), qsTr("All files (*)")]
        onAccepted: {
            const result = root.wallet.saveCurrentTransactionAsPsbt(selectedFile.toString())
            root.saveStatus = result.length === 0 ? qsTr("Saved.") : result
            root.saveError = result.length > 0
        }
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: width

        ColumnLayout {
            width: Math.min(620, parent.width - 32)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 18

            CoreText {
                Layout.fillWidth: true
                Layout.topMargin: 28
                text: qsTr("Review transaction")
                color: Theme.color.neutral9
                font: Theme.text.subtitle.font
                horizontalAlignment: Text.AlignHCenter
            }

            InfoBanner {
                objectName: "sendReviewCannotSignBanner"
                Layout.fillWidth: true
                visible: root.watchOnly || (root.wallet && root.wallet.currentTransactionReviewMessage.length > 0)
                title: root.watchOnly ? qsTr("Watch-only wallet") : ""
                message: root.watchOnly
                    ? qsTr("This wallet does not have the keys required to sign the transaction.")
                    : root.wallet.currentTransactionReviewMessage
                iconSource: "image://images/info-filled"
            }

            CardSurface {
                Layout.fillWidth: true
                implicitHeight: reviewContent.implicitHeight + 48

                contentItem: ColumnLayout {
                    id: reviewContent
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Loader {
                        Layout.fillWidth: true
                        sourceComponent: root.multipleRecipients ? multipleSummary : singleSummary
                    }

                    Component {
                        id: singleSummary
                        SingleRecipientSummary {
                            wallet: root.wallet
                            recipient: root.wallet ? root.wallet.recipients.current : null
                            transaction: root.wallet ? root.wallet.currentTransaction : null
                        }
                    }

                    Component {
                        id: multipleSummary
                        MultipleRecipientsSummary {
                            wallet: root.wallet
                            transaction: root.wallet ? root.wallet.currentTransaction : null
                        }
                    }
                }
            }

            ExternalSignerReviewActions {
                id: externalSignerActions
                Layout.fillWidth: true
                visible: !root.inspectionMode && root.wallet && root.wallet.hasExternalSigner
                wallet: root.wallet
                canSend: root.canSend
                buttonObjectName: "sendReviewExternalSignerButton"
                statusObjectName: "sendReviewStatusText"
                onSendRequested: root.commitSend()
            }

            ContinueButton {
                objectName: "sendReviewSendButton"
                Layout.fillWidth: true
                visible: !root.inspectionMode && !root.watchOnly
                    && (!root.wallet || !root.wallet.hasExternalSigner)
                enabled: root.canSend && !root.sending
                text: qsTr("Send")
                onClicked: root.commitSend()
            }

            ContinueButton {
                objectName: "sendReviewBroadcastButton"
                Layout.fillWidth: true
                visible: root.inspectionMode && root.canBroadcast
                enabled: !root.sending
                text: qsTr("Broadcast transaction")
                onClicked: root.commitBroadcast()
            }

            OutlineButton {
                objectName: "sendReviewSavePsbtButton"
                Layout.fillWidth: true
                enabled: root.wallet && root.wallet.currentTransaction && !root.sending
                text: qsTr("Save transaction")
                onClicked: {
                    root.saveStatus = ""
                    root.saveError = false
                    saveDialog.selectedFile = root.defaultSaveUrl()
                    saveDialog.open()
                }
            }

            ToastBanner {
                objectName: "sendReviewSavePsbtBanner"
                Layout.fillWidth: true
                Layout.bottomMargin: 24
                visible: root.saveStatus.length > 0
                backgroundColor: root.saveError ? Theme.color.red : Theme.color.green
                iconSource: root.saveError ? "image://images/alert-filled" : "image://images/check"
                text: root.saveStatus
                dismissAfter: 3
                onDismissed: root.saveStatus = ""
            }

            CoreText {
                objectName: "sendReviewErrorText"
                Layout.fillWidth: true
                Layout.bottomMargin: 24
                visible: text.length > 0
                text: root.wallet ? root.wallet.transactionError : ""
                color: Theme.color.red
                font: Theme.text.description.font
                horizontalAlignment: Text.AlignLeft
                wrap: true
            }
        }
    }

    WalletPassphrasePopup {
        id: sendPassphrasePopup
        parent: Overlay.overlay
        width: Math.min(420, Overlay.overlay.width - 40)
        popupObjectName: "sendReviewPassphrasePopup"
        passphraseFieldObjectName: "sendReviewPassphraseField"
        errorTextObjectName: "sendReviewPassphraseErrorText"
        cancelButtonObjectName: "sendReviewPassphraseCancelButton"
        confirmButtonObjectName: "sendReviewPassphraseConfirmButton"
        titleText: qsTr("Enter wallet password")
        descriptionText: qsTr("Enter your wallet password to send this transaction.")
        confirmText: qsTr("Unlock and send")
        busyConfirmText: qsTr("Unlocking…")
        onSubmitted: function(passphrase) {
            sendPassphrasePopup.busy = true
            if (root.wallet.sendTransactionWithPassphrase(passphrase)) {
                sendPassphrasePopup.busy = false
                sendPassphrasePopup.close()
                root.sending = true
                root.transactionSent()
            } else {
                sendPassphrasePopup.busy = false
                sendPassphrasePopup.errorText = root.wallet.transactionError
            }
        }
    }
}
