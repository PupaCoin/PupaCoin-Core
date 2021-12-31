/*
 * Qt5 bitcoin GUI.
 *
 * W.J. van der Laan 2011-2012
 * The PupaCoin Developers 2018-2020
 */

#include <QApplication>

#include "bitcoingui.h"

#include "transactiontablemodel.h"
#include "addressbookpage.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "editaddressdialog.h"
#include "editconfigdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "wallet.h"
#include "main.h"
#include "init.h"
#include "ui_interface.h"
#include "masternodemanager.h"
#include "messagemodel.h"
#include "messagepage.h"
#include "blockbrowser.h"
#include "settingspage.h"
#include "importprivatekeydialog.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QMimeData>
#include <QStyle>
#include <QToolButton>
#include <QScrollArea>
#include <QScroller>
#include <QTextDocument>
#include <QInputDialog>
#include <QFontDatabase>

#include <iostream>

extern bool fOnlyTor;
extern CWallet* pwalletMain;
extern int64_t nLastCoinStakeSearchInterval;
double GetPoSKernelPS();

bool settingsLock = true;
bool settingsChangePass = true;
bool settingsRelock = false;
bool settingsUncrypted = false;

PupaCoinGUI::PupaCoinGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    toolbar(0),
    //progressBarLabel(0),
    //progressBar(0),
    //progressDialog(0),
    encryptWalletAction(0),
    changePassphraseAction(0),
    unlockWalletAction(0),
    lockWalletAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0),
    prevBlocks(0),
    nWeight(0)
{
    setFixedSize(1280, 720);
    setWindowTitle(tr("PupaCoin") + " - " + tr("Wallet"));
#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(fUseDarkTheme ? ":/icons/dark/bitcoin-dark" : ":/icons/bitcoin"));
    setWindowIcon(QIcon(fUseDarkTheme ? ":/icons/dark/bitcoin-dark" : ":/icons/bitcoin"));
#else
    //setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    QFontDatabase fontDB;
    fontDB.addApplicationFont("/fonts/raleway");

    setObjectName("PupaCoin");
    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light); }");//background-color: #e6e6e6; color: #333333;

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create application menu bar
    //createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create the tray icon (or setup the dock icon)
    createTrayIcon();

    // Create tabs
    overviewPage = new OverviewPage();

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    transactionView = new TransactionView(this);
    vbox->addWidget(transactionView);
    transactionsPage->setLayout(vbox);

    blockBrowser = new BlockBrowser(this);

    settingsPage = new SettingsPage(this);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    sendCoinsPage = new SendCoinsDialog(this);

    signVerifyMessageDialog = new SignVerifyMessageDialog(this);

    masternodeManagerPage = new MasternodeManager(this);

    messagePage = new MessagePage(this);

    centralStackedWidget = new QStackedWidget(this);
    centralStackedWidget->setContentsMargins(0, 0, 0, 0);
    centralStackedWidget->addWidget(overviewPage);
    centralStackedWidget->addWidget(transactionsPage);
    centralStackedWidget->addWidget(addressBookPage);
    centralStackedWidget->addWidget(receiveCoinsPage);
    centralStackedWidget->addWidget(sendCoinsPage);
    centralStackedWidget->addWidget(masternodeManagerPage);
    centralStackedWidget->addWidget(messagePage);
    centralStackedWidget->addWidget(blockBrowser);
    centralStackedWidget->addWidget(settingsPage);

    QWidget *centralWidget = new QWidget();
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0,0,0,0);
    centralWidget->setContentsMargins(0,0,0,0);
    centralLayout->addWidget(centralStackedWidget);

    setCentralWidget(centralWidget);

    // Create status bar
    //statusBar();

    // Disable size grip because it looks ugly and nobody needs it
    //statusBar()->setSizeGripEnabled(false);

    // Status bar notification icons
    QWidget *frameBlocks = new QWidget();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    frameBlocks->setStyleSheet("QWidget { background: none; margin: 0px; }");
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(0,0,0,0);
    frameBlocksLayout->setSpacing(0);
    frameBlocksLayout->setAlignment(Qt::AlignHCenter);
    //labelEncryptionIcon = new QLabel();
    //labelStakingIcon = new QLabel();
    //labelConnectionsIcon = new QLabel();
    //labelBlocksIcon = new QLabel();
    //frameBlocksLayout->addWidget(netLabel);
    //frameBlocksLayout->addStretch();
    //frameBlocksLayout->addWidget(labelEncryptionIcon);
    //frameBlocksLayout->addStretch();
    //frameBlocksLayout->addWidget(labelStakingIcon);
    //frameBlocksLayout->addStretch();
    //frameBlocksLayout->addWidget(labelConnectionsIcon);
    //frameBlocksLayout->addStretch();
    //frameBlocksLayout->addWidget(labelBlocksIcon);
    //frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(netLabel);
    //frameBlocksLayout->addStretch();
     toolbar->addWidget(frameBlocks);


    if (GetBoolArg("-staking", true))
    {
        //QTimer *timerStakingIcon = new QTimer(labelStakingIcon);
        //connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
        //timerStakingIcon->start(20 * 1000);
        //updateStakingIcon();
    }

    // Progress bar and label for blocks download
    //progressBarLabel = new QLabel();
    //progressBarLabel->setVisible(false);
    //progressBar = new QProgressBar();
    //progressBar->setAlignment(Qt::AlignCenter);
    //progressBar->setVisible(false);

    if (!fUseDarkTheme)
    {
        // Override style sheet for progress bar for styles that have a segmented progress bar,
        // as they make the text unreadable (workaround for issue #1071)
        // See https://qt-project.org/doc/qt-4.8/gallery.html
        //QString curStyle = qApp->style()->metaObject()->className();
        //if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
        //{
        //    progressBar->setStyleSheet("QProgressBar { color: #333333;background-color: #cccccc; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #eb1f24, stop: 1 #4c5259); border-radius: 7px; margin: 0px; }");
        //}
    }

    //statusBar()->addWidget(progressBarLabel);
    //statusBar()->addWidget(progressBar);
    //statusBar()->addPermanentWidget(frameBlocks);
    //statusBar()->setObjectName("statusBar");
    //statusBar()->setStyleSheet("#statusBar { color: #3098c6; background-color: #1d1f22; }");

    if (!fUseDarkTheme)
    {
        //statusBar()->setStyleSheet("#statusBar { color: #333333; background-color: #cccccc; }");
    }

    //syncIconMovie = new QMovie(fUseDarkTheme ? ":/movies/update_spinner_black" : ":/movies/update_spinner", "mng", this);

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    rpcConsole = new RPCConsole(0);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    // clicking on automatic backups shows details
    connect(showBackupsAction, SIGNAL(triggered()), rpcConsole, SLOT(showBackups()));

    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

    // Clicking on "Verify Message" in the address book sends you to the verify message tab
    connect(addressBookPage, SIGNAL(verifyMessage(QString)), this, SLOT(gotoVerifyMessageTab(QString)));
    // Clicking on "Sign Message" in the receive coins page sends you to the sign message tab
    connect(receiveCoinsPage, SIGNAL(signMessage(QString)), this, SLOT(gotoSignMessageTab(QString)));

    gotoOverviewPage();
}

PupaCoinGUI::~PupaCoinGUI()
{
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
#endif

    delete rpcConsole;
}

void PupaCoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Dashboard"), this);
    overviewAction->setToolTip(tr("Show general overview of wallet"));
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setToolTip(tr("Show the list of addresses for receiving payments"));
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(receiveCoinsAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send-sidebar"), tr("&Send"), this);
    sendCoinsAction->setToolTip(tr("Send coins to a PupaCoin address"));
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(sendCoinsAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setToolTip(tr("Browse transaction history"));
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book-sidebar"), tr("&Addresses"), this);
    addressBookAction->setToolTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(addressBookAction);

    masternodeManagerAction = new QAction(QIcon(":/icons/mnodes"), tr("&Masternodes"), this);
    masternodeManagerAction->setToolTip(tr("Show Master Nodes status and configure your nodes."));
    masternodeManagerAction->setCheckable(true);
    tabGroup->addAction(masternodeManagerAction);

    messageAction = new QAction(QIcon(":/icons/message"), tr("&Messages"), this);
    messageAction->setToolTip(tr("View and Send Encrypted messages"));
    messageAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    messageAction->setCheckable(true);
    tabGroup->addAction(messageAction);

    blockAction = new QAction(QIcon(":/icons/block"), tr("&Block Explorer"), this);
    blockAction->setToolTip(tr("Explore the BlockChain"));
    blockAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    blockAction->setCheckable(true);
    tabGroup->addAction(blockAction);

    settingsAction = new QAction(QIcon(":/icons/settings"), tr("&Settings Page"), this);
    settingsAction->setToolTip(tr("Go to Settings and Options page"));
    settingsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_8));
    settingsAction->setCheckable(true);
    tabGroup->addAction(settingsAction);

    showBackupsAction = new QAction(QIcon(":/icons/browse"), tr("Show Auto&Backups"), this);
    showBackupsAction->setStatusTip(tr("S"));

    connect(blockAction, SIGNAL(triggered()), this, SLOT(gotoBlockBrowser()));
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(gotoSettingsPage()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));
    connect(masternodeManagerAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(masternodeManagerAction, SIGNAL(triggered()), this, SLOT(gotoMasternodeManagerPage()));
    connect(messageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(messageAction, SIGNAL(triggered()), this, SLOT(gotoMessagePage()));

    quitAction = new QAction(QIcon(":icons/quit"), tr("E&xit"), this);
    quitAction->setToolTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(fUseDarkTheme ? ":/icons/dark/bitcoin-dark" : ":/icons/bitcoin"), tr("&About PupaCoin"), this);
    aboutAction->setToolTip(tr("Show information about PupaCoin"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutQtAction = new QAction(QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setToolTip(tr("Modify configuration options for PupaCoin"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(fUseDarkTheme ? ":/icons/dark/bitcoin-dark" : ":/icons/bitcoin"), tr("&Show / Hide"), this);
    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed_toolbar"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setToolTip(tr("Encrypt or decrypt wallet"));
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setToolTip(tr("Backup wallet to another location"));
    importPrivateKeyAction = new QAction(QIcon(":/icons/key"), tr("&Import private key..."), this);
    importPrivateKeyAction->setToolTip(tr("Import a private key"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setToolTip(tr("Change the passphrase used for wallet encryption"));
    unlockWalletAction = new QAction(QIcon(":/icons/lock_open_toolbar"),tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(QIcon(":/icons/lock_closed_toolbar"),tr("&Lock Wallet"), this);
    lockWalletAction->setToolTip(tr("Lock wallet"));
    signMessageAction = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    verifyMessageAction = new QAction(QIcon(":/icons/transaction_0"), tr("&Verify message..."), this);

    exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    exportAction->setToolTip(tr("Export the data in the current tab to a file"));
    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setToolTip(tr("Open debugging and diagnostic console"));

    editConfigAction = new QAction(QIcon(":/icons/editconf"), tr("&Edit PupaCoin.conf"), this);
    editConfigAction->setToolTip(tr("Edit the configuration file for PupaCoin"));
    editConfigExtAction = new QAction(QIcon(":/icons/editconf"), tr("&Edit PupaCoin.conf (external)"), this);
    editConfigExtAction->setToolTip(tr("Edit the configuration file for PupaCoin (external editor)"));
    openDataDirAction = new QAction(QIcon(":/icons/folder"), tr("&Open data dir"), this);
    openDataDirAction->setToolTip(tr("Open the directory where PupaCoin data is stored"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered()), this, SLOT(encryptWallet()));
    connect(backupWalletAction, SIGNAL(triggered()), this, SLOT(backupWallet()));
    connect(importPrivateKeyAction, SIGNAL(triggered()), this, SLOT(importPrivateKey()));
    connect(changePassphraseAction, SIGNAL(triggered()), this, SLOT(changePassphrase()));
    connect(unlockWalletAction, SIGNAL(triggered()), this, SLOT(unlockWallet()));
    connect(lockWalletAction, SIGNAL(triggered()), this, SLOT(lockWallet()));
    connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
    connect(editConfigAction, SIGNAL(triggered()), this, SLOT(editConfig()));
    connect(editConfigExtAction, SIGNAL(triggered()), this, SLOT(editConfigExt()));
    connect(openDataDirAction, SIGNAL(triggered()), this, SLOT(openDataDir()));
}

//void PupaCoinGUI::createMenuBar()
//{
//#ifdef Q_OS_MAC
    //appMenuBar = new QMenuBar();
//#else
    //appMenuBar = menuBar();
//#endif

    // Configure the menus
    //QMenu *file = appMenuBar->addMenu(tr("&File"));
    //file->addAction(backupWalletAction);
    //file->addAction(importPrivateKeyAction);
    //file->addAction(exportAction);
    //file->addAction(signMessageAction);
    //file->addAction(verifyMessageAction);
    //file->addSeparator();
    //file->addAction(quitAction);

    //QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    //settings->addAction(encryptWalletAction);
    //settings->addAction(changePassphraseAction);
    //settings->addAction(unlockWalletAction);
    //settings->addAction(lockWalletAction);
    //settings->addSeparator();
    //settings->addAction(optionsAction);
    //settings->addAction(showBackupsAction);

    //QMenu *help = appMenuBar->addMenu(tr("&Help"));
    //help->addAction(openRPCConsoleAction);
    //help->addAction(openDataDirAction);
    //help->addAction(editConfigAction);
    //help->addAction(editConfigExtAction);
    //help->addSeparator();
    //help->addAction(aboutAction);
    //help->addAction(aboutQtAction);
//}

static QWidget* makeToolBarSpacer()
{
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacer->setStyleSheet("QWidget { background: none; }");
    return spacer;
}

void PupaCoinGUI::createToolBars()
{
    fLiteMode = GetBoolArg("-litemode", false);

    //QFontDatabase fontDB;
    //fontDB.addApplicationFont("/fonts/raleway");

    toolbar = new QToolBar(tr("Tabs toolbar"));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    toolbar->setObjectName("tabs");
    toolbar->setStyleSheet("QToolBar { background: none; } QToolButton { color: #ffffff; background: none; } QToolButton:hover { background-color: rgba(219, 205, 110, 0.4); } QToolButton:checked { background-color: rgba(219, 205, 110, 0.4); } QToolButton:pressed { background-color: rgba(219, 205, 110, 0.4); } #tabs { color: #ffffff; background: none; border: none;}");
    toolbar->setIconSize(QSize(64,64));

    if(!fUseDarkTheme)
    {
        toolbar->setStyleSheet("QToolBar { background: none; } QToolButton { color: #ffffff; background: none;} QToolButton:hover { background-color: rgba(219, 205, 110, 0.4); } QToolButton:checked { background-color: rgba(219, 205, 110, 0.4); } QToolButton:pressed { background-color: rgba(219, 205, 110, 0.4); } #tabs { color: #ffffff; background: none; border: none;}");//QToolBar { background: none; }
    }

    QLabel* header = new QLabel();
    header->setMinimumSize(70, 132);
    header->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    header->setPixmap(QPixmap(fUseDarkTheme ? ":/images/header-dark" : ":/images/header"));
    //header->setStyleSheet("QLabel {max-height: 95;}");
    //header->setStyleSheet("QLabel { margin-left: 26px; margin-bottom: 10px; }");
    header->setMaximumSize(70, 132);
    header->setScaledContents(false);
    toolbar->addWidget(header);

    toolbar->addAction(overviewAction);
    toolbar->addAction(sendCoinsAction);
    toolbar->addAction(receiveCoinsAction);
    toolbar->addAction(addressBookAction);
    toolbar->addAction(masternodeManagerAction);
    //toolbar->addAction(blockAction);
    toolbar->addAction(settingsAction);

    //toolbar->addAction(historyAction);

    //if (!fLiteMode){
        //toolbar->addAction(messageAction);
    //}
    netLabel = new QLabel();

    //QWidget *spacer = makeToolBarSpacer();
    netLabel->setObjectName("netLabel");
    netLabel->setStyleSheet("#netLabel { color: #ffffff; }");
    //toolbar->addWidget(spacer);

    //QLabel* footer = new QLabel();
    //footer->setMinimumSize(64, 64);
    //footer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    //footer->setPixmap(QPixmap(fUseDarkTheme ? ":/icons/footer" : ":/icons/footer"));
    //footer->setMaximumSize(64, 64);
   // footer->setScaledContents(true);
    //toolbar->addWidget(footer);

    toolbar->addWidget(makeToolBarSpacer());

    toolbar->setOrientation(Qt::Vertical);
    toolbar->setMovable(false);

    addToolBar(Qt::LeftToolBarArea, toolbar);

    foreach(QAction *action, toolbar->actions()) {
        toolbar->widgetForAction(action)->setFixedWidth(70);
        //toolbar->widgetForAction(action)->setFixedHeight(64);
    }
}

void PupaCoinGUI::setClientModel(ClientModel *clientModel)
{
    netLabel->setText("v1.0.0.8");// Version in GUI

    this->clientModel = clientModel;
    if(clientModel)
    {
        // Replace some strings and icons, when using the testnet
        if(clientModel->isTestNet())
        {
            setWindowTitle(windowTitle() + QString(" ") + tr("[testnet]"));
#ifndef Q_OS_MAC
            qApp->setWindowIcon(QIcon(fUseDarkTheme ? ":/icons/dark/bitcoin-dark_testnet" : ":/icons/bitcoin_testnet"));
            setWindowIcon(QIcon(fUseDarkTheme ? ":/icons/dark/bitcoin-dark_testnet" : ":/icons/bitcoin_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitcoin_testnet"));
#endif
            if(trayIcon)
            {
                trayIcon->setToolTip(tr("PupaCoin client") + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(fUseDarkTheme ? ":/icons/dark/toolbar-dark_testnet" : ":/icons/toolbar_testnet"));
                toggleHideAction->setIcon(QIcon(fUseDarkTheme ? ":/icons/dark/toolbar-dark_testnet" : ":/icons/toolbar_testnet"));
            }
        }

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

        // Receive and report messages from network/worker thread
        connect(clientModel, SIGNAL(message(QString,QString,bool,unsigned int)), this, SLOT(message(QString,QString,bool,unsigned int)));

        // Show progress dialog
        //connect(clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
        //connect(walletModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        overviewPage->setClientModel(clientModel);
        rpcConsole->setClientModel(clientModel);
        addressBookPage->setOptionsModel(clientModel->getOptionsModel());
        receiveCoinsPage->setOptionsModel(clientModel->getOptionsModel());
    }
}

void PupaCoinGUI::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    if(walletModel)
    {
        // Receive and report messages from wallet thread
        connect(walletModel, SIGNAL(message(QString,QString,bool,unsigned int)), this, SLOT(message(QString,QString,bool,unsigned int)));
        connect(sendCoinsPage, SIGNAL(message(QString,QString,bool,unsigned int)), this, SLOT(message(QString,QString,bool,unsigned int)));

        // Put transaction list in tabs
        transactionView->setModel(walletModel);
        overviewPage->setWalletModel(walletModel);
        settingsPage->setModel(walletModel);
        addressBookPage->setModel(walletModel->getAddressTableModel());
        receiveCoinsPage->setModel(walletModel->getAddressTableModel());
        sendCoinsPage->setModel(walletModel);
        signVerifyMessageDialog->setModel(walletModel);
        blockBrowser->setModel(walletModel);

        setEncryptionStatus(walletModel->getEncryptionStatus());
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
    }
}

void PupaCoinGUI::setMessageModel(MessageModel *messageModel)
{
    this->messageModel = messageModel;
    if(messageModel)
    {
        // Report errors from message thread
        connect(messageModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        // Put transaction list in tabs
        messagePage->setModel(messageModel);

        // Balloon pop-up for new message
        connect(messageModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingMessage(QModelIndex,int,int)));
    }
}

void PupaCoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip(tr("PupaCoin client"));
    trayIcon->setIcon(QIcon(fUseDarkTheme ? ":/icons/dark/toolbar-dark" : ":/icons/toolbar"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
    trayIconMenu->addAction(showBackupsAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif

    notificator = new Notificator(qApp->applicationName(), trayIcon);
}

#ifndef Q_OS_MAC
void PupaCoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void PupaCoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void PupaCoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void PupaCoinGUI::aboutQtExt_Internal()
{
    aboutQtAction->triggered();
}

void PupaCoinGUI::aboutQtExt_Static()
{
    guiref->aboutQtExt_Internal();
}

void PupaCoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = fUseDarkTheme ? ":/icons/dark/connect_0" : ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = fUseDarkTheme ? ":/icons/dark/connect_1" : ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = fUseDarkTheme ? ":/icons/dark/connect_2" : ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = fUseDarkTheme ? ":/icons/dark/connect_3" : ":/icons/connect_3"; break;
    default: icon = fUseDarkTheme ? ":/icons/dark/connect_4" : ":/icons/connect_4"; break;
    }
    //labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    //labelConnectionsIcon->setToolTip(tr("%n active connection(s) to PupaCoin network", "", count));
}

void PupaCoinGUI::setNumBlocks(int count)
{
    QString tooltip;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    QDateTime currentDate = QDateTime::currentDateTime();
    int totalSecs = GetTime() - Params().GenesisBlock().GetBlockTime();
    int secs = lastBlockDate.secsTo(currentDate);

    tooltip = tr("Processed %1 blocks of transaction history.").arg(count);

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        //labelBlocksIcon->setPixmap(QIcon(fUseDarkTheme ? ":/icons/dark/synced" : ":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        overviewPage->showOutOfSyncWarning(false);
        overviewPage->ShowSynchronizedMessage(true);

        //progressBarLabel->setVisible(false);
        //progressBar->setVisible(false);
    }
    else
    {
        // Represent time from last generated block in human readable text
        QString timeBehindText;
        const int HOUR_IN_SECONDS = 60*60;
        const int DAY_IN_SECONDS = 24*60*60;
        const int WEEK_IN_SECONDS = 7*24*60*60;
        const int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
        if(secs < 2*DAY_IN_SECONDS)
        {
            timeBehindText = tr("%n hour(s)","",secs/HOUR_IN_SECONDS);
        }
        else if(secs < 2*WEEK_IN_SECONDS)
        {
            timeBehindText = tr("%n day(s)","",secs/DAY_IN_SECONDS);
        }
        else if(secs < YEAR_IN_SECONDS)
        {
            timeBehindText = tr("%n week(s)","",secs/WEEK_IN_SECONDS);
        }
        else
        {
            int years = secs / YEAR_IN_SECONDS;
            int remainder = secs % YEAR_IN_SECONDS;
            timeBehindText = tr("%1 and %2").arg(tr("%n year(s)", "", years)).arg(tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
        }

        //progressBarLabel->setText(tr(clientModel->isImporting() ? "Importing blocks..." : "Synchronizing with network..."));
        //progressBarLabel->setVisible(true);
        //progressBarLabel->setStyleSheet("QLabel { color: #ffffff; }");
        //progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        //progressBar->setMaximum(totalSecs);
        //progressBar->setValue(totalSecs - secs);
        //progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        //labelBlocksIcon->setMovie(syncIconMovie);
        //if(count != prevBlocks)
            //syncIconMovie->jumpToNextFrame();
        prevBlocks = count;

        overviewPage->showOutOfSyncWarning(true);
        overviewPage->ShowSynchronizedMessage(false);

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    //labelBlocksIcon->setToolTip(tooltip);
    //progressBarLabel->setToolTip(tooltip);
    //progressBar->setToolTip(tooltip);

    //statusBar()->setVisible(true);
}

void PupaCoinGUI::message(const QString &title, const QString &message, bool modal, unsigned int style)
{
    QString strTitle = tr("PupaCoin") + " - ";
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    // Check for usage of predefined title
    switch (style) {
    case CClientUIInterface::MSG_ERROR:
        strTitle += tr("Error");
        break;
    case CClientUIInterface::MSG_WARNING:
        strTitle += tr("Warning");
        break;
    case CClientUIInterface::MSG_INFORMATION:
        strTitle += tr("Information");
        break;
    default:
        strTitle += title; // Use supplied title
    }

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (modal) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons);
        mBox.exec();
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void PupaCoinGUI::error(const QString &title, const QString &message, bool modal)
{
    // Report errors from network/worker thread
    if(modal)
    {
        QMessageBox::critical(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
    } else {
        notificator->notify(Notificator::Critical, title, message);
    }
}

void PupaCoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void PupaCoinGUI::closeEvent(QCloseEvent *event)
{
    if(clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            qApp->quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void PupaCoinGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    if (!clientModel || !clientModel->getOptionsModel())
        return;

    QString strMessage = tr("This transaction is over the size limit. You can still send it for a fee of %1, "
        "which goes to the nodes that process your transaction and helps to support the network. "
        "Do you want to pay the fee?").arg(PupaCoinUnits::formatWithUnit(clientModel->getOptionsModel()->getDisplayUnit(), nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Confirm transaction fee"), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void PupaCoinGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
	// Prevent balloon-spam when initial block download is in progress
    if(!walletModel || !clientModel || clientModel->inInitialBlockDownload() || walletModel->processingQueuedTransactions())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();

    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();
    QString date = ttm->index(start, TransactionTableModel::Date, parent)
                    .data().toString();
    QString type = ttm->index(start, TransactionTableModel::Type, parent)
                    .data().toString();
    QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                    .data().toString();
    QIcon icon = qvariant_cast<QIcon>(ttm->index(start,
                        TransactionTableModel::ToAddress, parent)
                    .data(Qt::DecorationRole));
    // On new transaction, make an info balloon
    notificator->notify(Notificator::Information,
                        (amount)<0 ? tr("Sent transaction") :
                                     tr("Incoming transaction"),
                          tr("Date: %1\n"
                             "Amount: %2\n"
                             "Type: %3\n"
                             "Address: %4\n")
                          .arg(date)
                          .arg(PupaCoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                          .arg(type)
                          .arg(address), icon);
}

void PupaCoinGUI::incomingMessage(const QModelIndex & parent, int start, int end)
{
    if(!messageModel)
        return;

    MessageModel *mm = messageModel;

    if (mm->index(start, MessageModel::TypeInt, parent).data().toInt() == MessageTableEntry::Received)
    {
        QString sent_datetime = mm->index(start, MessageModel::ReceivedDateTime, parent).data().toString();
        QString from_address  = mm->index(start, MessageModel::FromAddress,      parent).data().toString();
        QString to_address    = mm->index(start, MessageModel::ToAddress,        parent).data().toString();
        QString message       = mm->index(start, MessageModel::Message,          parent).data().toString();
        QTextDocument html;
        html.setHtml(message);
        QString messageText(html.toPlainText());
        notificator->notify(Notificator::Information,
                            tr("Incoming Message"),
                            tr("Date: %1\n"
                               "From Address: %2\n"
                               "To Address: %3\n"
                               "Message: %4\n")
                              .arg(sent_datetime)
                              .arg(from_address)
                              .arg(to_address)
                              .arg(messageText));
    };
}

void PupaCoinGUI::clearWidgets()
{
    centralStackedWidget->setCurrentWidget(centralStackedWidget->widget(0));
    for(int i = centralStackedWidget->count(); i>0; i--){
        QWidget* widget = centralStackedWidget->widget(i);
        centralStackedWidget->removeWidget(widget);
        widget->deleteLater();
    }
}

void PupaCoinGUI::gotoMasternodeManagerPage()
{
    masternodeManagerAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(masternodeManagerPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}

void PupaCoinGUI::gotoBlockBrowser()
{
    //blockAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(blockBrowser);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}

void PupaCoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(overviewPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light); }");
}

void PupaCoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(transactionsPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), transactionView, SLOT(exportClicked()));
    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}

void PupaCoinGUI::gotoHistoryPage_static()
{
    guiref->gotoHistoryPage();
}

void PupaCoinGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(addressBookPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), addressBookPage, SLOT(exportClicked()));

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}

void PupaCoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(receiveCoinsPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), receiveCoinsPage, SLOT(exportClicked()));

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}

void PupaCoinGUI::gotoSendCoinsPage()
{
    sendCoinsAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(sendCoinsPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}

void PupaCoinGUI::gotoSignMessageTab(QString addr)
{
    // call show() in showTab_SM()
    signVerifyMessageDialog->showTab_SM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void PupaCoinGUI::gotoVerifyMessageTab(QString addr)
{
    // call show() in showTab_VM()
    signVerifyMessageDialog->showTab_VM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

void PupaCoinGUI::gotoMessagePage()
{
    messageAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(messagePage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), messagePage, SLOT(exportClicked()));

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}

void PupaCoinGUI::gotoSettingsPage()
{
    settingsAction->setChecked(true);
    centralStackedWidget->setCurrentWidget(settingsPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    setStyleSheet("#PupaCoin { background-image: url(:/images/background_light_alt); }");
}


void PupaCoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void PupaCoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();
        foreach(const QUrl &uri, uris)
        {
            if (sendCoinsPage->handleURI(uri.toString()))
                nValidUrisFound++;
        }

        // if valid URIs were found
        if (nValidUrisFound)
            gotoSendCoinsPage();
        else
            notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid PupaCoin address or malformed URI parameters."));
    }

    event->acceptProposedAction();
}

void PupaCoinGUI::handleURI(QString strURI)
{
    // URI has to be valid
    if (sendCoinsPage->handleURI(strURI))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
    }
    else
        notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid PupaCoin address or malformed URI parameters."));
}

void PupaCoinGUI::setWalletUnlockStakingOnly()
{

    changePassphraseAction->setEnabled(false);
    unlockWalletAction->setVisible(true);
    lockWalletAction->setVisible(true);
    encryptWalletAction->setEnabled(false);
    settingsLock = true;
    settingsChangePass = false;
    settingsRelock = true;
    settingsUncrypted = false;
    settingsStatus = false;
}

void PupaCoinGUI::setUnencrypted()
{
    changePassphraseAction->setEnabled(false);
    unlockWalletAction->setVisible(false);
    lockWalletAction->setVisible(false);
    encryptWalletAction->setEnabled(true);
    settingsLock = false;
    settingsChangePass = false;
    settingsRelock = false;
    settingsUncrypted = true;
    settingsStatus = false;
}

void PupaCoinGUI::setUnlocked()
{
    changePassphraseAction->setEnabled(true);
    unlockWalletAction->setVisible(false);
    lockWalletAction->setVisible(true);
    encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
    settingsLock = true;
    settingsChangePass = true;
    settingsRelock = true;
    settingsUncrypted = false;
    settingsStatus = false;
}

void PupaCoinGUI::setLocked()
{
    changePassphraseAction->setEnabled(true);
    unlockWalletAction->setVisible(true);
    lockWalletAction->setVisible(false);
    encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
    settingsLock = true;
    settingsChangePass = false;
    settingsRelock = false;
    settingsUncrypted = false;
    settingsStatus = true;
}

void PupaCoinGUI::setEncryptionStatus(int status)
{
    if(fWalletUnlockStakingOnly)
    {
        //labelEncryptionIcon->setPixmap(QIcon(fUseDarkTheme ? ":/icons/dark/lock_open" : ":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        //labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked for staking only</b>"));
        guiref->setWalletUnlockStakingOnly();
    }
    else
    {

    switch(status)
    {
    case WalletModel::Unencrypted:
        //labelEncryptionIcon->setPixmap(QIcon(fUseDarkTheme ? ":/icons/dark/lock_open" : ":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        //labelEncryptionIcon->setToolTip(tr("Wallet is <b>not encrypted</b>"));
        guiref->setUnencrypted();
        break;
    case WalletModel::Unlocked:
        //labelEncryptionIcon->setPixmap(QIcon(fUseDarkTheme ? ":/icons/dark/lock_open" : ":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        //labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        guiref->setUnlocked();
        break;
    case WalletModel::Locked:
        //labelEncryptionIcon->setPixmap(QIcon(fUseDarkTheme ? ":/icons/dark/lock_closed" : ":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        //labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        guiref->setLocked();
        break;
    }

    }
}

void PupaCoinGUI::encryptWallet()
{
    if(!walletModel)
        return;

    AskPassphraseDialog dlg(AskPassphraseDialog::Encrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    setEncryptionStatus(walletModel->getEncryptionStatus());
}

void PupaCoinGUI::backupWallet()
{
    QString saveDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Wallet"), saveDir, tr("Wallet Data (*.dat)"));
    if(!filename.isEmpty()) {
        if(!walletModel->backupWallet(filename)) {
            QMessageBox::warning(this, tr("Backup Failed"), tr("There was an error trying to save the wallet data to the new location."));
        }
    }
}

void PupaCoinGUI::importPrivateKey()
{
    ImportPrivateKeyDialog dlg(this);
    dlg.setModel(walletModel->getAddressTableModel());
    dlg.exec();
}

void PupaCoinGUI::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void PupaCoinGUI::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog::Mode mode = sender() == unlockWalletAction ?
              AskPassphraseDialog::UnlockStaking : AskPassphraseDialog::Unlock;
        AskPassphraseDialog dlg(mode, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void PupaCoinGUI::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
}

void PupaCoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void PupaCoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void PupaCoinGUI::updateWeight()
{
    if (!pwalletMain)
        return;

    TRY_LOCK(cs_main, lockMain);
    if (!lockMain)
        return;

    TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
    if (!lockWallet)
        return;

    nWeight = pwalletMain->GetStakeWeight();
}

/*void PupaCoinGUI::updateStakingIcon()
{
    updateWeight();

    if (nLastCoinStakeSearchInterval && nWeight)
    {
        uint64_t nWeight = this->nWeight;
        uint64_t nNetworkWeight = GetPoSKernelPS();
        unsigned nEstimateTime = 0;
        nEstimateTime = GetTargetSpacing * nNetworkWeight / nWeight;

        QString text;
        if (nEstimateTime < 60)
        {
            text = tr("%n second(s)", "", nEstimateTime);
        }
        else if (nEstimateTime < 60*60)
        {
            text = tr("%n minute(s)", "", nEstimateTime/60);
        }
        else if (nEstimateTime < 24*60*60)
        {
            text = tr("%n hour(s)", "", nEstimateTime/(60*60));
        }
        else
        {
            text = tr("%n day(s)", "", nEstimateTime/(60*60*24));
        }

        nWeight /= COIN;
        nNetworkWeight /= COIN;

        labelStakingIcon->setPixmap(QIcon(fUseDarkTheme ? ":/icons/dark/staking_on" : ":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelStakingIcon->setToolTip(tr("Staking.<br>Your weight is %1<br>Network weight is %2<br>Expected time to earn reward is %3").arg(nWeight).arg(nNetworkWeight).arg(text));
    }
    else
    {
        labelStakingIcon->setPixmap(QIcon(fUseDarkTheme ? ":/icons/dark/staking_off" : ":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        if (pwalletMain && pwalletMain->IsLocked())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is locked"));
        else if (vNodes.empty())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is offline"));
        else if (IsInitialBlockDownload())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is syncing"));
        else if (!nWeight)
            labelStakingIcon->setToolTip(tr("Not staking because you don't have mature coins"));
        else
            labelStakingIcon->setToolTip(tr("Not staking"));
    }
}*/

void PupaCoinGUI::detectShutdown()
{
    if (ShutdownRequested())
        QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}

/*void PupaCoinGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}*/

void PupaCoinGUI::editConfig()
{
    EditConfigDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void PupaCoinGUI::editConfigExt()
{
    filesystem::path path = GetConfigFile();
    QString pathString = QString::fromStdString(path.string());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pathString));
}

void PupaCoinGUI::openDataDir()
{
    filesystem::path path = GetDataDir();
    QString pathString = QString::fromStdString(path.string());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pathString));
}
