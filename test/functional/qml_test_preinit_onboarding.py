#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Exercise the real pre-init onboarding flow.

This test intentionally launches bitcoin-core-app without -datadir so
RunPreInitOnboarding() is observable through the pre-init test bridge.
"""

import sys
import shutil
import time

from qml_test_harness import (
    QmlTestHarness,
    assert_node_shell_visible,
    assert_onboarding_wallet_creation_visible,
    assert_wallet_shell_visible,
    complete_preinit_onboarding,
    dump_qml_tree,
    parse_args,
)


def assert_create_wallet_onboarding_shown(gui, timeout_ms=30000):
    """Assert onboarding immediately shows wallet creation instead of an intermediate shell."""
    deadline = time.time() + timeout_ms / 1000
    while time.time() < deadline:
        object_names = {obj.get("objectName") for obj in gui.list_objects()}
        if "createWalletWizard" in object_names:
            break
        if "nodeRunner" in object_names:
            raise AssertionError("Expected wallet onboarding, but nodeRunner is present")
        time.sleep(0.05)
    else:
        raise AssertionError("Timed out waiting for create-wallet onboarding")

    gui.wait_for_object("desktopWalletsPage", timeout_ms=timeout_ms)
    assert not gui.object_exists("createWalletWizardBackButton"), "Create-wallet Back must not be shown during onboarding"
    if not gui.get_property("createWalletButton", "enabled"):
        gui.wait_for_property("createWalletDiscoveryBusyIndicator", "visible", True, timeout_ms=timeout_ms)
        gui.wait_for_property("importWalletButton", "enabled", False, timeout_ms=timeout_ms)


def assert_preinit_cover_about_available(gui):
    gui.wait_for_object("onboardingCoverInfoButton", timeout_ms=10000)
    gui.click("onboardingCoverInfoButton")
    gui.wait_for_page("settingsAbout", timeout_ms=10000)
    gui.click("settingsAboutBack")
    gui.wait_for_page("onboardingCover", timeout_ms=10000)


def finish_preinit_and_reconnect(harness):
    gui = harness.driver
    gui.wait_for_page("onboardingCover", timeout_ms=10000)
    assert_preinit_cover_about_available(gui)
    complete_preinit_onboarding(gui)
    return harness.wait_for_main_window_reconnect()


def run_wallet_enabled_flow():
    tmpdir = None
    harness = QmlTestHarness(
        use_datadir_arg=False,
        reset_settings=True,
        extra_args=["-regtest"],
    )
    gui = None
    try:
        harness.start()
        gui = finish_preinit_and_reconnect(harness)
        assert_create_wallet_onboarding_shown(gui)
        assert_onboarding_wallet_creation_visible(gui)
        assert not gui.object_exists("createWalletWizardBackButton"), "Create-wallet Back must not be shown during onboarding"
        gui.click("createWalletWizardExitButton")
        assert_wallet_shell_visible(gui)
        assert not gui.object_exists("onboardingStorageLocation"), "Create-wallet Skip must not return to datadir onboarding"

        tmpdir = harness.tmpdir
        harness.stop(cleanup=False)

        restart = QmlTestHarness(
            use_datadir_arg=False,
            reset_settings=False,
            tmpdir=tmpdir,
            extra_args=["-regtest"],
        )
        try:
            restart.start()
            gui = restart.driver
            assert_wallet_shell_visible(gui)
            assert not gui.object_exists("createWalletWizard"), "Did not expect create-wallet onboarding on normal restart"
        finally:
            restart.stop(cleanup=False)
    except Exception:
        if gui is not None:
            dump_qml_tree(gui)
        raise
    finally:
        harness.stop(cleanup=False)
        if tmpdir:
            shutil.rmtree(tmpdir, ignore_errors=True)


def run_wallet_disabled_flow():
    harness = QmlTestHarness(
        use_datadir_arg=False,
        reset_settings=True,
        extra_args=["-regtest", "-disablewallet"],
    )
    gui = None
    try:
        harness.start()
        gui = finish_preinit_and_reconnect(harness)
        assert_node_shell_visible(gui)
    except Exception:
        if gui is not None:
            dump_qml_tree(gui)
        raise
    finally:
        harness.stop()


def run_tests():
    args = parse_args()
    if args.socket_path:
        raise RuntimeError("qml_test_preinit_onboarding.py must launch the app itself")

    run_wallet_enabled_flow()
    run_wallet_disabled_flow()

    print("\n" + "=" * 50)
    print("All tests PASSED")
    print("=" * 50)


if __name__ == "__main__":
    try:
        run_tests()
    except Exception as e:
        print(f"\nFAILED: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)
