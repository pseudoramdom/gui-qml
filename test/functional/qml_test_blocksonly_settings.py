#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Smoke test for blocksonly node settings."""

import sys

from qml_test_harness import QmlTestHarness, complete_onboarding, dump_qml_tree, parse_args


def run_tests():
    args = parse_args()
    harness = QmlTestHarness(
        socket_path=args.socket_path,
        extra_args=[] if args.socket_path else ["-disablewallet", "-blocksonly"],
    )
    gui = None
    try:
        harness.start()
        gui = harness.driver

        complete_onboarding(gui)
        gui.wait_for_page("nodeSettingsButton", timeout_ms=30000)
        gui.click("nodeSettingsButton")
        # Node settings now uses a sidebar layout that lands on the About
        # section; each sidebar row has objectName "settings_<section>".
        gui.wait_for_page("settings_about", timeout_ms=10000)

        # The Mempool Information sidebar row is gated on
        # nodeModel.mempoolInformationAvailable, which is false in -blocksonly
        # mode, so the row must be hidden.
        mempool_visible = gui.get_property("settings_mempool", "visible")
        assert mempool_visible is False, (
            "Mempool Information settings row should be hidden in -blocksonly mode, "
            f"got {mempool_visible!r}"
        )

        print("Blocksonly settings smoke test PASSED")
    except Exception as err:
        print(f"\nFAILED: {err}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        if gui is not None:
            dump_qml_tree(gui)
        sys.exit(1)
    finally:
        harness.stop()


if __name__ == "__main__":
    run_tests()
