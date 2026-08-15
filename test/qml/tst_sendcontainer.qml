// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Window 2.15
import QtTest 1.2
import org.bitcoincore.qt 1.0
import "../../qml/pages/wallet/sendv2"

TestCase {
    id: testCase
    name: "SendContainer"
    when: windowShown
    width: 1180
    height: 780

    Component {
        id: sendContainerComponent

        SendContainer { wallet: testWalletModel }
    }

    function init() {
        testWalletModel.customFeeEnabled = false
        testWalletModel.customFeeRate = ""
        testWalletModel.targetBlocks = 2
        testWalletModel.prepareTransactionResult = true
        testWalletModel.sendAmountExhaustsBalance = false
        testRecipientsModel.allValid = true
        testRecipientsModel.validationError = ""
        testRecipientsModel.clearToFront()
        testCoinsListModel.reset()
    }

    function createContainer(width, height) {
        testCase.width = width
        testCase.height = height
        const page = createTemporaryObject(sendContainerComponent, testCase)
        verify(page !== null)
        page.width = width
        page.height = height
        page.visible = true
        wait(0)
        return page
    }

    function isDescendant(item, ancestor) {
        let current = item
        while (current) {
            if (current === ancestor) return true
            current = current.parent
        }
        return false
    }

    function test_compose_view_exposes_new_cards_and_stable_controls() {
        testRecipientsModel.allValid = false
        const page = createContainer(1180, 780)
        compare(page.objectName, "sendPage")
        const compose = findChild(page, "walletSendPage")
        verify(compose !== null)
        compare(compose.compact, false)
        compare(compose.stacked, false)

        const formCard = findChild(page, "sendContainerFormCard")
        verify(formCard !== null)
        verify(findChild(page, "sendTransactionSummaryCard") !== null)
        verify(findChild(page, "sendOptionsPopup") !== null)
        verify(findChild(page, "sendAddressField") !== null)
        verify(findChild(page, "sendAddressInput") !== null)
        verify(findChild(page, "sendAmountInput") !== null)
        verify(findChild(page, "sendNoteField") !== null)
        verify(findChild(page, "sendNoteInput") !== null)
        verify(findChild(page, "feeSelectionOption0") !== null)
        verify(findChild(page, "feeSelectionOption1") !== null)
        verify(findChild(page, "feeSelectionOption2") !== null)
        verify(findChild(page, "feeSelectionCustomRateInput") !== null)
        verify(findChild(page, "feeSelectionIncludeFeeToggle") !== null)
        verify(findChild(page, "sendTransactionSectionHeader") !== null)
        verify(findChild(page, "sendTotalAmountRow") !== null)
        verify(findChild(page, "sendTotalAmountValue") !== null)
        verify(findChild(page, "sendPrepareTransactionError") !== null)
        verify(findChild(page, "sendPrepareTransactionErrorText") !== null)

        const coinControlButton = findChild(page, "sendCoinControlButton")
        verify(coinControlButton !== null)
        verify(isDescendant(coinControlButton, formCard))
        compare(coinControlButton.iconSource.toString(), "")

        const reviewButton = findChild(page, "sendReviewButton")
        verify(reviewButton !== null)
        compare(reviewButton.text, "Review transaction")
        compare(reviewButton.enabled, false)

        testRecipientsModel.allValid = true
        tryCompare(reviewButton, "enabled", true)

        const highFee = findChild(page, "feeSelectionOption0")
        highFee.clicked()
        compare(testWalletModel.targetBlocks, 1)
    }

    function test_compact_view_stacks_cards_and_opens_new_review() {
        const page = createContainer(560, 780)
        const compose = findChild(page, "walletSendPage")
        verify(compose !== null)
        compare(compose.compact, true)
        compare(compose.stacked, true)
        verify(findChild(page, "sendContainerFormCard") !== null)
        verify(findChild(page, "sendTransactionSummaryCard") !== null)

        const callsBefore = testWalletModel.prepareTransactionCalls
        findChild(page, "sendReviewButton").clicked()
        compare(testWalletModel.prepareTransactionCalls, callsBefore + 1)
        tryCompare(page, "depth", 2)
        verify(findChild(page, "sendReviewPage") !== null)
    }

    function test_cards_stack_before_details_become_narrower_than_summary() {
        const page = createContainer(800, 780)
        const compose = findChild(page, "walletSendPage")
        verify(compose !== null)
        compare(compose.compact, false)
        compare(compose.stacked, true)
        verify(compose.horizontalDetailsWidth < compose.summaryPreferredWidth)
    }
}
