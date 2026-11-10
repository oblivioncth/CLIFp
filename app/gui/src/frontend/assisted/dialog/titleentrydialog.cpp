// Unit Include
#include "titleentrydialog.h"

// Qt Includes
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>

// magic enum
#include <magic_enum.hpp>

// Project Includes
#include "frontend/gui.h"

//===============================================================================================================
// TitleTitleEntryDialog
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------------
//Private:
TitleEntryDialog::TitleEntryDialog(const QString& windowTitle, QWidget* parent) :
    QDialog(parent),
    mSettings(FrontendGui::getSettingsGroup(u"dialog/title-entry"_s))
{
    // Format Dialog
    setWindowTitle(windowTitle);
    setMinimumWidth(360);

    //-ID tab------------------------------------------------
    mId = new QLineEdit(this);
    mId->setPlaceholderText(PLACEHOLDER_ID);
    mId->setClearButtonEnabled(true);

    auto* idPage = new QWidget;
    auto* idForm = new QFormLayout(idPage);
    idForm->addRow(makeHint(INSTRUCTIONS_ID, idPage));
    idForm->addRow(LAEBL_ID, mId);

    //-Title tab------------------------------------------------
    mTitle = new QLineEdit(this);
    mTitle->setPlaceholderText(PLACEHOLDER_TITLE);
    mTitle->setClearButtonEnabled(true);

    mSubtitle = new QLineEdit(this);
    mSubtitle->setPlaceholderText(PLACEHOLDER_SUBTITLE);
    mSubtitle->setClearButtonEnabled(true);

    auto* titlePage = new QWidget;
    auto* titleForm = new QFormLayout(titlePage);
    titleForm->addRow(makeHint(INSTRUCTIONS_TITLE, titlePage));
    titleForm->addRow(LAEBL_TITLE, mTitle);
    titleForm->addRow(LAEBL_SUBTITLE, mSubtitle);

    //-Tab/Overall Setup----------------------------------------------
    mTabs = new QTabWidget(this);
    mTabs->addTab(idPage, TAB_LABEL_ID);
    mTabs->addTab(titlePage, TAB_LABEL_TITLE);

    mButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(mButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* root = new QVBoxLayout(this);
    root->addWidget(mTabs);
    root->addWidget(mButtons);

    connect(mTabs, &QTabWidget::currentChanged, this, [this] {
        updateOkEnabled();
        focusActiveTab();
    });
    connect(mId, &QLineEdit::textChanged, this, [this] { updateOkEnabled(); });
    connect(mTitle, &QLineEdit::textChanged, this, [this] { updateOkEnabled(); });

    loadSettings();
    updateOkEnabled();
    focusActiveTab();
}

//-Destructor-------------------------------------------------------------------------------------------------------
//Public:
TitleEntryDialog::~TitleEntryDialog()
{
    saveSettings();
}

//-Class Functions--------------------------------------------------------------------------------------------------------
//Private:
QLabel* TitleEntryDialog::makeHint(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setWordWrap(true);

    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, pal.color(QPalette::PlaceholderText));
    label->setPalette(pal);

    return label;
}

//Public:
std::optional<TitleEntryDialog::Entry> TitleEntryDialog::getTitle(QWidget* parent, const QString& windowTitle)
{
    std::optional<Entry> result;

    {   // Dialog only exists in this scope
        TitleEntryDialog dlg(windowTitle, parent);
        if(dlg.exec() == QDialog::Accepted)
            result = dlg.input();
    }

    return result;
}

//-Insatnce Functions--------------------------------------------------------------------------------------------------------
//Private:
void TitleEntryDialog::loadSettings()
{
    // Tab
    Tab startTab = Tab::IdTab;
    if(auto optSet = mSettings.value<int>(SKEY_LAST_TAB))
        if(auto optTab = magic_enum::enum_cast<Tab>(*optSet))
            startTab = *optTab;

    mTabs->setCurrentIndex(startTab);
}

void TitleEntryDialog::saveSettings()
{
    mSettings.setValue(SKEY_LAST_TAB, mTabs->currentIndex());
}

TitleEntryDialog::Entry TitleEntryDialog::input() const
{
    if(mTabs->currentIndex() == IdTab)
        return IdEntry{ mId->text().trimmed() };

    return TitleEntry{ mTitle->text().trimmed(), mSubtitle->text().trimmed() };
}

void TitleEntryDialog::updateOkEnabled()
{
    // Only requires that the active tab's mandatory field isn't blank; the content isn't checked
    const QLineEdit* required = (mTabs->currentIndex() == IdTab) ? mId : mTitle;
    mButtons->button(QDialogButtonBox::Ok)->setEnabled(!required->text().trimmed().isEmpty());
}

void TitleEntryDialog::focusActiveTab()
{
    (mTabs->currentIndex() == IdTab ? mId : mTitle)->setFocus();
}