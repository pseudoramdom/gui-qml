// Copyright (c) 2024-2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../../controls"
import "../../components"

Page {
    id: root
    objectName: "sendReviewPage"
    background: null

    property WalletQmlModel wallet: walletController.selectedWallet
    property WalletQmlModelTransaction transaction: wallet ? wallet.currentTransaction : null
    property bool sending: false
    property string savePsbtStatus: ""
    property bool savePsbtError: false

    readonly property int recipientCount: wallet ? wallet.recipients.count : 0
    readonly property bool multipleRecipients: recipientCount > 1
    readonly property bool isWatchOnly: wallet && wallet.keySchemeKind === WalletQmlModel.WatchOnly
    readonly property string recipientCountText: recipientCount === 1
        ? qsTr("There is 1 recipient.")
        : qsTr("There are %1 recipients.").arg(recipientCount)

    signal finished()
    signal back()
    signal transactionSent()

    function commitSend() {
        if (root.sending) {
            return
        }
        if (root.wallet && root.wallet.sendTransaction()) {
            root.sending = true
            root.transactionSent()
        } else if (root.wallet && root.wallet.transactionNeedsUnlock) {
            sendPassphrasePopup.errorText = ""
            sendPassphrasePopup.open()
        }
    }

    function defaultSavePsbtFileUrl() {
        const stamp = Qt.formatDateTime(new Date(), "yyyy-MM-dd-HHmm")
        return "file://" + walletController.homePath() + "/transaction-" + stamp + ".psbt"
    }

    function savePsbt(path) {
        if (!root.wallet || String(path).length === 0) {
            return
        }

        const result = root.wallet.saveCurrentTransactionAsPsbt(String(path))
        root.savePsbtStatus = result.length === 0 ? qsTr("Saved.") : result
        root.savePsbtError = result.length > 0
    }

    function startSavePsbt() {
        root.savePsbtStatus = ""
        root.savePsbtError = false

        if (savePsbtAutomationPath.text.length > 0) {
            const path = savePsbtAutomationPath.text
            savePsbtAutomationPath.text = ""
            root.savePsbt(path)
            return
        }

        savePsbtDialog.selectedFile = root.defaultSavePsbtFileUrl()
        savePsbtDialog.open()
    }

    onVisibleChanged: {
        if (!visible) {
            externalSignerActions.reset()
            root.savePsbtStatus = ""
            root.savePsbtError = false
        }
    }

    header: NavigationBar2 {
        id: navbar
        leftItem: NavButton {
            objectName: "sendReviewBackButton"
            iconSource: "image://images/caret-left"
            text: root.wallet && root.wallet.hasExternalSigner ? qsTr("Edit") : qsTr("Back")
            onClicked: {
                externalSignerActions.reset()
                root.back()
            }
        }
    }

    FileDialog {
        id: savePsbtDialog
        objectName: "sendReviewSavePsbtDialog"
        title: qsTr("Save transaction as PSBT")
        fileMode: FileDialog.SaveFile
        currentFolder: "file://" + walletController.homePath()
        selectedFile: root.defaultSavePsbtFileUrl()
        defaultSuffix: "psbt"
        nameFilters: [qsTr("Partially Signed Bitcoin Transactions (*.psbt)"), qsTr("All files (*)")]
        onAccepted: root.savePsbt(savePsbtDialog.selectedFile.toString())
    }

    // Functional tests inject a destination here instead of driving a native dialog.
    TextField {
        id: savePsbtAutomationPath
        objectName: "sendReviewSavePsbtPathField"
        visible: false
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
            spacing: 10

            ColumnLayout {
                Layout.topMargin: 30
                Layout.bottomMargin: root.multipleRecipients ? 0 : 20
                Layout.fillWidth: true
                spacing: 5

                CoreText {
                    id: title
                    Layout.fillWidth: true
                    text: qsTr("Review transaction")
                    horizontalAlignment: root.multipleRecipients ? Text.AlignLeft : Text.AlignHCenter
                    font: Theme.text.subtitle.font
                    lineHeight: Theme.text.subtitle.lineHeight
                    lineHeightMode: Text.FixedHeight
                }

                CoreText {
                    objectName: "sendReviewRecipientCountText"
                    Layout.fillWidth: true
                    visible: root.multipleRecipients
                    text: root.recipientCountText
                    horizontalAlignment: Text.AlignLeft
                    font: Theme.text.caption.font
                    lineHeight: Theme.text.caption.lineHeight
                    lineHeightMode: Text.FixedHeight
                    color: Theme.color.neutral7
                }
            }

            Loader {
                id: bodyLoader
                Layout.fillWidth: true
                sourceComponent: root.multipleRecipients ? multipleBody : singleBody
            }

            Component {
                id: singleBody
                SingleRecipientSummary {
                    wallet: root.wallet
                    recipient: root.wallet ? root.wallet.recipients.current : null
                    transaction: root.transaction
                }
            }

            Component {
                id: multipleBody
                MultipleRecipientsSummary {
                    wallet: root.wallet
                    transaction: root.transaction
                }
            }

            InfoBanner {
                objectName: "sendReviewWatchOnlyBanner"
                visible: root.isWatchOnly
                Layout.fillWidth: true
                Layout.topMargin: 30
                contentMargin: 18
                contentSpacing: 10
                title: qsTr("Watch-only wallet")
                message: qsTr("This is a watch-only wallet. It does not have the keys to sign this transaction. Save it as a PSBT and sign it with another wallet.")
            }

            ExternalSignerReviewActions {
                id: externalSignerActions
                visible: root.wallet && root.wallet.hasExternalSigner
                wallet: root.wallet
                buttonObjectName: "sendReviewExternalSignerButton"
                statusObjectName: "sendReviewStatusText"
                Layout.fillWidth: true
                Layout.topMargin: 30
                onSendRequested: root.commitSend()
            }

            ContinueButton {
                id: confirmationButton
                objectName: "sendReviewSendButton"
                visible: !root.wallet || (!root.wallet.hasExternalSigner && !root.isWatchOnly)
                enabled: !root.sending
                Layout.fillWidth: true
                Layout.topMargin: 30
                text: qsTr("Send")
                onClicked: root.commitSend()
            }

            OutlineButton {
                id: savePsbtButton
                objectName: "sendReviewSavePsbtButton"
                visible: confirmationButton.visible || externalSignerActions.visible || root.isWatchOnly
                enabled: root.wallet && root.wallet.currentTransaction && !root.sending
                Layout.fillWidth: true
                Layout.topMargin: 10
                text: qsTr("Save transaction")
                onClicked: root.startSavePsbt()
            }

            CoreText {
                objectName: "sendReviewSavePsbtStatus"
                Layout.fillWidth: true
                visible: root.savePsbtStatus.length > 0
                text: root.savePsbtStatus
                color: root.savePsbtError ? Theme.color.red : Theme.color.green
                font: Theme.text.caption.font
                lineHeight: Theme.text.caption.lineHeight
                lineHeightMode: Text.FixedHeight
                wrapMode: Text.WordWrap
            }

            CoreText {
                objectName: "sendReviewErrorText"
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.wallet ? root.wallet.transactionError : ""
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
        popupObjectName: "sendReviewPassphrasePopup"
        passphraseFieldObjectName: "sendReviewPassphraseField"
        errorTextObjectName: "sendReviewPassphraseErrorText"
        cancelButtonObjectName: "sendReviewPassphraseCancelButton"
        confirmButtonObjectName: "sendReviewPassphraseConfirmButton"
        titleText: qsTr("Enter wallet password")
        descriptionText: qsTr("Enter your wallet password to send this transaction.")
        confirmText: qsTr("Unlock and send")
        busyConfirmText: qsTr("Unlocking...")
        onSubmitted: (passphrase) => {
            sendPassphrasePopup.busy = true
            if (root.wallet.sendTransactionWithPassphrase(passphrase)) {
                sendPassphrasePopup.busy = false
                sendPassphrasePopup.close()
                root.sending = true
                root.transactionSent()
                return
            }
            sendPassphrasePopup.busy = false
            sendPassphrasePopup.errorText = root.wallet.transactionError
        }
    }
}
