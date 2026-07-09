#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Regression test: restoring a saved window size must not crash at startup.

Background: the window-geometry restore originally assigned width/height
imperatively in main.qml's Component.onCompleted, i.e. *after* the scene
(including the closed Popups in DesktopWallets, which contain GridLayouts) was
already laid out. On Qt 6.4.x that post-layout resize trips a Quick Layouts
null-deref (QQuickGridLayoutBase::rearrange) and the app segfaults during
startup. The fix applies the saved size during construction (initial bindings,
later broken so the window stays resizable), so no live relayout occurs.

This test seeds a saved window size that differs from the minimum, boots
straight into an onboarded desktop wallet UI (wallet enabled, so the
DesktopWallets page loads), and asserts the QML scene loads without crashing.
Without the fix the scene crashes during load and the test bridge never
connects, failing the test.

This test requires:
  - bitcoin-core-app built with -DENABLE_TEST_AUTOMATION=ON
"""

import os
import sys

from qml_test_harness import QmlTestHarness, dump_qml_tree, parse_args


# Clearly larger than the 800x665 minimum so the restore forces a real resize,
# which is the exact condition that triggered the crash.
SAVED_WIDTH = 1200
SAVED_HEIGHT = 800


def seed_window_geometry(config_home):
    """Write a saved window geometry into the sandboxed QSettings location.

    The app is launched with -test-settings-dir=<config_home> on the regtest
    chain, so QSettings (IniFormat) live at
    <config_home>/BitcoinCore/BitcoinCore-App-regtest.ini under [General].
    """
    settings_dir = os.path.join(config_home, "BitcoinCore")
    os.makedirs(settings_dir, exist_ok=True)
    ini_path = os.path.join(settings_dir, "BitcoinCore-App-regtest.ini")
    with open(ini_path, "w", encoding="utf8") as f:
        f.write("[General]\n")
        f.write(f"windowWidth={SAVED_WIDTH}\n")
        f.write(f"windowHeight={SAVED_HEIGHT}\n")
        f.write("windowX=120\n")
        f.write("windowY=130\n")
    return ini_path


def run_tests():
    args = parse_args()

    # Keep the seeded geometry so the restore actually runs. The shared harness
    # starts managed runtime tests as onboarded, so the app boots straight into
    # DesktopWallets. Wallet stays enabled (no -disablewallet).
    harness = QmlTestHarness(socket_path=args.socket_path, reset_settings=False)
    gui = None
    try:
        if not harness.external:
            assert harness.config_home, "expected a managed config_home to seed"
            ini_path = seed_window_geometry(harness.config_home)
            print(f"Seeded saved window size {SAVED_WIDTH}x{SAVED_HEIGHT} -> {ini_path}")

        # If the geometry-restore crash regresses, the QML scene crashes during
        # load and the bridge socket never appears, so start() raises here.
        harness.start()
        gui = harness.driver

        # Reaching this point means the QML scene loaded. Confirm the root page
        # stack is present and the process is still alive.
        print("Verifying the QML scene loaded without crashing ...")
        object_names = {o["objectName"] for o in gui.list_objects()}
        assert "mainPageStack" in object_names, \
            "mainPageStack missing; the QML scene did not load"

        if harness.process is not None:
            rc = harness.process.poll()
            assert rc is None, (
                f"app exited during startup (rc={rc}); "
                "likely the window-geometry restore crash regressed"
            )

        page = gui.get_current_page()
        assert page and "onboarding" not in page.lower(), \
            f"expected to boot into the wallet UI, not onboarding (page={page!r})"
        print(f"  -> app started with restored geometry; current page: {page}  ✓")

        print("\n" + "=" * 50)
        print("All tests PASSED")
        print("=" * 50)

    except Exception as e:
        print(f"\nFAILED: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        if gui is not None:
            dump_qml_tree(gui)
        sys.exit(1)
    finally:
        harness.stop()


if __name__ == "__main__":
    run_tests()
