// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/integration/runner/test_cases.h>
#include <test/integration/fixture/test_util.h>

#include <QSettings>

class SettingsIntegrationTests : public QObject
{
    Q_OBJECT
    BitcoinQmlApplication& m_app;

public:
    explicit SettingsIntegrationTests(BitcoinQmlApplication& app) : m_app{app} {}

private Q_SLOTS:
    void displayControlsUpdateUnitsAndTranslations()
    {
        QVERIFY(qmltest::SelectSettings(m_app, "display"));
        QVERIFY(qmltest::WaitForVisible(m_app, "displaySettingsPage"));
        for (const auto* row : {"displayThemeRow", "displayBlockStatusSizeRow", "displayMoneyFontRow",
                                "displayUnitRow", "displayLanguageRow", "displayTransactionUrlsRow"}) {
            auto* object = qmltest::Find(m_app, row);
            QVERIFY2(object, row);
            QCOMPARE(object->property("description").toString(), QString{});
        }
        QVERIFY(qmltest::Click(m_app, "displayUnitPickerButton"));
        QVERIFY(qmltest::Click(m_app, "displayUnitSAT"));
        auto* picker = qmltest::Find(m_app, "displayUnitPicker");
        QVERIFY(picker);
        QTRY_COMPARE(picker->property("currentValue").toInt(), 3);
        QCOMPARE(picker->property("currentText").toString(), QStringLiteral("sat"));
        QVERIFY(qmltest::Click(m_app, "displayLanguageRow"));
        QVERIFY(qmltest::SetText(m_app, "languageSearch", QString::fromUtf8("español")));
        QVERIFY(qmltest::Click(m_app, "language_es"));
        QVERIFY(qmltest::WaitForVisible(m_app, "displayLanguageRow"));
        QTRY_COMPARE(qmltest::Find(m_app, "displayLanguageRow")->property("title").toString(), QStringLiteral("Idioma"));
        QCOMPARE(qmltest::Find(m_app, "displayUnitRow")->property("title").toString(), QString::fromUtf8("Unidad de visualización"));
        QVERIFY(qmltest::Click(m_app, "displayLanguageRow"));
        QVERIFY(qmltest::SetText(m_app, "languageSearch", ""));
        QVERIFY(qmltest::Click(m_app, "language_"));
        QVERIFY(qmltest::WaitForVisible(m_app, "displayLanguageRow"));
        QTRY_COMPARE(qmltest::Find(m_app, "displayLanguageRow")->property("title").toString(), QStringLiteral("Language"));
    }
};

std::unique_ptr<QObject> CreateSettingsTests(BitcoinQmlApplication& app, WalletMode)
{
    return std::make_unique<SettingsIntegrationTests>(app);
}

#include <test_settings.moc>
