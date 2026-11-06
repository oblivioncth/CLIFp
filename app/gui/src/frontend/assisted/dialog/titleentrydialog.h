#ifndef TITLEENTRYDIALOG_H
#define TITLEENTRYDIALOG_H

// Standard Library Includes
#include <variant>
#include <optional>

// Qt Includes
#include <QDialog>
#include <QString>
#include <QUuid>

using namespace Qt::StringLiterals;

class QDialogButtonBox;
class QLineEdit;
class QTabWidget;
class QLabel;

class TitleEntryDialog : public QDialog
{
//-Instance Types----------------------------------------------------------------------------------------------
private:
    enum Tab { IdTab, TitleTab };

public:
    struct IdEntry
    {
        QString id;
    };

    struct TitleEntry
    {
        QString title;
        QString subtitle; // may be empty
    };
    using Entry = std::variant<IdEntry, TitleEntry>;

//-Class Variables----------------------------------------------------------------------------------------------
private:
    // ID Tab
    static inline const QString PLACEHOLDER_ID = u"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"_s;
    static inline const QString LAEBL_ID = u"ID:"_s;
    static inline const QString INSTRUCTIONS_ID = u"Select Entry via Game or Additional App UUID. You can get a Game's or Additional App UUID by right clicking it in the "
                                                  "standard Flashpoint launcher and selecting the 'Copy' option."_s;

    // Title Tab
    static inline const QString PLACEHOLDER_TITLE = u"Game Title"_s;
    static inline const QString PLACEHOLDER_SUBTITLE = u"Additional Application Name (Optional)"_s;
    static inline const QString LAEBL_TITLE = u"Title:"_s;
    static inline const QString LAEBL_SUBTITLE = u"Additional App:"_s;
    static inline const QString INSTRUCTIONS_TITLE = u"Select Entry by Game title. If selecting an Additional App under that Game, enter its name in the second box; "_s
                                                     u"Otherwise leave that box blank."_s;

    // Overall
    static inline const QString TAB_LABEL_ID = u"By &ID"_s;
    static inline const QString TAB_LABEL_TITLE = u"By &Title"_s;

//-Instance Variables----------------------------------------------------------------------------------------------
private:
    QTabWidget* mTabs;
    QLineEdit* mId;
    QLineEdit* mTitle;
    QLineEdit* mSubtitle;
    QDialogButtonBox* mButtons;

//-Constructor-------------------------------------------------------------------------------------------------------
private:
    TitleEntryDialog(const QString& windowTitle, QWidget* parent);

//-Class Functions--------------------------------------------------------------------------------------------------------
private:
    static QLabel* makeHint(const QString& text, QWidget* parent);

public:
    // Shows the dialog modally. Returns what was entered on whichever tab was active when
    // the user accepted, or std::nullopt if cancelled. No validation is done on the content
    static std::optional<Entry> getTitle(QWidget* parent, const QString& windowTitle);

//-Instance Functions--------------------------------------------------------------------------------------------------------
private:
    Entry input() const;
    void updateOkEnabled();
    void focusActiveTab();
};

#endif // TITLEENTRYDIALOG_H