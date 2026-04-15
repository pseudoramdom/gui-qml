// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtTest/QtTest>

#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <qml/walletqmlcontroller.h>
#include <scheduler.h>
#include <test/mocks/mocknode.h>
#include <util/translation.h>
#include <wallet/walletutil.h>

#include <gmock/gmock.h>

namespace {
constexpr auto NOT_INITIALIZED_ERROR{"Wallets are still loading. Try again in a moment."};

std::unique_ptr<interfaces::Handler> MakeNoopHandler()
{
    return interfaces::MakeCleanupHandler([] {});
}

class FakeWalletLoader : public interfaces::WalletLoader
{
public:
    int create_wallet_calls{0};
    int migrate_wallet_calls{0};
    int handle_load_wallet_calls{0};
    int get_wallets_calls{0};
    int list_wallet_dir_calls{0};

    std::function<util::Result<std::unique_ptr<interfaces::Wallet>>(const std::string&, const SecureString&, uint64_t, std::vector<bilingual_str>&)>
        create_wallet_fn = [](const std::string&, const SecureString&, uint64_t, std::vector<bilingual_str>&) {
            return util::Error{Untranslated("Unexpected createWallet call")};
        };
    std::function<util::Result<interfaces::WalletMigrationResult>(const std::string&, const SecureString&)>
        migrate_wallet_fn = [](const std::string&, const SecureString&) {
            return util::Error{Untranslated("Unexpected migrateWallet call")};
        };
    std::function<std::vector<std::unique_ptr<interfaces::Wallet>>()> get_wallets_fn = [] {
        return std::vector<std::unique_ptr<interfaces::Wallet>>{};
    };
    std::vector<std::pair<std::string, std::string>> wallet_dir_entries;

    void registerRpcs() override {}
    bool verify() override { return true; }
    bool load() override { return true; }
    void start(CScheduler&) override {}
    void stop() override {}
    void setMockTime(int64_t) override {}
    void schedulerMockForward(std::chrono::seconds) override {}
    util::Result<std::unique_ptr<interfaces::Wallet>> createWallet(
        const std::string& name,
        const SecureString& passphrase,
        uint64_t wallet_creation_flags,
        std::vector<bilingual_str>& warnings) override
    {
        ++create_wallet_calls;
        return create_wallet_fn(name, passphrase, wallet_creation_flags, warnings);
    }
    util::Result<std::unique_ptr<interfaces::Wallet>> loadWallet(const std::string&, std::vector<bilingual_str>&) override
    {
        return util::Error{Untranslated("Unexpected loadWallet call")};
    }
    std::string getWalletDir() override { return {}; }
    util::Result<std::unique_ptr<interfaces::Wallet>> restoreWallet(const fs::path&, const std::string&, std::vector<bilingual_str>&) override
    {
        return util::Error{Untranslated("Unexpected restoreWallet call")};
    }
    util::Result<interfaces::WalletMigrationResult> migrateWallet(const std::string& name, const SecureString& passphrase) override
    {
        ++migrate_wallet_calls;
        return migrate_wallet_fn(name, passphrase);
    }
    bool isEncrypted(const std::string&) override { return false; }
    std::vector<std::pair<std::string, std::string>> listWalletDir() override
    {
        ++list_wallet_dir_calls;
        return wallet_dir_entries;
    }
    std::vector<std::unique_ptr<interfaces::Wallet>> getWallets() override
    {
        ++get_wallets_calls;
        return get_wallets_fn();
    }
    std::unique_ptr<interfaces::Handler> handleLoadWallet(LoadWalletFn) override
    {
        ++handle_load_wallet_calls;
        return MakeNoopHandler();
    }
};

void ExpectControllerInitialization(MockNode& node, FakeWalletLoader& loader)
{
    using ::testing::AtLeast;
    using ::testing::ReturnRef;

    ON_CALL(node, walletLoader()).WillByDefault(ReturnRef(loader));
    EXPECT_CALL(node, walletLoader()).Times(AtLeast(3)).WillRepeatedly(ReturnRef(loader));
}
} // namespace

class WalletQmlControllerTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createWalletBeforeInitializationReturnsFalseAndSetsError();
    void importWalletBeforeInitializationSetsLoadError();
    void migrateWalletBeforeInitializationSetsMigrationError();
    void selectWalletBeforeInitializationSetsLoadError();
    void initializedControllerPropagatesCreateErrors();
    void initializedControllerForwardsMigrationPassphrase();
};

void WalletQmlControllerTests::createWalletBeforeInitializationReturnsFalseAndSetsError()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    WalletQmlController controller(node);

    QVERIFY(!controller.createSingleSigWallet("test_wallet", "secret"));
    QCOMPARE(controller.walletCreateError(), QString{NOT_INITIALIZED_ERROR});
    QVERIFY(!controller.isWalletLoaded());
}

void WalletQmlControllerTests::importWalletBeforeInitializationSetsLoadError()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    WalletQmlController controller(node);

    controller.importWallet("/tmp/test_wallet.dat");
    QCOMPARE(controller.walletLoadError(), QString{NOT_INITIALIZED_ERROR});
}

void WalletQmlControllerTests::migrateWalletBeforeInitializationSetsMigrationError()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    WalletQmlController controller(node);

    controller.migrateWallet("legacy_wallet", "secret");
    QCOMPARE(controller.walletMigrationError(), QString{NOT_INITIALIZED_ERROR});
}

void WalletQmlControllerTests::selectWalletBeforeInitializationSetsLoadError()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    WalletQmlController controller(node);

    controller.setSelectedWallet("test_wallet");
    QCOMPARE(controller.walletLoadError(), QString{NOT_INITIALIZED_ERROR});
}

void WalletQmlControllerTests::initializedControllerPropagatesCreateErrors()
{
    using ::testing::_;
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    ExpectControllerInitialization(node, loader);

    WalletQmlController controller(node);
    controller.initialize();

    QVERIFY(controller.initialized());
    QVERIFY(controller.noWalletsFound());

    bool saw_expected_passphrase{false};
    bool saw_expected_flags{false};
    loader.create_wallet_fn = [&](const std::string&,
                                  const SecureString& passphrase,
                                  uint64_t wallet_creation_flags,
                                  std::vector<bilingual_str>&) {
        saw_expected_passphrase = (passphrase == SecureString{"secret"});
        saw_expected_flags = (wallet_creation_flags == wallet::WALLET_FLAG_DESCRIPTORS);
        return util::Result<std::unique_ptr<interfaces::Wallet>>{
            util::Error{Untranslated("Wallet creation failed.")}};
    };

    QVERIFY(!controller.createSingleSigWallet("test_wallet", "secret"));
    QVERIFY(saw_expected_passphrase);
    QVERIFY(saw_expected_flags);
    QCOMPARE(loader.create_wallet_calls, 1);
    QCOMPARE(controller.walletCreateError(), QString{"Wallet creation failed."});
    QVERIFY(!controller.isWalletLoaded());
}

void WalletQmlControllerTests::initializedControllerForwardsMigrationPassphrase()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    loader.wallet_dir_entries = {{"legacy_wallet", "bdb"}};
    ExpectControllerInitialization(node, loader);

    WalletQmlController controller(node);
    controller.initialize();

    QSignalSpy failed_spy(&controller, &WalletQmlController::walletMigrationFailed);

    bool saw_expected_name{false};
    bool saw_expected_passphrase{false};
    loader.migrate_wallet_fn = [&](const std::string& name, const SecureString& passphrase) {
        saw_expected_name = (name == "legacy_wallet");
        saw_expected_passphrase = (passphrase == SecureString{"secret"});
        return util::Result<interfaces::WalletMigrationResult>{
            util::Error{Untranslated("Migration failed.")}};
    };

    controller.migrateWallet("legacy_wallet", "secret");
    QVERIFY(failed_spy.wait(5000));
    QVERIFY(saw_expected_name);
    QVERIFY(saw_expected_passphrase);
    QCOMPARE(loader.migrate_wallet_calls, 1);
    QCOMPARE(controller.walletMigrationError(), QString{"Migration failed."});
    QVERIFY(!controller.walletMigrationInProgress());
}

int RunWalletQmlControllerTests(int argc, char* argv[])
{
    WalletQmlControllerTests tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_walletqmlcontroller.moc"
