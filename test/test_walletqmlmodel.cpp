// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtTest/QtTest>

#include <chainparams.h>
#include <common/signmessage.h>
#include <common/messages.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <outputtype.h>
#include <primitives/transaction.h>
#include <key_io.h>
#include <addresstype.h>
#include <qml/models/walletqmlmodel.h>
#include <wallet/coincontrol.h>
#include <wallet/types.h>

#include <QSettings>

namespace {
constexpr auto REGTEST_ADDRESS{"bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq3xueyj"};
constexpr auto REGTEST_LEGACY_ADDRESS{"mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn"};

std::unique_ptr<interfaces::Handler> MakeNoopHandler()
{
    return interfaces::MakeCleanupHandler([] {});
}

class FakeWallet : public interfaces::Wallet
{
public:
    bool encrypted{true};
    bool locked{true};
    bool private_keys_disabled{false};
    bool external_signer{false};
    bool taproot_enabled{true};
    OutputType default_address_type{OutputType::BECH32};
    CAmount balance{50'000};
    int encrypt_calls{0};
    int change_passphrase_calls{0};
    int backup_calls{0};
    std::string last_backup_path;
    std::vector<std::pair<std::string, std::string>> changed_passphrases;
    int unlock_calls{0};
    int lock_calls{0};
    int commit_calls{0};
    std::vector<std::string> unlock_passphrases;
    std::vector<bool> create_transaction_sign_args;
    std::vector<bool> fill_psbt_sign_args;
    int sign_message_calls{0};
    std::string last_signed_message;
    std::vector<OutputType> new_destination_types;

    std::function<util::Result<CTransactionRef>(const std::vector<wallet::CRecipient>&,
                                                const wallet::CCoinControl&,
                                                bool,
                                                int&,
                                                CAmount&)>
        create_transaction_fn = [](const std::vector<wallet::CRecipient>&,
                                   const wallet::CCoinControl&,
                                   bool,
                                   int& change_pos,
                                   CAmount& fee) {
            change_pos = -1;
            fee = 250;
            return MakeTransactionRef(CMutableTransaction{});
        };
    std::function<bool(const SecureString&)> unlock_fn = [this](const SecureString& passphrase) {
        ++unlock_calls;
        unlock_passphrases.emplace_back(passphrase.begin(), passphrase.end());
        locked = false;
        return true;
    };
    std::function<std::optional<common::PSBTError>(std::optional<int>,
                                                   bool,
                                                   bool,
                                                   size_t*,
                                                   PartiallySignedTransaction&,
                                                   bool&)>
        fill_psbt_fn = [this](std::optional<int>,
                              bool sign,
                              bool,
                              size_t*,
                              PartiallySignedTransaction&,
                              bool& complete) {
            fill_psbt_sign_args.push_back(sign);
            complete = sign;
            return std::nullopt;
        };
    std::function<SigningResult(const std::string&, const PKHash&, std::string&)>
        sign_message_fn = [this](const std::string& message, const PKHash&, std::string& signature) {
            ++sign_message_calls;
            last_signed_message = message;
            signature = "fake-signature";
            return SigningResult::OK;
        };

    bool encryptWallet(const SecureString& passphrase) override
    {
        ++encrypt_calls;
        encrypted = true;
        locked = true;
        unlock_passphrases.emplace_back(passphrase.begin(), passphrase.end());
        return !passphrase.empty();
    }
    bool isCrypted() override { return encrypted; }
    bool lock() override
    {
        ++lock_calls;
        locked = true;
        return true;
    }
    bool unlock(const SecureString& wallet_passphrase) override { return unlock_fn(wallet_passphrase); }
    bool isLocked() override { return locked; }
    bool changeWalletPassphrase(const SecureString& old_passphrase, const SecureString& new_passphrase) override
    {
        ++change_passphrase_calls;
        changed_passphrases.emplace_back(
            std::string(old_passphrase.begin(), old_passphrase.end()),
            std::string(new_passphrase.begin(), new_passphrase.end()));
        return !old_passphrase.empty() && !new_passphrase.empty() && old_passphrase == SecureString{"secret"};
    }
    void abortRescan() override {}
    bool backupWallet(const std::string& path) override
    {
        ++backup_calls;
        last_backup_path = path;
        return !path.empty();
    }
    std::string getWalletName() override { return "fake-wallet"; }
    util::Result<CTxDestination> getNewDestination(const OutputType type, const std::string&) override
    {
        new_destination_types.push_back(type);
        return DecodeDestination(REGTEST_LEGACY_ADDRESS);
    }
    bool getPubKey(const CScript&, const CKeyID&, CPubKey&) override { return false; }
    SigningResult signMessage(const std::string& message, const PKHash& pkhash, std::string& signature) override
    {
        return sign_message_fn(message, pkhash, signature);
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
    util::Result<CTransactionRef> createTransaction(const std::vector<wallet::CRecipient>& recipients,
                                                    const wallet::CCoinControl& coin_control,
                                                    bool sign,
                                                    int& change_pos,
                                                    CAmount& fee) override
    {
        create_transaction_sign_args.push_back(sign);
        return create_transaction_fn(recipients, coin_control, sign, change_pos, fee);
    }
    void commitTransaction(CTransactionRef, interfaces::WalletValueMap, interfaces::WalletOrderForm) override
    {
        ++commit_calls;
    }
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
    std::optional<common::PSBTError> fillPSBT(std::optional<int> sighash_type,
                                              bool sign,
                                              bool bip32derivs,
                                              size_t* n_signed,
                                              PartiallySignedTransaction& psbtx,
                                              bool& complete) override
    {
        return fill_psbt_fn(sighash_type, sign, bip32derivs, n_signed, psbtx, complete);
    }
    interfaces::WalletBalances getBalances() override { return {.balance = balance}; }
    bool tryGetBalances(interfaces::WalletBalances& balances_out, uint256&) override
    {
        balances_out = {.balance = balance};
        return true;
    }
    CAmount getBalance() override { return balance; }
    CAmount getAvailableBalance(const wallet::CCoinControl&) override { return balance; }
    wallet::isminetype txinIsMine(const CTxIn&) override { return wallet::ISMINE_NO; }
    wallet::isminetype txoutIsMine(const CTxOut&) override { return wallet::ISMINE_NO; }
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
    bool taprootEnabled() override { return taproot_enabled; }
    bool hasExternalSigner() override { return external_signer; }
    OutputType getDefaultAddressType() override { return default_address_type; }
    CAmount getDefaultMaxTxFee() override { return COIN; }
    void remove() override {}
    std::unique_ptr<interfaces::Handler> handleUnload(UnloadFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleShowProgress(ShowProgressFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleStatusChanged(StatusChangedFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleAddressBookChanged(AddressBookChangedFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleTransactionChanged(TransactionChangedFn) override { return MakeNoopHandler(); }
    std::unique_ptr<interfaces::Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn) override { return MakeNoopHandler(); }
};

std::unique_ptr<WalletQmlModel> MakeWalletModel(FakeWallet*& wallet_out)
{
    auto wallet = std::make_unique<FakeWallet>();
    wallet_out = wallet.get();
    return std::make_unique<WalletQmlModel>(std::move(wallet));
}

void ConfigureRecipient(WalletQmlModel& model, qint64 satoshis)
{
    auto* recipient = model.sendRecipientList()->currentRecipient();
    recipient->setAddress(QString::fromLatin1(REGTEST_ADDRESS));
    recipient->amount()->setSatoshi(satoshis);
}
} // namespace

class WalletQmlModelTests : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<ECC_Context> m_ecc_context;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void displayNameDefaultsToWalletName();
    void detailPropertiesReflectWalletCapabilities();
    void encryptWalletUpdatesSecurityState();
    void changeWalletPassphraseForwardsPasswords();
    void backupWalletForwardsPath();
    void availableReceiveAddressTypesHideUnavailableTaproot();
    void commitPaymentRequestUsesSelectedAddressType();
    void receiveAddressTypeDefaultPersistsPerWallet();
    void prepareTransactionOnLockedWalletMarksUnlockNeeded();
    void sendTransactionOnLockedWalletRequiresPassword();
    void sendTransactionWithPassphraseUnlocksCommitsAndRelocks();
    void signVerifyMessageRejectsNonP2PKHAddress();
    void signVerifyMessageSignsWithLegacyP2PKHAddress();
    void signVerifyMessageWithPassphraseUnlocksSignsAndRelocks();
    void signVerifyMessageSurfacesSigningFailure();
    void signVerifyMessageVerifiesValidSignature();
};

void WalletQmlModelTests::initTestCase()
{
    SelectParams(ChainType::REGTEST);
    m_ecc_context = std::make_unique<ECC_Context>();
}

void WalletQmlModelTests::cleanupTestCase()
{
    m_ecc_context.reset();
}

void WalletQmlModelTests::displayNameDefaultsToWalletName()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);

    QCOMPARE(model->displayName(), QString("fake-wallet"));
    model->setDisplayName("Personal");
    QCOMPARE(model->displayName(), QString("Personal"));
}

void WalletQmlModelTests::detailPropertiesReflectWalletCapabilities()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);

    wallet->private_keys_disabled = true;
    wallet->external_signer = true;
    QCOMPARE(model->keyScheme(), QString("Watch-only"));
    QCOMPARE(model->privateKeysStatus(), QString("Disabled"));
    QCOMPARE(model->externalSignerStatus(), QString("Enabled"));

    wallet->private_keys_disabled = false;

    QCOMPARE(model->keyScheme(), QString("Single-key"));
    QCOMPARE(model->privateKeysStatus(), QString("Enabled"));
}

void WalletQmlModelTests::encryptWalletUpdatesSecurityState()
{
    auto wallet = std::make_unique<FakeWallet>();
    wallet->encrypted = false;
    wallet->locked = false;
    FakeWallet* raw_wallet = wallet.get();
    auto model = std::make_unique<WalletQmlModel>(std::move(wallet));

    QVERIFY(model->encryptWallet("secret"));
    QCOMPARE(raw_wallet->encrypt_calls, 1);
    QVERIFY(model->isEncrypted());
    QVERIFY(model->isLocked());
    QVERIFY(model->settingsError().isEmpty());
}

void WalletQmlModelTests::changeWalletPassphraseForwardsPasswords()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);

    QVERIFY(model->changeWalletPassphrase("secret", "new-secret"));
    QCOMPARE(wallet->change_passphrase_calls, 1);
    QCOMPARE(wallet->changed_passphrases.size(), size_t{1});
    QCOMPARE(wallet->changed_passphrases.front().first, std::string("secret"));
    QCOMPARE(wallet->changed_passphrases.front().second, std::string("new-secret"));

    QVERIFY(!model->changeWalletPassphrase("wrong", "new-secret"));
    QCOMPARE(model->settingsError(), QString("The current wallet password was incorrect."));
}

void WalletQmlModelTests::backupWalletForwardsPath()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);

    QVERIFY(model->backupWallet("/tmp/fake-wallet.bak"));
    QCOMPARE(wallet->backup_calls, 1);
    QCOMPARE(wallet->last_backup_path, std::string("/tmp/fake-wallet.bak"));
    QVERIFY(model->settingsError().isEmpty());
}

void WalletQmlModelTests::availableReceiveAddressTypesHideUnavailableTaproot()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);

    QVariantList types = model->availableReceiveAddressTypes();
    QCOMPARE(types.size(), 4);
    QCOMPARE(types.front().toMap().value("id").toString(), QString("bech32m"));

    wallet->taproot_enabled = false;
    types = model->availableReceiveAddressTypes();
    QCOMPARE(types.size(), 3);
    for (const QVariant& type : types) {
        QVERIFY(type.toMap().value("id").toString() != QString("bech32m"));
    }
}

void WalletQmlModelTests::commitPaymentRequestUsesSelectedAddressType()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);

    auto* request = model->currentPaymentRequest();
    request->setAddressType("legacy");
    model->commitPaymentRequest();

    QCOMPARE(wallet->new_destination_types.size(), size_t{1});
    QCOMPARE(wallet->new_destination_types.front(), OutputType::LEGACY);
    QCOMPARE(request->addressType(), QString("legacy"));
}

void WalletQmlModelTests::receiveAddressTypeDefaultPersistsPerWallet()
{
    QSettings settings;
    settings.remove("receiveAddressTypes/fake-wallet");
    settings.sync();

    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    wallet->default_address_type = OutputType::BECH32;

    QCOMPARE(model->defaultReceiveAddressType(), QString("bech32"));

    model->setDefaultReceiveAddressType("p2sh-segwit");
    QCOMPARE(model->defaultReceiveAddressType(), QString("p2sh-segwit"));

    model->setDefaultReceiveAddressType("bech32m");
    QCOMPARE(model->defaultReceiveAddressType(), QString("bech32m"));

    wallet->taproot_enabled = false;
    QCOMPARE(model->defaultReceiveAddressType(), QString("bech32"));

    settings.remove("receiveAddressTypes/fake-wallet");
    settings.sync();
}

void WalletQmlModelTests::prepareTransactionOnLockedWalletMarksUnlockNeeded()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    ConfigureRecipient(*model, 1'000);

    wallet->create_transaction_fn = [](const std::vector<wallet::CRecipient>&,
                                       const wallet::CCoinControl&,
                                       bool,
                                       int&,
                                       CAmount&) -> util::Result<CTransactionRef> {
        return util::Error{Untranslated("Transaction needs a change address, but we can't generate it. Error: Keypool ran out, please call keypoolrefill first")};
    };

    QVERIFY(!model->prepareTransaction());
    QVERIFY(model->isEncrypted());
    QVERIFY(model->isLocked());
    QVERIFY(model->transactionNeedsUnlock());
    QCOMPARE(model->transactionError(), QString("Transaction needs a change address, but we can't generate it. Error: Keypool ran out, please call keypoolrefill first"));
    QVERIFY(wallet->create_transaction_sign_args == std::vector<bool>{false});
    QCOMPARE(wallet->unlock_calls, 0);
    QCOMPARE(wallet->lock_calls, 0);
}

void WalletQmlModelTests::sendTransactionOnLockedWalletRequiresPassword()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    ConfigureRecipient(*model, 1'000);

    QVERIFY(model->prepareTransactionWithPassphrase("secret"));
    QVERIFY(wallet->locked);
    QCOMPARE(wallet->unlock_calls, 1);
    QCOMPARE(wallet->lock_calls, 1);

    QVERIFY(!model->sendTransaction());
    QCOMPARE(model->transactionError(), QString("Enter your wallet password to sign this transaction."));
    QCOMPARE(wallet->commit_calls, 0);
    QVERIFY(wallet->fill_psbt_sign_args.empty());
}

void WalletQmlModelTests::sendTransactionWithPassphraseUnlocksCommitsAndRelocks()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    ConfigureRecipient(*model, 1'000);

    QVERIFY(model->prepareTransactionWithPassphrase("secret"));
    QVERIFY(wallet->locked);

    QVERIFY(model->sendTransactionWithPassphrase("secret"));
    QCOMPARE(wallet->unlock_calls, 2);
    QCOMPARE(wallet->lock_calls, 2);
    QCOMPARE(wallet->commit_calls, 1);
    QVERIFY(wallet->locked);
    QVERIFY(wallet->fill_psbt_sign_args == std::vector<bool>({false, true}));
    QVERIFY(model->transactionError().isEmpty());
    QVERIFY(!model->transactionNeedsUnlock());
}

void WalletQmlModelTests::signVerifyMessageRejectsNonP2PKHAddress()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    auto* sign_verify = model->signVerifyMessageModel();

    QVERIFY(!sign_verify->isLegacyP2PKHAddress(QString::fromLatin1(REGTEST_ADDRESS)));
    QVERIFY(!sign_verify->signMessage(QString::fromLatin1(REGTEST_ADDRESS), "message"));
    QCOMPARE(sign_verify->signingError(), QString("Enter a legacy P2PKH bitcoin address."));
    QCOMPARE(wallet->sign_message_calls, 0);
}

void WalletQmlModelTests::signVerifyMessageSignsWithLegacyP2PKHAddress()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    wallet->encrypted = false;
    wallet->locked = false;
    auto* sign_verify = model->signVerifyMessageModel();

    QVERIFY(sign_verify->isLegacyP2PKHAddress(QString::fromLatin1(REGTEST_LEGACY_ADDRESS)));
    QVERIFY(sign_verify->signMessage(QString::fromLatin1(REGTEST_LEGACY_ADDRESS), "hello"));
    QCOMPARE(wallet->sign_message_calls, 1);
    QCOMPARE(wallet->last_signed_message, std::string("hello"));
    QCOMPARE(sign_verify->signature(), QString("fake-signature"));
    QVERIFY(sign_verify->signingError().isEmpty());
    QVERIFY(!sign_verify->signingNeedsUnlock());
}

void WalletQmlModelTests::signVerifyMessageWithPassphraseUnlocksSignsAndRelocks()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    auto* sign_verify = model->signVerifyMessageModel();

    QVERIFY(!sign_verify->signMessage(QString::fromLatin1(REGTEST_LEGACY_ADDRESS), "hello"));
    QVERIFY(sign_verify->signingNeedsUnlock());
    QCOMPARE(wallet->unlock_calls, 0);

    QVERIFY(sign_verify->signMessageWithPassphrase(QString::fromLatin1(REGTEST_LEGACY_ADDRESS), "hello", "secret"));
    QCOMPARE(wallet->unlock_calls, 1);
    QCOMPARE(wallet->lock_calls, 1);
    QVERIFY(wallet->locked);
    QCOMPARE(sign_verify->signature(), QString("fake-signature"));
}

void WalletQmlModelTests::signVerifyMessageSurfacesSigningFailure()
{
    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    wallet->encrypted = false;
    wallet->locked = false;
    wallet->sign_message_fn = [](const std::string&, const PKHash&, std::string&) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    };
    auto* sign_verify = model->signVerifyMessageModel();

    QVERIFY(!sign_verify->signMessage(QString::fromLatin1(REGTEST_LEGACY_ADDRESS), "hello"));
    QVERIFY(sign_verify->signature().isEmpty());
    QCOMPARE(sign_verify->signingError(), QString("Private key not available"));
}

void WalletQmlModelTests::signVerifyMessageVerifiesValidSignature()
{
    CKey key;
    key.MakeNewKey(true);
    const QString address{QString::fromStdString(EncodeDestination(PKHash(key.GetPubKey())))};
    const QString message{"hello"};
    std::string signature;
    QVERIFY(MessageSign(key, message.toStdString(), signature));

    FakeWallet* wallet{nullptr};
    auto model = MakeWalletModel(wallet);
    auto* sign_verify = model->signVerifyMessageModel();

    QVERIFY(sign_verify->isLegacyP2PKHAddress(address));
    QVERIFY(sign_verify->verifyMessage(address, message, QString::fromStdString(signature)));
    QVERIFY(!sign_verify->verifyMessage(address, message + "!", QString::fromStdString(signature)));
    QVERIFY(!sign_verify->verifyMessage(QString::fromLatin1(REGTEST_ADDRESS), message, QString::fromStdString(signature)));
}

int RunWalletQmlModelTests(int argc, char* argv[])
{
    WalletQmlModelTests tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_walletqmlmodel.moc"
