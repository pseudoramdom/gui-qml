// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtTest 1.2
import org.bitcoincore.qt 1.0
import "../../qml/components"

TestCase {
    id: testCase
    name: "DebugLogOutputView"
    when: windowShown
    width: 440
    height: 300

    Component {
        id: outputViewComponent

        DebugLogOutputView {
            objectName: "testDebugLogOutputView"
            width: 400
            height: 240
            listModel: testDebugLogModel
            accessibleName: "Test debug log"
        }
    }

    function init() {
        testDebugLogModel.resetForTest(0, false)
    }

    function createOutputView() {
        const view = createTemporaryObject(outputViewComponent, testCase.Window.window.contentItem)
        verify(view !== null)
        tryCompare(view, "count", testDebugLogModel.count)
        return view
    }

    function test_virtualizes_rows_and_scroll_helpers_reach_each_end() {
        testDebugLogModel.resetForTest(250, false)
        const view = createOutputView()
        const list = findChild(view, "testDebugLogOutputView_list")
        verify(list !== null)

        tryVerify(function() { return view.instantiatedDelegateCount > 0 })
        verify(view.instantiatedDelegateCount < view.count,
               "ListView should instantiate only a viewport-sized subset")
        verify(list.itemAtIndex(0) !== null)
        compare(list.itemAtIndex(200), null)
        compare(view.atTop, true)
        compare(view.atBottom, false)

        view.scrollToBottom()
        tryCompare(view, "atBottom", true)
        tryVerify(function() { return list.itemAtIndex(249) !== null })
        compare(findChild(list.itemAtIndex(249), "testDebugLogOutputView_lineNumber_249").text, "250")

        view.scrollToTop()
        tryCompare(view, "atTop", true)
        tryVerify(function() { return list.itemAtIndex(0) !== null })
    }

    function test_variable_height_prepend_and_tail_prune_keep_anchor() {
        testDebugLogModel.resetForTest(160, false)
        const wrappedMessage = "A wrapped debug-log message with selectable text. ".repeat(24)
        testDebugLogModel.setMessageForTest(10, wrappedMessage)
        testDebugLogModel.setMessageForTest(50, wrappedMessage)
        const view = createOutputView()
        const list = findChild(view, "testDebugLogOutputView_list")
        verify(list !== null)

        list.positionViewAtIndex(50, ListView.Beginning)
        tryVerify(function() {
            const item = list.itemAtIndex(50)
            return !view.atTop && item !== null && item.height > view.textLineHeight * 3
        })
        // Visiting another wrapped row first makes ListView refine its delegate
        // size estimate. Returning nearer the beginning then exercises an
        // anchor whose contentY is relative to a shifted (non-zero) origin.
        list.positionViewAtIndex(10, ListView.Beginning)
        tryVerify(function() {
            const item = list.itemAtIndex(10)
            return item !== null && item.height > view.textLineHeight * 3
        })

        const anchorMessage = testDebugLogModel.messageAt(10)
        const anchorOffset = list.itemAtIndex(10).y - view.contentY
        testDebugLogModel.prependAndPruneRowsForTest(3, 3)

        tryCompare(view, "count", 160)
        compare(testDebugLogModel.messageAt(13), anchorMessage)
        tryVerify(function() {
            const shiftedAnchor = list.itemAtIndex(13)
            return shiftedAnchor !== null
                && Math.abs((shiftedAnchor.y - view.contentY) - anchorOffset) < 0.5
        })
        compare(view.atTop, false)

        const shiftedMessage = findChild(
            list.itemAtIndex(13), "testDebugLogOutputView_message_13")
        verify(shiftedMessage !== null)
        compare(shiftedMessage.visible, true)
        shiftedMessage.selectAll()
        compare(shiftedMessage.selectedText, wrappedMessage)

        // Exercise the end calculation after a variable-height incremental
        // update, when ListView's logical origin is allowed to be non-zero.
        view.scrollToBottom()
        tryCompare(view, "atBottom", true)
    }

    function test_prepend_while_at_top_keeps_newest_rows_visible() {
        testDebugLogModel.resetForTest(80, false)
        const view = createOutputView()
        const list = findChild(view, "testDebugLogOutputView_list")
        verify(list !== null)
        compare(view.atTop, true)

        testDebugLogModel.prependRowsForTest(2)

        tryCompare(view, "count", 82)
        tryCompare(view, "atTop", true)
        tryVerify(function() { return list.itemAtIndex(0) !== null })
        compare(testDebugLogModel.messageAt(0), "new-0")
        compare(findChild(list.itemAtIndex(0), "testDebugLogOutputView_lineNumber_0").text, "1")
    }

    function test_appending_older_rows_does_not_jump_to_new_bottom() {
        testDebugLogModel.resetForTest(100, true)
        const view = createOutputView()
        view.scrollToBottom()
        tryCompare(view, "atBottom", true)
        const anchoredContentY = view.contentY

        testDebugLogModel.appendRowsForTest(20)

        tryCompare(view, "count", 120)
        tryVerify(function() { return !view.atBottom })
        verify(Math.abs(view.contentY - anchoredContentY) < 0.5,
               "Appending older rows should preserve the current viewport")
    }

    function test_full_snapshot_prefix_and_suffix_keep_prepend_anchor_data() {
        return [
            { tag: "prefix-first", prependFirst: true },
            { tag: "suffix-first", prependFirst: false },
        ]
    }

    function test_full_snapshot_prefix_and_suffix_keep_prepend_anchor(data) {
        testDebugLogModel.resetForTest(160, false)
        const view = createOutputView()
        const list = findChild(view, "testDebugLogOutputView_list")
        verify(list !== null)

        list.positionViewAtIndex(50, ListView.Beginning)
        tryVerify(function() { return list.itemAtIndex(50) !== null && !view.atTop })
        const anchorMessage = testDebugLogModel.messageAt(50)
        const anchorOffset = list.itemAtIndex(50).y - view.contentY

        // A reconciled snapshot can expose both ends in one GUI event turn.
        // The viewport must follow the shifted original row regardless of the
        // order in which those two insertion batches are published.
        testDebugLogModel.prependAndAppendRowsForTest(3, 2, data.prependFirst)

        tryCompare(view, "count", 165)
        compare(testDebugLogModel.messageAt(53), anchorMessage)
        tryVerify(function() {
            const shiftedAnchor = list.itemAtIndex(53)
            return shiftedAnchor !== null
                && Math.abs((shiftedAnchor.y - view.contentY) - anchorOffset) < 0.5
        })
    }
}
