// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.bitcoincore.qt 1.0
import "../controls"

Item {
    id: root

    Accessible.role: Accessible.List
    Accessible.name: accessibleName

    property var listModel: null
    property string accessibleName: ""

    property int horizontalPadding: 0
    property int topPadding: 10
    property int bottomPadding: 16
    property int rowSpacing: 10
    property int columnSpacing: 10
    property int contentSpacing: 2
    property int lineNumberWidth: 20
    property int fontPixelSize: 12
    property int textLineHeight: 17
    property string fontFamily: Theme.text.family
    property string fontStyleName: "Regular"
    property bool autoScrollToBottom: false

    readonly property int lineNumberDigits: String(Math.max(1, count)).length
    readonly property string lineNumberSampleText: lineNumberDigits <= 3
                                                    ? ""
                                                    : lineNumberDigits === 4
                                                        ? "8888"
                                                        : "88888"
    readonly property int effectiveLineNumberWidth: lineNumberDigits <= 3
                                                     ? lineNumberWidth
                                                     : Math.max(lineNumberWidth, Math.ceil(lineNumberMetrics.advanceWidth))
    readonly property bool atBottom: flick.contentHeight <= flick.height ||
                                     flick.contentY + flick.height >= flick.contentHeight - 1
    readonly property bool atTop: flick.contentY <= 0
    readonly property real contentY: flick.contentY
    readonly property int count: rowRepeater.count

    signal scrolled(real y)

    function scrollToTop() {
        flick.contentY = 0
        flick.returnToBounds()
    }

    function scrollToBottom() {
        if (flick.contentHeight > flick.height) {
            flick.contentY = flick.contentHeight - flick.height
        } else {
            flick.contentY = 0
        }
        flick.returnToBounds()
    }

    TextMetrics {
        id: lineNumberMetrics
        font.family: root.fontFamily
        font.styleName: root.fontStyleName
        font.pixelSize: root.fontPixelSize
        text: root.lineNumberSampleText
    }

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: contentColumn.height
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            minimumSize: 0.05
        }

        onContentYChanged: root.scrolled(contentY)

        Column {
            id: contentColumn
            objectName: root.objectName.length > 0 ? root.objectName + "_contentColumn" : ""
            x: root.horizontalPadding
            width: flick.width - (root.horizontalPadding * 2)
            topPadding: root.topPadding
            bottomPadding: root.bottomPadding
            spacing: root.rowSpacing

            Repeater {
                id: rowRepeater
                model: root.listModel

                delegate: RowLayout {
                    id: rowRoot

                    required property var model
                    required property int index

                    readonly property string rowCommand: rowRoot.model.command ?? ""
                    readonly property string rowDate: rowRoot.model.dateLabel ?? ""
                    readonly property string rowMessage: rowRoot.model.message ?? ""
                    readonly property string rowNumber: rowRoot.model.lineNumber ?? ""
                    readonly property int rowSeverity: Number(rowRoot.model.severity ?? DebugLogModel.InfoSeverity)
                    readonly property bool hasCommand: rowCommand.length > 0

                    objectName: root.objectName.length > 0 ? root.objectName + "_row_" + index : ""
                    width: contentColumn.width
                    spacing: root.columnSpacing

                    Accessible.role: Accessible.ListItem
                    Accessible.name: rowCommand.length > 0
                                     ? rowCommand + " " + rowMessage
                                     : rowMessage

                    Text {
                        objectName: root.objectName.length > 0 ? root.objectName + "_lineNumber_" + rowRoot.index : ""
                        text: rowRoot.rowNumber
                        color: Theme.color.neutral7
                        font.family: root.fontFamily
                        font.styleName: root.fontStyleName
                        font.pixelSize: root.fontPixelSize
                        lineHeight: root.textLineHeight
                        lineHeightMode: Text.FixedHeight
                        horizontalAlignment: Text.AlignRight
                        wrapMode: Text.NoWrap

                        Layout.preferredWidth: root.effectiveLineNumberWidth
                        Layout.alignment: Qt.AlignTop
                    }

                    ColumnLayout {
                        objectName: root.objectName.length > 0 ? root.objectName + "_entryContent_" + rowRoot.index : ""
                        spacing: root.contentSpacing

                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop

                        RowLayout {
                            objectName: root.objectName.length > 0 ? root.objectName + "_header_" + rowRoot.index : ""
                            spacing: root.columnSpacing

                            Layout.fillWidth: true

                            Text {
                                objectName: root.objectName.length > 0 ? root.objectName + "_command_" + rowRoot.index : ""
                                text: rowRoot.rowCommand
                                visible: rowRoot.hasCommand
                                color: rowRoot.rowSeverity === DebugLogModel.ErrorSeverity
                                       ? Theme.color.red
                                       : Theme.color.green
                                font.family: root.fontFamily
                                font.styleName: root.fontStyleName
                                font.pixelSize: root.fontPixelSize
                                lineHeight: root.textLineHeight
                                lineHeightMode: Text.FixedHeight
                                elide: Text.ElideRight
                                wrapMode: Text.NoWrap

                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }

                            TextEdit {
                                objectName: root.objectName.length > 0 ? root.objectName + "_commandlessMessage_" + rowRoot.index : ""
                                text: rowRoot.rowMessage
                                visible: !rowRoot.hasCommand
                                readOnly: true
                                selectByMouse: true
                                persistentSelection: false
                                textFormat: Text.PlainText
                                wrapMode: Text.WrapAnywhere
                                font.family: root.fontFamily
                                font.styleName: root.fontStyleName
                                font.pixelSize: root.fontPixelSize
                                color: Theme.color.neutral9
                                selectionColor: Theme.color.orange
                                selectedTextColor: Theme.color.white
                                activeFocusOnPress: true

                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }

                            Text {
                                objectName: root.objectName.length > 0 ? root.objectName + "_date_" + rowRoot.index : ""
                                text: rowRoot.rowDate
                                color: Theme.color.neutral7
                                font.family: root.fontFamily
                                font.styleName: root.fontStyleName
                                font.pixelSize: root.fontPixelSize
                                lineHeight: root.textLineHeight
                                lineHeightMode: Text.FixedHeight
                                horizontalAlignment: Text.AlignRight
                                wrapMode: Text.NoWrap

                                Layout.alignment: Qt.AlignTop
                            }
                        }

                        TextEdit {
                            objectName: root.objectName.length > 0 ? root.objectName + "_message_" + rowRoot.index : ""
                            text: rowRoot.rowMessage
                            visible: rowRoot.hasCommand
                            readOnly: true
                            selectByMouse: true
                            persistentSelection: false
                            textFormat: Text.PlainText
                            wrapMode: Text.WrapAnywhere
                            font.family: root.fontFamily
                            font.styleName: root.fontStyleName
                            font.pixelSize: root.fontPixelSize
                            color: Theme.color.neutral9
                            selectionColor: Theme.color.orange
                            selectedTextColor: Theme.color.white
                            activeFocusOnPress: true

                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: flick
        enabled: root.autoScrollToBottom
        function onContentHeightChanged() {
            root.scrollToBottom()
        }
    }
}
