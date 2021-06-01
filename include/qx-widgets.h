#ifndef QXWIDGETS_H
#define QXWIDGETS_H

#include <QDialog>
#include <QLabel>
#include <QTreeView>
#include <QDialogButtonBox>
#include <QStandardItemModel>

namespace Qx
{

//-Classes------------------------------------------------------------------------------------------------------------
class TreeInputDialog : public QDialog
{
//-QObject Macro (Required for all QObject Derived Classes)-----------------------------------------------------------
    Q_OBJECT

//-Instance Members---------------------------------------------------------------------------------------------------
private:
    QTreeView* mTreeView;
    QDialogButtonBox* mButtonBox;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    explicit TreeInputDialog(QWidget *parent = nullptr);

//-Signals---------------------------------------------------------------------------------------------------------
signals:
    void selectAllClicked();
    void selectNoneClicked();
};

class LoginDialog : public QDialog
{
//-QObject Macro (Required for all QObject Derived Classes)-----------------------------------------------------------
    Q_OBJECT

//-Class Members-------------------------------------------------------------------------------------------------------
private:
    static inline const QString LABEL_DEF_PRMT= "Login Required";
    static inline const QString LABEL_USRNAME = "&Username";
    static inline const QString LABEL_PSSWD = "&Password";

//-Instance Members---------------------------------------------------------------------------------------------------
private:
    QLabel* mPromptLabel;
    QLabel* mUsernameLabel;
    QLabel* mPasswordLabel;
    QLineEdit* mUsernameLineEdit;
    QLineEdit* mPasswordLineEdit;
    QDialogButtonBox* mButtonBox;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    explicit LoginDialog(QWidget *parent = nullptr, QString prompt = LABEL_DEF_PRMT);

//-Instance Functions----------------------------------------------------------------------------------------------
public:
    void setPrompt(QString prompt);
    QString getUsername();
    QString getPassword();

//-Slots----------------------------------------------------------------------------------------------------------
private slots:
    void acceptHandler();
    void rejectHandler();
};

class StandardItemModelX : public QStandardItemModel
{
//-Instance Members---------------------------------------------------------------------------------------------------
private:
    bool mUpdatingParentTristate = false;
    bool mAutoTristate = false;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    StandardItemModelX();

//-Instance Functions----------------------------------------------------------------------------------------------
public:
    void autoTristateChildren(QStandardItem* changingItem, const QVariant & value, int role);
    void autoTristateParents(QStandardItem* changingItem, const QVariant & changingValue);

public:
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    bool isAutoTristate();
    void setAutoTristate(bool autoTristate);
    void forEachItem(std::function<void(QStandardItem*)> const& func, QModelIndex parent = QModelIndex());

    void selectAll();
    void selectNone();
};

}

#endif // S_H
