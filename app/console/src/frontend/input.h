#ifndef INPUT_H
#define INPUT_H

// Qt Includes
#include <QString>

/* Technically we could use QVariant or QTextStream::operator>>() to facilitate
 * parsing an entire line as a given type, but both of those methods have
 * caveats that IMO make this explicit, albeit annoying, conversion setup
 * better.
 *
 * QVariant/QMetaType - Cannot check that QString can be converted to T at compile
 * time, so a runtime assert would be needed, a failure of which might not be discovered
 * for a while.
 *
 * Qx::cin >> - will accept things like "123 non-number-text" and stops once a space is hit
 * so that case would have to be checked for and in all cases we'd have to manually read
 * until '\n' is hit
 */

namespace Input
{

bool to(const QString& in, QString& out);
bool to(const QString& in, int& out);
bool to(const QString& in, double& out);

template<typename T, typename F>
concept Validator = std::predicate<F, T>;

template<typename T>
concept Type = requires(T t){ to({}, t); };

}

#endif // INPUT_H
