#ifndef DIRECTOR_H
#define DIRECTOR_H

// Qt Includes
#include <QString>
#include <QPointer>

// Qx Includes
#include <qx/io/qx-applicationlogger.h>
#include <qx/utility/qx-concepts.h>

// Project Includes
#include "kernel/directive.h"
#include "kernel/errorcode.h"

class Task;

class QX_ERROR_TYPE(DirectorError, "DirectorError", 1201)
{
    friend class Director;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        InternalError
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {InternalError, u"Internal error."_s},
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;
    Qx::Severity mSeverity;

//-Constructor-------------------------------------------------------------
private:
    DirectorError(Type t = NoError, const QString& s = {}, Qx::Severity sv = Qx::Critical);

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class Director : public QObject
{
    Q_OBJECT;
//-Class Enums-----------------------------------------------------------------------
public:
    enum class Verbosity { Full, Quiet, Silent };

//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Qt Message Handling
    static inline constinit QtMessageHandler smDefaultMessageHandler = nullptr;
    static inline QPointer<Director> smCanonDirector;

    // Logging
    static const int LOG_MAX_ENTRIES = 50;
    static inline const QString LOG_FILE_EXT = u"log"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_NOTIFCATION_LEVEL = u"Notification Level is: %1"_s;

    // Logging - Errors
    static inline const QString LOG_ERR_CRITICAL = u"Aborting execution due to previous critical errors"_s;

    // Meta
    static inline const QString NAME = u"Director"_s;

//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    Qx::ApplicationLogger mLogger;
    Verbosity mVerbosity;
    bool mCriticalErrorOccurred;

//-Constructor------------------------------------------------------------------------------------------------------------
public:
    Director();

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    // Qt Message Handling - NOTE: Storing a static instance of director is required due to the C-function pointer interface of qInstallMessageHandler()
    static bool establishCanonDirector(Director& cd);
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Logging
    template<typename F>
        requires Qx::defines_call_for_s<F, Qx::IoOpReport>
    void log(F logFunc);

    bool isLogOpen() const;
    void logQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    // Helper
    template<DirectiveT T>
    auto ddr() // Default directive return helper
    {
        if constexpr(RequestDirectiveT<T>)
            return typename T::response_type{};
    }

public:
    // Data
    Verbosity verbosity() const;
    bool criticalErrorOccurred() const;

    // Logging
    void openLog(const QStringList& arguments);
    void setVerbosity(Verbosity verbosity);
    void logError(const QString& src, const Qx::Error& error);
    void logEvent(const QString& src, const QString& event);
    ErrorCode logFinish(const QString& src, const Qx::Error& errorState);

    // Directives
    template<DirectiveT T>
    auto postDirective(const QString& src, const T& directive)
    {
        // Special handling
        if constexpr(Qx::any_of<T, DError, DBlockingError>)
        {
            logError(src, directive.error);
            if(mVerbosity == Verbosity::Silent || (mVerbosity == Verbosity::Quiet && directive.error.severity() != Qx::Critical))
                return ddr<T>();
        }
        else
        {
            if(mVerbosity != Verbosity::Full)
                return ddr<T>();
        }

        // Send
        if constexpr(AsyncDirectiveT<T>)
            emit announceAsyncDirective(directive);
        else if constexpr(SyncDirectiveT<T>)
            emit announceSyncDirective(directive);
        else
        {
            static_assert(RequestDirectiveT<T>);
            /* Normally using a void pointer is basically obsolete, and the much newer std::any should be used
             * instead; however, here we must use a pointer to get a result back since this is a signal, and the
             * type safety of std::any is less important since the pointer is cast automatically by our scaffolding
             * based on ResponseDirectiveT::response_type. So, mind as well take advantage of the performance of
             * void* then anyway.
             *
             * Also: If a default response is needed other than the default provided by value-initialization, we
             * can add a static member "DEFAULT_RESPONSE" to each RequestDirectiveT struct and use that here. If
             * various defaults are needed on a case-by-case basis, that gets tricky as we'd need another postDirective()
             * overload.
             */
            auto response = ddr<T>();
            emit announceRequestDirective(directive, &response);
            return response;
        }
    }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void announceAsyncDirective(const AsyncDirective& aDirective);
    void announceSyncDirective(const SyncDirective& sDirective);
    void announceRequestDirective(const RequestDirective& rDirective, void* response);
};

#endif // DIRECTOR_H
