// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "optionsmodel.h"

#include "bitcoinunits.h"
#include "guiutil.h"

#include "init.h"
#include "main.h"
#include "net.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#include "walletdb.h"
#endif


#include <QSettings>
#include <QStringList>

bool fUseDarkTheme;

OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    Init();
}

void OptionsModel::addOverriddenOption(const std::string &option)
{
    strOverriddenByCommandLine += QString::fromStdString(option) + "=" + QString::fromStdString(mapArgs[option]) + " ";
}

// Writes all missing QSettings with their default values
void OptionsModel::Init()
{
    QSettings settings;

    // Ensure restart flag is unset on client startup
    setRestartRequired(false);

    // These are Qt-only settings:

    // Window
    if (!settings.contains("fMinimizeToTray"))
        settings.setValue("fMinimizeToTray", false);
    fMinimizeToTray = settings.value("fMinimizeToTray").toBool();

    if (!settings.contains("fMinimizeOnClose"))
        settings.setValue("fMinimizeOnClose", false);
    fMinimizeOnClose = settings.value("fMinimizeOnClose").toBool();


    // Display
    if (!settings.contains("nDisplayUnit"))
        settings.setValue("nDisplayUnit", PupaCoinUnits::PPCN);
    nDisplayUnit = settings.value("nDisplayUnit").toInt();
    
    fUseDarkTheme = settings.value("fUseDarkTheme", false).toBool();
    
    if (!settings.contains("fCoinControlFeatures"))
        settings.setValue("fCoinControlFeatures", false);
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();

    // Dark Send
    if (!settings.contains("nMNengineRounds"))
        settings.setValue("nMNengineRounds", 2);
    nMNengineRounds = settings.value("nMNengineRounds").toLongLong();
    if (!settings.contains("nAnonymizePupaCoinAmount"))
        settings.setValue("nAnonymizePupaCoinAmount", 1000);
    nAnonymizePupaCoinAmount = settings.value("nAnonymizePupaCoinAmount").toLongLong();
    if (settings.contains("nMNengineRounds"))
        SoftSetArg("-mnenginerounds", settings.value("nMNengineRounds").toString().toStdString());
    if (settings.contains("nAnonymizePupaCoinAmount"))
        SoftSetArg("-anonymizePupaCoinamount", settings.value("nAnonymizePupaCoinAmount").toString().toStdString());






    // These are shared with the core or have a command-line parameter
    // and we want command-line parameters to overwrite the GUI settings.
    //
    // If setting doesn't exist create it with defaults.
    //
    // If SoftSetArg() or SoftSetBoolArg() return false we were overridden
    // by command-line and show this in the UI.
    // Wallet
#ifdef ENABLE_WALLET
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)MIN_TX_FEE);
    nTransactionFee = settings.value("nTransactionFee").toLongLong(); // if -paytxfee is set, this will be overridden later in init.cpp
    if (mapArgs.count("-paytxfee"))
        addOverriddenOption("-paytxfee");
    nReserveBalance = settings.value("nReserveBalance").toLongLong();
#endif


    // Network
    if (!settings.contains("fUseUPnP"))
#ifdef USE_UPNP
        settings.setValue("fUseUPnP", true);
#else
        settings.setValue("fUseUPnP", false);
#endif
    if (!SoftSetBoolArg("-upnp", settings.value("fUseUPnP").toBool()))
        addOverriddenOption("-upnp");

    // Display
    if (!settings.contains("language"))
        settings.setValue("language", "");
    if (!SoftSetArg("-lang", settings.value("language").toString().toStdString()))
        addOverriddenOption("-lang");




    language = settings.value("language").toString();
}

void OptionsModel::Reset()
{
    QSettings settings;

    // Remove all entries from our QSettings object
    settings.clear();

    // default setting for OptionsModel::StartAtStartup - disabled
    if (GUIUtil::GetStartOnSystemStartup())
        GUIUtil::SetStartOnSystemStartup(false);
}
int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

// read QSettings values and return them
QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            return GUIUtil::GetStartOnSystemStartup();
        case MinimizeToTray:
            return fMinimizeToTray;
        case MapPortUPnP:
#ifdef USE_UPNP
            return settings.value("fUseUPnP");
#else
            return false;
#endif
        case MinimizeOnClose:
            return fMinimizeOnClose;

#ifdef ENABLE_WALLET
        case Fee:
            // Attention: Init() is called before nTransactionFee is set in AppInit2()!
            // To ensure we can change the fee on-the-fly update our QSetting when
            // opening OptionsDialog, which queries Fee via the mapper.
            if (nTransactionFee != settings.value("nTransactionFee").toLongLong())
                settings.setValue("nTransactionFee", (qint64)nTransactionFee);
            // Todo: Consider to revert back to use just nTransactionFee here, if we don't want
            // -paytxfee to update our QSettings!
            return settings.value("nTransactionFee");            
        case ReserveBalance:
            return QVariant((qint64) nReserveBalance);
#endif
        case DisplayUnit:
            return nDisplayUnit;
        case Language:
            return settings.value("language");
        case CoinControlFeatures:
            return fCoinControlFeatures;
        case MNengineRounds:
            return QVariant(nMNengineRounds);
        case AnonymizePupaCoinAmount:
            return QVariant(nAnonymizePupaCoinAmount);
        case UseDarkTheme:
            return QVariant(fUseDarkTheme);
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// write QSettings values
bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            settings.setValue("fMinimizeToTray", fMinimizeToTray);
            break;
        case MapPortUPnP: // core option - can be changed on-the-fly
            settings.setValue("fUseUPnP", value.toBool());
            MapPort(value.toBool());
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
            break;

#ifdef ENABLE_WALLET
        case Fee: // core option - can be changed on-the-fly
            // Todo: Add is valid check  and warn via message, if not
            nTransactionFee = value.toLongLong();
            settings.setValue("nTransactionFee", (qint64) nTransactionFee);
            emit transactionFeeChanged(nTransactionFee);
            break;
        case ReserveBalance:
            nReserveBalance = value.toLongLong();
            settings.setValue("nReserveBalance", (qint64) nReserveBalance);
            emit reserveBalanceChanged(nReserveBalance);
            break;
#endif
        case DisplayUnit:
            nDisplayUnit = value.toInt();
            settings.setValue("nDisplayUnit", nDisplayUnit);
            emit displayUnitChanged(nDisplayUnit);
            break;
        case Language:
            if (settings.value("language") != value) {
                settings.setValue("language", value);
                setRestartRequired(true);
            }
            break;
        case CoinControlFeatures:
            fCoinControlFeatures = value.toBool();
            settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
            emit coinControlFeaturesChanged(fCoinControlFeatures);
            break;
        case UseDarkTheme:
            fUseDarkTheme = value.toBool();
            settings.setValue("fUseDarkTheme", fUseDarkTheme);
            break;
        case MNengineRounds:
            nMNengineRounds = value.toInt();
            settings.setValue("nMNengineRounds", nMNengineRounds);
            emit mnengineRoundsChanged(nMNengineRounds);
            break;
        case AnonymizePupaCoinAmount:
            nAnonymizePupaCoinAmount = value.toInt();
            settings.setValue("nAnonymizePupaCoinAmount", nAnonymizePupaCoinAmount);
            emit AnonymizePupaCoinAmountChanged(nAnonymizePupaCoinAmount);
            break;
        default:
            break;
        }
    }
    emit dataChanged(index, index);

    return successful;
}

void OptionsModel::setRestartRequired(bool fRequired)
{
    QSettings settings;
    return settings.setValue("fRestartRequired", fRequired);
}

bool OptionsModel::isRestartRequired()
{
    QSettings settings;
    return settings.value("fRestartRequired", false).toBool();
}


qint64 OptionsModel::getReserveBalance()
{
    return nReserveBalance;
}
