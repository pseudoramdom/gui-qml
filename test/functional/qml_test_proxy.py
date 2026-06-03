#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Functional test for the runtime proxy settings UI.

Tests navigation to the proxy settings page from the runtime settings shell,
toggling the default proxy and Tor proxy enable switches, entering valid and
invalid addresses, and verifying that settings persist to disk immediately.

This test requires the binary to be built with -DENABLE_TEST_AUTOMATION=ON.
"""

import json
import os
import sys
import time

from qml_driver import QmlDriverError
from qml_test_harness import (
    QmlTestHarness,
    complete_preinit_onboarding,
    dump_qml_tree,
    parse_args,
)


def navigate_to_proxy_settings(gui):
    """Navigate from the runtime settings shell to the Proxy Settings page."""
    if gui.object_exists("desktopWalletSettingsTabButton"):
        gui.wait_for_property("desktopWalletSettingsTabButton", "visible", True, timeout_ms=10000)
        gui.click("desktopWalletSettingsTabButton")
    else:
        gui.wait_for_page("nodeSettingsButton", timeout_ms=10000)
        gui.click("nodeSettingsButton")

    gui.settle()
    gui.wait_for_property("settings_connection", "visible", True, timeout_ms=5000)
    gui.click("settings_connection")
    gui.wait_for_page("gotoProxy", timeout_ms=5000)
    gui.click("gotoProxy")
    gui.wait_for_page("settingsProxy", timeout_ms=5000)
    print("  Navigated to Proxy Settings page.")


def navigate_back_from_proxy_settings(gui):
    """Navigate back from Proxy Settings to the runtime settings shell."""
    gui.click("settingsProxyBack")
    gui.wait_for_page("gotoProxy", timeout_ms=5000)
    gui.click("settingsConnectionBack")
    gui.settle()
    if gui.object_exists("desktopWalletSettingsTabButton"):
        gui.wait_for_property("desktopWalletSettingsTabButton", "visible", True, timeout_ms=5000)
    else:
        gui.wait_for_page("nodeSettingsButton", timeout_ms=5000)
    print("  Navigated back to runtime settings.")


def test_default_proxy_toggle(gui):
    print("\n── test_default_proxy_toggle ──────────────────────────────────────")

    # Proxy should be disabled by default (fresh datadir, no prior config).
    checked = gui.get_property("proxyEnableSwitch", "checked")
    assert not checked, f"Expected proxy disabled by default, got checked={checked}"

    dirty = gui.get_property("settingsProxy", "proxySettingsDirty")
    assert not dirty, "Expected proxySettingsDirty=False before any change"

    # Enable proxy.
    gui.click("proxyEnableSwitch")
    gui.wait_for_property("proxyEnableSwitch", "checked", True, timeout_ms=2000)
    checked = gui.get_property("proxyEnableSwitch", "checked")
    assert checked, "Expected proxyEnableSwitch to be checked after click"
    print("  Default proxy toggled ON: OK")

    gui.wait_for_property("settingsProxy", "proxySettingsDirty", True, timeout_ms=2000)
    print("  proxySettingsDirty=True after runtime proxy change: OK")

    # Disable proxy.
    gui.click("proxyEnableSwitch")
    gui.wait_for_property("proxyEnableSwitch", "checked", False, timeout_ms=2000)
    checked = gui.get_property("proxyEnableSwitch", "checked")
    assert not checked, "Expected proxyEnableSwitch to be unchecked after second click"
    print("  Default proxy toggled OFF: OK")

    gui.wait_for_property("settingsProxy", "proxySettingsDirty", False, timeout_ms=2000)
    print("  proxySettingsDirty=False after reverting proxy change: OK")


def test_proxy_valid_address(gui):
    print("\n── test_proxy_valid_address ────────────────────────────────────────")

    # Enable proxy so the address field becomes active.
    if not gui.get_property("proxyEnableSwitch", "checked"):
        gui.click("proxyEnableSwitch")
        gui.wait_for_property("proxyEnableSwitch", "checked", True, timeout_ms=2000)

    gui.wait_for_property("proxyAddressInput", "enabled", True, timeout_ms=2000)
    gui.set_text("proxyAddressInput", "")
    gui.wait_for_property("proxyAddressInput", "text", "", timeout_ms=2000)
    gui.click("proxyAddressInput")
    gui.type_text("proxyAddressInput", "10.0.0.1:9050")
    gui.wait_for_property("proxyAddressInput", "text", "10.0.0.1:9050", timeout_ms=2000)
    gui.wait_for_property("proxyAddressInput", "validInput", True, timeout_ms=2000)

    valid = gui.get_property("proxyAddressInput", "validInput")
    assert valid, f"Expected '10.0.0.1:9050' to pass validation, got validInput={valid}"
    print("  Valid address accepted: OK")


def test_proxy_invalid_address(gui):
    print("\n── test_proxy_invalid_address ──────────────────────────────────────")

    # Enable proxy so the address field becomes active.
    if not gui.get_property("proxyEnableSwitch", "checked"):
        gui.click("proxyEnableSwitch")
        gui.wait_for_property("proxyEnableSwitch", "checked", True, timeout_ms=2000)

    gui.wait_for_property("proxyAddressInput", "enabled", True, timeout_ms=2000)
    # Enter an address with invalid IP octets.
    gui.set_text("proxyAddressInput", "999.999.999.999:9050")
    gui.wait_for_property("proxyAddressInput", "validInput", False, timeout_ms=2000)

    valid = gui.get_property("proxyAddressInput", "validInput")
    assert not valid, f"Expected invalid address to fail validation, got validInput={valid}"
    print("  Invalid address rejected: OK")

    # Restore to a valid address for subsequent tests.
    gui.set_text("proxyAddressInput", "127.0.0.1:9050")
    gui.wait_for_property("proxyAddressInput", "validInput", True, timeout_ms=2000)


def test_tor_proxy_toggle(gui):
    print("\n── test_tor_proxy_toggle ───────────────────────────────────────────")

    # Tor proxy should be disabled by default.
    checked = gui.get_property("torEnableSwitch", "checked")
    assert not checked, f"Expected Tor proxy disabled by default, got checked={checked}"

    # Enable Tor proxy.
    gui.click("torEnableSwitch")
    gui.wait_for_property("torEnableSwitch", "checked", True, timeout_ms=2000)
    gui.wait_for_property("torAddressInput", "enabled", True, timeout_ms=2000)
    checked = gui.get_property("torEnableSwitch", "checked")
    assert checked, "Expected torEnableSwitch to be checked after click"
    print("  Tor proxy toggled ON: OK")

    # Disable Tor proxy.
    gui.click("torEnableSwitch")
    gui.wait_for_property("torEnableSwitch", "checked", False, timeout_ms=2000)
    checked = gui.get_property("torEnableSwitch", "checked")
    assert not checked, "Expected torEnableSwitch to be unchecked after second click"
    print("  Tor proxy toggled OFF: OK")


def wait_for_settings_key(datadir, key, timeout=10.0):
    """Poll regtest/settings.json until `key` is present, up to timeout seconds."""
    settings_path = os.path.join(datadir, "regtest", "settings.json")
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if os.path.exists(settings_path):
            try:
                with open(settings_path, encoding="utf-8") as f:
                    data = json.load(f)
                if key in data:
                    return data
            except (json.JSONDecodeError, OSError):
                pass
        time.sleep(0.1)
    raise AssertionError(
        f"Timed out after {timeout}s waiting for '{key}' in {settings_path}"
    )


def test_proxy_settings_persist(datadir, settings):
    print("\n── test_proxy_settings_persist ─────────────────────────────────────")

    assert settings.get("proxy") == "10.0.0.1:9050", (
        f"Expected proxy='10.0.0.1:9050' in settings.json, got: {settings}"
    )
    print(f"  settings.json proxy entry '{settings['proxy']}': OK")

    assert settings.get("onion") == "127.0.0.1:9150", (
        f"Expected onion='127.0.0.1:9150' in settings.json, got: {settings}"
    )
    print(f"  settings.json onion entry '{settings['onion']}': OK")


def run_tests():
    args = parse_args()
    harness = QmlTestHarness(
        socket_path=args.socket_path,
        reset_settings=False,
    )
    gui = None
    try:
        harness.start()
        gui = harness.driver
        if not args.socket_path and gui.object_exists("onboardingCover"):
            complete_preinit_onboarding(gui)
            gui = harness.wait_for_main_window_reconnect()

        navigate_to_proxy_settings(gui)

        test_default_proxy_toggle(gui)
        test_proxy_valid_address(gui)
        test_proxy_invalid_address(gui)
        test_tor_proxy_toggle(gui)

        # Prepare state for the persistence test: runtime settings write to
        # settings.json immediately, so no onboarding completion step is needed.
        if harness.datadir:
            if not gui.get_property("proxyEnableSwitch", "checked"):
                gui.click("proxyEnableSwitch")
                gui.wait_for_property("proxyEnableSwitch", "checked", True, timeout_ms=2000)
            gui.wait_for_property("proxyAddressInput", "enabled", True, timeout_ms=2000)
            gui.set_text("proxyAddressInput", "10.0.0.1:9050")
            gui.wait_for_property("proxyAddressInput", "validInput", True, timeout_ms=2000)

            if not gui.get_property("torEnableSwitch", "checked"):
                gui.click("torEnableSwitch")
                gui.wait_for_property("torEnableSwitch", "checked", True, timeout_ms=2000)
            gui.wait_for_property("torAddressInput", "enabled", True, timeout_ms=2000)
            gui.set_text("torAddressInput", "127.0.0.1:9150")
            gui.wait_for_property("torAddressInput", "validInput", True, timeout_ms=2000)

        navigate_back_from_proxy_settings(gui)

        if harness.datadir:
            settings = wait_for_settings_key(harness.datadir, "proxy")
            test_proxy_settings_persist(harness.datadir, settings)
        else:
            print("\n── test_proxy_settings_persist: skipped (external harness) ──")

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


if __name__ == '__main__':
    run_tests()
