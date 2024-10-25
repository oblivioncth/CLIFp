#ifndef DIRECTORATE_H
#define DIRECTORATE_H

// Standard Library Includes
#include <utility>

// Qt Includes
#include <QString>

// Project Includes
#include "kernel/director.h"

/* TODO: It would be challenging (at least to do in a clean way), but we could try to split the template
 * definitions of postDirective et. al. so that the directives that should return a value actually return
 * the value instead of a pointer within the directive.
 */

class Directorate
{
//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    Director* mDirector;

//-Constructor------------------------------------------------------------------------------------------------------------
public:
    Directorate(Director* director = nullptr);

//-Destructor-------------------------------------------------------------------------------------------------------------
public:
    virtual ~Directorate() = default; // Future proofing if this is ever used polymorphically

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    //TODO: Some of these probably should be only under specific derivatives
    void logCommand(const QString& commandName) const;
    void logCommandOptions(const QString& commandOptions) const;
    void logError(const Qx::Error& error) const;
    void logEvent(const QString& event) const;
    void logTask(const Task* task) const;
    ErrorCode logFinish(const Qx::Error& errorState) const;

    template<DirectiveT T>
    void postDirective(const T& t) const
    {
        Q_ASSERT(mDirector);
        mDirector->postDirective(name(), t);
    }

    template<typename T, typename... Args>
    void postDirective(Args&&... args) const
    {
        Q_ASSERT(mDirector);
        mDirector->postDirective(name(), T{std::forward<Args>(args)...});
    }

public:
    virtual QString name() const = 0;

    void setDirector(Director* director);
};

#endif // DIRECTORATE_H
