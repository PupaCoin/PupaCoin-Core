#include "sendmessagesentry.h"
#include "ui_sendmessagesentry.h"
#include "guiutil.h"
#include "addressbookpage.h"
#include "walletmodel.h"
#include "messagemodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"

#include "smessage.h"

#include <QApplication>
#include <QClipboard>

SendMessagesEntry::SendMessagesEntry(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SendMessagesEntry),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    ui->sendToLayout->setSpacing(4);
#endif
#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));
    ui->sendTo->setPlaceholderText(tr("Enter recipient PupaCoin smsginfo (e.g. dZFKfe559tX7LA8BDMeiPH3C8DbpTJ92Y8:dZv7b41skzqG8cj2d5S56MG1hVQbcK8jMoorX8Wdmf5v)"));
    ui->messageText->setErrorText(tr("You cannot send a blank message!"));
#endif
    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(ui->sendTo);

    GUIUtil::setupAddressWidget(ui->sendTo, this);
}

SendMessagesEntry::~SendMessagesEntry()
{
    delete ui;
}

void SendMessagesEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->sendTo->setText(QApplication::clipboard()->text());
}

void SendMessagesEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;

    AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::SendingTab, this);

    dlg.setModel(model->getWalletModel()->getAddressTableModel());

    if(dlg.exec())
    {
        QString address = dlg.getReturnValue();
        QString pubkey;
        QString smsgInfo = address + ":";
        QMessageBox::warning(this, tr("debug"),address);

        if(model->getAddressOrPubkey(address, pubkey))
        {
            ui->sendTo->setText(smsgInfo + pubkey);
            ui->messageText->setFocus();
        } else {
            ui->sendTo->setText(smsgInfo);
            ui->sendTo->setFocus();
        }
    }
}

void SendMessagesEntry::on_sendTo_textChanged(const QString &smsgInfo)
{
    if(!model)
        return;

    QString addr = smsgInfo.split(":")[0];

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getWalletModel()->getAddressTableModel()->labelForAddress(addr);

    if(!associatedLabel.isEmpty())
        ui->addAsLabel->setText(associatedLabel);
}

void SendMessagesEntry::setModel(MessageModel *model)
{

    this->model = model;

    //clear();
}

void SendMessagesEntry::loadRow(int row)
{
    if(model->data(model->index(row, model->Type, QModelIndex()), Qt::DisplayRole).toString() == MessageModel::Received)
        ui->sendTo->setText(model->data(model->index(row, model->FromAddress, QModelIndex()), Qt::DisplayRole).toString());
    else
        ui->sendTo->setText(model->data(model->index(row, model->ToAddress, QModelIndex()), Qt::DisplayRole).toString());
}

void SendMessagesEntry::setRemoveEnabled(bool enabled)
{
    ui->deleteButton->setEnabled(enabled);
}

void SendMessagesEntry::clear()
{
    ui->sendTo->clear();
    ui->addAsLabel->clear();
    ui->messageText->clear();
    ui->sendTo->setFocus();
}

void SendMessagesEntry::on_deleteButton_clicked()
{
    emit removeEntry(this);
}


bool SendMessagesEntry::validate()
{
    // Check input validity
    bool retval = true;

    if(ui->messageText->toPlainText() == "")
    {
        ui->messageText->setValid(false);

        retval = false;
    }

    QStringList smsgInfo = ui->sendTo->text().split(":");

    if(smsgInfo.length() != 2 || !model->getWalletModel()->validateAddress(smsgInfo[0]) || smsgInfo[1] == "")
    {
        ui->sendTo->setValid(false);

        retval = false;
    }
    // Removed the below snippet from the "else if" check, caused ":" to be an unnacceptable
    // character and throw an address invalid when in fact it's valid as it verifies it still
    // with the remaining active check below.
    // REMOVED:    !ui->sendTo->hasAcceptableInput() ||
    else if((!model->getWalletModel()->validateAddress(smsgInfo[0])))
    {
        ui->sendTo->setValid(false);

        retval = false;
    }

    return retval;
}

SendMessagesRecipient SendMessagesEntry::getValue()
{
    SendMessagesRecipient rv;
    QStringList smsgInfo = ui->sendTo->text().split(":");

    rv.address = smsgInfo[0];
    rv.label = ui->addAsLabel->text();
    rv.pubkey = smsgInfo[1];
    rv.message = ui->messageText->toPlainText();

    return rv;
}


QWidget *SendMessagesEntry::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, ui->sendTo);
    QWidget::setTabOrder(ui->sendTo, ui->addressBookButton);
    QWidget::setTabOrder(ui->addressBookButton, ui->pasteButton);
    QWidget::setTabOrder(ui->pasteButton, ui->deleteButton);
    QWidget::setTabOrder(ui->deleteButton, ui->addAsLabel);
    QWidget::setTabOrder(ui->addAsLabel, ui->messageText);

    return ui->messageText;

}

void SendMessagesEntry::setValue(const SendMessagesRecipient &value)
{
    ui->sendTo->setText(value.address + ":" + value.pubkey);
    ui->addAsLabel->setText(value.label);
    ui->messageText->setPlainText(value.message);
}

bool SendMessagesEntry::isClear()
{
    return ui->sendTo->text().isEmpty();
}

void SendMessagesEntry::setFocus()
{
    ui->sendTo->setFocus();
}
