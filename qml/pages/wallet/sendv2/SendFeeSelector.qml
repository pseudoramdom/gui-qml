// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0

import "../../../controls"

ColumnLayout {
    id: root
    objectName: "feeSelectionControl"

    property var wallet: null
    readonly property int selectedTarget: wallet ? wallet.targetBlocks : 2
    readonly property bool customSelected: wallet ? wallet.customFeeEnabled : false

    signal feeChanged(int target)

    spacing: 12

    ButtonGroup { id: feeGroup }

    GridLayout {
        objectName: "sendFeeGrid"
        Layout.fillWidth: true
        columns: width < 620 ? 1 : 3
        columnSpacing: 12
        rowSpacing: 12

        SelectableCard {
            objectName: "feeSelectionOption2"
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            title: qsTr("Low")
            subtitle: qsTr("~60 min")
            detail: root.wallet
                ? (root.wallet.feeEstimateRevision, root.wallet.estimatedFeeRateForTarget(6) || qsTr("Calculating…"))
                : "—"
            checked: !root.customSelected && root.selectedTarget === 6
            ButtonGroup.group: feeGroup
            onClicked: {
                if (!root.wallet) return
                root.wallet.customFeeEnabled = false
                root.wallet.targetBlocks = 6
                root.feeChanged(6)
            }
        }

        SelectableCard {
            objectName: "feeSelectionOption1"
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            title: qsTr("Default")
            subtitle: qsTr("~20 min")
            detail: root.wallet
                ? (root.wallet.feeEstimateRevision, root.wallet.estimatedFeeRateForTarget(2) || qsTr("Calculating…"))
                : "—"
            checked: !root.customSelected && root.selectedTarget === 2
            ButtonGroup.group: feeGroup
            onClicked: {
                if (!root.wallet) return
                root.wallet.customFeeEnabled = false
                root.wallet.targetBlocks = 2
                root.feeChanged(2)
            }
        }

        SelectableCard {
            objectName: "feeSelectionOption0"
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            title: qsTr("High")
            subtitle: qsTr("~10 min")
            detail: root.wallet
                ? (root.wallet.feeEstimateRevision, root.wallet.estimatedFeeRateForTarget(1) || qsTr("Calculating…"))
                : "—"
            checked: !root.customSelected && root.selectedTarget === 1
            ButtonGroup.group: feeGroup
            onClicked: {
                if (!root.wallet) return
                root.wallet.customFeeEnabled = false
                root.wallet.targetBlocks = 1
                root.feeChanged(1)
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: root.customSelected
        spacing: 8

        Icon {
            source: "image://images/info-filled"
            color: Theme.color.orange
            size: 18
        }

        CoreText {
            objectName: "feeSelectionCustomEstimateLabel"
            Layout.fillWidth: true
            text: root.wallet && root.wallet.customFeeRateValid
                ? qsTr("Custom fee rate: %1 sat/vB").arg(root.wallet.customFeeRate)
                : qsTr("Enter a valid custom fee rate from the form menu")
            color: root.wallet && root.wallet.customFeeRateValid ? Theme.color.neutral7 : Theme.color.red
            font: Theme.text.description.font
            lineHeight: Theme.text.description.lineHeight
            lineHeightMode: Text.FixedHeight
            horizontalAlignment: Text.AlignLeft
        }
    }
}
