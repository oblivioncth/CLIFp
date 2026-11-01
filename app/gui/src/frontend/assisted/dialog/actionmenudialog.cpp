// Unit Include
#include "actionmenudialog.h"

// Qt Includes
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

// Qx Includes
#include <qx/utility/qx-helpers.h>

//===============================================================================================================
// ActionMenuDialog
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------------
//Private:
ActionMenuDialog::ActionMenuDialog(const QString& title, const QString& subtitle, QWidget* parent)
    : QDialog(parent)
{
    // Format the menu
    setWindowTitle(title);
    setMinimumWidth(320);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(8);

    auto* titleLabel = new QLabel(title, this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    root->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(subtitle, this);
    subtitleLabel->setWordWrap(true);
    root->addWidget(subtitleLabel);

    root->addSpacing(8);

    // Build the menu
    for(const ActionRegistry::Entry& entry : qxAsConst(ActionRegistry::instance().entries()))
    {
        auto* button = new QPushButton(entry.text, this);
        button->setMinimumHeight(32);

        // Each button captures its own factory by value, so the mapping is correct by construction
        connect(button, &QPushButton::clicked, this, [this, factory = entry.factory] {
            mSelected = factory;
            accept();
        });

        root->addWidget(button);
    }
}

//-Class Functions--------------------------------------------------------------------------------------------------------
//Public:
std::unique_ptr<Action> ActionMenuDialog::getAction(QWidget* parent, const QString& title, const QString& subtitle)
{
    ActionRegistry::Factory chosen = nullptr;

    {   // Scope menu for quick destruction
        ActionMenuDialog dlg(title, subtitle, parent);
        if(dlg.exec() == QDialog::Accepted)
            chosen = dlg.mSelected;
    }

    return chosen ? chosen() : nullptr;
}
