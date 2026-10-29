// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/integration/runner/test_cases.h>
#include <test/integration/fixture/test_wallet.h>
#include <qml/models/activityfilterproxymodel.h>

#include <QSignalSpy>

class ReceiveIntegrationTests : public QObject
{
    Q_OBJECT
    BitcoinQmlApplication& m_app;

public:
    explicit ReceiveIntegrationTests(BitcoinQmlApplication& app) : m_app{app} {}

private Q_SLOTS:
    void privateNotesSyncWithoutChangingPublicPaymentMetadata()
    {
        auto* wallet = qmltest::CreateWallet(m_app);
        QVERIFY(wallet);
        auto* request = wallet->currentPaymentRequest();
        request->amount()->setSatoshi(10'000);
        request->setLabel("Alice");
        request->setMessage("pizza");
        request->setNoteSelf("Private lunch note");
        QVERIFY(wallet->commitPaymentRequest());
        const auto id = request->id();
        const auto address = request->address();
        const auto uri = request->qrPayload();
        QVERIFY(!id.isEmpty());
        QVERIFY(!address.isEmpty());
        QVERIFY(!uri.contains("Private"));
        QCOMPARE(wallet->receiveRequests()->count(), 1);
        QCOMPARE(wallet->getAddressLabel(address), QStringLiteral("Private lunch note"));
        QSignalSpy changed{wallet->receiveRequests(), &QAbstractItemModel::dataChanged};
        QVERIFY(wallet->addressListModel()->setAddressLabel(address, "Renamed on Addresses page"));
        QTRY_VERIFY(!changed.isEmpty());
        QVERIFY(wallet->loadPaymentRequestDetail(id));
        QCOMPARE(wallet->detailPaymentRequest()->noteSelf(), QStringLiteral("Renamed on Addresses page"));
        QCOMPARE(wallet->detailPaymentRequest()->label(), QStringLiteral("Alice"));
        QCOMPARE(wallet->detailPaymentRequest()->message(), QStringLiteral("pizza"));
        QCOMPARE(wallet->detailPaymentRequest()->qrPayload(), uri);
        QVERIFY(qmltest::Click(m_app, "activityTabButton"));
        QVERIFY(qmltest::SetText(m_app, "activitySearchField", "Renamed on Addresses page"));
        auto* filter = qobject_cast<ActivityFilterProxyModel*>(qmltest::Find(m_app, "activityFilterProxyModel"));
        QVERIFY(filter);
        QTRY_COMPARE(filter->rowCount(), 1);
        QCOMPARE(filter->data(filter->index(0, 0), TransactionActivityModel::LabelRole).toString(), QStringLiteral("Renamed on Addresses page"));
        wallet->reloadReceiveRequests();
        QVERIFY(wallet->loadPaymentRequestDetail(id));
        QCOMPARE(wallet->detailPaymentRequest()->noteSelf(), QStringLiteral("Renamed on Addresses page"));
        QCOMPARE(wallet->detailPaymentRequest()->qrPayload(), uri);
        QVERIFY(wallet->removeReceiveRequest(id));
        QCOMPARE(wallet->receiveRequests()->count(), 0);
        QVERIFY(!wallet->loadPaymentRequestDetail(id));
    }

    void receivingAddressCanBeReusedOrReplaced()
    {
        auto* wallet = qmltest::CreateWallet(m_app);
        QVERIFY(wallet);
        QVERIFY(wallet->ensureReceivingAddress());
        const auto address = wallet->receivingAddress()->address();
        QVERIFY(!address.isEmpty());
        QVERIFY(wallet->ensureReceivingAddress());
        QCOMPARE(wallet->receivingAddress()->address(), address);
        QVERIFY(wallet->ensureReceivingAddress(true));
        QVERIFY(wallet->receivingAddress()->address() != address);
        const auto types = wallet->availableReceiveAddressTypes();
        QVERIFY(!types.isEmpty());
        QVERIFY(wallet->ensureReceivingAddress(true, "legacy"));
        const auto legacy = wallet->receivingAddress()->address();
        QVERIFY(legacy.startsWith('m') || legacy.startsWith('n'));
    }

    void signingUnlocksOnlyForTheOperation()
    {
        auto* wallet = qmltest::CreateWallet(m_app);
        QVERIFY(wallet);
        const auto address = qmltest::NewAddress(m_app, wallet->name(), "legacy");
        QVERIFY(wallet->encryptWallet("signing password"));
        auto* signer = wallet->signVerifyMessageModel();
        QVERIFY(!signer->signMessage(address, "integration message"));
        QVERIFY(signer->signingNeedsUnlock());
        QVERIFY(!signer->signMessageWithPassphrase(address, "integration message", "incorrect"));
        QVERIFY(signer->signMessageWithPassphrase(address, "integration message", "signing password"));
        const auto signature = signer->signature();
        QVERIFY(!signature.isEmpty());
        QVERIFY(signer->verifyMessage(address, "integration message", signature));
        QVERIFY(!signer->verifyMessage(address, "changed message", signature));
        QVERIFY(wallet->isLocked());
        QCOMPARE(qmltest::Rpc(m_app, "getwalletinfo", {}, wallet->name()).find_value("unlocked_until").getInt<int64_t>(), 0);
    }
};

std::unique_ptr<QObject> CreateReceiveTests(BitcoinQmlApplication& app, WalletMode)
{
    return std::make_unique<ReceiveIntegrationTests>(app);
}

#include <test_receive.moc>
