// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/integration/runner/test_cases.h>
#include <test/integration/fixture/test_wallet.h>

#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>

class WalletIntegrationTests : public QObject
{
    Q_OBJECT
    BitcoinQmlApplication& m_app;

public:
    explicit WalletIntegrationTests(BitcoinQmlApplication& app) : m_app{app} {}

private Q_SLOTS:
    void namesRejectLoadedClosedAndAliasedWallets()
    {
        auto* controller = qmltest::Model<WalletQmlController>(m_app, "walletController");
        QVERIFY(controller);
        QVERIFY(qmltest::CreateWallet(m_app, "closed"));
        controller->closeWallet("closed");
        QTRY_VERIFY(!controller->isWalletOpen("closed"));
        QVERIFY(qmltest::CreateWallet(m_app, "loaded"));
        QVERIFY(qmltest::CreateWallet(m_app, "alias_target"));
        QVERIFY(controller->setWalletDisplayName("alias_target", "Savings"));
        QVERIFY(qmltest::Click(m_app, "walletBadge"));
        QVERIFY(qmltest::Click(m_app, "walletSelectAddWalletButton"));
        QVERIFY(qmltest::Click(m_app, "walletTypeRegular"));
        QVERIFY(qmltest::SetText(m_app, "createWalletPasswordInput", "test wallet password"));
        QVERIFY(qmltest::SetText(m_app, "createWalletPasswordRepeatInput", "test wallet password"));
        QVERIFY(qmltest::Click(m_app, "createWalletPasswordConfirmToggle"));
        for (const auto* name : {"closed", "loaded", "Savings", "savings"}) {
            QVERIFY2(controller->walletNameAvailabilityError(name).contains("already exists"), name);
            QVERIFY(qmltest::SetText(m_app, "createWalletNameInput", name));
            QVERIFY(qmltest::Click(m_app, "createWalletFormCreateButton"));
            auto* error = qmltest::Find(m_app, "createWalletFormErrorText");
            QVERIFY(error);
            QTRY_VERIFY(error->property("text").toString().contains("already exists"));
        }
        QCOMPARE(controller->walletNameAvailabilityError("My new wallet"), QString{});
        QSignalSpy created{controller, &WalletQmlController::walletCreateSucceeded};
        QVERIFY(qmltest::SetText(m_app, "createWalletNameInput", "My new wallet"));
        QVERIFY(qmltest::Click(m_app, "createWalletFormCreateButton"));
        QTRY_COMPARE_WITH_TIMEOUT(created.count(), 1, 30'000);
        QVERIFY(qmltest::WaitForVisible(m_app, "walletCreationReadyPage"));
        QVERIFY(qmltest::Click(m_app, "createWalletReadyDoneButton"));
        auto* wallet = qmltest::WaitForWallet(m_app, "My new wallet");
        QVERIFY(wallet);
        QVERIFY(wallet->isEncrypted());
        QVERIFY(wallet->isLocked());
        const auto info = qmltest::Rpc(m_app, "getwalletinfo", {}, wallet->name());
        QVERIFY(info.find_value("private_keys_enabled").get_bool());
        QCOMPARE(info.find_value("unlocked_until").getInt<int64_t>(), 0);
    }

    void closingAndReloadingReplacesTheModel()
    {
        auto* controller = qmltest::Model<WalletQmlController>(m_app, "walletController");
        QVERIFY(controller);
        QPointer<WalletQmlModel> original{qmltest::CreateWallet(m_app, "reload")};
        QVERIFY(original);
        QSignalSpy selected{controller, &WalletQmlController::selectedWalletChanged};
        controller->closeWallet("reload");
        QTRY_VERIFY(!controller->isWalletLoaded());
        QTRY_VERIFY(original.isNull());
        QVERIFY(!selected.isEmpty());
        QCOMPARE(qmltest::Rpc(m_app, "listwallets").size(), 0U);
        controller->setSelectedWallet("reload");
        QVERIFY(qmltest::WaitForWallet(m_app, "reload"));
        QCOMPARE(qmltest::Rpc(m_app, "listwallets").size(), 1U);
        auto* list = qmltest::Model<WalletListModel>(m_app, "walletListModel");
        QVERIFY(list);
        QTRY_COMPARE(list->rowCount(), 1);
        QCOMPARE(list->data(list->index(0), WalletListModel::LoadStateRole).toInt(), static_cast<int>(WalletListModel::LoadState::Open));
    }

    void passwordChangesRelockAndPreserveTheWallet()
    {
        auto* wallet = qmltest::CreateWallet(m_app);
        QVERIFY(wallet);
        QVERIFY(wallet->encryptWallet("old password"));
        QTRY_VERIFY(wallet->isLocked());
        QVERIFY(!wallet->changeWalletPassphrase("wrong password", "new password"));
        QVERIFY(!wallet->settingsError().isEmpty());
        QVERIFY(wallet->isLocked());
        QVERIFY(wallet->changeWalletPassphrase("old password", "new password"));
        QVERIFY(wallet->settingsError().isEmpty());
        QVERIFY(wallet->isLocked());
        qmltest::Rpc(m_app, "walletpassphrase", qmltest::Json("[\"new password\", 10]"), wallet->name());
        QTRY_VERIFY(!wallet->isLocked());
        qmltest::Rpc(m_app, "walletlock", {}, wallet->name());
        QTRY_VERIFY(wallet->isLocked());
    }

    void walletTypeSelectorRejectsInvalidBackups()
    {
        QVERIFY(qmltest::Click(m_app, "walletBadge"));
        QVERIFY(qmltest::WaitForVisible(m_app, "walletCreationTypePage"));
        QVERIFY(!qmltest::Find(m_app, "walletTypeMultiKey"));
        QVERIFY(!qmltest::Find(m_app, "walletTypeCustom"));
        auto* controller = qmltest::Model<WalletQmlController>(m_app, "walletController");
        QVERIFY(controller);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile invalid{directory.filePath("invalid.bak")};
        QVERIFY(invalid.open(QIODevice::WriteOnly));
        QVERIFY(invalid.write("not a wallet backup") > 0);
        invalid.close();
        auto* wizard = qmltest::Find(m_app, "createWalletWizard");
        QVERIFY(wizard);
        QVERIFY(QMetaObject::invokeMethod(wizard, "importFile", Q_ARG(QVariant, invalid.fileName())));
        QVERIFY(qmltest::WaitForVisible(m_app, "walletImportErrorPage"));
        QVERIFY(!controller->walletLoadError().isEmpty());
        QCOMPARE(qmltest::Rpc(m_app, "listwallets").size(), 0U);
    }

    void backupRestoresAddressesAndRejectsInvalidFiles()
    {
        auto* wallet = qmltest::CreateWallet(m_app, "original");
        QVERIFY(wallet);
        const auto address = qmltest::NewAddress(m_app, wallet->name());
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto backup = directory.filePath(QString::fromUtf8("wallet backup ü.dat"));
        QVERIFY(wallet->backupWallet(backup));
        QVERIFY(QFileInfo{backup}.size() > 0);
        auto* controller = qmltest::Model<WalletQmlController>(m_app, "walletController");
        QVERIFY(controller);
        controller->closeWallet("original");
        QTRY_VERIFY(!controller->isWalletLoaded());
        QSignalSpy imported{controller, &WalletQmlController::walletImportSucceeded};
        controller->importWallet(backup);
        QTRY_COMPARE_WITH_TIMEOUT(imported.count(), 1, 30'000);
        QVERIFY(controller->isWalletLoaded());
        UniValue params{UniValue::VARR};
        params.push_back(address.toStdString());
        QVERIFY(qmltest::Rpc(m_app, "getaddressinfo", params, controller->selectedWallet()->name()).find_value("ismine").get_bool());
        controller->importWallet(directory.filePath("missing.dat"));
        QTRY_VERIFY(!controller->walletLoadError().isEmpty());
        QCOMPARE(imported.count(), 1);
        QVERIFY(controller->isWalletLoaded());
    }
};

std::unique_ptr<QObject> CreateWalletTests(BitcoinQmlApplication& app, WalletMode)
{
    return std::make_unique<WalletIntegrationTests>(app);
}

#include <test_wallet.moc>
