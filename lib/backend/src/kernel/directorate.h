#ifndef DIRECTORATE_H
#define DIRECTORATE_H

// Standard Library Includes
#include <utility>

// Qt Includes
#include <QString>

// Project Includes
#include "kernel/director.h"

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
    void logError(const Qx::Error& error) const;
    void logEvent(const QString& event) const;

    // These two only return a value when a RequestDirective is used, otherwise 'return' is ignored
    template<DirectiveT T>
    [[nodiscard]] auto postDirective(const T& t) const
    {
        Q_ASSERT(mDirector);
        return mDirector->postDirective(name(), t);
    }

    template<DirectiveT T, typename... Args>
    [[nodiscard]] auto postDirective(Args&&... args) const
    {
        return postDirective(T{std::forward<Args>(args)...});
    }

protected:
    Director* director() const;

public:
    virtual QString name() const = 0;
    void setDirector(Director* director);
};

#endif // DIRECTORATE_H
