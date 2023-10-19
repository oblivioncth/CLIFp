#ifndef MESSAGE_H
#define MESSAGE_H

// Qt Includes
#include <QString>

struct Message
{
    QString text;
    bool blocking = false;
    bool selectable = false;
};

#endif // MESSAGE_H
