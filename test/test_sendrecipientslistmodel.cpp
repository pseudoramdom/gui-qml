// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtTest/QtTest>

#include <qml/models/sendrecipient.h>
#include <qml/models/sendrecipientslistmodel.h>

#include <QCoreApplication>
#include <QEvent>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QUrl>

#include <memory>

class SendRecipientsListModelTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clearKeepsCurrentRecipientValidForQmlBindings();
    void unitChangesApplyToEveryRecipient();
};

void SendRecipientsListModelTests::clearKeepsCurrentRecipientValidForQmlBindings()
{
    SendRecipientsListModel recipients;
    recipients.currentRecipient()->address()->setAddress(QStringLiteral("first"));
    recipients.currentRecipient()->setLabel(QStringLiteral("First recipient"));
    recipients.currentRecipient()->amount()->setSatoshi(1'000);
    recipients.add();
    recipients.currentRecipient()->address()->setAddress(QStringLiteral("second"));
    recipients.currentRecipient()->setLabel(QStringLiteral("Second recipient"));
    recipients.currentRecipient()->amount()->setSatoshi(2'000);

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("recipientsModel"), &recipients);
    QQmlComponent component{&engine};
    component.setData(R"QML(
        import QtQml

        QtObject {
            property QtObject currentRecipient: recipientsModel.current
            property bool sawNullRecipient: false
            property string address: currentRecipient.address.address
            property string label: currentRecipient.label
            property QtObject amount: currentRecipient.amount

            onCurrentRecipientChanged: {
                if (currentRecipient === null) sawNullRecipient = true
            }
        }
    )QML", QUrl{});
    std::unique_ptr<QObject> observer{component.create()};
    QVERIFY2(observer, qPrintable(component.errorString()));
    QCOMPARE(observer->property("address").toString(), QStringLiteral("second"));
    QCOMPARE(observer->property("label").toString(), QStringLiteral("Second recipient"));

    recipients.clear();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QVERIFY(!observer->property("sawNullRecipient").toBool());
    QCOMPARE(observer->property("currentRecipient").value<QObject*>(), recipients.currentRecipient());
    QVERIFY(observer->property("address").toString().isEmpty());
    QVERIFY(observer->property("label").toString().isEmpty());
    QCOMPARE(observer->property("amount").value<QObject*>(), recipients.currentRecipient()->amount());
}

void SendRecipientsListModelTests::unitChangesApplyToEveryRecipient()
{
    SendRecipientsListModel recipients;
    auto* first = recipients.currentRecipient();
    first->amount()->setSatoshi(COIN / 2);

    recipients.add();
    auto* second = recipients.currentRecipient();
    second->amount()->setSatoshi(2'000);
    second->amount()->setUnit(BitcoinAmount::Unit::SAT);

    QCOMPARE(first->amount()->unit(), BitcoinAmount::Unit::SAT);
    QCOMPARE(second->amount()->unit(), BitcoinAmount::Unit::SAT);
    QCOMPARE(
        recipients.data(recipients.index(0, 0), SendRecipientsListModel::AmountRole).toString(),
        QStringLiteral("50000000"));
    QCOMPARE(
        recipients.data(recipients.index(0, 0), SendRecipientsListModel::AmountUnitLabelRole).toString(),
        QStringLiteral("sats"));

    first->amount()->setUnit(BitcoinAmount::Unit::BTC);
    QCOMPARE(first->amount()->unit(), BitcoinAmount::Unit::BTC);
    QCOMPARE(second->amount()->unit(), BitcoinAmount::Unit::BTC);
}

#ifdef BITCOINQML_NO_TEST_MAIN
#include <test/qt_test_registry.h>
BITCOINQML_REGISTER_QT_TEST(SendRecipientsListModelTests)
#else
QTEST_MAIN(SendRecipientsListModelTests)
#endif
#include "test_sendrecipientslistmodel.moc"
