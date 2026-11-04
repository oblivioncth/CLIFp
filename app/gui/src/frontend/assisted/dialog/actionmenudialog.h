#ifndef ACTIONMENUDIALOG_H
#define ACTIONMENUDIALOG_H

// Standard Library Includes
#include <memory>

// Qt Includes
#include <QDialog>

// Project Includes
#include "frontend/assisted/action/action.h"

class ActionMenuDialog : public QDialog
{
//-Instance Variables----------------------------------------------------------------------------------------------
private:
    ActionRegistry::Factory mSelected = nullptr;

//-Constructor-------------------------------------------------------------------------------------------------------
private:
    ActionMenuDialog(const QString& title, const QString& subtitle, QWidget* parent = nullptr);

//-Class Functions--------------------------------------------------------------------------------------------------------
public:
    // Shows the menu modally, destroys it, and returns the chosen action, or nullptr if the user canceled.
    static std::unique_ptr<Action> getAction(QWidget* parent, const QString& title, const QString& subtitle);

};

#endif // ACTIONMENUDIALOG_H
