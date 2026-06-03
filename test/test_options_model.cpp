// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtTest/QtTest>

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <test/gmocktestfixture.h>
#include <test/mocks/mocknode.h>
#include <qml/models/options_model.h>
#include <qml/models/settings_keys.h>
#include <init.h>
#include <net_processing.h>
#include <common/args.h>
#include <common/settings.h>
#include <util/translation.h>

#ifndef BITCOINQML_NO_TEST_MAIN
const TranslateFn G_TRANSLATION_FUN{nullptr};
#endif

class OptionsModelTests : public GmockTestFixture
{
    Q_OBJECT

public:
    enum class LegacyDisplayUnit {
        BTC = 0,
        mBTC = 1,
        uBTC = 2,
        SAT = 3,
    };
    Q_ENUM(LegacyDisplayUnit)

private Q_SLOTS:
    void proxyDisabledRemovesKey();
    void torDisabledRemovesKey();
    void proxyEnabledWritesAddress();
    void proxyDirtySetAtRuntime();
    void proxyDirtyResetWhenReverted();
    void mempoolSizeLoadedFromSettings();
    void mempoolSizeWritesSetting();
    void mempoolSizeDoesNotRewriteUnchangedSetting();
    void externalSignerPathWritesSigner();
    void externalSignerPathClearedRemovesKey();
    void walletSettingsDirtyTracksExternalSignerPath();
    void signerPathLoadedFromSettings();
    void signerPathWritesSetting();
    void signerDirtySetAtRuntime();
    void signerDirtyResetWhenReverted();
    void externalSignerPathValidationRejectsMissingPath();
    void externalSignerPathValidationAcceptsExecutablePath();
    void connectionDirtyTracksRestartSettings();
    void natpmpAppliesLiveWithoutRestartDirty();
    void storageDirtyIgnoresDisabledPruneSize();
    void developerDirtyTracksRestartSettings();
    void proxyValidationAndCommit();
    void proxyDisabledPreservesPreviousValue();
    void customDataDirSelectionCreatesDirectoryAndPersists();
    void thirdPartyTransactionLinksParseValidUrls();
    void moneyFontChoicePersists();
    void displayUnitUsesQtCompatibleSettingsKey();
};

// Convenience: set up a NiceMock whose getPersistentSetting returns null for
// all keys by default, but returns a given address for the specified key.
static common::SettingsValue MakeAddress(const std::string& addr)
{
    return common::SettingsValue{addr};
}

static common::SettingsValue MakeInt(int value)
{
    return common::SettingsValue{value};
}

void OptionsModelTests::proxyDisabledRemovesKey()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    // Simulate a previously-saved proxy address so that m_proxy_enabled=true on construction.
    ON_CALL(node, getPersistentSetting(std::string{"proxy"}))
        .WillByDefault(Return(MakeAddress("127.0.0.1:9050")));

    OptionsQmlModel model(node);
    QVERIFY(model.proxyEnabled());

    // When proxy is disabled, updateRwSetting must be called with a null (not
    // empty-string) SettingsValue so that the key is erased from settings.json.
    EXPECT_CALL(node, updateRwSetting(std::string{"proxy-prev"}, _));
    EXPECT_CALL(node, updateRwSetting(std::string{"proxy"},
        Truly([](const common::SettingsValue& v) { return v.isNull(); })));

    model.setProxyEnabled(false);
    QVERIFY(!model.proxyEnabled());
}

void OptionsModelTests::torDisabledRemovesKey()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    ON_CALL(node, getPersistentSetting(std::string{"onion"}))
        .WillByDefault(Return(MakeAddress("127.0.0.1:9150")));

    OptionsQmlModel model(node);
    QVERIFY(model.torEnabled());

    EXPECT_CALL(node, updateRwSetting(std::string{"onion-prev"}, _));
    EXPECT_CALL(node, updateRwSetting(std::string{"onion"},
        Truly([](const common::SettingsValue& v) { return v.isNull(); })));

    model.setTorEnabled(false);
    QVERIFY(!model.torEnabled());
}

void OptionsModelTests::proxyEnabledWritesAddress()
{
    using ::testing::_;
    using ::testing::Eq;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    // Construct with no saved proxy — m_proxy_enabled=false, m_proxy_address="".
    OptionsQmlModel model(node);
    QVERIFY(!model.proxyEnabled());

    // Pre-load an address into the model (as QML does before toggling the switch).
    model.setProxyAddress("10.0.0.1:9050");

    // Enabling proxy must write the address string to settings.
    EXPECT_CALL(node, updateRwSetting(std::string{"proxy"},
        Truly([](const common::SettingsValue& v) {
            return v.isStr() && v.get_str() == "10.0.0.1:9050";
        })));
    EXPECT_CALL(node, updateRwSetting(std::string{"proxy-prev"},
        Truly([](const common::SettingsValue& v) { return v.isNull(); })));

    model.setProxyEnabled(true);
    QVERIFY(model.proxyEnabled());
}

void OptionsModelTests::proxyDirtySetAtRuntime()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(!model.proxySettingsDirty());

    model.setProxyEnabled(true);
    QVERIFY(model.proxySettingsDirty());
}

void OptionsModelTests::proxyDirtyResetWhenReverted()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    // Start with proxy disabled (no saved proxy).
    OptionsQmlModel model(node);
    QVERIFY(!model.proxySettingsDirty());

    // Simulate what ProxySettings.qml does: set address before enabling.
    model.setProxyAddress("127.0.0.1:9050");
    // Address changed but proxy is still disabled — should NOT be dirty since
    // the address is irrelevant when proxy is off.
    QVERIFY(!model.proxySettingsDirty());

    // Enable proxy — now dirty (enabled differs from initial disabled).
    model.setProxyEnabled(true);
    QVERIFY(model.proxySettingsDirty());

    // Revert enable state — dirty should clear even though address is populated,
    // because the address is ignored when proxy is disabled.
    model.setProxyEnabled(false);
    QVERIFY(!model.proxySettingsDirty());
}

void OptionsModelTests::mempoolSizeLoadedFromSettings()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    ON_CALL(node, getPersistentSetting(std::string{"maxmempool"}))
        .WillByDefault(Return(MakeInt(456)));

    OptionsQmlModel model(node);
    QCOMPARE(model.maxMempoolSizeMB(), 456);
}

void OptionsModelTests::mempoolSizeWritesSetting()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);

    EXPECT_CALL(node, updateRwSetting(std::string{"maxmempool"},
        Truly([](const common::SettingsValue& v) {
            return v.isNum() && v.getInt<int64_t>() == 456;
        })));

    model.setMaxMempoolSizeMB(456);
    QCOMPARE(model.maxMempoolSizeMB(), 456);
}

void OptionsModelTests::mempoolSizeDoesNotRewriteUnchangedSetting()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    ON_CALL(node, getPersistentSetting(std::string{"maxmempool"}))
        .WillByDefault(Return(MakeInt(456)));

    OptionsQmlModel model(node);

    EXPECT_CALL(node, updateRwSetting(std::string{"maxmempool"}, _)).Times(0);

    model.setMaxMempoolSizeMB(456);
    QCOMPARE(model.maxMempoolSizeMB(), 456);
}

void OptionsModelTests::externalSignerPathWritesSigner()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);

    EXPECT_CALL(node, updateRwSetting(std::string{"signer"},
        Truly([](const common::SettingsValue& v) {
            return v.isStr() && v.get_str() == "/usr/local/bin/hwi";
        })));

    model.setExternalSignerPath("/usr/local/bin/hwi");
    QCOMPARE(model.externalSignerPath(), QString("/usr/local/bin/hwi"));
}

void OptionsModelTests::externalSignerPathClearedRemovesKey()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    ON_CALL(node, getPersistentSetting(std::string{"signer"}))
        .WillByDefault(Return(MakeAddress("/usr/local/bin/hwi")));

    OptionsQmlModel model(node);
    QCOMPARE(model.externalSignerPath(), QString("/usr/local/bin/hwi"));

    EXPECT_CALL(node, updateRwSetting(std::string{"signer"},
        Truly([](const common::SettingsValue& v) { return v.isNull(); })));

    model.setExternalSignerPath("");
    QVERIFY(model.externalSignerPath().isEmpty());
}

void OptionsModelTests::walletSettingsDirtyTracksExternalSignerPath()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(!model.walletSettingsDirty());

    model.setExternalSignerPath("/usr/local/bin/hwi");
    QVERIFY(model.walletSettingsDirty());

    model.setExternalSignerPath("");
    QVERIFY(!model.walletSettingsDirty());
}

void OptionsModelTests::signerPathLoadedFromSettings()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    ON_CALL(node, getPersistentSetting(std::string{"signer"}))
        .WillByDefault(Return(MakeAddress("/opt/hwi/ledger.py")));

    OptionsQmlModel model(node);
    QCOMPARE(model.externalSignerPath(), QString("/opt/hwi/ledger.py"));
}

void OptionsModelTests::signerPathWritesSetting()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(model.externalSignerPath().isEmpty());

    EXPECT_CALL(node, updateRwSetting(std::string{"signer"},
        Truly([](const common::SettingsValue& v) {
            return v.isStr() && v.get_str() == "/opt/hwi/ledger.py";
        })));

    model.setExternalSignerPath("/opt/hwi/ledger.py");
    QCOMPARE(model.externalSignerPath(), QString("/opt/hwi/ledger.py"));
}

void OptionsModelTests::signerDirtySetAtRuntime()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(!model.walletSettingsDirty());

    model.setExternalSignerPath("/opt/hwi/ledger.py");
    QVERIFY(model.walletSettingsDirty());
}

void OptionsModelTests::signerDirtyResetWhenReverted()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(!model.walletSettingsDirty());

    model.setExternalSignerPath("/opt/hwi/ledger.py");
    QVERIFY(model.walletSettingsDirty());

    model.setExternalSignerPath("");
    QVERIFY(!model.walletSettingsDirty());
}

void OptionsModelTests::externalSignerPathValidationRejectsMissingPath()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QCOMPARE(model.externalSignerPathValidationError("/definitely/not/a/real/signer"),
        QString("The configured signer path does not exist."));
}

void OptionsModelTests::externalSignerPathValidationAcceptsExecutablePath()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    const QString script_path = temp_dir.filePath("fake-signer");
    QFile script(script_path);
    QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));
    script.write("#!/bin/sh\nexit 0\n");
    script.close();
    QVERIFY(script.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));

    OptionsQmlModel model(node);
    QVERIFY(model.externalSignerPathValidationError(script_path).isEmpty());
}

void OptionsModelTests::connectionDirtyTracksRestartSettings()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    ON_CALL(node, getPersistentSetting(std::string{"listen"})).WillByDefault(Return(common::SettingsValue{true}));

    OptionsQmlModel model(node);
    QVERIFY(!model.connectionSettingsDirty());
    QVERIFY(!model.restartRequired());

    model.setListen(false);
    QVERIFY(model.connectionSettingsDirty());
    QVERIFY(model.restartRequired());

    model.setListen(true);
    QVERIFY(!model.connectionSettingsDirty());
    QVERIFY(!model.restartRequired());
}

void OptionsModelTests::natpmpAppliesLiveWithoutRestartDirty()
{
    using ::testing::_;
    using ::testing::InSequence;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    InSequence sequence;
    EXPECT_CALL(node, updateRwSetting(std::string{"natpmp"},
        Truly([](const common::SettingsValue& value) {
            const std::optional<bool> parsed = SettingToBool(value);
            return parsed.has_value() && *parsed;
        })));
    EXPECT_CALL(node, mapPort(true));
    EXPECT_CALL(node, updateRwSetting(std::string{"natpmp"},
        Truly([](const common::SettingsValue& value) {
            const std::optional<bool> parsed = SettingToBool(value);
            return value.isNull() || (parsed.has_value() && !*parsed);
        })));
    EXPECT_CALL(node, mapPort(false));

    model.setNatpmp(true);
    QVERIFY(model.natpmp());
    QVERIFY(!model.connectionSettingsDirty());
    QVERIFY(!model.restartRequired());
    QTest::qWait(300);

    model.setNatpmp(false);
    QVERIFY(!model.natpmp());
    QVERIFY(!model.connectionSettingsDirty());
    QVERIFY(!model.restartRequired());
    QTest::qWait(300);
}

void OptionsModelTests::storageDirtyIgnoresDisabledPruneSize()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(!model.prune());
    model.setPruneSizeGB(10);
    QVERIFY(!model.storageSettingsDirty());

    model.setPrune(true);
    QVERIFY(model.storageSettingsDirty());
    QVERIFY(model.restartRequired());
}

void OptionsModelTests::developerDirtyTracksRestartSettings()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(!model.developerSettingsDirty());

    model.setScriptThreads(model.scriptThreads() + 1);
    QVERIFY(model.developerSettingsDirty());
    QVERIFY(model.restartRequired());

    model.setScriptThreads(model.scriptThreads() - 1);
    QVERIFY(!model.developerSettingsDirty());
    QVERIFY(!model.restartRequired());

    model.setMaxMempoolSizeMB(model.maxMempoolSizeMB() + 1);
    QVERIFY(model.developerSettingsDirty());
    QVERIFY(model.restartRequired());

    model.setMaxMempoolSizeMB(model.maxMempoolSizeMB() - 1);
    QVERIFY(!model.developerSettingsDirty());
    QVERIFY(!model.restartRequired());
}

void OptionsModelTests::proxyValidationAndCommit()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(model.validateProxyLocation("127.0.0.1:9050").isEmpty());
    QVERIFY(model.validateProxyLocation("[::1]:9050").isEmpty());
    QVERIFY(model.validateProxyLocation("127.0.0.1").isEmpty());
    QVERIFY(model.validateProxyLocation("[::1]").isEmpty());
    QVERIFY(model.validateProxyLocation("proxy.example:9050").isEmpty());
    QVERIFY(model.validateProxyLocation("proxy.example").isEmpty());
#ifdef HAVE_SOCKADDR_UN
    QVERIFY(model.validateProxyLocation("unix:/tmp/bitcoin-core-proxy.sock").isEmpty());
#else
    QVERIFY(!model.validateProxyLocation("unix:/tmp/bitcoin-core-proxy.sock").isEmpty());
#endif
    QVERIFY(!model.validateProxyLocation("abc..abc:23456").isEmpty());
    QVERIFY(!model.validateProxyLocation("999.999.999.999:9050").isEmpty());
    QVERIFY(!model.validateProxyLocation("127.0.0.1:0").isEmpty());
    QVERIFY(!model.validateProxyLocation("127.0.0.1:65536").isEmpty());
    QVERIFY(!model.validateProxyLocation("127.0.0.1:9050=ipv4").isEmpty());
    QVERIFY(!model.validateProxyLocation("0=cjdns").isEmpty());
    QVERIFY(!model.commitProxyLocation("999.999.999.999:9050"));
    QVERIFY(model.proxyAddress().isEmpty());

    QVERIFY(model.commitProxyLocation("proxy.example"));
    QCOMPARE(model.proxyAddress(), QString("proxy.example"));
}

void OptionsModelTests::proxyDisabledPreservesPreviousValue()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Truly;

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));
    ON_CALL(node, getPersistentSetting(std::string{"proxy"}))
        .WillByDefault(Return(MakeAddress("127.0.0.1:9050")));

    OptionsQmlModel model(node);
    EXPECT_CALL(node, updateRwSetting(std::string{"proxy-prev"},
        Truly([](const common::SettingsValue& v) {
            return v.isStr() && v.get_str() == "127.0.0.1:9050";
        })));
    EXPECT_CALL(node, updateRwSetting(std::string{"proxy"},
        Truly([](const common::SettingsValue& v) { return v.isNull(); })));

    model.setProxyEnabled(false);
    QVERIFY(!model.proxyEnabled());
}

void OptionsModelTests::customDataDirSelectionCreatesDirectoryAndPersists()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    QSettings settings;
    const bool had_data_dir_setting = settings.contains(SettingsKeys::DATA_DIR);
    const QVariant previous_data_dir_setting = settings.value(SettingsKeys::DATA_DIR);
    settings.remove(SettingsKeys::DATA_DIR);

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString data_dir = QDir(temp_dir.path()).filePath("selected-data-dir");
    QVERIFY(!QFileInfo::exists(data_dir));

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(model.validateCustomDataDir(data_dir).isEmpty());
    QVERIFY(model.selectCustomDataDir(data_dir));

    QVERIFY(QFileInfo(data_dir).isDir());
    QVERIFY(QFileInfo(QDir(data_dir).filePath("wallets")).isDir());
    QCOMPARE(model.dataDir(), data_dir);
    QCOMPARE(settings.value(SettingsKeys::DATA_DIR).toString(), data_dir);

    if (had_data_dir_setting) {
        settings.setValue(SettingsKeys::DATA_DIR, previous_data_dir_setting);
    } else {
        settings.remove(SettingsKeys::DATA_DIR);
    }
}

void OptionsModelTests::thirdPartyTransactionLinksParseValidUrls()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    QSettings settings;
    const bool had_urls_setting = settings.contains(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS);
    const QVariant previous_urls_setting = settings.value(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS);

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    model.setThirdPartyTransactionUrls("https://example.com/tx/%s|https://example.org/tx|not-a-url");

    const QVariantList links = model.thirdPartyTransactionLinks("abc");
    QCOMPARE(links.size(), 1);
    const QVariantMap link = links.front().toMap();
    QCOMPARE(link.value("host").toString(), QString("example.com"));
    QCOMPARE(link.value("url").toString(), QString("https://example.com/tx/abc"));

    if (had_urls_setting) {
        settings.setValue(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS, previous_urls_setting);
    } else {
        settings.remove(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS);
    }
}

void OptionsModelTests::moneyFontChoicePersists()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    QSettings settings;
    const bool had_font_setting = settings.contains(SettingsKeys::MONEY_FONT_CHOICE);
    const QVariant previous_font_setting = settings.value(SettingsKeys::MONEY_FONT_CHOICE);
    settings.remove(SettingsKeys::MONEY_FONT_CHOICE);

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QCOMPARE(model.moneyFontChoice(), QString("embedded"));
    model.setMoneyFontChoice("best_system");
    QCOMPARE(model.moneyFontChoice(), QString("best_system"));
    QCOMPARE(settings.value(SettingsKeys::MONEY_FONT_CHOICE).toString(), QString("best_system"));

    if (had_font_setting) {
        settings.setValue(SettingsKeys::MONEY_FONT_CHOICE, previous_font_setting);
    } else {
        settings.remove(SettingsKeys::MONEY_FONT_CHOICE);
    }
}

void OptionsModelTests::displayUnitUsesQtCompatibleSettingsKey()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    QSettings settings;
    const bool had_display_unit_setting = settings.contains(SettingsKeys::DISPLAY_UNIT);
    const QVariant previous_display_unit_setting = settings.value(SettingsKeys::DISPLAY_UNIT);
    settings.setValue(SettingsKeys::DISPLAY_UNIT, QVariant::fromValue(LegacyDisplayUnit::SAT));

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QCOMPARE(QString::fromUtf8(SettingsKeys::DISPLAY_UNIT), QStringLiteral("DisplayBitcoinUnit"));
    QCOMPARE(model.displayUnit(), 3);
    QCOMPARE(model.displayUnitLabel(), QStringLiteral("sat"));
    QCOMPARE(model.displayUnitLabelForAmount(2), QStringLiteral("sats"));

    model.setDisplayUnit(1);
    QCOMPARE(model.displayUnit(), 1);
    QCOMPARE(model.displayUnitLabel(), QStringLiteral("mBTC"));
    QCOMPARE(model.displayUnitLabelForAmount(2), QStringLiteral("mBTC"));
    QCOMPARE(settings.value(SettingsKeys::DISPLAY_UNIT).toInt(), 1);

    model.setDisplayUnit(2);
    QCOMPARE(model.displayUnit(), 2);
    QCOMPARE(model.displayUnitLabel(), QStringLiteral("bits"));
    QCOMPARE(model.displayUnitLabelForAmount(2), QStringLiteral("bits"));
    QCOMPARE(settings.value(SettingsKeys::DISPLAY_UNIT).toInt(), 2);

    model.setDisplayUnit(9);
    QCOMPARE(model.displayUnit(), 0);
    QCOMPARE(settings.value(SettingsKeys::DISPLAY_UNIT).toInt(), 0);

    if (had_display_unit_setting) {
        settings.setValue(SettingsKeys::DISPLAY_UNIT, previous_display_unit_setting);
    } else {
        settings.remove(SettingsKeys::DISPLAY_UNIT);
    }
}

#ifdef BITCOINQML_NO_TEST_MAIN
#include <test/qt_test_registry.h>
BITCOINQML_REGISTER_QT_TEST(OptionsModelTests)
#else
QTEST_MAIN(OptionsModelTests)
#endif
#include "test_options_model.moc"
