
// Copyright (c) 2024-2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qml/models/walletqmlmodel.h>

#include <common/messages.h>
#include <qml/models/activitylistmodel.h>
#include <qml/models/paymentrequest.h>
#include <qml/models/sendrecipient.h>
#include <qml/models/sendrecipientslistmodel.h>
#include <qml/models/walletqmlmodeltransaction.h>

#include <consensus/amount.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <addresstype.h>
#include <outputtype.h>
#include <psbt.h>
#include <qml/bitcoinunits.h>
#include <serialize.h>
#include <streams.h>
#include <support/allocators/secure.h>
#include <util/result.h>
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>

#include <QDateTime>
#include <QMetaObject>

namespace {
struct QmlReceiveRequestRecipient
{
    static constexpr int CURRENT_VERSION{1};
    int nVersion{CURRENT_VERSION};
    std::string address;
    std::string label;
    CAmount amount{0};
    std::string message;
    std::string sPaymentRequest;
    std::string authenticatedMerchant;

    SERIALIZE_METHODS(QmlReceiveRequestRecipient, obj)
    {
        READWRITE(obj.nVersion, obj.address, obj.label, obj.amount, obj.message, obj.sPaymentRequest, obj.authenticatedMerchant);
    }
};

struct QmlRecentRequestEntry
{
    static constexpr int CURRENT_VERSION{1};
    int nVersion{CURRENT_VERSION};
    int64_t id{0};
    QDateTime date;
    QmlReceiveRequestRecipient recipient;

    SERIALIZE_METHODS(QmlRecentRequestEntry, obj)
    {
        unsigned int date_timet;
        SER_WRITE(obj, date_timet = obj.date.toSecsSinceEpoch());
        READWRITE(obj.nVersion, obj.id, date_timet, obj.recipient);
        SER_READ(obj, obj.date = QDateTime::fromSecsSinceEpoch(date_timet));
    }
};

QString LocalizedString(const bilingual_str& value)
{
    return QString::fromStdString(value.translated.empty() ? value.original : value.translated);
}

bool NeedsUnlockForPreviewBuild(const QString& error)
{
    return error.contains("Transaction needs a change address", Qt::CaseInsensitive) ||
           error.contains("Keypool ran out", Qt::CaseInsensitive) ||
           error.contains("generate it", Qt::CaseInsensitive);
}
} // namespace

WalletQmlModel::WalletQmlModel(std::unique_ptr<interfaces::Wallet> wallet, QObject *parent)
    : QObject(parent)
{
    m_wallet = std::move(wallet);
    m_activity_list_model = new ActivityListModel(this);
    m_coins_list_model = new CoinsListModel(this);
    m_send_recipients = new SendRecipientsListModel(this);
    m_current_payment_request = new PaymentRequest(this);
    refreshSecurityState();
    subscribeToWalletSignals();
}

WalletQmlModel::WalletQmlModel(QObject* parent)
    : QObject(parent)
{
    m_activity_list_model = new ActivityListModel(this);
    m_coins_list_model = new CoinsListModel(this);
    m_send_recipients = new SendRecipientsListModel(this);
    m_current_payment_request = new PaymentRequest(this);
}

WalletQmlModel::~WalletQmlModel()
{
    unsubscribeFromWalletSignals();
    delete m_activity_list_model;
    delete m_coins_list_model;
    delete m_send_recipients;
    delete m_current_payment_request;
    if (m_current_transaction) {
        delete m_current_transaction;
    }
}

QString WalletQmlModel::balance() const
{
    if (!m_wallet) {
        return "0";
    }
    return QmlBitcoinUnits::format(QmlBitcoinUnits::Unit::BTC, m_wallet->getBalance());
}

CAmount WalletQmlModel::balanceSatoshi() const
{
    if (!m_wallet) {
        return 0;
    }
    return m_wallet->getBalance();
}

QString WalletQmlModel::name() const
{
    if (!m_wallet) {
        return QString();
    }
    return QString::fromStdString(m_wallet->getWalletName());
}

QString WalletQmlModel::newAddress(QString label)
{
    if (!m_wallet) {
        return QString();
    }
    OutputType output_type = m_wallet->getDefaultAddressType();
    util::Result<CTxDestination> dest{m_wallet->getNewDestination(output_type, label.toStdString())};
    return QString::fromStdString(EncodeDestination(dest.value()));
}

void WalletQmlModel::removeWallet()
{
    if (!m_wallet) {
        return;
    }
    m_wallet->remove();
}

void WalletQmlModel::commitPaymentRequest()
{
    if (!m_wallet || !m_current_payment_request) {
        return;
    }

    if (m_current_payment_request->id().isEmpty()) {
        m_current_payment_request->setId(nextPaymentRequestId());
    }

    if (m_current_payment_request->address().isEmpty()) {
        const OutputType output_type = m_wallet->getDefaultAddressType();
        const auto destination{m_wallet->getNewDestination(output_type, m_current_payment_request->label().toStdString())};
        if (!destination) {
            return;
        }
        m_current_payment_request->setDestination(destination.value());
    }

    bool parse_ok{false};
    const int64_t request_id{m_current_payment_request->id().toLongLong(&parse_ok)};
    if (!parse_ok || request_id <= 0) {
        return;
    }

    QmlRecentRequestEntry request_entry;
    request_entry.id = request_id;
    request_entry.date = QDateTime::currentDateTime();
    request_entry.recipient.address = m_current_payment_request->address().toStdString();
    request_entry.recipient.label = m_current_payment_request->label().toStdString();
    request_entry.recipient.amount = m_current_payment_request->amount()->satoshi();
    request_entry.recipient.message = m_current_payment_request->message().toStdString();

    DataStream ss{};
    ss << request_entry;

    m_wallet->setAddressReceiveRequest(
        m_current_payment_request->destination(),
        m_current_payment_request->id().toStdString(),
        ss.str());
}

unsigned int WalletQmlModel::nextPaymentRequestId() const
{
    if (!m_wallet) {
        return 1;
    }

    int64_t max_id{0};
    for (const std::string& request : m_wallet->getAddressReceiveRequests()) {
        std::vector<uint8_t> data(request.begin(), request.end());
        DataStream ss{data};
        QmlRecentRequestEntry entry;
        try {
            ss >> entry;
        } catch (const std::ios_base::failure&) {
            continue;
        }
        if (entry.id > max_id) {
            max_id = entry.id;
        }
    }

    if (max_id <= 0 || max_id >= std::numeric_limits<unsigned int>::max() - 1) {
        return 1;
    }

    return static_cast<unsigned int>(max_id + 1);
}

std::set<interfaces::WalletTx> WalletQmlModel::getWalletTxs() const
{
    if (!m_wallet) {
        return {};
    }
    return m_wallet->getWalletTxs();
}

interfaces::WalletTx WalletQmlModel::getWalletTx(const uint256& hash) const
{
    if (!m_wallet) {
        return {};
    }
    return m_wallet->getWalletTx(Txid::FromUint256(hash));
}

bool WalletQmlModel::tryGetTxStatus(const uint256& txid,
                                    interfaces::WalletTxStatus& tx_status,
                                    int& num_blocks,
                                    int64_t& block_time) const
{
    if (!m_wallet) {
        return false;
    }
    return m_wallet->tryGetTxStatus(Txid::FromUint256(txid), tx_status, num_blocks, block_time);
}

QString WalletQmlModel::getAddressLabel(const QString& address) const
{
    if (!m_wallet || address.isEmpty()) {
        return {};
    }

    const CTxDestination destination = DecodeDestination(address.toStdString());
    if (!IsValidDestination(destination)) {
        return {};
    }

    std::string label;
    if (!m_wallet->getAddress(destination, &label, nullptr, nullptr)) {
        return {};
    }

    return QString::fromStdString(label);
}

std::unique_ptr<interfaces::Handler> WalletQmlModel::handleTransactionChanged(TransactionChangedFn fn)
{
    if (!m_wallet) {
        return nullptr;
    }
    return m_wallet->handleTransactionChanged(fn);
}

bool WalletQmlModel::prepareTransaction()
{
    return prepareTransactionInternal(std::nullopt);
}

bool WalletQmlModel::prepareTransactionWithPassphrase(const QString& passphrase)
{
    return prepareTransactionInternal(passphrase);
}

bool WalletQmlModel::prepareTransactionInternal(const std::optional<QString>& passphrase)
{
    clearTransactionStatus();
    if (!m_wallet || !m_send_recipients || m_send_recipients->recipients().empty()) {
        setTransactionStatus(tr("Enter at least one valid recipient to continue."));
        return false;
    }

    bool relock{false};
    if (!unlockForAction(passphrase, relock)) {
        return false;
    }

    std::vector<wallet::CRecipient> vecSend;
    CAmount total = 0;
    for (auto* recipient : m_send_recipients->recipients()) {
        CTxDestination destination = DecodeDestination(recipient->address()->address().toStdString());
        wallet::CRecipient c_recipient = {destination, recipient->cAmount(), recipient->subtractFeeFromAmount()};
        m_coin_control.m_feerate = CFeeRate(1000);
        vecSend.push_back(c_recipient);
        total += recipient->cAmount();
    }

    CAmount balance = m_wallet->getBalance();
    if (balance < total) {
        if (relock) {
            m_wallet->lock();
            refreshSecurityState();
        }
        setTransactionStatus(tr("The wallet does not have enough balance for this transaction."));
        return false;
    }

    int nChangePosRet = -1;
    CAmount nFeeRequired = 0;
    const auto& res = m_wallet->createTransaction(vecSend, m_coin_control, false, nChangePosRet, nFeeRequired);
    if (res) {
        if (m_current_transaction) {
            delete m_current_transaction;
        }
        CTransactionRef newTx = *res;
        m_current_transaction = new WalletQmlModelTransaction(m_send_recipients, this);
        m_current_transaction->setWtx(newTx);
        m_current_transaction->setTransactionFee(nFeeRequired);
        if (relock) {
            m_wallet->lock();
            refreshSecurityState();
        }
        Q_EMIT currentTransactionChanged();
        return true;
    }

    if (relock) {
        m_wallet->lock();
        refreshSecurityState();
    }
    const QString error = LocalizedString(util::ErrorString(res));
    const bool needs_unlock = !passphrase.has_value() && m_is_encrypted && m_is_locked && NeedsUnlockForPreviewBuild(error);
    setTransactionStatus(error, needs_unlock);
    return false;
}

bool WalletQmlModel::sendTransaction()
{
    return sendTransactionInternal(std::nullopt);
}

bool WalletQmlModel::sendTransactionWithPassphrase(const QString& passphrase)
{
    return sendTransactionInternal(passphrase);
}

bool WalletQmlModel::sendTransactionInternal(const std::optional<QString>& passphrase)
{
    clearTransactionStatus();
    if (!m_wallet || !m_current_transaction) {
        setTransactionStatus(tr("Review a transaction before sending it."));
        return false;
    }
    if (m_wallet->isCrypted() && m_wallet->isLocked() && !passphrase.has_value()) {
        setTransactionStatus(tr("Enter your wallet password to sign this transaction."));
        return false;
    }

    CTransactionRef preview_tx = m_current_transaction->getWtx();
    if (!preview_tx) {
        setTransactionStatus(tr("Review a transaction before sending it."));
        return false;
    }

    bool relock{false};
    if (!unlockForAction(passphrase, relock)) {
        return false;
    }

    CMutableTransaction mutable_tx(*preview_tx);
    PartiallySignedTransaction psbtx(mutable_tx);
    bool complete{false};
    if (const auto err = m_wallet->fillPSBT(std::nullopt, /*sign=*/false, /*bip32derivs=*/true, nullptr, psbtx, complete)) {
        if (relock) {
            m_wallet->lock();
            refreshSecurityState();
        }
        setTransactionStatus(LocalizedString(common::PSBTErrorString(*err)));
        return false;
    }
    if (const auto err = m_wallet->fillPSBT(std::nullopt, /*sign=*/true, /*bip32derivs=*/false, nullptr, psbtx, complete)) {
        if (relock) {
            m_wallet->lock();
            refreshSecurityState();
        }
        setTransactionStatus(LocalizedString(common::PSBTErrorString(*err)));
        return false;
    }

    CMutableTransaction finalized_tx;
    if (!FinalizeAndExtractPSBT(psbtx, finalized_tx)) {
        if (relock) {
            m_wallet->lock();
            refreshSecurityState();
        }
        setTransactionStatus(tr("The transaction could not be finalized."));
        return false;
    }

    CTransactionRef new_tx = MakeTransactionRef(std::move(finalized_tx));
    interfaces::WalletValueMap value_map;
    interfaces::WalletOrderForm order_form;
    m_wallet->commitTransaction(new_tx, value_map, order_form);
    m_current_transaction->setWtx(new_tx);

    if (relock) {
        m_wallet->lock();
        refreshSecurityState();
    }
    clearTransactionStatus();
    return true;
}

interfaces::Wallet::CoinsList WalletQmlModel::listCoins() const
{
    if (!m_wallet) {
        return {};
    }
    return m_wallet->listCoins();
}

bool WalletQmlModel::lockCoin(const COutPoint& output)
{
    if (!m_wallet) {
        return false;
    }
    return m_wallet->lockCoin(output, true);
}

bool WalletQmlModel::unlockCoin(const COutPoint& output)
{
    if (!m_wallet) {
        return false;
    }
    return m_wallet->unlockCoin(output);
}

bool WalletQmlModel::isLockedCoin(const COutPoint& output)
{
    if (!m_wallet) {
        return false;
    }
    return m_wallet->isLockedCoin(output);
}

void WalletQmlModel::listLockedCoins(std::vector<COutPoint>& outputs)
{
    if (!m_wallet) {
        return;
    }
    m_wallet->listLockedCoins(outputs);
}

void WalletQmlModel::selectCoin(const COutPoint& output)
{
    m_coin_control.Select(output);
}

void WalletQmlModel::unselectCoin(const COutPoint& output)
{
    m_coin_control.UnSelect(output);
}

bool WalletQmlModel::isSelectedCoin(const COutPoint& output)
{
    return m_coin_control.IsSelected(output);
}

std::vector<COutPoint> WalletQmlModel::listSelectedCoins() const
{
    return m_coin_control.ListSelected();
}

unsigned int WalletQmlModel::feeTargetBlocks() const
{
    return m_coin_control.m_confirm_target.value_or(wallet::DEFAULT_TX_CONFIRM_TARGET);
}

void WalletQmlModel::setFeeTargetBlocks(unsigned int target_blocks)
{
    if (m_coin_control.m_confirm_target != target_blocks) {
        m_coin_control.m_confirm_target = target_blocks;
        Q_EMIT feeTargetBlocksChanged();
    }
}

void WalletQmlModel::subscribeToWalletSignals()
{
    if (!m_wallet) {
        return;
    }
    m_handler_status_changed = m_wallet->handleStatusChanged([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            refreshSecurityState();
        }, Qt::QueuedConnection);
    });
}

void WalletQmlModel::unsubscribeFromWalletSignals()
{
    if (m_handler_status_changed) {
        m_handler_status_changed->disconnect();
    }
}

void WalletQmlModel::refreshSecurityState()
{
    const bool encrypted = m_wallet ? m_wallet->isCrypted() : false;
    const bool locked = m_wallet ? m_wallet->isLocked() : false;
    if (m_is_encrypted != encrypted || m_is_locked != locked) {
        m_is_encrypted = encrypted;
        m_is_locked = locked;
        Q_EMIT securityStateChanged();
    }
}

bool WalletQmlModel::unlockForAction(const std::optional<QString>& passphrase, bool& relock)
{
    relock = false;
    if (!m_wallet || !m_wallet->isCrypted() || !m_wallet->isLocked()) {
        return true;
    }
    if (!passphrase.has_value()) {
        return true;
    }

    const SecureString secure_passphrase{passphrase->toStdString()};
    if (!m_wallet->unlock(secure_passphrase)) {
        setTransactionStatus(tr("The wallet password you entered was incorrect."));
        return false;
    }

    relock = true;
    refreshSecurityState();
    return true;
}

void WalletQmlModel::clearTransactionStatus()
{
    setTransactionStatus(QString());
}

void WalletQmlModel::setTransactionStatus(const QString& error, bool needs_unlock)
{
    if (m_transaction_error != error) {
        m_transaction_error = error;
        Q_EMIT transactionErrorChanged();
    }
    if (m_transaction_needs_unlock != needs_unlock) {
        m_transaction_needs_unlock = needs_unlock;
        Q_EMIT transactionNeedsUnlockChanged();
    }
}
