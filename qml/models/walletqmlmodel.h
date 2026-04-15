// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QML_MODELS_WALLETQMLMODEL_H
#define BITCOIN_QML_MODELS_WALLETQMLMODEL_H

#include <qml/models/activitylistmodel.h>
#include <qml/models/coinslistmodel.h>
#include <qml/models/paymentrequest.h>
#include <qml/models/sendrecipient.h>
#include <qml/models/sendrecipientslistmodel.h>
#include <qml/models/walletqmlmodeltransaction.h>

#include <consensus/amount.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <wallet/coincontrol.h>

#include <memory>
#include <optional>
#include <vector>

#include <QObject>

class WalletQmlModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString balance READ balance NOTIFY balanceChanged)
    Q_PROPERTY(ActivityListModel* activityListModel READ activityListModel CONSTANT)
    Q_PROPERTY(CoinsListModel* coinsListModel READ coinsListModel CONSTANT)
    Q_PROPERTY(SendRecipientsListModel* recipients READ sendRecipientList CONSTANT)
    Q_PROPERTY(PaymentRequest* currentPaymentRequest READ currentPaymentRequest CONSTANT)
    Q_PROPERTY(WalletQmlModelTransaction* currentTransaction READ currentTransaction NOTIFY currentTransactionChanged)
    Q_PROPERTY(unsigned int targetBlocks READ feeTargetBlocks WRITE setFeeTargetBlocks NOTIFY feeTargetBlocksChanged)
    Q_PROPERTY(bool isWalletLoaded READ isWalletLoaded NOTIFY walletIsLoadedChanged)
    Q_PROPERTY(bool isEncrypted READ isEncrypted NOTIFY securityStateChanged)
    Q_PROPERTY(bool isLocked READ isLocked NOTIFY securityStateChanged)
    Q_PROPERTY(QString transactionError READ transactionError NOTIFY transactionErrorChanged)
    Q_PROPERTY(bool transactionNeedsUnlock READ transactionNeedsUnlock NOTIFY transactionNeedsUnlockChanged)

public:
    WalletQmlModel(std::unique_ptr<interfaces::Wallet> wallet, QObject* parent = nullptr);
    WalletQmlModel(QObject *parent = nullptr);
    ~WalletQmlModel();

    QString name() const;
    QString balance() const;
    CAmount balanceSatoshi() const;
    Q_INVOKABLE void commitPaymentRequest();

    ActivityListModel* activityListModel() const { return m_activity_list_model; }
    CoinsListModel* coinsListModel() const { return m_coins_list_model; }
    SendRecipientsListModel* sendRecipientList() const { return m_send_recipients; }
    PaymentRequest* currentPaymentRequest() const { return m_current_payment_request; }
    WalletQmlModelTransaction* currentTransaction() const { return m_current_transaction; }
    Q_INVOKABLE bool prepareTransaction();
    Q_INVOKABLE bool prepareTransactionWithPassphrase(const QString& passphrase);
    Q_INVOKABLE bool sendTransaction();
    Q_INVOKABLE bool sendTransactionWithPassphrase(const QString& passphrase);
    Q_INVOKABLE QString newAddress(QString label);

    std::set<interfaces::WalletTx> getWalletTxs() const;
    interfaces::WalletTx getWalletTx(const uint256& hash) const;
    bool tryGetTxStatus(const uint256& txid,
                        interfaces::WalletTxStatus& tx_status,
                        int& num_blocks,
                        int64_t& block_time) const;
    QString getAddressLabel(const QString& address) const;

    using TransactionChangedFn = std::function<void(const uint256& txid, ChangeType status)>;
    virtual std::unique_ptr<interfaces::Handler> handleTransactionChanged(TransactionChangedFn fn);

    interfaces::Wallet::CoinsList listCoins() const;
    bool lockCoin(const COutPoint& output);
    bool unlockCoin(const COutPoint& output);
    bool isLockedCoin(const COutPoint& output);
    void listLockedCoins(std::vector<COutPoint>& outputs);
    void selectCoin(const COutPoint& output);
    void unselectCoin(const COutPoint& output);
    bool isSelectedCoin(const COutPoint& output);
    std::vector<COutPoint> listSelectedCoins() const;
    unsigned int feeTargetBlocks() const;
    void setFeeTargetBlocks(unsigned int target_blocks);

    bool isWalletLoaded() const { return m_is_wallet_loaded; }
    void setWalletLoaded(bool loaded);
    bool isEncrypted() const { return m_is_encrypted; }
    bool isLocked() const { return m_is_locked; }
    QString transactionError() const { return m_transaction_error; }
    bool transactionNeedsUnlock() const { return m_transaction_needs_unlock; }

Q_SIGNALS:
    void nameChanged();
    void balanceChanged();
    void currentTransactionChanged();
    void feeTargetBlocksChanged();
    void walletIsLoadedChanged();
    void securityStateChanged();
    void transactionErrorChanged();
    void transactionNeedsUnlockChanged();

private:
    unsigned int nextPaymentRequestId() const;
    void subscribeToWalletSignals();
    void unsubscribeFromWalletSignals();
    void refreshSecurityState();
    bool prepareTransactionInternal(const std::optional<QString>& passphrase);
    bool sendTransactionInternal(const std::optional<QString>& passphrase);
    bool unlockForAction(const std::optional<QString>& passphrase, bool& relock);
    void clearTransactionStatus();
    void setTransactionStatus(const QString& error, bool needs_unlock = false);

    std::unique_ptr<interfaces::Wallet> m_wallet;
    ActivityListModel* m_activity_list_model{nullptr};
    CoinsListModel* m_coins_list_model{nullptr};
    SendRecipientsListModel* m_send_recipients{nullptr};
    PaymentRequest* m_current_payment_request{nullptr};
    WalletQmlModelTransaction* m_current_transaction{nullptr};
    wallet::CCoinControl m_coin_control;
    bool m_is_wallet_loaded{false};
    bool m_is_encrypted{false};
    bool m_is_locked{false};
    QString m_transaction_error;
    bool m_transaction_needs_unlock{false};
    std::unique_ptr<interfaces::Handler> m_handler_status_changed;
};

#endif // BITCOIN_QML_MODELS_WALLETQMLMODEL_H
