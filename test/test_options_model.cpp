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
#include <chainparams.h>
#include <qml/core_settings.h>
#include <qml/datadir.h>
#include <qml/guiargs.h>
#include <qml/models/core_settings_model.h>
#include <qml/models/onboardingoptionsmodel.h>
#include <qml/models/options_model.h>
#include <qml/models/settings_keys.h>
#include <qml/onboarding_settings.h>
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
    void customDataDirValidationRejectsFile();
    void customDataDirSelectionCreatesDirectoryAndPersists();
    void guiDataDirSettingSoftSetsCustomPath();
    void guiDataDirSettingAbsolutizesSavedRelativePath();
    void guiDataDirSettingSkipsUnusableConfiguredDir();
    void guiDataDirSettingSkipsExplicitDatadir();
    void guiDataDirSettingLeavesDefaultOverridable();
    void guiDataDirChooserShowsForMissingConfiguredDir();
    void guiDataDirChooserShowsForUnwritableConfiguredDir();
    void resetGuiSettingsClearsQSettings();
    void resetGuiSettingsStartsOnboardingFromDefaultDataDir();
    void resetGuiSettingsClearsSettingsJson();
    void resetGuiSettingsPreviewIgnoresSelectedCustomDataDirSettingsJson();
    void resetGuiSettingsApplyClearsSelectedCustomDataDirSettingsJson();
    void resetGuiSettingsPreservesCommandLineOverrides();
    void resetGuiSettingsPreservesBitcoinConfOverrides();
    void resetGuiSettingsExplicitDatadirClearsThatDatadirSettingsJson();
    void onboardingPreviewAppliesParameterInteractions();
    void storageSpaceCheckAcceptsExistingDirectory();
    void storageSpaceCheckRejectsExistingFile();
    void thirdPartyTransactionLinksParseValidUrls();
    void moneyFontChoicePersists();
    void displayUnitUsesQtCompatibleSettingsKey();
    void sharedCoreSettingHelpersDeduplicateOverrides();
    void coreSettingsModelEntryMutatesRuntimeModel();
    void coreSettingsSessionCommandLineSettingDoesNotWrite();
    void coreSettingsSessionDoesNotCopyUntouchedConfig();
    void coreSettingsSessionWritesTouchedConfigOverride();
    void coreSettingsSessionRevertingToDefaultDeletesOverride();
    void coreSettingsSessionProxyDisabledPreservesPreviousValue();
    void coreSettingsSessionPreviewRefreshPreservesTouchedValues();
    void coreSettingsSessionChangeReportsFieldDiffs();
    void coreSettingsSessionProxyCommitAcceptsUnchangedAddressWithoutValueChange();
    void coreSettingStatusTracksSourcePrecedence();
    void commandLineOverriddenSettingDoesNotWrite();
    void revertingToConfigValueDeletesRwOverride();
    void revertingToDefaultValueDeletesRwOverride();
    void languageCommandLineOverrideDoesNotPersist();
    void onboardingPreviewHelperReadsSelectedDatadirConfig();
    void onboardingPreviewReadsSelectedDatadirConfig();
    void onboardingApplyDoesNotCopyUntouchedConfig();
    void onboardingApplyWritesTouchedConfigOverride();
    void onboardingApplyWritesTouchedParameterInteractionOverride();
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

static void InstallPersistentSettings(MockNode& node, ArgsManager& args)
{
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(node, getPersistentSetting(_))
        .WillByDefault(Invoke([&args](const std::string& name) {
            return args.GetPersistentSetting(name);
        }));
}

static void InstallRwSettingsWriter(MockNode& node, ArgsManager& args)
{
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(node, updateRwSetting(_, _))
        .WillByDefault(Invoke([&args](const std::string& name, const common::SettingsValue& value) {
            args.LockSettings([&](common::Settings& settings) {
                if (value.isNull()) {
                    settings.rw_settings.erase(name);
                } else {
                    settings.rw_settings[name] = value;
                }
            });
        }));
}

static std::vector<std::string> TestArgv()
{
    return {std::string{"bitcoinqml"}, std::string{"-regtest"}};
}

static bool PrepareTestArgs(ArgsManager& args, const std::vector<std::string>& argv, std::string& error)
{
    SetupServerArgs(args, /*can_listen_ipc=*/false);
    SetupQmlGuiArgs(args);
    std::vector<const char*> raw_argv;
    raw_argv.reserve(argv.size());
    for (const std::string& arg : argv) raw_argv.push_back(arg.c_str());
    return args.ParseParameters(static_cast<int>(raw_argv.size()), raw_argv.data(), error);
}

class SavedGuiDataDirSettings
{
public:
    SavedGuiDataDirSettings()
    {
        QSettings settings;
        for (const QString& key : settings.allKeys()) {
            m_values.insert(key, settings.value(key));
        }
    }

    ~SavedGuiDataDirSettings()
    {
        QSettings settings;
        settings.clear();
        for (auto it = m_values.cbegin(); it != m_values.cend(); ++it) {
            settings.setValue(it.key(), it.value());
        }
    }

private:
    QVariantMap m_values;
};

class CurrentDirectoryRestorer
{
public:
    CurrentDirectoryRestorer() : m_original{QDir::currentPath()} {}
    ~CurrentDirectoryRestorer() { QDir::setCurrent(m_original); }

private:
    const QString m_original;
};

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

    // Start onboarded with proxy disabled (no saved proxy).
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

void OptionsModelTests::customDataDirValidationRejectsFile()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString file_path = QDir(temp_dir.path()).filePath("not-a-directory");
    QFile file(file_path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    NiceMock<MockNode> node;
    ON_CALL(node, getPersistentSetting(_)).WillByDefault(Return(common::SettingsValue{}));

    OptionsQmlModel model(node);
    QVERIFY(!model.validateCustomDataDir(file_path).isEmpty());
    QVERIFY(!model.selectCustomDataDir(file_path));
}

void OptionsModelTests::customDataDirSelectionCreatesDirectoryAndPersists()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    settings.remove(SettingsKeys::DATA_DIR);
    settings.remove("fReset");

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
    QCOMPARE(settings.value("fReset").toBool(), false);
}

void OptionsModelTests::guiDataDirSettingSoftSetsCustomPath()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString data_dir = QDir(temp_dir.path()).filePath("selected-data-dir");
    settings.setValue(SettingsKeys::DATA_DIR, data_dir);

    ArgsManager args;
    QVERIFY(QmlDataDir::ApplyGuiDataDirSetting(args));
    QVERIFY(args.IsArgSet("-datadir"));
    QCOMPARE(QString::fromStdString(args.GetArg("-datadir", "")), data_dir);
    QVERIFY(QFileInfo(data_dir).isDir());
    QVERIFY(QFileInfo(QDir(data_dir).filePath("wallets")).isDir());
}

void OptionsModelTests::guiDataDirSettingAbsolutizesSavedRelativePath()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    CurrentDirectoryRestorer restore_current_dir;
    QVERIFY(QDir::setCurrent(temp_dir.path()));

    const QString relative_data_dir = QStringLiteral("relative-data-dir");
    const QString data_dir = QDir::current().absoluteFilePath(relative_data_dir);
    settings.setValue(SettingsKeys::DATA_DIR, relative_data_dir);

    QCOMPARE(QmlDataDir::ReadGuiDataDir(), data_dir);

    ArgsManager args;
    QVERIFY(QmlDataDir::ApplyGuiDataDirSetting(args));
    QVERIFY(args.IsArgSet("-datadir"));
    QCOMPARE(QString::fromStdString(args.GetArg("-datadir", "")), data_dir);
    QVERIFY(QFileInfo(data_dir).isDir());
    QVERIFY(QFileInfo(QDir(data_dir).filePath("wallets")).isDir());
}

void OptionsModelTests::guiDataDirSettingSkipsUnusableConfiguredDir()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString file_path = QDir(temp_dir.path()).filePath("not-a-directory");
    QFile file(file_path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    settings.setValue(SettingsKeys::DATA_DIR, file_path);

    ArgsManager args;
    QVERIFY(!QmlDataDir::ApplyGuiDataDirSetting(args));
    QVERIFY(!args.IsArgSet("-datadir"));
}

void OptionsModelTests::guiDataDirSettingSkipsExplicitDatadir()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString saved_data_dir = QDir(temp_dir.path()).filePath("saved-data-dir");
    const QString explicit_data_dir = QDir(temp_dir.path()).filePath("explicit-data-dir");
    settings.setValue(SettingsKeys::DATA_DIR, saved_data_dir);

    ArgsManager args;
    args.ForceSetArg("-datadir", explicit_data_dir.toStdString());
    QVERIFY(!QmlDataDir::ApplyGuiDataDirSetting(args));
    QCOMPARE(QString::fromStdString(args.GetArg("-datadir", "")), explicit_data_dir);
    QVERIFY(!QFileInfo(saved_data_dir).exists());
}

void OptionsModelTests::guiDataDirSettingLeavesDefaultOverridable()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    settings.setValue(SettingsKeys::DATA_DIR, QmlDataDir::DefaultDataDirString());

    ArgsManager args;
    QVERIFY(!QmlDataDir::ApplyGuiDataDirSetting(args));
    QVERIFY(!args.IsArgSet("-datadir"));
}

void OptionsModelTests::guiDataDirChooserShowsForMissingConfiguredDir()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString missing_data_dir = QDir(temp_dir.path()).filePath("missing-data-dir");
    settings.setValue(SettingsKeys::DATA_DIR, missing_data_dir);
    QVERIFY(!QFileInfo::exists(missing_data_dir));

    ArgsManager args;
    QVERIFY(QmlDataDir::ShouldShowDataDirChooser(args));

    args.ForceSetArg("-datadir", QDir(temp_dir.path()).filePath("explicit-data-dir").toStdString());
    QVERIFY(!QmlDataDir::ShouldShowDataDirChooser(args));
}

void OptionsModelTests::guiDataDirChooserShowsForUnwritableConfiguredDir()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString data_dir = QDir(temp_dir.path()).filePath("unwritable-data-dir");
    QVERIFY(QDir().mkpath(data_dir));
    settings.setValue(SettingsKeys::DATA_DIR, data_dir);

    const QFileDevice::Permissions original_permissions = QFileInfo(data_dir).permissions();
    QVERIFY(QFile(data_dir).setPermissions(QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    if (QFileInfo(data_dir).isWritable()) {
        QVERIFY(QFile(data_dir).setPermissions(original_permissions));
        QSKIP("Cannot make temporary data directory unwritable on this platform.");
    }

    ArgsManager args;
    const bool should_show = QmlDataDir::ShouldShowDataDirChooser(args);

    args.ForceSetArg("-datadir", QDir(temp_dir.path()).filePath("explicit-data-dir").toStdString());
    const bool explicit_datadir_should_show = QmlDataDir::ShouldShowDataDirChooser(args);

    QVERIFY(QFile(data_dir).setPermissions(original_permissions));
    QVERIFY(should_show);
    QVERIFY(!explicit_datadir_should_show);
}

void OptionsModelTests::resetGuiSettingsClearsQSettings()
{
    SavedGuiDataDirSettings saved_settings;
    QSettings settings;
    settings.setValue(SettingsKeys::DATA_DIR, QStringLiteral("/tmp/old-bitcoin-data"));
    settings.setValue(SettingsKeys::LANGUAGE, QStringLiteral("de"));
    settings.setValue(SettingsKeys::DISPLAY_UNIT, 3);
    settings.setValue(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS, QStringLiteral("https://example.com/%s"));
    settings.setValue(SettingsKeys::MONEY_FONT_CHOICE, QStringLiteral("best_system"));
    settings.setValue("fReset", true);

    std::vector<std::string> argv = TestArgv();
    argv.emplace_back("-settings=");
    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());

    QString reset_error;
    QVERIFY2(QmlDataDir::ResetGuiSettings(args, &reset_error), qPrintable(reset_error));

    QVERIFY(!settings.contains(SettingsKeys::DATA_DIR));
    QVERIFY(!settings.contains(SettingsKeys::LANGUAGE));
    QVERIFY(!settings.contains(SettingsKeys::DISPLAY_UNIT));
    QVERIFY(!settings.contains(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS));
    QVERIFY(!settings.contains(SettingsKeys::MONEY_FONT_CHOICE));
    QCOMPARE(settings.value("fReset").toBool(), false);
}

void OptionsModelTests::resetGuiSettingsStartsOnboardingFromDefaultDataDir()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir old_data_dir;
    QVERIFY(old_data_dir.isValid());
    QSettings settings;
    settings.setValue(SettingsKeys::DATA_DIR, old_data_dir.path());

    std::vector<std::string> argv = TestArgv();
    argv.emplace_back("-settings=");
    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());

    QString reset_error;
    QVERIFY2(QmlDataDir::ResetGuiSettings(args, &reset_error), qPrintable(reset_error));

    OnboardingOptionsModel model(argv, /*can_listen_ipc=*/false);
    QCOMPARE(model.dataDir(), QmlDataDir::DefaultDataDirString());
    QVERIFY(model.getCustomDataDirString().isEmpty());
}

void OptionsModelTests::resetGuiSettingsClearsSettingsJson()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());
    QVERIFY(QDir(data_dir.path()).mkpath(QStringLiteral("regtest")));

    const std::vector<std::string> argv{
        std::string{"bitcoinqml"},
        std::string{"-regtest"},
        "-datadir=" + data_dir.path().toStdString(),
    };

    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());
    SelectParams(args.GetChainType());
    args.SelectConfigNetwork(args.GetChainTypeString());
    args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["prune"] = MakeInt(2048);
        settings.rw_settings["proxy"] = common::SettingsValue{std::string{"10.0.0.1:9050"}};
        settings.rw_settings["onion"] = common::SettingsValue{std::string{"127.0.0.1:9150"}};
    });
    std::vector<std::string> settings_errors;
    QVERIFY2(args.WriteSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());

    QString reset_error;
    QVERIFY2(QmlDataDir::ResetGuiSettings(args, &reset_error), qPrintable(reset_error));

    fs::path backup_path;
    QVERIFY(args.GetSettingsPath(&backup_path, /*temp=*/false, /*backup=*/true));
    QVERIFY(fs::exists(backup_path));

    ArgsManager check_args;
    QVERIFY2(PrepareTestArgs(check_args, argv, parse_error), parse_error.c_str());
    SelectParams(check_args.GetChainType());
    check_args.SelectConfigNetwork(check_args.GetChainTypeString());
    QVERIFY2(check_args.ReadSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());
    check_args.LockSettings([](common::Settings& settings) {
        QVERIFY(settings.rw_settings.count("prune") == 0);
        QVERIFY(settings.rw_settings.count("proxy") == 0);
        QVERIFY(settings.rw_settings.count("onion") == 0);
    });
}

void OptionsModelTests::resetGuiSettingsPreviewIgnoresSelectedCustomDataDirSettingsJson()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());
    QVERIFY(QDir(data_dir.path()).mkpath(QStringLiteral("regtest")));

    const std::vector<std::string> write_argv{
        std::string{"bitcoinqml"},
        std::string{"-regtest"},
        "-datadir=" + data_dir.path().toStdString(),
    };
    ArgsManager write_args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(write_args, write_argv, parse_error), parse_error.c_str());
    SelectParams(write_args.GetChainType());
    write_args.SelectConfigNetwork(write_args.GetChainTypeString());
    write_args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["listen"] = common::SettingsValue{false};
        settings.rw_settings["natpmp"] = common::SettingsValue{true};
        settings.rw_settings["server"] = common::SettingsValue{true};
        settings.rw_settings["proxy"] = common::SettingsValue{std::string{"10.0.0.1:9050"}};
        settings.rw_settings["onion"] = common::SettingsValue{std::string{"127.0.0.1:9150"}};
    });
    std::vector<std::string> settings_errors;
    QVERIFY2(write_args.WriteSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());

    std::vector<std::string> preview_argv = TestArgv();
    preview_argv.emplace_back("-resetguisettings");
    const QmlOnboardingSettings::PreviewResult preview{
        QmlOnboardingSettings::Preview(preview_argv, /*can_listen_ipc=*/false, data_dir.path())
    };
    QVERIFY2(preview.ok, qPrintable(preview.error));
    QVERIFY(preview.values.listen);
    QVERIFY(!preview.values.natpmp);
    QVERIFY(!preview.values.server);
    QVERIFY(!preview.values.proxy_enabled);
    QVERIFY(!preview.values.tor_enabled);
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("proxy")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("default"));
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("server")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("default"));
}

void OptionsModelTests::resetGuiSettingsApplyClearsSelectedCustomDataDirSettingsJson()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());
    QVERIFY(QDir(data_dir.path()).mkpath(QStringLiteral("regtest")));

    const std::vector<std::string> write_argv{
        std::string{"bitcoinqml"},
        std::string{"-regtest"},
        "-datadir=" + data_dir.path().toStdString(),
    };
    ArgsManager write_args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(write_args, write_argv, parse_error), parse_error.c_str());
    SelectParams(write_args.GetChainType());
    write_args.SelectConfigNetwork(write_args.GetChainTypeString());
    write_args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["listen"] = common::SettingsValue{false};
        settings.rw_settings["natpmp"] = common::SettingsValue{true};
        settings.rw_settings["server"] = common::SettingsValue{true};
        settings.rw_settings["proxy"] = common::SettingsValue{std::string{"10.0.0.1:9050"}};
    });
    std::vector<std::string> settings_errors;
    QVERIFY2(write_args.WriteSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());

    std::vector<std::string> argv = TestArgv();
    argv.emplace_back("-resetguisettings");
    OnboardingOptionsModel model(argv, /*can_listen_ipc=*/false);
    QVERIFY(model.selectCustomDataDir(data_dir.path()));
    QCOMPARE(model.previewError(), QString{});
    QVERIFY(model.listen());
    QVERIFY(!model.natpmp());
    QVERIFY(!model.server());
    QVERIFY(!model.proxyEnabled());

    ArgsManager apply_args;
    QVERIFY2(PrepareTestArgs(apply_args, argv, parse_error), parse_error.c_str());
    QString apply_error;
    QVERIFY2(model.applyToArgs(apply_args, &apply_error), qPrintable(apply_error));

    fs::path backup_path;
    QVERIFY(apply_args.GetSettingsPath(&backup_path, /*temp=*/false, /*backup=*/true));
    QVERIFY(fs::exists(backup_path));

    ArgsManager check_args;
    QVERIFY2(PrepareTestArgs(check_args, write_argv, parse_error), parse_error.c_str());
    SelectParams(check_args.GetChainType());
    check_args.SelectConfigNetwork(check_args.GetChainTypeString());
    QVERIFY2(check_args.ReadSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());
    check_args.LockSettings([](common::Settings& settings) {
        QVERIFY(settings.rw_settings.count("listen") == 0);
        QVERIFY(settings.rw_settings.count("natpmp") == 0);
        QVERIFY(settings.rw_settings.count("server") == 0);
        QVERIFY(settings.rw_settings.count("proxy") == 0);
    });
}

void OptionsModelTests::resetGuiSettingsPreservesCommandLineOverrides()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());
    QVERIFY(QDir(data_dir.path()).mkpath(QStringLiteral("regtest")));

    const std::vector<std::string> argv{
        std::string{"bitcoinqml"},
        std::string{"-regtest"},
        "-datadir=" + data_dir.path().toStdString(),
        std::string{"-proxy=10.0.0.2:9050"},
        std::string{"-prune=2048"},
    };

    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());
    SelectParams(args.GetChainType());
    args.SelectConfigNetwork(args.GetChainTypeString());
    args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["proxy"] = common::SettingsValue{std::string{"10.0.0.1:9050"}};
        settings.rw_settings["prune"] = MakeInt(4096);
    });
    std::vector<std::string> settings_errors;
    QVERIFY2(args.WriteSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());

    QString reset_error;
    QVERIFY2(QmlDataDir::ResetGuiSettings(args, &reset_error), qPrintable(reset_error));

    const QmlOnboardingSettings::PreviewResult preview{
        QmlOnboardingSettings::Preview(argv, /*can_listen_ipc=*/false, QmlDataDir::DefaultDataDirString())
    };
    QVERIFY2(preview.ok, qPrintable(preview.error));
    QVERIFY(preview.values.proxy_enabled);
    QCOMPARE(preview.values.proxy_address, QStringLiteral("10.0.0.2:9050"));
    QVERIFY(preview.values.prune);
    QCOMPARE(preview.values.prune_size_gb, QmlCoreSettings::PruneMiBToGB(2048));
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("proxy")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("command_line"));
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("prune")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("command_line"));
}

void OptionsModelTests::resetGuiSettingsPreservesBitcoinConfOverrides()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());
    QVERIFY(QDir(data_dir.path()).mkpath(QStringLiteral("regtest")));

    QFile conf(QDir(data_dir.path()).filePath(QStringLiteral("bitcoin.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(conf.write("regtest=1\n[regtest]\nserver=1\nproxy=10.0.0.3:9050\n") > 0);
    conf.close();

    const std::vector<std::string> write_argv{
        std::string{"bitcoinqml"},
        std::string{"-regtest"},
        "-datadir=" + data_dir.path().toStdString(),
    };
    ArgsManager write_args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(write_args, write_argv, parse_error), parse_error.c_str());
    SelectParams(write_args.GetChainType());
    write_args.SelectConfigNetwork(write_args.GetChainTypeString());
    write_args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["server"] = common::SettingsValue{false};
        settings.rw_settings["proxy"] = common::SettingsValue{std::string{"10.0.0.1:9050"}};
    });
    std::vector<std::string> settings_errors;
    QVERIFY2(write_args.WriteSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());

    std::vector<std::string> preview_argv = TestArgv();
    preview_argv.emplace_back("-resetguisettings");
    const QmlOnboardingSettings::PreviewResult preview{
        QmlOnboardingSettings::Preview(preview_argv, /*can_listen_ipc=*/false, data_dir.path())
    };
    QVERIFY2(preview.ok, qPrintable(preview.error));
    QVERIFY(preview.values.server);
    QVERIFY(preview.values.proxy_enabled);
    QCOMPARE(preview.values.proxy_address, QStringLiteral("10.0.0.3:9050"));
    QVERIFY(!preview.values.listen);
    QVERIFY(!preview.values.natpmp);
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("server")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("bitcoin_conf"));
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("proxy")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("bitcoin_conf"));
}

void OptionsModelTests::resetGuiSettingsExplicitDatadirClearsThatDatadirSettingsJson()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());
    QVERIFY(QDir(data_dir.path()).mkpath(QStringLiteral("regtest")));

    const std::vector<std::string> argv{
        std::string{"bitcoinqml"},
        std::string{"-regtest"},
        "-datadir=" + data_dir.path().toStdString(),
        std::string{"-resetguisettings"},
    };

    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());
    SelectParams(args.GetChainType());
    args.SelectConfigNetwork(args.GetChainTypeString());
    args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["server"] = common::SettingsValue{true};
        settings.rw_settings["proxy"] = common::SettingsValue{std::string{"10.0.0.1:9050"}};
    });
    std::vector<std::string> settings_errors;
    QVERIFY2(args.WriteSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());

    QString reset_error;
    QVERIFY2(QmlDataDir::ResetGuiSettings(args, &reset_error), qPrintable(reset_error));

    ArgsManager check_args;
    QVERIFY2(PrepareTestArgs(check_args, argv, parse_error), parse_error.c_str());
    SelectParams(check_args.GetChainType());
    check_args.SelectConfigNetwork(check_args.GetChainTypeString());
    QVERIFY2(check_args.ReadSettingsFile(&settings_errors), settings_errors.empty() ? "" : settings_errors.front().c_str());
    check_args.LockSettings([](common::Settings& settings) {
        QVERIFY(settings.rw_settings.count("server") == 0);
        QVERIFY(settings.rw_settings.count("proxy") == 0);
    });
}

void OptionsModelTests::onboardingPreviewAppliesParameterInteractions()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());

    QFile conf(QDir(data_dir.path()).filePath(QStringLiteral("bitcoin.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(conf.write("regtest=1\n[regtest]\nproxy=10.0.0.3:9050\n") > 0);
    conf.close();

    const QmlOnboardingSettings::PreviewResult preview{
        QmlOnboardingSettings::Preview(TestArgv(), /*can_listen_ipc=*/false, data_dir.path())
    };
    QVERIFY2(preview.ok, qPrintable(preview.error));
    QVERIFY(preview.values.proxy_enabled);
    QCOMPARE(preview.values.proxy_address, QStringLiteral("10.0.0.3:9050"));
    QVERIFY(!preview.values.listen);
    QVERIFY(!preview.values.natpmp);
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("proxy")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("bitcoin_conf"));
}

void OptionsModelTests::storageSpaceCheckAcceptsExistingDirectory()
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    const QmlDataDir::StorageSpaceResult result = QmlDataDir::CheckStorageSpace(temp_dir.path());
    QVERIFY(result.ok);
    QVERIFY(result.path_exists);
    QVERIFY(result.path_is_directory);
    QVERIFY(!result.checked_path.isEmpty());
    QVERIFY(result.capacity_bytes >= result.available_bytes);
}

void OptionsModelTests::storageSpaceCheckRejectsExistingFile()
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString file_path = QDir(temp_dir.path()).filePath("not-a-directory");
    QFile file(file_path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    const QmlDataDir::StorageSpaceResult result = QmlDataDir::CheckStorageSpace(file_path);
    QVERIFY(!result.ok);
    QVERIFY(result.path_exists);
    QVERIFY(!result.path_is_directory);
    QVERIFY(!result.message.isEmpty());
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

void OptionsModelTests::sharedCoreSettingHelpersDeduplicateOverrides()
{
    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.ro_config[""]["listen"].push_back(common::SettingsValue{false});
    });

    QVariantMap status = QmlCoreSettings::CoreSettingStatus(args, QStringLiteral("listen"));
    QCOMPARE(status.value("source").toString(), QString("bitcoin_conf"));
    QCOMPARE(status.value("canEdit").toBool(), true);

    QVERIFY(QmlCoreSettings::WriteCoreSettingOverride(args, QStringLiteral("listen"), common::SettingsValue{false}));
    bool has_listen_override{true};
    args.LockSettings([&](common::Settings& settings) {
        has_listen_override = settings.rw_settings.count("listen") > 0;
    });
    QVERIFY(!has_listen_override);

    QVERIFY(QmlCoreSettings::WriteCoreSettingOverride(args, QStringLiteral("listen"), common::SettingsValue{true}));
    common::SettingsValue listen_override;
    args.LockSettings([&](common::Settings& settings) {
        listen_override = settings.rw_settings.at("listen");
    });
    QVERIFY(listen_override.isBool());
    QVERIFY(listen_override.get_bool());

    QVERIFY(QmlCoreSettings::WriteCoreSettingOverride(args, QStringLiteral("maxmempool"), common::SettingsValue{DEFAULT_MAX_MEMPOOL_SIZE_MB}));
    bool has_mempool_override{true};
    args.LockSettings([&](common::Settings& settings) {
        has_mempool_override = settings.rw_settings.count("maxmempool") > 0;
    });
    QVERIFY(!has_mempool_override);
}

void OptionsModelTests::coreSettingsModelEntryMutatesRuntimeModel()
{
    using ::testing::NiceMock;

    ArgsManager args;
    std::string error;
    QVERIFY(PrepareTestArgs(args, TestArgv(), error));

    NiceMock<MockNode> node;
    InstallPersistentSettings(node, args);
    InstallRwSettingsWriter(node, args);

    OptionsQmlModel model(node, args);
    auto* core_settings = qobject_cast<CoreSettingsModel*>(model.coreSettings());
    QVERIFY(core_settings);
    auto* listen_entry = qobject_cast<CoreSettingEntryModel*>(core_settings->entry(QStringLiteral("listen")));
    QVERIFY(listen_entry);

    QVERIFY(model.listen());
    QVERIFY(!model.connectionSettingsDirty());

    listen_entry->setValue(false);

    QVERIFY(!model.listen());
    QVERIFY(model.connectionSettingsDirty());
    QCOMPARE(SettingToBool(args.GetPersistentSetting("listen")), false);
}

void OptionsModelTests::coreSettingsSessionCommandLineSettingDoesNotWrite()
{
    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.command_line_options["listen"].push_back(common::SettingsValue{true});
    });

    QmlCoreSettings::Values values;
    values.listen = false;
    QmlCoreSettings::Session session{values, QmlCoreSettings::BuildCoreSettingStatuses(args, QmlCoreSettings::OnboardingCoreSettingNames())};

    QVERIFY(!session.canEdit(QStringLiteral("listen")));
    QVERIFY(!session.setListen(true));
    session.markTouched(QStringLiteral("listen"));
    QVERIFY(!session.writeTouchedToArgs(args));

    bool has_listen_override{false};
    args.LockSettings([&](common::Settings& settings) {
        has_listen_override = settings.rw_settings.count("listen") > 0;
    });
    QVERIFY(!has_listen_override);
}

void OptionsModelTests::coreSettingsSessionDoesNotCopyUntouchedConfig()
{
    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.ro_config[""]["listen"].push_back(common::SettingsValue{false});
    });

    QmlCoreSettings::Session session{
        QmlCoreSettings::Values{.listen = false},
        QmlCoreSettings::BuildCoreSettingStatuses(args, QmlCoreSettings::OnboardingCoreSettingNames())
    };
    QVERIFY(session.writeTouchedToArgs(args));

    bool has_listen_override{false};
    args.LockSettings([&](common::Settings& settings) {
        has_listen_override = settings.rw_settings.count("listen") > 0;
    });
    QVERIFY(!has_listen_override);
}

void OptionsModelTests::coreSettingsSessionWritesTouchedConfigOverride()
{
    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.ro_config[""]["listen"].push_back(common::SettingsValue{false});
    });

    QmlCoreSettings::Session session{
        QmlCoreSettings::Values{.listen = true},
        QmlCoreSettings::BuildCoreSettingStatuses(args, QmlCoreSettings::OnboardingCoreSettingNames())
    };
    session.markTouched(QStringLiteral("listen"));
    QVERIFY(session.writeTouchedToArgs(args));

    common::SettingsValue listen_override;
    args.LockSettings([&](common::Settings& settings) {
        listen_override = settings.rw_settings.at("listen");
    });
    QVERIFY(listen_override.isBool());
    QVERIFY(listen_override.get_bool());
}

void OptionsModelTests::coreSettingsSessionRevertingToDefaultDeletesOverride()
{
    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["listen"] = common::SettingsValue{false};
    });

    QmlCoreSettings::Values values;
    values.listen = DEFAULT_LISTEN;
    QmlCoreSettings::Session session{values, QmlCoreSettings::BuildCoreSettingStatuses(args, QmlCoreSettings::OnboardingCoreSettingNames())};
    session.markTouched(QStringLiteral("listen"));
    QVERIFY(session.writeTouchedToArgs(args));

    bool has_listen_override{true};
    args.LockSettings([&](common::Settings& settings) {
        has_listen_override = settings.rw_settings.count("listen") > 0;
    });
    QVERIFY(!has_listen_override);
}

void OptionsModelTests::coreSettingsSessionProxyDisabledPreservesPreviousValue()
{
    ArgsManager args;
    args.SelectConfigNetwork("main");

    QmlCoreSettings::Values values;
    values.proxy_enabled = false;
    values.proxy_address = QStringLiteral("10.0.0.2:9050");
    QmlCoreSettings::Session session{values, QmlCoreSettings::BuildCoreSettingStatuses(args, QmlCoreSettings::OnboardingCoreSettingNames())};
    session.markTouched(QStringLiteral("proxy"));
    QVERIFY(session.writeTouchedToArgs(args));

    common::SettingsValue proxy_prev;
    bool has_proxy_override{true};
    args.LockSettings([&](common::Settings& settings) {
        proxy_prev = settings.rw_settings.at("proxy-prev");
        has_proxy_override = settings.rw_settings.count("proxy") > 0;
    });
    QCOMPARE(QString::fromStdString(proxy_prev.get_str()), QStringLiteral("10.0.0.2:9050"));
    QVERIFY(!has_proxy_override);
}

void OptionsModelTests::coreSettingsSessionPreviewRefreshPreservesTouchedValues()
{
    QmlCoreSettings::Values initial;
    initial.listen = true;
    initial.natpmp = false;
    QmlCoreSettings::Session session{initial};

    QVERIFY(session.setListen(false));

    QmlCoreSettings::Values preview;
    preview.listen = true;
    preview.natpmp = true;
    session.applyPreviewValuesPreservingTouched(preview, {});

    QVERIFY(!session.values().listen);
    QVERIFY(session.values().natpmp);
    QVERIFY(session.isTouched(QStringLiteral("listen")));
    QVERIFY(!session.isTouched(QStringLiteral("natpmp")));
}

void OptionsModelTests::coreSettingsSessionChangeReportsFieldDiffs()
{
    QmlCoreSettings::Session session;

    const QmlCoreSettings::Change change = session.changeListen(false);

    QVERIFY(change.accepted);
    QCOMPARE(change.setting_name, QStringLiteral("listen"));
    QVERIFY(QmlCoreSettings::ValuesChanged(change));
    QVERIFY(QmlCoreSettings::ListenChanged(change));
    QVERIFY(!QmlCoreSettings::NatpmpChanged(change));
    QVERIFY(!QmlCoreSettings::StatusesChanged(change));
    QVERIFY(session.isTouched(QStringLiteral("listen")));
}

void OptionsModelTests::coreSettingsSessionProxyCommitAcceptsUnchangedAddressWithoutValueChange()
{
    QmlCoreSettings::Session session;
    const QString address = session.defaultProxyAddress();

    const QmlCoreSettings::Change change = session.changeProxyLocation(address);

    QVERIFY(change.accepted);
    QCOMPARE(change.setting_name, QStringLiteral("proxy"));
    QVERIFY(!QmlCoreSettings::ValuesChanged(change));
    QVERIFY(!QmlCoreSettings::ProxyAddressChanged(change));
    QVERIFY(!session.isTouched(QStringLiteral("proxy")));
}

void OptionsModelTests::coreSettingStatusTracksSourcePrecedence()
{
    using ::testing::NiceMock;

    {
        ArgsManager args;
        args.SelectConfigNetwork("main");
        args.LockSettings([](common::Settings& settings) {
            settings.ro_config[""]["listen"].push_back(common::SettingsValue{true});
        });

        NiceMock<MockNode> node;
        InstallPersistentSettings(node, args);

        OptionsQmlModel model(node, args);
        const QVariantMap status = model.coreSettingStatus(QStringLiteral("listen"));
        QCOMPARE(status.value("source").toString(), QString("bitcoin_conf"));
        QCOMPARE(status.value("canEdit").toBool(), true);
        QCOMPARE(status.value("createsGuiOverride").toBool(), true);
        QVERIFY(status.value("infoText").toString().contains("settings.json"));
    }

    {
        ArgsManager args;
        args.SelectConfigNetwork("main");
        args.LockSettings([](common::Settings& settings) {
            settings.ro_config[""]["listen"].push_back(common::SettingsValue{true});
            settings.rw_settings["listen"] = common::SettingsValue{false};
        });

        NiceMock<MockNode> node;
        InstallPersistentSettings(node, args);

        OptionsQmlModel model(node, args);
        const QVariantMap status = model.coreSettingStatus(QStringLiteral("listen"));
        QCOMPARE(status.value("source").toString(), QString("settings_json"));
        QCOMPARE(status.value("createsGuiOverride").toBool(), false);
    }

    {
        ArgsManager args;
        args.SelectConfigNetwork("main");
        args.LockSettings([](common::Settings& settings) {
            settings.ro_config[""]["listen"].push_back(common::SettingsValue{true});
            settings.rw_settings["listen"] = common::SettingsValue{false};
            settings.command_line_options["listen"].push_back(common::SettingsValue{true});
        });

        NiceMock<MockNode> node;
        InstallPersistentSettings(node, args);

        OptionsQmlModel model(node, args);
        const QVariantMap status = model.coreSettingStatus(QStringLiteral("listen"));
        QCOMPARE(status.value("source").toString(), QString("command_line"));
        QCOMPARE(status.value("canEdit").toBool(), false);
        QCOMPARE(status.value("commandLineOverridden").toBool(), true);
        QVERIFY(status.value("infoText").toString().contains("-listen"));
    }
}

void OptionsModelTests::commandLineOverriddenSettingDoesNotWrite()
{
    using ::testing::_;
    using ::testing::NiceMock;

    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.command_line_options["listen"].push_back(common::SettingsValue{false});
        settings.ro_config[""]["listen"].push_back(common::SettingsValue{true});
    });

    NiceMock<MockNode> node;
    InstallPersistentSettings(node, args);

    OptionsQmlModel model(node, args);
    QVERIFY(!model.listen());
    QCOMPARE(model.coreSettingStatus(QStringLiteral("listen")).value("source").toString(), QString("command_line"));
    QCOMPARE(model.coreSettingStatus(QStringLiteral("listen")).value("canEdit").toBool(), false);

    EXPECT_CALL(node, updateRwSetting(std::string{"listen"}, _)).Times(0);
    model.setListen(true);

    QVERIFY(!model.listen());
}

void OptionsModelTests::revertingToConfigValueDeletesRwOverride()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Truly;

    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.ro_config[""]["listen"].push_back(common::SettingsValue{true});
        settings.rw_settings["listen"] = common::SettingsValue{false};
    });

    NiceMock<MockNode> node;
    InstallPersistentSettings(node, args);
    InstallRwSettingsWriter(node, args);

    OptionsQmlModel model(node, args);
    QVERIFY(!model.listen());

    EXPECT_CALL(node, updateRwSetting(std::string{"listen"},
        Truly([](const common::SettingsValue& value) { return value.isNull(); })));

    model.setListen(true);
}

void OptionsModelTests::revertingToDefaultValueDeletesRwOverride()
{
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Truly;

    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.rw_settings["maxmempool"] = common::SettingsValue{456};
    });

    NiceMock<MockNode> node;
    InstallPersistentSettings(node, args);
    InstallRwSettingsWriter(node, args);

    OptionsQmlModel model(node, args);
    QCOMPARE(model.maxMempoolSizeMB(), 456);

    EXPECT_CALL(node, updateRwSetting(std::string{"maxmempool"},
        Truly([](const common::SettingsValue& value) { return value.isNull(); })));

    model.setMaxMempoolSizeMB(DEFAULT_MAX_MEMPOOL_SIZE_MB);
}

void OptionsModelTests::languageCommandLineOverrideDoesNotPersist()
{
    using ::testing::_;
    using ::testing::NiceMock;

    QSettings settings;
    const bool had_language_setting = settings.contains(SettingsKeys::LANGUAGE);
    const QVariant previous_language_setting = settings.value(SettingsKeys::LANGUAGE);
    settings.setValue(SettingsKeys::LANGUAGE, QStringLiteral("de"));

    ArgsManager args;
    args.SelectConfigNetwork("main");
    args.LockSettings([](common::Settings& settings) {
        settings.command_line_options["lang"].push_back(common::SettingsValue{std::string{"fr"}});
    });

    NiceMock<MockNode> node;
    InstallPersistentSettings(node, args);

    OptionsQmlModel model(node, args);
    QCOMPARE(model.language(), QString("fr"));
    QCOMPARE(model.coreSettingStatus(QStringLiteral("lang")).value("canEdit").toBool(), false);

    model.setLanguage(QStringLiteral("es"));
    QCOMPARE(model.language(), QString("fr"));
    QCOMPARE(settings.value(SettingsKeys::LANGUAGE).toString(), QString("de"));

    if (had_language_setting) {
        settings.setValue(SettingsKeys::LANGUAGE, previous_language_setting);
    } else {
        settings.remove(SettingsKeys::LANGUAGE);
    }
}

void OptionsModelTests::onboardingPreviewHelperReadsSelectedDatadirConfig()
{
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());

    QFile conf(data_dir.filePath(QStringLiteral("bitcoin.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(conf.write("listen=0\n") > 0);
    conf.close();

    const QmlOnboardingSettings::PreviewResult preview{
        QmlOnboardingSettings::Preview(TestArgv(), /*can_listen_ipc=*/false, data_dir.path())
    };
    QVERIFY2(preview.ok, qPrintable(preview.error));
    QCOMPARE(preview.core_setting_statuses.value(QStringLiteral("listen")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("bitcoin_conf"));
    QVERIFY(!preview.values.listen);
}

void OptionsModelTests::onboardingPreviewReadsSelectedDatadirConfig()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());

    QFile conf(data_dir.filePath(QStringLiteral("bitcoin.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(conf.write("listen=0\n") > 0);
    conf.close();

    OnboardingOptionsModel model(TestArgv(), /*can_listen_ipc=*/false);
    QVERIFY(model.selectCustomDataDir(data_dir.path()));
    QCOMPARE(model.previewError(), QString{});
    QCOMPARE(model.coreSettingStatuses().value(QStringLiteral("listen")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("bitcoin_conf"));
    QVERIFY(!model.listen());
}

void OptionsModelTests::onboardingApplyDoesNotCopyUntouchedConfig()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());

    QFile conf(data_dir.filePath(QStringLiteral("bitcoin.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(conf.write("listen=0\n") > 0);
    conf.close();

    const std::vector<std::string> argv = TestArgv();
    OnboardingOptionsModel model(argv, /*can_listen_ipc=*/false);
    QVERIFY(model.selectCustomDataDir(data_dir.path()));
    QVERIFY(!model.listen());

    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());
    QString apply_error;
    QVERIFY2(model.applyToArgs(args, &apply_error), qPrintable(apply_error));

    bool has_listen_override{false};
    args.LockSettings([&](common::Settings& settings) {
        has_listen_override = settings.rw_settings.count("listen") > 0;
    });
    QVERIFY(!has_listen_override);
}

void OptionsModelTests::onboardingApplyWritesTouchedConfigOverride()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());

    QFile conf(data_dir.filePath(QStringLiteral("bitcoin.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(conf.write("listen=0\n") > 0);
    conf.close();

    const std::vector<std::string> argv = TestArgv();
    OnboardingOptionsModel model(argv, /*can_listen_ipc=*/false);
    QVERIFY(model.selectCustomDataDir(data_dir.path()));
    QVERIFY(!model.listen());
    model.setListen(true);
    QVERIFY(model.listen());

    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());
    QString apply_error;
    QVERIFY2(model.applyToArgs(args, &apply_error), qPrintable(apply_error));

    common::SettingsValue listen_override;
    args.LockSettings([&](common::Settings& settings) {
        if (const auto* value = common::FindKey(settings.rw_settings, "listen")) {
            listen_override = *value;
        }
    });
    QVERIFY(listen_override.isBool());
    QVERIFY(listen_override.get_bool());
}

void OptionsModelTests::onboardingApplyWritesTouchedParameterInteractionOverride()
{
    SavedGuiDataDirSettings saved_settings;
    QTemporaryDir data_dir;
    QVERIFY(data_dir.isValid());

    QFile conf(data_dir.filePath(QStringLiteral("bitcoin.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(conf.write("regtest=1\n[regtest]\nproxy=10.0.0.3:9050\n") > 0);
    conf.close();

    const std::vector<std::string> argv = TestArgv();
    OnboardingOptionsModel model(argv, /*can_listen_ipc=*/false);
    QVERIFY(model.selectCustomDataDir(data_dir.path()));
    QVERIFY(model.proxyEnabled());
    QVERIFY(!model.listen());

    const QVariantMap listen_status = model.coreSettingStatuses().value(QStringLiteral("listen")).toMap();
    QCOMPARE(listen_status.value(QStringLiteral("source")).toString(), QStringLiteral("startup"));
    QCOMPARE(listen_status.value(QStringLiteral("canEdit")).toBool(), true);

    model.setListen(true);
    QVERIFY(model.listen());

    ArgsManager args;
    std::string parse_error;
    QVERIFY2(PrepareTestArgs(args, argv, parse_error), parse_error.c_str());
    QString apply_error;
    QVERIFY2(model.applyToArgs(args, &apply_error), qPrintable(apply_error));

    common::SettingsValue listen_override;
    bool has_forced_listen{true};
    bool has_forced_natpmp{true};
    bool has_forced_discover{true};
    args.LockSettings([&](common::Settings& settings) {
        if (const auto* value = common::FindKey(settings.rw_settings, "listen")) {
            listen_override = *value;
        }
        has_forced_listen = settings.forced_settings.count("listen") > 0;
        has_forced_natpmp = settings.forced_settings.count("natpmp") > 0;
        has_forced_discover = settings.forced_settings.count("discover") > 0;
    });

    QVERIFY(listen_override.isBool());
    QVERIFY(listen_override.get_bool());
    QVERIFY(!has_forced_listen);
    QVERIFY(!has_forced_natpmp);
    QVERIFY(!has_forced_discover);
}

#ifdef BITCOINQML_NO_TEST_MAIN
#include <test/qt_test_registry.h>
BITCOINQML_REGISTER_QT_TEST(OptionsModelTests)
#else
QTEST_MAIN(OptionsModelTests)
#endif
#include "test_options_model.moc"
