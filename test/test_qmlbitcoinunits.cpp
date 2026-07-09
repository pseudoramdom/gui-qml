// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtTest/QtTest>

#include <qml/bitcoinunits.h>

#include <consensus/amount.h>

class QmlBitcoinUnitsTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void format_btc_basic();
    void format_btc_negative();
    void format_btc_thinSpaceSeparators();
    void format_mbtc_and_ubtc();
    void format_sat_noDecimals();
    void display_unit_mapping_and_labels();
};

void QmlBitcoinUnitsTests::format_btc_basic()
{
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::BTC, 0), QString("0.00000000"));
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::BTC, COIN), QString("1.00000000"));
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::BTC, 123456789), QString("1.23456789"));
}

void QmlBitcoinUnitsTests::format_btc_negative()
{
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::BTC, -1), QString("-0.00000001"));
}

void QmlBitcoinUnitsTests::format_btc_thinSpaceSeparators()
{
    const QString formatted = QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::BTC, 12345 * COIN);
    QCOMPARE(formatted, QString("12") + QChar(0x2009) + QString("345.00000000"));
}

void QmlBitcoinUnitsTests::format_mbtc_and_ubtc()
{
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::mBTC, COIN, false, QmlBitcoinUnits::SeparatorStyle::NEVER), QString("1000.00000"));
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::uBTC, COIN, false, QmlBitcoinUnits::SeparatorStyle::NEVER), QString("1000000.00"));
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::uBTC, 123, false, QmlBitcoinUnits::SeparatorStyle::NEVER), QString("1.23"));
}

void QmlBitcoinUnitsTests::format_sat_noDecimals()
{
    QCOMPARE(QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::SAT, 123), QString("123"));
}

void QmlBitcoinUnitsTests::display_unit_mapping_and_labels()
{
    QCOMPARE(QmlBitcoinUnits::fromDisplayUnit(0), QmlBitcoinUnits::Unit::BTC);
    QCOMPARE(QmlBitcoinUnits::fromDisplayUnit(1), QmlBitcoinUnits::Unit::mBTC);
    QCOMPARE(QmlBitcoinUnits::fromDisplayUnit(2), QmlBitcoinUnits::Unit::uBTC);
    QCOMPARE(QmlBitcoinUnits::fromDisplayUnit(3), QmlBitcoinUnits::Unit::SAT);
    QCOMPARE(QmlBitcoinUnits::fromDisplayUnit(99), QmlBitcoinUnits::Unit::BTC);

    QCOMPARE(QmlBitcoinUnits::label(QmlBitcoinUnits::Unit::mBTC), QString("mBTC"));
    QCOMPARE(QmlBitcoinUnits::label(QmlBitcoinUnits::Unit::uBTC), QString("bits"));
    QCOMPARE(QmlBitcoinUnits::displayLabel(QmlBitcoinUnits::Unit::SAT, 1), QString("sat"));
    QCOMPARE(QmlBitcoinUnits::displayLabel(QmlBitcoinUnits::Unit::SAT, 2), QString("sats"));
}

#ifdef BITCOINQML_NO_TEST_MAIN
#include <test/qt_test_registry.h>
BITCOINQML_REGISTER_QT_TEST(QmlBitcoinUnitsTests)
#else
QTEST_MAIN(QmlBitcoinUnitsTests)
#endif
#include "test_qmlbitcoinunits.moc"
