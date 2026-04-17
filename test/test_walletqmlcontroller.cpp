// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtTest/QtTest>

#include <common/types.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <outputtype.h>
#include <qml/walletqmlcontroller.h>
#include <scheduler.h>
#include <test/mocks/mocknode.h>
#include <util/translation.h>
#include <wallet/walletutil.h>

#include <gmock/gmock.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>

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
    std::string wallet_dir;

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
    std::string getWalletDir() override { return wallet_dir; }
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
        if (wallet_dir.empty()) {
            return wallet_dir_entries;
        }

        std::vector<std::pair<std::string, std::string>> entries;
        const QString root_dir = QString::fromStdString(wallet_dir);
        for (const auto& [path, format] : wallet_dir_entries) {
            if (QFileInfo::exists(QDir(root_dir).filePath(QString::fromStdString(path)))) {
                entries.emplace_back(path, format);
            }
        }
        return entries;
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

class FakeWallet : public interfaces::Wallet
{
public:
    struct State {
        int remove_calls{0};
    };

    explicit FakeWallet(std::string wallet_name, State* state)
        : m_wallet_name(std::move(wallet_name)), m_state(state) {}

    bool private_keys_disabled{false};
    bool external_signer{false};

    bool encryptWallet(const SecureString&) override { return true; }
    bool isCrypted() override { return false; }
    bool lock() override { return true; }
    bool unlock(const SecureString&) override { return true; }
    bool isLocked() override { return false; }
    bool changeWalletPassphrase(const SecureString&, const SecureString&) override { return true; }
    void abortRescan() override {}
    bool backupWallet(const std::string&) override { return true; }
    std::string getWalletName() override { return m_wallet_name; }
    util::Result<CTxDestination> getNewDestination(const OutputType, const std::string&) override
    {
        return CTxDestination{CNoDestination{}};
    }
    bool getPubKey(const CScript&, const CKeyID&, CPubKey&) override { return false; }
    SigningResult signMessage(const std::string&, const PKHash&, std::string&) override
    {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }
    bool isSpendable(const CTxDestination&) override { return false; }
    bool setAddressBook(const CTxDestination&, const std::string&, const std::optional<wallet::AddressPurpose>&) override { return true; }
    bool delAddressBook(const CTxDestination&) override { return true; }
    bool getAddress(const CTxDestination&, std::string*, wallet::isminetype*, wallet::AddressPurpose*) override { return false; }
    std::vector<interfaces::WalletAddress> getAddresses() override { return {}; }
    std::vector<std::string> getAddressReceiveRequests() override { return {}; }
    bool setAddressReceiveRequest(const CTxDestination&, const std::string&, const std::string&) override { return true; }
    util::Result<void> displayAddress(const CTxDestination&) override { return {}; }
    bool lockCoin(const COutPoint&, const bool) override { return true; }
    bool unlockCoin(const COutPoint&) override { return true; }
    bool isLockedCoin(const COutPoint&) override { return false; }
    void listLockedCoins(std::vector<COutPoint>& outputs) override { outputs.clear(); }
    util::Result<CTransactionRef> createTransaction(const std::vector<wallet::CRecipient>&,
                                                    const wallet::CCoinControl&,
                                                    bool,
                                                    int&,
                                                    CAmount&) override
    {
        return util::Error{Untranslated("Unexpected createTransaction call")};
    }
    void commitTransaction(CTransactionRef, interfaces::WalletValueMap, interfaces::WalletOrderForm) override {}
    bool transactionCanBeAbandoned(const Txid&) override { return false; }
    bool abandonTransaction(const Txid&) override { return false; }
    bool transactionCanBeBumped(const Txid&) override { return false; }
    bool createBumpTransaction(const Txid&, const wallet::CCoinControl&, std::vector<bilingual_str>&, CAmount&, CAmount&, CMutableTransaction&) override
    {
        return false;
    }
    bool signBumpTransaction(CMutableTransaction&) override { return false; }
    bool commitBumpTransaction(const Txid&, CMutableTransaction&&, std::vector<bilingual_str>&, Txid&) override { return false; }
    CTransactionRef getTx(const Txid&) override { return {}; }
    interfaces::WalletTx getWalletTx(const Txid&) override { return {}; }
    std::set<interfaces::WalletTx> getWalletTxs() override { return {}; }
    bool tryGetTxStatus(const Txid&, interfaces::WalletTxStatus&, int&, int64_t&) override { return false; }
    interfaces::WalletTx getWalletTxDetails(const Txid&, interfaces::WalletTxStatus&, interfaces::WalletOrderForm&, bool&, int&) override { return {}; }
    std::optional<common::PSBTError> fillPSBT(std::optional<int>, bool, bool, size_t*, PartiallySignedTransaction&, bool&) override
    {
        return common::PSBTError::UNSUPPORTED;
    }
    interfaces::WalletBalances getBalances() override { return {}; }
    bool tryGetBalances(interfaces::WalletBalances&, uint256&) override { return false; }
    CAmount getBalance() override { return 0; }
    CAmount getAvailableBalance(const wallet::CCoinControl&) override { return 0; }
    wallet::isminetype txinIsMine(const CTxIn&) override { return {}; }
    wallet::isminetype txoutIsMine(const CTxOut&) override { return {}; }
    CAmount getDebit(const CTxIn&, wallet::isminefilter) override { return 0; }
    CAmount getCredit(const CTxOut&, wallet::isminefilter) override { return 0; }
    CoinsList listCoins() override { return {}; }
    std::vector<interfaces::WalletTxOut> getCoins(const std::vector<COutPoint>&) override { return {}; }
    CAmount getRequiredFee(unsigned int) override { return 0; }
    CAmount getMinimumFee(unsigned int, const wallet::CCoinControl&, int*, FeeReason*) override { return 0; }
    unsigned int getConfirmTarget() override { return 6; }
    bool hdEnabled() override { return true; }
    bool canGetAddresses() override { return true; }
    bool privateKeysDisabled() override { return private_keys_disabled; }
    bool taprootEnabled() override { return true; }
    bool hasExternalSigner() override { return external_signer; }
    OutputType getDefaultAddressType() override { return OutputType::BECH32; }
    CAmount getDefaultMaxTxFee() override { return COIN; }
    void remove() override
    {
        if (m_state) {
            ++m_state->remove_calls;
        }
    }
    std::unique_ptr<interfaces::Handler> handleUnload(UnloadFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleShowProgress(ShowProgressFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleStatusChanged(StatusChangedFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleAddressBookChanged(AddressBookChangedFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleTransactionChanged(TransactionChangedFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn) override { return MakeNoopHandler(); }

private:
    std::string m_wallet_name;
    State* m_state;
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
    void init();
    void createWalletBeforeInitializationReturnsFalseAndSetsError();
    void importWalletBeforeInitializationSetsLoadError();
    void migrateWalletBeforeInitializationSetsMigrationError();
    void selectWalletBeforeInitializationSetsLoadError();
    void initializedControllerPropagatesCreateErrors();
    void initializedControllerForwardsMigrationPassphrase();
    void initializedControllerClosesSelectedWalletAndSelectsRemainingLoadedWallet();
    void initializedControllerClosesNonSelectedWalletWithoutChangingSelection();
    void initializedControllerEmitsOpenWalletsChanged();
    void initializedControllerUnloadWalletsClearsSelectionAndOpenWallets();
    void initializedControllerDeleteWalletRemovesStorageAndClosesWallet();
    void initializedControllerUpdatesDisplayNameAlias();
};

void WalletQmlControllerTests::init()
{
    QSettings settings;
    settings.remove("walletDisplayNames");
    settings.sync();
}

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

void WalletQmlControllerTests::initializedControllerClosesSelectedWalletAndSelectsRemainingLoadedWallet()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    FakeWallet::State alpha_state;
    FakeWallet::State beta_state;
    loader.get_wallets_fn = [&]() {
        std::vector<std::unique_ptr<interfaces::Wallet>> wallets;
        wallets.emplace_back(std::make_unique<FakeWallet>("alpha_wallet", &alpha_state));
        wallets.emplace_back(std::make_unique<FakeWallet>("beta_wallet", &beta_state));
        return wallets;
    };
    ExpectControllerInitialization(node, loader);

    WalletQmlController controller(node);
    controller.initialize();

    QCOMPARE(controller.selectedWallet()->name(), QString{"alpha_wallet"});
    QVERIFY(controller.isWalletLoaded());
    QVERIFY(controller.isWalletOpen("alpha_wallet"));
    QVERIFY(controller.isWalletOpen("beta_wallet"));

    QSignalSpy selected_spy(&controller, &WalletQmlController::selectedWalletChanged);
    QSignalSpy open_wallets_spy(&controller, &WalletQmlController::openWalletsChanged);

    controller.closeWallet("alpha_wallet");

    QCOMPARE(alpha_state.remove_calls, 1);
    QCOMPARE(beta_state.remove_calls, 0);
    QCOMPARE(selected_spy.count(), 1);
    QCOMPARE(open_wallets_spy.count(), 1);
    QCOMPARE(open_wallets_spy.at(0).at(0).toStringList(), QStringList({"beta_wallet"}));
    QCOMPARE(controller.selectedWallet()->name(), QString{"beta_wallet"});
    QVERIFY(controller.isWalletLoaded());
    QVERIFY(!controller.isWalletOpen("alpha_wallet"));
    QVERIFY(controller.isWalletOpen("beta_wallet"));
}

void WalletQmlControllerTests::initializedControllerClosesNonSelectedWalletWithoutChangingSelection()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    FakeWallet::State alpha_state;
    FakeWallet::State beta_state;
    loader.get_wallets_fn = [&]() {
        std::vector<std::unique_ptr<interfaces::Wallet>> wallets;
        wallets.emplace_back(std::make_unique<FakeWallet>("alpha_wallet", &alpha_state));
        wallets.emplace_back(std::make_unique<FakeWallet>("beta_wallet", &beta_state));
        return wallets;
    };
    ExpectControllerInitialization(node, loader);

    WalletQmlController controller(node);
    controller.initialize();

    QSignalSpy selected_spy(&controller, &WalletQmlController::selectedWalletChanged);
    QSignalSpy open_wallets_spy(&controller, &WalletQmlController::openWalletsChanged);

    controller.closeWallet("beta_wallet");

    QCOMPARE(alpha_state.remove_calls, 0);
    QCOMPARE(beta_state.remove_calls, 1);
    QCOMPARE(selected_spy.count(), 0);
    QCOMPARE(open_wallets_spy.count(), 1);
    QCOMPARE(open_wallets_spy.at(0).at(0).toStringList(), QStringList({"alpha_wallet"}));
    QCOMPARE(controller.selectedWallet()->name(), QString{"alpha_wallet"});
    QVERIFY(controller.isWalletLoaded());
    QVERIFY(controller.isWalletOpen("alpha_wallet"));
    QVERIFY(!controller.isWalletOpen("beta_wallet"));
}

void WalletQmlControllerTests::initializedControllerEmitsOpenWalletsChanged()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    FakeWallet::State alpha_state;
    FakeWallet::State beta_state;
    loader.get_wallets_fn = [&]() {
        std::vector<std::unique_ptr<interfaces::Wallet>> wallets;
        wallets.emplace_back(std::make_unique<FakeWallet>("alpha_wallet", &alpha_state));
        wallets.emplace_back(std::make_unique<FakeWallet>("beta_wallet", &beta_state));
        return wallets;
    };
    ExpectControllerInitialization(node, loader);

    WalletQmlController controller(node);
    QSignalSpy open_wallets_spy(&controller, &WalletQmlController::openWalletsChanged);

    controller.initialize();

    QCOMPARE(open_wallets_spy.count(), 1);
    QCOMPARE(open_wallets_spy.at(0).at(0).toStringList(), QStringList({"alpha_wallet", "beta_wallet"}));
}

void WalletQmlControllerTests::initializedControllerUnloadWalletsClearsSelectionAndOpenWallets()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    FakeWallet::State alpha_state;
    FakeWallet::State beta_state;
    loader.get_wallets_fn = [&]() {
        std::vector<std::unique_ptr<interfaces::Wallet>> wallets;
        wallets.emplace_back(std::make_unique<FakeWallet>("alpha_wallet", &alpha_state));
        wallets.emplace_back(std::make_unique<FakeWallet>("beta_wallet", &beta_state));
        return wallets;
    };
    ExpectControllerInitialization(node, loader);

    WalletQmlController controller(node);
    controller.initialize();

    QSignalSpy selected_spy(&controller, &WalletQmlController::selectedWalletChanged);
    QSignalSpy open_wallets_spy(&controller, &WalletQmlController::openWalletsChanged);

    controller.unloadWallets();

    QCOMPARE(selected_spy.count(), 1);
    QCOMPARE(open_wallets_spy.count(), 1);
    QCOMPARE(open_wallets_spy.at(0).at(0).toStringList(), QStringList{});
    QCOMPARE(controller.selectedWallet()->name(), QString{});
    QVERIFY(!controller.isWalletOpen("alpha_wallet"));
    QVERIFY(!controller.isWalletOpen("beta_wallet"));
    QCOMPARE(alpha_state.remove_calls, 0);
    QCOMPARE(beta_state.remove_calls, 0);
}

void WalletQmlControllerTests::initializedControllerDeleteWalletRemovesStorageAndClosesWallet()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    FakeWallet::State alpha_state;
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    loader.wallet_dir = temp_dir.path().toStdString();
    loader.wallet_dir_entries = {{"alpha_wallet", "sqlite"}};
    loader.get_wallets_fn = [&]() {
        std::vector<std::unique_ptr<interfaces::Wallet>> wallets;
        wallets.emplace_back(std::make_unique<FakeWallet>("alpha_wallet", &alpha_state));
        return wallets;
    };
    ExpectControllerInitialization(node, loader);

    const QString wallet_dir = QDir(temp_dir.path()).filePath("alpha_wallet");
    QVERIFY(QDir().mkpath(wallet_dir));
    QFile marker(QDir(wallet_dir).filePath("wallet.dat"));
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.write("wallet");
    marker.close();

    WalletQmlController controller(node);
    controller.initialize();

    QSignalSpy selected_spy(&controller, &WalletQmlController::selectedWalletChanged);
    QSignalSpy open_wallets_spy(&controller, &WalletQmlController::openWalletsChanged);

    QVERIFY(controller.deleteWallet("alpha_wallet"));

    QCOMPARE(alpha_state.remove_calls, 1);
    QVERIFY(!QFileInfo::exists(wallet_dir));
    QCOMPARE(selected_spy.count(), 1);
    QCOMPARE(open_wallets_spy.count(), 1);
    QCOMPARE(open_wallets_spy.at(0).at(0).toStringList(), QStringList{});
    QVERIFY(!controller.isWalletLoaded());
    QVERIFY(controller.noWalletsFound());
}

void WalletQmlControllerTests::initializedControllerUpdatesDisplayNameAlias()
{
    using ::testing::StrictMock;

    StrictMock<MockNode> node;
    FakeWalletLoader loader;
    FakeWallet::State alpha_state;
    loader.get_wallets_fn = [&]() {
        std::vector<std::unique_ptr<interfaces::Wallet>> wallets;
        wallets.emplace_back(std::make_unique<FakeWallet>("alpha_wallet", &alpha_state));
        return wallets;
    };
    ExpectControllerInitialization(node, loader);

    WalletQmlController controller(node);
    controller.initialize();

    QSignalSpy display_name_spy(&controller, &WalletQmlController::walletDisplayNamesChanged);

    QVERIFY(controller.setWalletDisplayName("alpha_wallet", "Personal"));
    QCOMPARE(display_name_spy.count(), 1);
    QCOMPARE(controller.walletDisplayName("alpha_wallet"), QString{"Personal"});
    QCOMPARE(controller.selectedWallet()->displayName(), QString{"Personal"});
}

int RunWalletQmlControllerTests(int argc, char* argv[])
{
    WalletQmlControllerTests tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_walletqmlcontroller.moc"
