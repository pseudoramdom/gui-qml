// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qml/models/options_model.h>

#include <common/args.h>
#include <common/settings.h>
#include <common/system.h>
#include <interfaces/node.h>
#include <mapport.h>
#include <netbase.h>
#include <node/caches.h>
#include <node/chainstatemanager_args.h>
#include <qml/guiconstants.h>
#include <txdb.h>
#include <univalue.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <validation.h>

#include <cassert>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QUrl>

namespace {
int PruneMiBtoGB(int64_t mib)
{
    return (mib * 1024 * 1024 + GB_BYTES - 1) / GB_BYTES;
}

int64_t PruneGBtoMiB(int gb)
{
    return gb * GB_BYTES / 1024 / 1024;
}

QString NormalizeCommandPath(const QString& path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QUrl url(trimmed);
    return url.isLocalFile() ? url.toLocalFile() : trimmed;
}

QString FirstCommandToken(const QString& command)
{
    static const QRegularExpression TOKEN_RE(QStringLiteral(R"re(^\s*(?:"([^"]+)"|'([^']+)'|(\S+)))re"));
    const auto match = TOKEN_RE.match(command);
    if (!match.hasMatch()) {
        return {};
    }
    for (int i = 1; i <= 3; ++i) {
        const QString captured = match.captured(i);
        if (!captured.isEmpty()) {
            return captured;
        }
    }
    return {};
}

QString ExpandUserPath(const QString& path)
{
    if (path == QStringLiteral("~")) {
        return QDir::homePath();
    }
    if (path.startsWith(QStringLiteral("~/"))) {
        return QDir::homePath() + path.mid(1);
    }
    return path;
}

bool TokenLooksLikePath(const QString& token)
{
    return token.startsWith(QStringLiteral("/")) ||
           token.startsWith(QStringLiteral("./")) ||
           token.startsWith(QStringLiteral("../")) ||
           token.startsWith(QStringLiteral("~/")) ||
           token.contains(QLatin1Char('/')) ||
           token.contains(QLatin1Char('\\')) ||
           QDir::isAbsolutePath(token);
}

constexpr const char* DEFAULT_PROXY_HOST{"127.0.0.1"};
constexpr int DEFAULT_PROXY_PORT{9050};
constexpr const char* MONEY_FONT_EMBEDDED{"embedded"};
constexpr const char* MONEY_FONT_BEST_SYSTEM{"best_system"};

QString DefaultProxyAddress()
{
    return QStringLiteral("%1:%2").arg(DEFAULT_PROXY_HOST).arg(DEFAULT_PROXY_PORT);
}

QString NormalizeLocalPath(QString path)
{
    path = path.trimmed();
    if (path.isEmpty()) return {};
    const QUrl url(path);
    if (url.isLocalFile()) return url.toLocalFile();
    return path;
}

fs::path QStringToPath(const QString& path)
{
    return fs::u8path(path.toStdString());
}

QString ProxyValidationError(const QString& location)
{
    const QString trimmed = location.trimmed();
    if (trimmed.isEmpty()) {
        return QObject::tr("Proxy location is required.");
    }

    const std::string value = trimmed.toStdString();
    if (value.starts_with(ADDR_PREFIX_UNIX)) {
        if (IsUnixSocketPath(value)) return {};
        return QObject::tr("This Unix socket proxy path is not supported on this platform or is too long.");
    }

    uint16_t port{0};
    std::string host;
    if (!SplitHostPort(value, port, host) || port == 0) {
        return QObject::tr("Enter a proxy location as host:port, [IPv6]:port, or unix:/path.");
    }

    const CService service{LookupNumeric(host, port)};
    const Proxy proxy{service, /*tor_stream_isolation=*/true};
    if (!proxy.IsValid()) {
        return QObject::tr("The supplied proxy address is invalid.");
    }
    return {};
}
} // namespace

OptionsQmlModel::OptionsQmlModel(interfaces::Node& node, bool is_onboarded)
    : m_node{node}
    , m_onboarded{is_onboarded}
{
    m_dbcache_size_mib = SettingToInt(m_node.getPersistentSetting("dbcache"), DEFAULT_DB_CACHE >> 20);

    m_listen = SettingToBool(m_node.getPersistentSetting("listen"), DEFAULT_LISTEN);

    m_max_mempool_size_mb = SettingToInt(m_node.getPersistentSetting("maxmempool"), DEFAULT_MAX_MEMPOOL_SIZE_MB);

    m_natpmp = SettingToBool(m_node.getPersistentSetting("natpmp"), DEFAULT_NATPMP);

    int64_t prune_value{SettingToInt(m_node.getPersistentSetting("prune"), 0)};
    m_prune = (prune_value > 1);
    m_prune_size_gb = m_prune ? PruneMiBtoGB(prune_value) : DEFAULT_PRUNE_TARGET_GB;

    m_script_threads = SettingToInt(m_node.getPersistentSetting("par"), DEFAULT_SCRIPTCHECK_THREADS);

    m_server = SettingToBool(m_node.getPersistentSetting("server"), false);

    QString proxy_setting = QString::fromStdString(SettingToString(m_node.getPersistentSetting("proxy"), ""));
    if (proxy_setting == "0") proxy_setting.clear();
    m_proxy_enabled = !proxy_setting.isEmpty();
    m_proxy_address = m_proxy_enabled
        ? proxy_setting
        : QString::fromStdString(SettingToString(m_node.getPersistentSetting("proxy-prev"), ""));

    QString onion_setting = QString::fromStdString(SettingToString(m_node.getPersistentSetting("onion"), ""));
    if (onion_setting == "0") onion_setting.clear();
    m_tor_enabled = !onion_setting.isEmpty();
    m_tor_address = m_tor_enabled
        ? onion_setting
        : QString::fromStdString(SettingToString(m_node.getPersistentSetting("onion-prev"), ""));

    m_external_signer_path = QString::fromStdString(SettingToString(m_node.getPersistentSetting("signer"), ""));

    resetDirtySnapshots();

    QSettings settings;
    m_dataDir = settings.value(SettingsKeys::DATA_DIR, getDefaultDataDirString()).toString();
    if (m_dataDir != getDefaultDataDirString()) {
        m_custom_datadir_string = m_dataDir;
    }
    m_language = settings.value(SettingsKeys::LANGUAGE, "").toString();
    m_display_unit = settings.value(SettingsKeys::DISPLAY_UNIT, 0).toInt();
    m_third_party_transaction_urls = settings.value(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS, "").toString();
    m_money_font_choice = settings.value(SettingsKeys::MONEY_FONT_CHOICE, MONEY_FONT_EMBEDDED).toString();
    if (m_money_font_choice != MONEY_FONT_EMBEDDED && m_money_font_choice != MONEY_FONT_BEST_SYSTEM) {
        m_money_font_choice = MONEY_FONT_EMBEDDED;
    }

    buildAvailableLanguages();
}

bool OptionsQmlModel::connectionSettingsDirty() const
{
    if (!m_onboarded) return false;
    return m_listen != m_initial_listen || m_server != m_initial_server;
}

bool OptionsQmlModel::storageSettingsDirty() const
{
    if (!m_onboarded) return false;
    if (m_prune != m_initial_prune) return true;
    return m_prune && m_prune_size_gb != m_initial_prune_size_gb;
}

bool OptionsQmlModel::developerSettingsDirty() const
{
    if (!m_onboarded) return false;
    return m_dbcache_size_mib != m_initial_dbcache_size_mib ||
           m_max_mempool_size_mb != m_initial_max_mempool_size_mb ||
           m_script_threads != m_initial_script_threads;
}

bool OptionsQmlModel::restartRequired() const
{
    return connectionSettingsDirty() ||
           storageSettingsDirty() ||
           developerSettingsDirty() ||
           proxySettingsDirty() ||
           walletSettingsDirty();
}

OptionsQmlModel::DirtySnapshot OptionsQmlModel::dirtySnapshot() const
{
    DirtySnapshot snapshot;
    snapshot.connection = connectionSettingsDirty();
    snapshot.storage = storageSettingsDirty();
    snapshot.developer = developerSettingsDirty();
    snapshot.proxy = proxySettingsDirty();
    snapshot.wallet = walletSettingsDirty();
    snapshot.restart = restartRequired();
    return snapshot;
}

void OptionsQmlModel::emitDirtySignals(const DirtySnapshot& before)
{
    if (connectionSettingsDirty() != before.connection) Q_EMIT connectionSettingsDirtyChanged();
    if (storageSettingsDirty() != before.storage) Q_EMIT storageSettingsDirtyChanged();
    if (developerSettingsDirty() != before.developer) Q_EMIT developerSettingsDirtyChanged();
    if (proxySettingsDirty() != before.proxy) Q_EMIT proxySettingsDirtyChanged();
    if (walletSettingsDirty() != before.wallet) Q_EMIT walletSettingsDirtyChanged();
    if (restartRequired() != before.restart) Q_EMIT restartRequiredChanged();
}

void OptionsQmlModel::resetDirtySnapshots()
{
    m_initial_listen = m_listen;
    m_initial_server = m_server;
    m_initial_prune = m_prune;
    m_initial_prune_size_gb = m_prune_size_gb;
    m_initial_dbcache_size_mib = m_dbcache_size_mib;
    m_initial_max_mempool_size_mb = m_max_mempool_size_mb;
    m_initial_script_threads = m_script_threads;
    m_initial_proxy_enabled = m_proxy_enabled;
    m_initial_proxy_address = m_proxy_address;
    m_initial_tor_enabled = m_tor_enabled;
    m_initial_tor_address = m_tor_address;
    m_initial_external_signer_path = m_external_signer_path;
}

common::SettingsValue OptionsQmlModel::proxySetting(bool enabled, const QString& address) const
{
    return enabled && !address.trimmed().isEmpty()
        ? common::SettingsValue{address.trimmed().toStdString()}
        : common::SettingsValue{};
}

bool OptionsQmlModel::writeProxySetting(const QString& key, bool enabled, const QString& address)
{
    const QString trimmed = address.trimmed();
    if (enabled && !validateProxyLocation(trimmed).isEmpty()) return false;

    const std::string setting_key = key.toStdString();
    const std::string prev_key = QString{key + QStringLiteral("-prev")}.toStdString();
    if (m_onboarded) {
        if (enabled) {
            m_node.updateRwSetting(setting_key, proxySetting(true, trimmed));
            m_node.updateRwSetting(prev_key, common::SettingsValue{});
        } else {
            if (!trimmed.isEmpty()) {
                m_node.updateRwSetting(prev_key, common::SettingsValue{trimmed.toStdString()});
            }
            m_node.updateRwSetting(setting_key, common::SettingsValue{});
        }
    }
    return true;
}

void OptionsQmlModel::setDbcacheSizeMiB(int new_dbcache_size_mib)
{
    if (new_dbcache_size_mib != m_dbcache_size_mib) {
        const DirtySnapshot before = dirtySnapshot();
        m_dbcache_size_mib = new_dbcache_size_mib;
        if (m_onboarded) {
            m_node.updateRwSetting("dbcache", new_dbcache_size_mib);
        }
        Q_EMIT dbcacheSizeMiBChanged(new_dbcache_size_mib);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setListen(bool new_listen)
{
    if (new_listen != m_listen) {
        const DirtySnapshot before = dirtySnapshot();
        m_listen = new_listen;
        if (m_onboarded) {
            m_node.updateRwSetting("listen", new_listen);
        }
        Q_EMIT listenChanged(new_listen);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setMaxMempoolSizeMB(int new_max_mempool_size_mb)
{
    if (new_max_mempool_size_mb != m_max_mempool_size_mb) {
        const DirtySnapshot before = dirtySnapshot();
        m_max_mempool_size_mb = new_max_mempool_size_mb;
        if (m_onboarded) {
            m_node.updateRwSetting("maxmempool", new_max_mempool_size_mb);
        }
        Q_EMIT maxMempoolSizeMBChanged(new_max_mempool_size_mb);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setNatpmp(bool new_natpmp)
{
    if (new_natpmp != m_natpmp) {
        m_natpmp = new_natpmp;
        if (m_onboarded) {
            m_node.updateRwSetting("natpmp", new_natpmp);
            m_node.mapPort(new_natpmp);
        }
        Q_EMIT natpmpChanged(new_natpmp);
    }
}

void OptionsQmlModel::setPrune(bool new_prune)
{
    if (new_prune != m_prune) {
        const DirtySnapshot before = dirtySnapshot();
        m_prune = new_prune;
        if (m_onboarded) {
            m_node.updateRwSetting("prune", pruneSetting());
        }
        Q_EMIT pruneChanged(new_prune);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setPruneSizeGB(int new_prune_size_gb)
{
    if (new_prune_size_gb < 1) return;
    if (new_prune_size_gb != m_prune_size_gb) {
        const DirtySnapshot before = dirtySnapshot();
        m_prune_size_gb = new_prune_size_gb;
        if (m_onboarded) {
            m_node.updateRwSetting("prune", pruneSetting());
        }
        Q_EMIT pruneSizeGBChanged(new_prune_size_gb);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setScriptThreads(int new_script_threads)
{
    if (new_script_threads != m_script_threads) {
        const DirtySnapshot before = dirtySnapshot();
        m_script_threads = new_script_threads;
        if (m_onboarded) {
            m_node.updateRwSetting("par", new_script_threads);
        }
        Q_EMIT scriptThreadsChanged(new_script_threads);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setServer(bool new_server)
{
    if (new_server != m_server) {
        const DirtySnapshot before = dirtySnapshot();
        m_server = new_server;
        if (m_onboarded) {
            m_node.updateRwSetting("server", new_server);
        }
        Q_EMIT serverChanged(new_server);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setProxyEnabled(bool enabled)
{
    if (enabled != m_proxy_enabled) {
        const DirtySnapshot before = dirtySnapshot();
        if (enabled && m_proxy_address.isEmpty()) {
            m_proxy_address = defaultProxyAddress();
            Q_EMIT proxyAddressChanged(m_proxy_address);
        }
        if (enabled && !validateProxyLocation(m_proxy_address).isEmpty()) {
            return;
        }
        m_proxy_enabled = enabled;
        writeProxySetting(QStringLiteral("proxy"), m_proxy_enabled, m_proxy_address);
        Q_EMIT proxyEnabledChanged(enabled);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setProxyAddress(const QString& address)
{
    commitProxyLocation(address);
}

void OptionsQmlModel::setTorEnabled(bool enabled)
{
    if (enabled != m_tor_enabled) {
        const DirtySnapshot before = dirtySnapshot();
        if (enabled && m_tor_address.isEmpty()) {
            m_tor_address = defaultProxyAddress();
            Q_EMIT torAddressChanged(m_tor_address);
        }
        if (enabled && !validateProxyLocation(m_tor_address).isEmpty()) {
            return;
        }
        m_tor_enabled = enabled;
        writeProxySetting(QStringLiteral("onion"), m_tor_enabled, m_tor_address);
        Q_EMIT torEnabledChanged(enabled);
        emitDirtySignals(before);
    }
}

void OptionsQmlModel::setTorAddress(const QString& address)
{
    commitTorLocation(address);
}

QString OptionsQmlModel::validateProxyLocation(const QString& location) const
{
    return ProxyValidationError(location);
}

bool OptionsQmlModel::commitProxyLocation(const QString& location)
{
    const QString trimmed = location.trimmed();
    const QString error = validateProxyLocation(trimmed);
    if (!error.isEmpty()) return false;
    if (trimmed == m_proxy_address) return true;

    const DirtySnapshot before = dirtySnapshot();
    m_proxy_address = trimmed;
    writeProxySetting(QStringLiteral("proxy"), m_proxy_enabled, m_proxy_address);
    Q_EMIT proxyAddressChanged(m_proxy_address);
    emitDirtySignals(before);
    return true;
}

bool OptionsQmlModel::commitTorLocation(const QString& location)
{
    const QString trimmed = location.trimmed();
    const QString error = validateProxyLocation(trimmed);
    if (!error.isEmpty()) return false;
    if (trimmed == m_tor_address) return true;

    const DirtySnapshot before = dirtySnapshot();
    m_tor_address = trimmed;
    writeProxySetting(QStringLiteral("onion"), m_tor_enabled, m_tor_address);
    Q_EMIT torAddressChanged(m_tor_address);
    emitDirtySignals(before);
    return true;
}

QString OptionsQmlModel::defaultProxyAddress() const
{
    return DefaultProxyAddress();
}

void OptionsQmlModel::setExternalSignerPath(const QString& path)
{
    const QString normalized_path = NormalizeCommandPath(path);
    if (normalized_path != m_external_signer_path) {
        const DirtySnapshot before = dirtySnapshot();
        m_external_signer_path = normalized_path;
        if (m_external_signer_path.isEmpty()) {
            m_node.forceSetting("signer", common::SettingsValue{});
        } else {
            m_node.forceSetting("signer", m_external_signer_path.toStdString());
        }
        if (m_onboarded) {
            if (m_external_signer_path.isEmpty()) {
                m_node.updateRwSetting("signer", common::SettingsValue{});
            } else {
                m_node.updateRwSetting("signer", m_external_signer_path.toStdString());
            }
        }
        Q_EMIT externalSignerPathChanged(m_external_signer_path);
        emitDirtySignals(before);
    }
}

QString OptionsQmlModel::externalSignerPathValidationError(const QString& path) const
{
    const QString normalized_path = NormalizeCommandPath(path);
    if (normalized_path.isEmpty()) {
        return {};
    }

    const QString token = FirstCommandToken(normalized_path);
    if (token.isEmpty() || !TokenLooksLikePath(token)) {
        return {};
    }

    const QFileInfo info(ExpandUserPath(token));
    if (!info.exists()) {
        return tr("The configured signer path does not exist.");
    }
    if (!info.isFile()) {
        return tr("The configured signer path is not a file.");
    }
    if (!info.isExecutable()) {
        return tr("The configured signer path is not executable.");
    }
    return {};
}

common::SettingsValue OptionsQmlModel::pruneSetting() const
{
    assert(!m_prune || m_prune_size_gb >= 1);
    return m_prune ? PruneGBtoMiB(m_prune_size_gb) : 0;
}

QString PathToQString(const fs::path &path)
{
    return QString::fromStdString(path.utf8string());
}

QString OptionsQmlModel::getDefaultDataDirString()
{
    return PathToQString(GetDefaultDataDir());
}


QUrl OptionsQmlModel::getDefaultDataDirectory()
{
    QString path = getDefaultDataDirString();
    return QUrl::fromLocalFile(path);
}

bool OptionsQmlModel::setCustomDataDirArgs(QString path)
{
    return selectCustomDataDir(path);
}

QString OptionsQmlModel::getCustomDataDirString()
{
#ifdef __ANDROID__
    m_custom_datadir_string = m_custom_datadir_string.replace("content://com.android.externalstorage.documents/tree/primary%3A", "/storage/self/primary/");
#endif // __ANDROID__
    return m_custom_datadir_string;
}

QString OptionsQmlModel::validateCustomDataDir(const QString& path) const
{
#ifdef __ANDROID__
    Q_UNUSED(path);
    return tr("Custom data directories are not supported on this platform yet.");
#else
    const QString local_path = NormalizeLocalPath(path);
    if (local_path.isEmpty()) {
        return tr("Choose a data directory.");
    }

    QFileInfo target_info(local_path);
    if (target_info.exists() && !target_info.isDir()) {
        return tr("The selected path exists and is not a directory.");
    }
    if (target_info.exists() && !target_info.isWritable()) {
        return tr("The selected directory is not writable.");
    }

    QDir parent = target_info.absoluteDir();
    while (!parent.exists()) {
        const QString current = parent.absolutePath();
        if (!parent.cdUp() || parent.absolutePath() == current) {
            return tr("The data directory cannot be created here.");
        }
    }
    QFileInfo parent_info(parent.absolutePath());
    if (!parent_info.isDir() || !parent_info.isWritable()) {
        return tr("The parent directory is not writable.");
    }
    return {};
#endif
}

bool OptionsQmlModel::selectCustomDataDir(const QString& path)
{
    const QString local_path = NormalizeLocalPath(path);
    if (!validateCustomDataDir(local_path).isEmpty()) {
        return false;
    }
    if (local_path == m_custom_datadir_string && m_dataDir == local_path) {
        return true;
    }

    try {
        const fs::path data_dir_path = QStringToPath(local_path);
        if (TryCreateDirectories(data_dir_path)) {
            TryCreateDirectories(data_dir_path / "wallets");
        }
    } catch (const fs::filesystem_error&) {
        return false;
    }

    m_custom_datadir_string = local_path;
    QSettings settings;
    settings.setValue(SettingsKeys::DATA_DIR, local_path);
    Q_EMIT customDataDirStringChanged(local_path);
    setDataDir(local_path);
    return true;
}

void OptionsQmlModel::useDefaultDataDir()
{
    m_custom_datadir_string.clear();
    QSettings settings;
    settings.remove(SettingsKeys::DATA_DIR);
    Q_EMIT customDataDirStringChanged({});
    setDataDir(getDefaultDataDirString());
}

void OptionsQmlModel::setDataDir(QString new_data_dir)
{
    const QString normalized = NormalizeLocalPath(new_data_dir);
    const QString effective = normalized.isEmpty() ? getDefaultDataDirString() : normalized;
    if (effective == m_dataDir) return;
    m_dataDir = effective;
    Q_EMIT dataDirChanged(m_dataDir);
}

void OptionsQmlModel::buildAvailableLanguages()
{
    m_available_languages.clear();
    m_available_languages << "";  // empty = system default

    QDir translations_dir(":/translations");
    QStringList files = translations_dir.entryList({"bitcoin_*.qm"}, QDir::Files);
    QStringList tags;
    for (const QString& file : files) {
        // Strip "bitcoin_" prefix and ".qm" suffix to get locale tag.
        QString tag = file;
        tag.remove(0, 8);       // remove "bitcoin_"
        tag.chop(3);            // remove ".qm"
        // Skip QML-app-specific translation resources (e.g. "qml_es").
        if (tag.startsWith(QStringLiteral("qml_"))) continue;
        tags << tag;
    }
    tags.sort(Qt::CaseInsensitive);
    m_available_languages << tags;
}

void OptionsQmlModel::setLanguage(const QString& new_language)
{
    if (new_language != m_language) {
        m_language = new_language;
        QSettings settings;
        settings.setValue(SettingsKeys::LANGUAGE, m_language);
        Q_EMIT languageChanged();
    }
}

QString OptionsQmlModel::languageSummary() const
{
    return languageLabel(m_language);
}

QString OptionsQmlModel::languageLabel(const QString& locale_tag) const
{
    if (locale_tag.isEmpty()) {
        return QObject::tr("System default");
    }
    QLocale locale(locale_tag);
    QString native = locale.nativeLanguageName();
    if (native.isEmpty()) {
        return locale_tag;
    }
    // Capitalize first letter of native name.
    native[0] = native[0].toUpper();
    QString english = QLocale::languageToString(locale.language());
    // Append territory disambiguation when the tag includes a territory code.
    if (locale_tag.contains('_')) {
        QString native_territory = locale.nativeTerritoryName();
        if (!native_territory.isEmpty()) {
            native += QStringLiteral(" (%1)").arg(native_territory);
        }
        english += QStringLiteral(" (%1)").arg(QLocale::territoryToString(locale.territory()));
    }
    return QStringLiteral("%1 \u2014 %2").arg(native, english);
}

void OptionsQmlModel::setDisplayUnit(int new_display_unit)
{
    if (new_display_unit != m_display_unit) {
        m_display_unit = new_display_unit;
        QSettings settings;
        settings.setValue(SettingsKeys::DISPLAY_UNIT, m_display_unit);
        Q_EMIT displayUnitChanged(m_display_unit);
    }
}

void OptionsQmlModel::setThirdPartyTransactionUrls(const QString& urls)
{
    if (urls == m_third_party_transaction_urls) return;
    m_third_party_transaction_urls = urls;
    QSettings settings;
    settings.setValue(SettingsKeys::THIRD_PARTY_TRANSACTION_URLS, m_third_party_transaction_urls);
    Q_EMIT thirdPartyTransactionUrlsChanged();
}

QVariantList OptionsQmlModel::thirdPartyTransactionLinks(const QString& txid) const
{
    QVariantList links;
    const QStringList urls = m_third_party_transaction_urls.split(QLatin1Char('|'), Qt::SkipEmptyParts);
    for (QString url : urls) {
        url = url.trimmed();
        if (!url.contains(QStringLiteral("%s"))) continue;
        const QUrl parsed{url, QUrl::StrictMode};
        const QString host = parsed.host();
        if (host.isEmpty()) continue;
        QVariantMap link;
        link.insert(QStringLiteral("host"), host);
        link.insert(QStringLiteral("url"), url.replace(QStringLiteral("%s"), txid));
        links.push_back(link);
    }
    return links;
}

void OptionsQmlModel::setMoneyFontChoice(const QString& choice)
{
    const QString normalized = choice == MONEY_FONT_BEST_SYSTEM ? QString{MONEY_FONT_BEST_SYSTEM} : QString{MONEY_FONT_EMBEDDED};
    if (normalized == m_money_font_choice) return;
    m_money_font_choice = normalized;
    QSettings settings;
    settings.setValue(SettingsKeys::MONEY_FONT_CHOICE, m_money_font_choice);
    Q_EMIT moneyFontChoiceChanged();
    Q_EMIT moneyFontChanged();
}

QFont OptionsQmlModel::moneyFont() const
{
    if (m_money_font_choice == MONEY_FONT_BEST_SYSTEM) {
        return QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }
    QFont font{QStringLiteral("Roboto Mono")};
    font.setStyleName(QStringLiteral("Regular"));
    return font;
}

QString OptionsQmlModel::displayUnitLabel() const
{
    return (m_display_unit == 1) ? QStringLiteral("sat") : QStringLiteral("BTC");
}

QString OptionsQmlModel::displayUnitLabelForAmount(qint64 satoshi) const
{
    if (m_display_unit != 1) return QStringLiteral("₿");
    return (qAbs(satoshi) == 1) ? QStringLiteral("sat") : QStringLiteral("sats");
}


void OptionsQmlModel::onboard()
{
    m_node.resetSettings();
    if (m_external_signer_path.isEmpty()) {
        m_node.forceSetting("signer", common::SettingsValue{});
    } else {
        m_node.forceSetting("signer", m_external_signer_path.toStdString());
    }
    if (m_dbcache_size_mib != DEFAULT_DB_CACHE >> 20) {
        m_node.updateRwSetting("dbcache", m_dbcache_size_mib);
    }
    if (m_listen) {
        m_node.updateRwSetting("listen", m_listen);
    }
    if (m_natpmp) {
        m_node.updateRwSetting("natpmp", m_natpmp);
    }
    if (m_prune) {
        m_node.updateRwSetting("prune", pruneSetting());
    }
    if (m_script_threads != DEFAULT_SCRIPTCHECK_THREADS) {
        m_node.updateRwSetting("par", m_script_threads);
    }
    if (m_server) {
        m_node.updateRwSetting("server", m_server);
    }
    if (m_proxy_enabled && !m_proxy_address.isEmpty()) {
        m_node.updateRwSetting("proxy", m_proxy_address.toStdString());
    }
    if (m_tor_enabled && !m_tor_address.isEmpty()) {
        m_node.updateRwSetting("onion", m_tor_address.toStdString());
    }
    if (!m_external_signer_path.isEmpty()) {
        m_node.updateRwSetting("signer", m_external_signer_path.toStdString());
    }
    m_onboarded = true;
    resetDirtySnapshots();
    Q_EMIT connectionSettingsDirtyChanged();
    Q_EMIT storageSettingsDirtyChanged();
    Q_EMIT developerSettingsDirtyChanged();
    Q_EMIT proxySettingsDirtyChanged();
    Q_EMIT walletSettingsDirtyChanged();
    Q_EMIT restartRequiredChanged();
}
