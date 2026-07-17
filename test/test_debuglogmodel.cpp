// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtTest/QtTest>

#include <QByteArray>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <qml/models/debuglogmodel.h>

#include <util/fs.h>

class DebugLogModelTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inactiveModel_ignoresRefreshUntilActivated();
    void parsed_roles_extract_structured_log_lines();
    void refresh_resetsOnlyOnContentChange();
};

void DebugLogModelTests::inactiveModel_ignoresRefreshUntilActivated()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString log_path = dir.filePath("debug.log");

    QFile file(log_path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("2026-06-19T10:00:00Z line one\n");
    file.close();

    DebugLogModel model(fs::PathFromString(log_path.toStdString()));
    QVERIFY(!model.active());

    model.refresh(/*full_load=*/true);
    QTest::qWait(50);
    QCOMPARE(model.rowCount(), 0);

    model.setActive(true);
    QTRY_COMPARE(model.rowCount(), 1);

    model.setActive(false);
    QVERIFY(!model.active());

    QVERIFY(file.open(QIODevice::Append | QIODevice::WriteOnly));
    file.write("2026-06-19T10:00:01Z line two\n");
    file.close();
    model.refresh(/*full_load=*/true);
    QTest::qWait(50);
    QCOMPARE(model.rowCount(), 1);

    model.setActive(true);
    QTRY_COMPARE(model.rowCount(), 2);
}

void DebugLogModelTests::parsed_roles_extract_structured_log_lines()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString log_path = dir.filePath("debug.log");

    auto append_line = [&](const QByteArray& line) {
        QFile file(log_path);
        QVERIFY(file.open(QIODevice::Append | QIODevice::WriteOnly));
        file.write(line + '\n');
    };

    append_line("2026-06-19T09:59:59Z connect() to 127.0.0.1:9050 failed after wait: Connection refused (61)");
    append_line("2026-06-19T10:00:00Z Writing 0 mempool transactions to file...");
    append_line("2026-06-19T10:00:01Z ERROR: boom <bad>");
    append_line("2026-06-19T10:00:02Z UpdateTip: new best=abc height=1");

    DebugLogModel model(fs::PathFromString(log_path.toStdString()));
    model.setActive(true);
    QTRY_COMPARE(model.rowCount(), 4);

    const QModelIndex update_tip = model.index(0, 0);
    QCOMPARE(model.data(update_tip, DebugLogModel::LineNumberRole).toString(), QStringLiteral("1"));
    QCOMPARE(model.data(update_tip, DebugLogModel::CommandRole).toString(), QStringLiteral("UpdateTip"));
    QCOMPARE(model.data(update_tip, DebugLogModel::MessageRole).toString(), QStringLiteral("new best=abc height=1"));
    QCOMPARE(model.data(update_tip, DebugLogModel::ContentRole).toString(), QStringLiteral("UpdateTip: new best=abc height=1"));
    QCOMPARE(model.data(update_tip, DebugLogModel::SeverityRole).toInt(), int(DebugLogModel::InfoSeverity));
    QVERIFY(!model.data(update_tip, DebugLogModel::DateLabelRole).toString().isEmpty());

    const QModelIndex error = model.index(1, 0);
    QCOMPARE(model.data(error, DebugLogModel::CommandRole).toString(), QStringLiteral("ERROR"));
    QCOMPARE(model.data(error, DebugLogModel::MessageRole).toString(), QStringLiteral("boom <bad>"));
    QCOMPARE(model.data(error, DebugLogModel::ContentRole).toString(), QStringLiteral("ERROR: boom &lt;bad&gt;"));
    QCOMPARE(model.data(error, DebugLogModel::SeverityRole).toInt(), int(DebugLogModel::ErrorSeverity));

    const QModelIndex plain = model.index(2, 0);
    QCOMPARE(model.data(plain, DebugLogModel::CommandRole).toString(), QString{});
    QCOMPARE(model.data(plain, DebugLogModel::MessageRole).toString(), QStringLiteral("Writing 0 mempool transactions to file..."));
    QCOMPARE(model.data(plain, DebugLogModel::SeverityRole).toInt(), int(DebugLogModel::InfoSeverity));

    const QModelIndex endpoint = model.index(3, 0);
    QCOMPARE(model.data(endpoint, DebugLogModel::CommandRole).toString(), QString{});
    QCOMPARE(model.data(endpoint, DebugLogModel::MessageRole).toString(),
             QStringLiteral("connect() to 127.0.0.1:9050 failed after wait: Connection refused (61)"));
    QCOMPARE(model.data(endpoint, DebugLogModel::SeverityRole).toInt(), int(DebugLogModel::InfoSeverity));
}

// buildDisplayLines() skips the model reset when the displayed lines are
// unchanged, so a redundant refresh on an idle log does not force the
// (non-virtualised) viewer to rebuild every delegate on the GUI thread.
//
// The model reads on a worker thread, so the test asserts the observable
// consequence: the number of modelReset() emissions equals the number of real
// content changes, regardless of how many refresh() calls are issued in
// between. Lines are appended (never truncated) in a single write each, so a
// concurrent read can only ever observe a complete prefix of the file and the
// reset count stays deterministic.
void DebugLogModelTests::refresh_resetsOnlyOnContentChange()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString log_path = dir.filePath("debug.log");

    auto append_line = [&](const QByteArray& line) {
        QFile file(log_path);
        QVERIFY(file.open(QIODevice::Append | QIODevice::WriteOnly));
        file.write(line + '\n');
    };

    append_line("2026-06-19T10:00:00Z line one");
    append_line("2026-06-19T10:00:01Z line two");
    append_line("2026-06-19T10:00:02Z line three");

    DebugLogModel model(fs::PathFromString(log_path.toStdString()));
    model.setActive(true);
    QTRY_COMPARE(model.rowCount(), 3);

    QSignalSpy reset_spy(&model, &QAbstractItemModel::modelReset);

    // First real change: one new line must reset exactly once.
    append_line("2026-06-19T10:00:03Z line four");
    model.refresh(/*full_load=*/true);
    QTRY_COMPARE(model.rowCount(), 4);

    // Redundant refresh on the now-unchanged file: must not reset.
    model.refresh(/*full_load=*/true);

    // Second real change flushes any pending read and resets once more.
    append_line("2026-06-19T10:00:04Z line five");
    model.refresh(/*full_load=*/true);
    QTRY_COMPARE(model.rowCount(), 5);

    // Two content changes, three refreshes: exactly two resets.
    QCOMPARE(reset_spy.count(), 2);
}

#ifdef BITCOINQML_NO_TEST_MAIN
#include <test/qt_test_registry.h>
BITCOINQML_REGISTER_QT_TEST(DebugLogModelTests)
#else
QTEST_MAIN(DebugLogModelTests)
#endif
#include "test_debuglogmodel.moc"
