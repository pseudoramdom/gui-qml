// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtTest 1.2

TestCase {
    name: "SettingsWindowBehavior"

    function init() {
        desktopWindowBehaviorModel.showTrayIcon = true
        desktopWindowBehaviorModel.minimizeToTray = false
        desktopWindowBehaviorModel.minimizeOnClose = false
    }

    function test_desktopPlatform_isTrue() {
        compare(desktopWindowBehaviorModel.desktopPlatform, true)
    }

    function test_showTrayIcon_defaultsTrue() {
        compare(desktopWindowBehaviorModel.showTrayIcon, true)
    }

    function test_minimizeToTray_defaultsFalse() {
        compare(desktopWindowBehaviorModel.minimizeToTray, false)
    }

    function test_minimizeOnClose_defaultsFalse() {
        compare(desktopWindowBehaviorModel.minimizeOnClose, false)
    }

    function test_showTrayIcon_toggleRoundTrip() {
        desktopWindowBehaviorModel.showTrayIcon = false
        compare(desktopWindowBehaviorModel.showTrayIcon, false)
        desktopWindowBehaviorModel.showTrayIcon = true
        compare(desktopWindowBehaviorModel.showTrayIcon, true)
    }

    function test_minimizeToTray_cascadesWhenTrayDisabled() {
        desktopWindowBehaviorModel.minimizeToTray = true
        compare(desktopWindowBehaviorModel.minimizeToTray, true)
        desktopWindowBehaviorModel.showTrayIcon = false
        compare(desktopWindowBehaviorModel.minimizeToTray, false)
    }

    function test_minimizeToTray_blockedWithoutTray() {
        desktopWindowBehaviorModel.showTrayIcon = false
        desktopWindowBehaviorModel.minimizeToTray = true
        compare(desktopWindowBehaviorModel.minimizeToTray, false)
    }

    function test_minimizeOnClose_independentOfTray() {
        desktopWindowBehaviorModel.showTrayIcon = false
        desktopWindowBehaviorModel.minimizeOnClose = true
        compare(desktopWindowBehaviorModel.minimizeOnClose, true)
    }

    function test_shouldHideToTrayOnMinimize_requiresAllConditions() {
        compare(desktopWindowBehaviorModel.shouldHideToTrayOnMinimize(), false)
        desktopWindowBehaviorModel.minimizeToTray = true
        compare(desktopWindowBehaviorModel.shouldHideToTrayOnMinimize(), true)
        desktopWindowBehaviorModel.showTrayIcon = false
        compare(desktopWindowBehaviorModel.shouldHideToTrayOnMinimize(), false)
    }

    function test_shouldMinimizeWindowOnClose() {
        compare(desktopWindowBehaviorModel.shouldMinimizeWindowOnClose(), false)
        desktopWindowBehaviorModel.minimizeOnClose = true
        compare(desktopWindowBehaviorModel.shouldMinimizeWindowOnClose(), true)
    }
}
