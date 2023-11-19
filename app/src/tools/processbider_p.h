#ifndef PROCESSWAITER_P_H
#define PROCESSWAITER_P_H

#include <QString>
#include <QMutex>
#ifdef __linux__
    #include <QWaitCondition>
#endif
#ifdef _WIN32
    typedef void* HANDLE;
#endif

class ProcessWaiter
{
//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mWaiting;
    QString mName;
    quint32 mId;
    uint mPollRate;
    QMutex mMutex;

#ifdef _WIN32
    HANDLE mHandle;
#endif
#ifdef __linux__
    QWaitCondition mCloseNotifier;
#endif

//-Constructor-------------------------------------------------------------------------------------------------
public:
    ProcessWaiter(const QString& name) :
        mWaiting(false),
        mName(name),
        mId(0),
        mPollRate(500)
    {}

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    bool _wait();
    bool _close();

public:
    // Used in waiting thread
    bool wait()
    {
        mWaiting = true;
        bool r =_wait();
        mWaiting = false;
        return r;
    }

    // Used from external thread;
    void updateId(quint32 id)
    {
        mMutex.lock();
        mId = id;
        mMutex.unlock();
    };

    void setPollRate(uint pollRate)
    {
        mMutex.lock();
        mPollRate = pollRate;
        mMutex.unlock();
    }

    bool isWaiting() { return mWaiting; }
    bool close() { return mWaiting ? _close() : true; }
};

#endif // PROCESSWAITER_P_H
