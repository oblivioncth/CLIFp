#ifndef QXNET_H
#define QXNET_H

#include <queue>
#include <QtNetwork>
#include "qx.h"
#include "qx-io.h"

namespace Qx
{

//-Structs------------------------------------------------------------------------------------------------------------
struct DownloadTask
{
    QUrl target;
    QFile* dest;
};

//-Classes------------------------------------------------------------------------------------------------------------
class NetworkReplyError
{
//-Instance Members----------------------------------------------------------------------------------------------
private:
    QNetworkReply::NetworkError mErrorType;
    QUrl mUrl;
    QString mErrorText;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    NetworkReplyError();
    NetworkReplyError(QNetworkReply* reply, QUrl url);

//-Instance Functions--------------------------------------------------------------------------------------------
public:
    bool isValid();
    QNetworkReply::NetworkError getType();
    QUrl getUrl();
    QString getText();
};

//TODO: Add AsyncDownloadManager using a finished signal instead and a dynamically managed progress output
//TODO: Potentially implement a full DownloadOpReport class similar to IOOpReport that is good for info on
//      success and fail
class SyncDownloadManager: public QObject
{
//-QObject Macro (Required for all QObject Derived Classes)-----------------------------------------------------------
    Q_OBJECT

//-Class Enums--------------------------------------------------------------------------------------------------------
public:
    enum class FinishStatus {Success, UserAbort, AutoAbort, Error};

//-Inner Classes------------------------------------------------------------------------------------------------------
public:
    class Report
    {
    //-Instance Variables---------------------------------------------------------------------------------------------
    private:
        FinishStatus mFinishStatus;
        GenericError mErrorInfo;

    //-Constructor-------------------------------------------------------------------------------------------------------
    public:
        Report();
        Report(FinishStatus finishStatus, GenericError errorInfo);

    //-Instance Functions----------------------------------------------------------------------------------------------
    public:
        FinishStatus finishStatus() const;
        GenericError errorInfo() const;
        bool wasSuccessful() const;
    };

//-Class Members------------------------------------------------------------------------------------------------------
private:
    // Errors - Primary
    static inline const QString ERR_QUEUE_INCOMPL = "The download(s) failed to complete successfully";

    // Errors - Secondary
    static inline const QString ERR_OUTCOME_FAIL = "One or more downloads failed due to the following errors.";
    static inline const QString ERR_OUTCOME_USER_ABORT = "The remaining downloads were aborted by the user.";
    static inline const QString ERR_OUTCOME_AUTO_ABORT = "The remaining downloads were aborted due to previous errors.";

    // Errors - Detail
    static inline const QString ERR_ENUM_TOTAL_SIZE = "[%1] Error enumerating download size";
    static inline const QString ERR_SINGLE_ABORT = "[%1] Aborted by user";
    static inline const QString ERR_GEN_FAIL = "[%1] %2";

    // Errors - Messages
    static inline const QString SSL_ERR = "The following SSL issues occured while attempting to download %1";
    static inline const QString CONTINUE_QUES = "Continue downloading?";
    static inline const QString AUTH_REQUIRED = "Authentication is required to connect to %1";
    static inline const QString PROXY_AUTH_REQUIRED = "Authentication is required to conenct to the proxy %1";

    // Prompts
    static inline const QString PROMPT_AUTH = "Authentication is required for %1";
    static inline const QString PROMPT_PROXY_AUTH = "Proxy authentication is required for %1";

//-Instance Members---------------------------------------------------------------------------------------------------
private:
    // Network Access
    QNetworkAccessManager mDownloadAccessMan;
    QNetworkAccessManager mQueryAccessMan;

    // Properties
    int mMaxSimultaneous = 3; // < 1 is unlimited
    QNetworkRequest::RedirectPolicy mRedirectPolicy = QNetworkRequest::NoLessSafeRedirectPolicy;  // Applied to each request as well as manager because of priority levels
    bool mOverwrite = false;
    bool mAutoAbort = false;

    // Downloads
    QList<DownloadTask> mPendingDownloads;
    QHash<QNetworkReply*, std::shared_ptr<FileStreamWriter>> mActiveDownloads;

    // Progress
    quint64 mTotalBytes = 0;
    quint64 mCurrentBytes = 0;
    QHash<QNetworkReply*, quint64> mInvididualBytes;

    // Synchronus elements
    QEventLoop mDownloadWait;
    QStringList mErrorList;

    // Status tracking
    FinishStatus mFinishStatus = FinishStatus::Success;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    SyncDownloadManager();

//-Instance Functions----------------------------------------------------------------------------------------------
private:
    NetworkReplyError enumerateTotalSize();
    NetworkReplyError getFileSize(quint64& returnBuffer, QUrl target);
    IOOpReport startDownload(DownloadTask task);
    void cancelAll();

public:
    void appendTask(DownloadTask task);
    void setMaxSimultaneous(int maxSimultaneous);
    void setRedirectPolicy(QNetworkRequest::RedirectPolicy redirectPolicy);
    void setOverwrite(bool overwrite);
    void setAutoAbort(bool autoAbort);
    Report processQueue();

//-Slots------------------------------------------------------------------------------------------------------------
private slots:
    void downloadProgressHandler(qint64 bytesCurrent, qint64 bytesTotal);
    void downloadFinished(QNetworkReply* reply);
    void readyRead();
    void sslErrorHandler(QNetworkReply* reply, const QList<QSslError>& errors);
    void authHandler(QNetworkReply* reply, QAuthenticator* authenticator);
    void proxyAuthHandler(const QNetworkProxy& proxy, QAuthenticator* authenticator);

public slots:
    void abort();

    // TODO: Add preshared key auth support

//-Signals------------------------------------------------------------------------------------------------------------
signals:
    void downloadProgress(qint64 bytesCurrent);
    void downloadTotalChanged(quint64 bytesTotal);

    void sslErrors(Qx::GenericError errorMsg, bool* abort);
    void authenticationRequired(QString prompt, QString* username, QString* password, bool* abort);
};

}

#endif // QXNET_H
