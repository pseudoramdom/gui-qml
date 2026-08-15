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
import ".."

PageStack {
    id: root
    objectName: "sendPage"
    vertical: true

    property WalletQmlModel wallet: walletController.selectedWallet
    property string prepareError: ""
    property string paymentRequestStatus: ""
    property string paymentRequestMessage: ""
    property bool paymentRequestError: false
    property bool showClipboardBanner: false
    property string pendingClipboardUri: ""
    property string filledClipboardUri: ""
    property string dismissedClipboardUri: ""
    property bool applyingUri: false

    readonly property bool selectedInputsActive: wallet
        && wallet.coinsListModel
        && wallet.coinsListModel.selectedCoinsCount > 0
    readonly property string formError: {
        if (!wallet) return ""
        if (wallet.recipients.validationError.length > 0) return wallet.recipients.validationError
        if (wallet.sendAmountExhaustsBalance) {
            return selectedInputsActive
                ? qsTr("Selected inputs do not cover the amount plus fee")
                : qsTr("Amount plus fee exceeds available balance")
        }
        return prepareError
    }

    signal exitRequested()
    signal viewTransactionRequested(string txid)

    function clearPrepareError() {
        prepareError = ""
    }

    function checkClipboard() {
        if (!root.visible || !root.wallet) {
            root.showClipboardBanner = false
            return
        }

        const text = Clipboard.text()
        const parsed = BitcoinUri.parseBitcoinUri(text)
        if (!parsed.success || text === root.dismissedClipboardUri) {
            root.showClipboardBanner = false
            root.pendingClipboardUri = ""
            return
        }

        if (text === root.filledClipboardUri) {
            const recipient = root.wallet.recipients.current
            const matches = recipient.address.address === parsed.address
                && (!parsed.hasAmount || recipient.amount.satoshi === parsed.amountSats)
                && (!parsed.hasLabel || recipient.label === parsed.label)
            if (matches) {
                root.showClipboardBanner = false
                return
            }
            root.filledClipboardUri = ""
        }

        root.pendingClipboardUri = text
        root.showClipboardBanner = true
    }

    function applyParsedPaymentRequest(result, source) {
        if (!result.success) {
            root.paymentRequestStatus = result.error
            root.paymentRequestError = true
            return false
        }

        const recipient = root.wallet.recipients.current
        root.applyingUri = true
        recipient.address.setAddress(result.address, 0)
        if (result.hasAmount) recipient.amount.satoshi = result.amountSats
        if (result.hasLabel) recipient.label = result.label
        root.applyingUri = false
        root.paymentRequestStatus = qsTr("Payment request imported from %1").arg(source)
        root.paymentRequestMessage = result.hasMessage ? result.uriMessage : ""
        root.paymentRequestError = false
        root.wallet.scheduleFeeEstimates()
        return true
    }

    function applyPaymentRequestText(text, source) {
        const result = BitcoinUri.parseBitcoinUri(text)
        if (root.applyParsedPaymentRequest(result, source) && Clipboard.text() === text) {
            root.filledClipboardUri = text
            root.showClipboardBanner = false
        }
    }

    function applyPaymentRequestFile(path) {
        root.applyParsedPaymentRequest(BitcoinUri.parseBitcoinUriFromFile(path), qsTr("file"))
    }

    function handleDrop(drop) {
        if (drop.hasUrls && drop.urls.length > 0) {
            const url = drop.urls[0].toString()
            if (url.startsWith("file://")) root.applyPaymentRequestFile(url)
            else root.applyPaymentRequestText(url, qsTr("drag and drop"))
        } else if (drop.hasText) {
            root.applyPaymentRequestText(drop.text, qsTr("drag and drop"))
        }
    }

    function prepareForReview() {
        root.clearPrepareError()
        if (!root.wallet) return
        if (root.wallet.prepareTransaction()) {
            root.push(reviewPage)
        } else if (root.wallet.transactionNeedsUnlock) {
            preparePassphrasePopup.errorText = ""
            preparePassphrasePopup.open()
        } else {
            root.prepareError = root.wallet.transactionError.length > 0
                ? root.wallet.transactionError
                : root.formError
        }
    }

    function handlePsbtResult(result) {
        if (result === WalletQmlModel.WalletCanSign) {
            root.push(reviewPage)
        } else if (result === WalletQmlModel.WalletCannotSign) {
            root.push(reviewPage, {"inspectionMode": true})
        } else if (result === WalletQmlModel.TransactionAlreadyKnown) {
            knownPsbtPopup.txid = root.wallet.importedPsbt.matchedTxid
            knownPsbtPopup.open()
        } else {
            unsupportedPsbtPopup.message = root.wallet.importedPsbt.error.length > 0
                ? root.wallet.importedPsbt.error
                : qsTr("This PSBT is not supported yet.")
            unsupportedPsbtPopup.open()
        }
    }

    function finishTransaction() {
        if (root.wallet) root.wallet.recipients.clear()
        root.push(resultPage)
    }

    initialItem: Page {
        id: composePage
        background: null

        header: NavigationBar2 {
            leftItem: NavButton {
                objectName: "sendContainerBackButton"
                iconSource: "image://images/caret-left"
                text: qsTr("Back")
                onClicked: root.exitRequested()
            }
            centerItem: Header {
                header: qsTr("Send bitcoin")
                headerBold: true
                headerSize: 18
            }
        }

        SendComposeView {
            anchors.fill: parent
            wallet: root.wallet
            errorText: root.formError
            showClipboardBanner: root.showClipboardBanner
            paymentRequestStatus: root.paymentRequestStatus
            paymentRequestMessage: root.paymentRequestMessage
            paymentRequestError: root.paymentRequestError
            onReviewRequested: root.prepareForReview()
            onCoinControlRequested: {
                root.wallet.coinsListModel.update()
                root.push(coinSelectionPage)
            }
            onOpenPaymentRequestRequested: {
                uriInput.text = ""
                uriPopup.open()
            }
            onImportPsbtRequested: psbtOpenDialog.open()
            onFillClipboardRequested: {
                root.applyPaymentRequestText(root.pendingClipboardUri, qsTr("clipboard"))
                root.filledClipboardUri = root.pendingClipboardUri
                root.showClipboardBanner = false
            }
            onDismissClipboardRequested: {
                root.dismissedClipboardUri = root.pendingClipboardUri
                root.showClipboardBanner = false
            }
            onDropped: function(drop) { root.handleDrop(drop) }
        }
    }

    Component {
        id: coinSelectionPage
        CoinSelection { wallet: root.wallet; onDone: root.pop() }
    }

    Component {
        id: reviewPage
        SendReviewView {
            wallet: root.wallet
            onBack: {
                if (inspectionMode && root.wallet) root.wallet.discardCurrentTransaction()
                root.pop()
            }
            onTransactionSent: root.finishTransaction()
        }
    }

    Component {
        id: resultPage
        SendResult {
            onDone: root.exitRequested()
            onViewNewTransaction: root.viewTransactionRequested("")
        }
    }

    Connections {
        target: Clipboard
        function onDataChanged() { root.checkClipboard() }
    }

    Connections {
        target: root.wallet ? root.wallet.recipients : null
        function onCurrentRecipientChanged() {
            root.clearPrepareError()
            root.paymentRequestStatus = ""
            root.paymentRequestMessage = ""
            root.paymentRequestError = false
        }
        function onCountChanged() { root.clearPrepareError() }
        function onListCleared() { root.clearPrepareError() }
    }

    Connections {
        target: walletController
        function onSelectedWalletChanged() {
            root.pop(null)
            root.exitRequested()
        }
    }

    Component.onCompleted: root.checkClipboard()

    Popup {
        id: uriPopup
        objectName: "sendUriImportPopup"
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        width: Math.min(440, Overlay.overlay.width - 40)
        modal: true
        focus: true
        padding: 24
        background: Rectangle {
            color: Theme.color.neutral0
            border.color: Theme.color.neutral2
            radius: 14
        }
        contentItem: ColumnLayout {
            spacing: 16
            CoreText {
                Layout.fillWidth: true
                text: qsTr("Open payment request")
                font: Theme.text.heading.font
                horizontalAlignment: Text.AlignLeft
            }
            CoreTextField {
                id: uriInput
                objectName: "sendUriImportInput"
                Layout.fillWidth: true
                placeholderText: qsTr("bitcoin:address?amount=…")
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                OutlineButton {
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    onClicked: uriPopup.close()
                }
                ContinueButton {
                    objectName: "sendUriImportApplyButton"
                    Layout.fillWidth: true
                    text: qsTr("Apply")
                    enabled: uriInput.text.trim().length > 0
                    onClicked: {
                        uriPopup.close()
                        root.applyPaymentRequestText(uriInput.text, qsTr("manual entry"))
                    }
                }
            }
        }
    }

    FileDialog {
        id: psbtOpenDialog
        title: qsTr("Import PSBT")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Partially Signed Bitcoin Transactions (*.psbt)"), qsTr("All files (*)")]
        onAccepted: root.handlePsbtResult(root.wallet.importPsbtFromFile(selectedFile.toString()))
    }

    AlertPopup {
        id: unsupportedPsbtPopup
        objectName: "unsupportedPsbtPopup"
        title: qsTr("Not supported")
        onClosed: if (root.wallet) root.wallet.importedPsbt.clear()
        AlertAction { text: qsTr("OK") }
    }

    AlertPopup {
        id: knownPsbtPopup
        objectName: "knownPsbtTxPopup"
        property string txid: ""
        title: qsTr("Cannot import transaction")
        message: qsTr("This transaction is already in your wallet activity.")
        onClosed: if (root.wallet) root.wallet.importedPsbt.clear()
        AlertAction {
            text: qsTr("View transaction")
            onTriggered: root.viewTransactionRequested(knownPsbtPopup.txid)
        }
        AlertAction { text: qsTr("Close"); role: AlertAction.Cancel }
    }

    WalletPassphrasePopup {
        id: preparePassphrasePopup
        parent: Overlay.overlay
        width: Math.min(420, Overlay.overlay.width - 40)
        popupObjectName: "reviewPassphrasePopup"
        passphraseFieldObjectName: "reviewPassphraseField"
        errorTextObjectName: "reviewPassphraseErrorText"
        cancelButtonObjectName: "reviewPassphraseCancelButton"
        confirmButtonObjectName: "reviewPassphraseConfirmButton"
        titleText: qsTr("Enter wallet password")
        descriptionText: qsTr("Enter your wallet password to prepare this transaction for review.")
        confirmText: qsTr("Unlock and continue")
        busyConfirmText: qsTr("Unlocking…")
        onSubmitted: function(passphrase) {
            preparePassphrasePopup.busy = true
            if (root.wallet.prepareTransactionWithPassphrase(passphrase)) {
                preparePassphrasePopup.busy = false
                preparePassphrasePopup.close()
                root.push(reviewPage)
            } else {
                preparePassphrasePopup.busy = false
                preparePassphrasePopup.errorText = root.wallet.transactionError
            }
        }
    }
}
