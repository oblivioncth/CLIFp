// Unit Include
#include "progressprinter.h"

using namespace Qt::StringLiterals;

//===============================================================================================================
// ProgressPrinter
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------------
//Public:
ProgressPrinter::ProgressPrinter() { reset(); }

//-Instance Functions------------------------------------------------------------------------------------------------------
//Private:
void ProgressPrinter::rescale()
{
    // Check for busy state special case
    if(mCurrent == 0 && mMax == 0)
    {
        Qx::cout << u"..."_s << Qt::endl;
        return;
    }

    qint64 scaled = (100.0 * mCurrent)/mMax;
    if(scaled != mScaled)
    {
        mScaled = scaled;
        print();
    }
}

void ProgressPrinter::print()
{
    Qx::cout << '\r' << mScaled << Qt::flush;
    mHoldsEndlCtrl = true;
}

void ProgressPrinter::reset()
{
    mCurrent = 0;
    mMax = 100;
    mScaled = -1;
    mProcedure = {};
    mActive = false;
    mHoldsEndlCtrl = false;
}

//Public:
void ProgressPrinter::start(const QString& procedure)
{
    mProcedure = procedure;
    mActive = true;
}

void ProgressPrinter::finish()
{
    if(!mActive)
        return;

    // Maybe have DProcedureStart pass a short name that's unused in GUI (or used as window label)
    // so that here can we can print "Short Name: Done" in case there's other messages printed between
    checkAndResetEndlCtrl();
    Qx::cout << u"Done"_s << Qt::endl;
    reset();
}

void ProgressPrinter::setValue(qint64 value)
{
    if(!mActive || mCurrent == value || value > mMax)
        return;
    mCurrent = value;
    rescale();
}

void ProgressPrinter::setMaximum(qint64 max)
{
    if(mMax == max)
        return;
    mMax = max;
    if(mCurrent > mMax)
        mCurrent = mMax;

    if(mActive)
        rescale();
}

bool ProgressPrinter::isActive() const { return mActive; }
bool ProgressPrinter::checkAndResetEndlCtrl()
{
    if(!mHoldsEndlCtrl)
        return false;

    Qx::cout << Qt::endl;
    mHoldsEndlCtrl = false;
    return true;
}
