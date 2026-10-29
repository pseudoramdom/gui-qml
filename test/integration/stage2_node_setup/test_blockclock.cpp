// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/integration/runner/test_cases.h>
#include <test/integration/fixture/test_util.h>

#include <QSignalSpy>

class BlockClockIntegrationTests : public QObject
{
    Q_OBJECT
    BitcoinQmlApplication& m_app;

public:
    explicit BlockClockIntegrationTests(BitcoinQmlApplication& app) : m_app{app} {}

private Q_SLOTS:
    void blockClockTracksNetworkActivity()
    {
        auto* clock = qmltest::Find(m_app, "blockClock");
        QVERIFY(clock);
        QTRY_COMPARE(clock->property("state").toString(), QStringLiteral("CONNECTING"));
        QCOMPARE(clock->property("header").toString(), QStringLiteral("Connecting"));
        QCOMPARE(clock->property("subText").toString(), QStringLiteral("Please wait"));
        m_app.nodeModel().setPause(true);
        QTRY_COMPARE(clock->property("state").toString(), QStringLiteral("PAUSE"));
        QTRY_VERIFY(!qmltest::Call(m_app, &interfaces::Node::getNetworkActive));
        QCOMPARE(clock->property("header").toString(), QStringLiteral("Paused"));
        QCOMPARE(clock->property("subText").toString(), QString{});
        m_app.nodeModel().setPause(false);
        QTRY_COMPARE(clock->property("state").toString(), QStringLiteral("CONNECTING"));
        QTRY_VERIFY(qmltest::Call(m_app, &interfaces::Node::getNetworkActive));
        QCOMPARE(clock->property("header").toString(), QStringLiteral("Connecting"));
        QCOMPARE(clock->property("subText").toString(), QStringLiteral("Please wait"));
    }

    void chainSyncFollowsRealCoreBlockNotifications()
    {
        auto* model = &m_app.nodeModel();
        QVERIFY(model);
        const int previous_height = qmltest::Call(m_app, &interfaces::Node::getNumBlocks);
        QSignalSpy tips{model, &NodeModel::blockTipHeightChanged};
        UniValue params{UniValue::VARR};
        params.push_back(1);
        params.push_back("raw(51)");
        QCOMPARE(qmltest::Call(m_app, &interfaces::Node::executeRpc, "generatetodescriptor", params, "").size(), 1U);
        QTRY_COMPARE(model->blockTipHeight(), previous_height + 1);
        QCOMPARE(model->blockTipHeight(), qmltest::Call(m_app, &interfaces::Node::getNumBlocks));
        QVERIFY(!tips.isEmpty());
        QVERIFY(model->verificationProgress() > 0.0);
    }
};

std::unique_ptr<QObject> CreateBlockClockTests(BitcoinQmlApplication& app, WalletMode)
{
    return std::make_unique<BlockClockIntegrationTests>(app);
}

#include <test_blockclock.moc>
