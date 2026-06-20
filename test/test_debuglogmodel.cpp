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
    void refresh_resetsOnlyOnContentChange();
};

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
    model.refresh(/*full_load=*/true);
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
