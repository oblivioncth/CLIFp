// Unit Include
#include "t-titleexec.h"

// Qx Includes
#include <qx/core/qx-processbider.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "kernel/core.h"

//===============================================================================================================
// TTitleExec
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Private:
Qx::Error TTitleExec::setupBide()
{
    // Check for bide
    logEvent(LOG_EVENT_CHECKING_FOR_BIDE);

    const Fp::Toolkit* tk = mCore.fpInstall().toolkit();
    bool involvesSecurePlayer;
    Qx::Error securePlayerCheckError = tk->appInvolvesSecurePlayer(involvesSecurePlayer, QFileInfo(executable()));
    if(securePlayerCheckError.isValid())
    {
        postDirective<DError>(securePlayerCheckError);
        return securePlayerCheckError;
    }

    logEvent(LOG_EVENT_BIDE_DETERMINED.arg(involvesSecurePlayer ? u"does"_s : u"doesn't"_s));

    if(involvesSecurePlayer)
    {
        // Setup bider
        using namespace std::chrono_literals;
        static const auto grace = 2s;
        static const QString processName = tk->SECURE_PLAYER_INFO.fileName();

        mBider = new Qx::ProcessBider(this, processName);
        mBider->setRespawnGrace(grace);
        mBider->setInitialGrace(true); // Process will be stopped at first
        connect(mBider, &Qx::ProcessBider::established, this, [this]{
            logEvent(LOG_EVENT_BIDE_RUNNING.arg(processName));
            logEvent(LOG_EVENT_BIDE_ON.arg(processName));
        });
        connect(mBider, &Qx::ProcessBider::processStopped, this, [this]{
            logEvent(LOG_EVENT_BIDE_QUIT.arg(processName));
        });
        connect(mBider, &Qx::ProcessBider::graceStarted, this, [this]{
            logEvent(LOG_EVENT_BIDE_GRACE.arg(QString::number(grace.count()), processName));
        });
        connect(mBider, &Qx::ProcessBider::errorOccurred, this, [this](Qx::ProcessBiderError err){
            postDirective<DError>(err);
        });
        connect(mBider, &Qx::ProcessBider::finished, this, [this](Qx::ProcessBider::ResultType type){
            mBider = nullptr; // Child, will be deleted automatically
            if(type == Qx::ProcessBider::Fail)
                cleanup(TTitleExecError(processName, TTitleExecError::BideFail));
            else
            {
                logEvent(LOG_EVENT_BIDE_FINISHED.arg(processName));
                cleanup(TTitleExecError());
            }
        });
    }

    return {};
}

void TTitleExec::startBide()
{
    logEvent(LOG_EVENT_BIDE_START);
    mBider->start();
}
