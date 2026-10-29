// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/integration/runner/test_cases.h>
#include <test/integration/fixture/test_util.h>
#include <qml/models/debuglogmodel.h>
#include <common/args.h>
#include <common/system.h>

#include <QDateTime>
#include <QFile>

class DebugLogIntegrationTests : public QObject
{
    Q_OBJECT
    BitcoinQmlApplication& m_app;

public:
    explicit DebugLogIntegrationTests(BitcoinQmlApplication& app) : m_app{app} {}

private Q_SLOTS:
    void liveLogFilteringAndPagination()
    {
        QFile log{QString::fromStdString(fs::PathToString(gArgs.GetDataDirNet() / "debug.log"))};
        QVERIFY(log.open(QIODevice::WriteOnly | QIODevice::Append));
        for (int i{0}; i < 1200; ++i) {
            const auto line = QString{"2026-01-01T00:00:00.123456Z [net] integration history %1\n"}.arg(i).toUtf8();
            QCOMPARE(log.write(line), line.size());
        }
        QVERIFY(log.flush());
        auto* model = qmltest::Model<DebugLogModel>(m_app, "debugLogModel");
        QVERIFY(model);
        QVERIFY(qmltest::SelectSettings(m_app, "debug-log"));
        QVERIFY(qmltest::WaitForVisible(m_app, "debugLogView"));
        QTRY_VERIFY(model->active());
        QTRY_COMPARE(model->rowCount(), model->loadLimit());
        QVERIFY(model->hasMoreLines());
        QVERIFY(qmltest::SetText(m_app, "debugLogSearchField", "integration live"));
        QTRY_COMPARE(model->rowCount(), 0);
        const QByteArray lines{"2026-01-01T00:00:00.654321Z [net:warning] integration live warning\n"
                               "2026-01-01T00:00:01.654321Z [rpc:error] integration live error\n"
                               "2026-01-01T00:00:02.654321Z [net] integration live normal\n"};
        QCOMPARE(log.write(lines), lines.size());
        log.close();
        QTRY_COMPARE_WITH_TIMEOUT(model->rowCount(), 3, 10'000);
        QCOMPARE(model->data(model->index(0), DebugLogModel::TimestampRole).toString(),
                 QDateTime::fromString("2026-01-01T00:00:00.654321Z", Qt::ISODateWithMs).toLocalTime().toString("HH:mm:ss"));
        model->setWarningsAndErrorsOnly(true);
        QTRY_COMPARE(model->rowCount(), 2);
        QVERIFY(model->data(model->index(1), DebugLogModel::IsErrorRole).toBool());
        QVERIFY(model->data(model->index(0), DebugLogModel::IsWarningRole).toBool());
        model->setWarningsAndErrorsOnly(false);
        QVERIFY(qmltest::SetText(m_app, "debugLogSearchField", ""));
        model->loadMore();
        QTRY_VERIFY(model->rowCount() > 1200);
        QVERIFY(qmltest::SetText(m_app, "debugLogSearchField", "integration history 0"));
        QTRY_COMPARE(model->rowCount(), 1);
        QVERIFY(qmltest::ReturnToNode(m_app));
        QTRY_VERIFY(!model->active());
    }
};

std::unique_ptr<QObject> CreateDebugLogTests(BitcoinQmlApplication& app, WalletMode)
{
    return std::make_unique<DebugLogIntegrationTests>(app);
}

#include <test_debug_log.moc>
