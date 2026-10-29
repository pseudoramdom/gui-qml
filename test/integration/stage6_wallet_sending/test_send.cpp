// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/integration/runner/test_cases.h>
#include <test/integration/fixture/test_wallet.h>
#include <rpc/util.h>
#include <sync.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

#include <cmath>

class SendIntegrationTests : public QObject
{
    Q_OBJECT
    BitcoinQmlApplication& m_app;

public:
    explicit SendIntegrationTests(BitcoinQmlApplication& app) : m_app{app} {}

private Q_SLOTS:
    void customFeesRespectRelayMinimum_data()
    {
        QTest::addColumn<QString>("rate");
        QTest::addColumn<bool>("reject_low_rate");
        QTest::newRow("override-wallet-minimum") << "0.1" << false;
        QTest::newRow("respect-relay-minimum") << "1" << true;
    }

    void customFeesRespectRelayMinimum()
    {
        QFETCH(QString, rate);
        QFETCH(bool, reject_low_rate);
        auto* wallet = qmltest::CreateWallet(m_app, "custom_fee");
        QVERIFY(wallet);
        QVERIFY(qmltest::FundWallet(m_app, *wallet));
        const auto destination = qmltest::NewAddress(m_app, wallet->name());
        QVERIFY(qmltest::Click(m_app, "sendTabButton"));
        QVERIFY(qmltest::SetText(m_app, "sendAddressInput", destination));
        QVERIFY(qmltest::SetText(m_app, "sendAmountInput", "0.01"));
        QVERIFY(qmltest::Click(m_app, "feeSelectionPickerButton"));
        QVERIFY(qmltest::Click(m_app, "feeSelectionOption3"));
        if (reject_low_rate) {
            QVERIFY(qmltest::SetText(m_app, "feeSelectionCustomRateInput", "0.1"));
            QTRY_VERIFY(!wallet->customFeeRateValid());
            QTRY_VERIFY(!wallet->feeEstimatePending());
            QCOMPARE(wallet->estimatedFee(), QString{});
            QVERIFY(!qmltest::Find(m_app, "sendReviewButton")->property("enabled").toBool());
            QCOMPARE(qmltest::Find(m_app, "feeSelectionEstimateLabel")->property("value").toString(), QString::fromUtf8("—"));
            QCOMPARE(qmltest::Find(m_app, "feeSelectionCustomRateRow")->property("errorText").toString(), QStringLiteral("Enter a fee rate of at least 1 sat/vB."));
            QVERIFY(!wallet->prepareTransaction());
            QCOMPARE(qmltest::Rpc(m_app, "getrawmempool").size(), 0U);
        }
        QVERIFY(qmltest::SetText(m_app, "feeSelectionCustomRateInput", rate));
        QTRY_VERIFY(wallet->customFeeRateValid());
        QVERIFY(qmltest::Click(m_app, "sendReviewButton"));
        QVERIFY(qmltest::WaitForVisible(m_app, "sendTransactionReviewPage"));
        QVERIFY(wallet->currentTransaction());
        const auto fee = wallet->currentTransaction()->getTransactionFee();
        auto* core_wallet = wallet->wallet()->wallet();
        QVERIFY(core_wallet);
        int64_t estimated_vsize;
        {
            LOCK(core_wallet->cs_wallet);
            estimated_vsize = wallet::CalculateMaximumSignedTxSize(*wallet->currentTransaction()->getWtx(), core_wallet).vsize;
        }
        QVERIFY(estimated_vsize > 0);
        QVERIFY(qmltest::Click(m_app, "sendTransactionReviewSendButton"));
        auto* complete = qmltest::WaitForVisible(m_app, "sendCompletePage");
        QVERIFY(complete);
        const auto txid = complete->property("txid").toString();
        const auto mempool = qmltest::Rpc(m_app, "getrawmempool");
        QCOMPARE(mempool.size(), 1U);
        QCOMPARE(QString::fromStdString(mempool[0].get_str()), txid);
        UniValue params{UniValue::VARR};
        params.push_back(txid.toStdString());
        const auto entry = qmltest::Rpc(m_app, "getmempoolentry", params);
        const auto vsize = entry.find_value("vsize").getInt<int64_t>();
        QVERIFY(vsize <= estimated_vsize);
        // Core prices the conservative signed size; actual ECDSA signatures can be shorter.
        const auto expected_fee = static_cast<CAmount>(std::ceil(rate.toDouble() * estimated_vsize));
        QCOMPARE(fee, expected_fee);
        QCOMPARE(AmountFromValue(entry.find_value("fees").find_value("base")), expected_fee);
    }

    void coinSelectionCancelApplyAndLockReachTheWallet()
    {
        auto* wallet = qmltest::CreateWallet(m_app);
        QVERIFY(wallet);
        QVERIFY(qmltest::FundWallet(m_app, *wallet));
        auto* coins = wallet->coinsListModel();
        coins->update();
        QTRY_VERIFY(coins->rowCount() > 0);
        const auto id = coins->data(coins->index(0), CoinsListModel::CoinIdRole).toString();
        coins->beginSelection();
        QVERIFY(coins->toggleCoinSelectionById(id));
        QCOMPARE(coins->selectedCoinsCount(), 1);
        coins->setSearchText("no matching note");
        QCOMPARE(coins->rowCount(), 0);
        QCOMPARE(coins->selectedCoinsCount(), 1);
        coins->cancelSelection();
        QCOMPARE(coins->selectedCoinsCount(), 0);
        QVERIFY(wallet->listSelectedCoins().empty());
        coins->setSearchText("");
        coins->beginSelection();
        QVERIFY(coins->toggleCoinSelectionById(id));
        coins->applySelection();
        QCOMPARE(wallet->listSelectedCoins().size(), 1U);
        QVERIFY(coins->setCoinsLocked({id}, true));
        QCOMPARE(qmltest::Rpc(m_app, "listlockunspent", {}, wallet->name()).size(), 1U);
        QVERIFY(coins->setCoinsLocked({id}, false));
        QCOMPARE(qmltest::Rpc(m_app, "listlockunspent", {}, wallet->name()).size(), 0U);
    }
};

QStringList SendTestArguments(const QString& selector)
{
    if (selector.endsWith(":respect-relay-minimum")) return {"-mintxfee=0.00000100", "-minrelaytxfee=0.00001000"};
    return {"-mintxfee=0.00001000", "-minrelaytxfee=0.00000100"};
}

std::unique_ptr<QObject> CreateSendTests(BitcoinQmlApplication& app, WalletMode)
{
    return std::make_unique<SendIntegrationTests>(app);
}

#include <test_send.moc>
