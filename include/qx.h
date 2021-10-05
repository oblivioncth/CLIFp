#ifndef QX_H
#define QX_H

#define ENABLE_IF(...) std::enable_if_t<__VA_ARGS__::value, int> = 0 // enable_if Macro; allows ENABLE_IF(std::is_arithmetic<T>) for example
#define ENABLE_IF2(...) std::enable_if_t<__VA_ARGS__::value, int> // enable_if Macro with no default argument, use if template was already forward declared

#ifdef QT_WIDGETS_LIB // Only enabled for Widget applications
#include <QWidget>
#include <QMessageBox>
#endif

#include <QHash>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QtEndian>
#include <QJsonObject>
#include <QJsonArray>

#include <QSet>
#include <QDateTime>

#include <QBitArray>
#include <stdexcept>
#include "assert.h"

//-Non-namspace Functions-------------------------------------------------------------------------------------------------
template <typename T>
const T qAsConstR(T &&t) { return std::move(t); }

namespace Qx
{
//-std::functional Extensions-----------------------------------------------------------------------------------
struct left_shift {

    template <typename L, typename R>
    constexpr auto operator()(L&& l, R&& r) const
    noexcept(noexcept(std::forward<L>(l) << std::forward<R>(r)))
    -> decltype(std::forward<L>(l) << std::forward<R>(r))
    {
        return std::forward<L>(l) << std::forward<R>(r);
    }
};

struct right_shift {

    template <typename L, typename R>
    constexpr auto operator()(L&& l, R&& r) const
    noexcept(noexcept(std::forward<L>(l) >> std::forward<R>(r)))
    -> decltype(std::forward<L>(l) >> std::forward<R>(r))
    {
        return std::forward<L>(l) >> std::forward<R>(r);
    }
};

//-Traits-------------------------------------------------------------------------------------------------------
// TODO: Get these working where they make sense
//template <class T, template <class...> class Template>
//struct is_specialization : std::false_type {};

//template <template <class...> class Template, class... Args>
//struct is_specialization<Template<Args...>, Template> : std::true_type {};

template<typename T, typename... Others>
struct is_any : std::disjunction<std::is_same<T, Others>...>
{};

template<typename X, typename Y, typename Op>
struct defines_op_impl
{
    template<typename U, typename L, typename R>
    static auto test(int) -> decltype(std::declval<U>()(std::declval<L>(), std::declval<R>()),
                                      void(), std::true_type());

    template<typename U, typename L, typename R>
    static auto test(...) -> std::false_type;

    using type = decltype(test<Op, X, Y>(0));

};

template<typename X, typename Y, typename Op> using defines_op = typename defines_op_impl<X, Y, Op>::type;

template<class X, class Y> using defines_equality = defines_op<X, Y, std::equal_to<>>;
template<class X, class Y> using defines_inequality = defines_op<X, Y, std::not_equal_to<>>;
template<class X, class Y> using defines_less_than = defines_op<X, Y, std::less<>>;
template<class X, class Y> using defines_less_equal = defines_op<X, Y, std::less_equal<>>;
template<class X, class Y> using defines_greater_than = defines_op<X, Y, std::greater<>>;
template<class X, class Y> using defines_greater_equal = defines_op<X, Y, std::greater_equal<>>;
template<class X, class Y> using defines_bit_xor = defines_op<X, Y, std::bit_xor<>>;
template<class X, class Y> using defines_bit_or = defines_op<X, Y, std::bit_or<>>;
template<class X, class Y> using defines_left_shift = defines_op<X, Y, left_shift>;
template<class X, class Y> using defines_right_shift = defines_op<X, Y, right_shift>;

template<typename T>
using is_json_type = std::bool_constant<is_any<T, bool, double, QString, QJsonArray, QJsonObject>::value>;

template<typename T>
using is_datastream_type = std::bool_constant<is_any<T, bool, double, QString, QJsonArray, QJsonObject>::value>;

//-Class Forward Declarations---------------------------------------------------------------------------------------------
template <typename T, ENABLE_IF(std::is_arithmetic<T>)>
class NII;

//-Functions----------------------------------------------------------------------------------------------------
template <typename T>
struct typeIdentifier {typedef T type; }; // Forces compiler to deduce the type of T from only one argument so that implicit conversions can be used for the others

template<typename T, ENABLE_IF(std::is_integral<T>)>
T rangeToLength(T start, T end)
{
    // Returns the length from start to end including start, primarily for support of NII (Negative Is Infinity)
    T length = end - start;
    length++;
    return length;
}

template<typename T, ENABLE_IF(std::is_arithmetic<T>)>
static bool isOdd(T num) { return num % 2; }

template<typename T, ENABLE_IF(std::is_arithmetic<T>)>
static bool isEven(T num) { return !isOdd(num); }

//-Classes------------------------------------------------------------------------------------------------------
class Endian // Must be before its use, hence why this is out of alphabetical order
{
//-Class Types----------------------------------------------------------------------------------------------
public:
    enum Endianness{LE,BE};
};

class Array
{
//-Class Functions----------------------------------------------------------------------------------------------
public:
    template <typename T, int N>
    static constexpr int constDim(T(&)[N]) { return N; } // Allows using the size of a const array at runtime

    template <typename T, int N>
    static int indexOf(T(&array) [N], typename typeIdentifier<T>::type query)
    {
        for(int i = 0; i < N; i++)
            if(array[i] == query)
                return i;

        return -1;
    }

    template<typename T, int N>
    static T maxOf(T(&array) [N])
    {
       T max = array[0];

       for(int i = 1; i < N; i++)
           if(array[i] > max)
               max = array[i];

       return max;
    }

    template<typename T, int N>
    static T minOf(T(&array) [N])
    {
        T min = array[0];

        for(int i = 1; i < N; i++)
            if(array[i] < min)
                min = array[i];

        return min;
    }

    template<typename T, int N>
    static T mostFrequent(T(&array) [N])
    {
        // Load all array elements into a hash
        QHash<T,int> hash;
        for(int i = 0; i < N; i++)
            hash[array[i]]++;

        // Determine greatest frequency
        int maxFreq = 0;
        T maxFreqVal = array[0]; // Assume first value is most frequent to start
        QHashIterator<T,int> i(hash);

        while(i.hasNext())
        {
            i.next();
            if(maxFreq < i.value())
            {
                maxFreqVal = i.key();
                maxFreq = i.value();
            }
        }

         return maxFreqVal;
    }
};

class ByteArray
{
//-Class Functions----------------------------------------------------------------------------------------------
public:
    template<typename T, ENABLE_IF(std::is_integral<T>)>
    static QByteArray RAWFromPrimitive(T primitive, Endian::Endianness endianness = Endian::LE)
    {
        QByteArray rawBytes;

        if(typeid(T) == typeid(bool))
            rawBytes.append(static_cast<char>(static_cast<int>(primitive))); // Ensures true -> 0x01 and false -> 0x00
        else
        {
            for(int i = 0; i != sizeof(T); ++i)
            {
                #pragma warning( push )          // Disable "Unsafe mix of type 'bool' and 'int' warning because the
                #pragma warning( disable : 4805) // function will never reach this point when bool is used
                if(endianness == Endian::LE)
                    rawBytes.append(static_cast<char>(((primitive & (0xFF << (i*8))) >> (i*8))));
                else
                    rawBytes.prepend(static_cast<char>(((primitive & (0xFF << (i*8))) >> (i*8))));
                #pragma warning( pop )
            }
        }

        return rawBytes;
    }

    template<typename T, ENABLE_IF(std::is_floating_point<T>)>
    static QByteArray RAWFromPrimitive(T primitive, Endian::Endianness endianness = Endian::LE)
    {
        QByteArray rawBytes;

        if(typeid(T) == typeid(float))
        {
            int* temp = reinterpret_cast<int*>(&primitive);
            int intStandIn = (*temp);

            for(uint8_t i = 0; i != sizeof(float); ++i)
            {
                if(endianness == Endian::LE)
                    rawBytes.append(static_cast<char>(((intStandIn & (0xFF << (i*8))) >> (i*8))));
                else
                    rawBytes.prepend(static_cast<char>(((intStandIn & (0xFF << (i*8))) >> (i*8))));
            }
        }

        if(typeid(T) == typeid(double))
        {
            long* temp = reinterpret_cast<long*>(&primitive);
            long intStandIn = (*temp);

            for(uint8_t i = 0; i != sizeof(double); ++i)
            {
                if(endianness == Endian::LE)
                    rawBytes.append(static_cast<char>(((intStandIn & (0xFF << (i*8))) >> (i*8))));
                else
                    rawBytes.prepend(static_cast<char>(((intStandIn & (0xFF << (i*8))) >> (i*8))));
            }
        }

        return rawBytes;
    }

    template<typename T, ENABLE_IF(std::is_fundamental<T>)>
    static T RAWToPrimitive(QByteArray ba, Endian::Endianness endianness = Endian::LE)
    {
        static_assert(std::numeric_limits<float>::is_iec559, "Only supports IEC 559 (IEEE 754) float"); // For floats
        assert((ba.size() >= 2 && ba.size() <= 8 && isEven(ba.size())) || ba.size() == 1);

        if(sizeof(T) == 1)
        {
            quint8 temp;

            if(endianness == Endian::BE)
                 temp = qFromBigEndian<quint8>(ba);
            else if(endianness == Endian::LE)
                 temp = qFromLittleEndian<quint8>(ba);

            T* out = reinterpret_cast<T*>(&temp);
            return(*out);
        }
        else if(sizeof(T) == 2)
        {
            quint16 temp;

            if(endianness == Endian::BE)
                 temp = qFromBigEndian<quint16>(ba);
            else if(endianness == Endian::LE)
                 temp = qFromLittleEndian<quint16>(ba);

            T* out = reinterpret_cast<T*>(&temp);
            return(*out);
        }
        else if(sizeof(T) == 4)
        {
            quint32 temp;

            if(endianness ==Endian::BE)
                 temp = qFromBigEndian<quint32>(ba);
            else if(endianness == Endian::LE)
                 temp = qFromLittleEndian<quint32>(ba);

            T* out = reinterpret_cast<T*>(&temp);
            return(*out);
        }
        else if(sizeof(T) == 8)
        {
            quint64 temp;

            if(endianness == Endian::BE)
                 temp = qFromBigEndian<quint64>(ba);
            else if(endianness == Endian::LE)
                 temp = qFromLittleEndian<quint64>(ba);

            T* out = reinterpret_cast<T*>(&temp);
            return(*out);
        }
    }
};

class BitArray : public QBitArray
{
//-Constructor--------------------------------------------------------------------------------------------------
public:
    BitArray();
    BitArray(int size, bool value = false);

//-Class Functions----------------------------------------------------------------------------------------------
public:
    template<typename T, ENABLE_IF(std::is_integral<T>)>
    static BitArray fromInteger(const T& integer)
    {
        int bitCount = sizeof(T)*8;

        BitArray bitRep(bitCount);

        for(int i = 0; i < bitCount; ++i)
            if(integer & 0b1 << i)
                bitRep.setBit(i);

        return bitRep;
    }

//-Instance Functions-------------------------------------------------------------------------------------------
public:
    template<typename T, ENABLE_IF(std::is_integral<T>)>
    T toInteger()
    {
        int bitCount = sizeof(T)*8;
        T integer = 0;

        for(int i = 0; i < bitCount && i < count(); ++i)
            integer |= (testBit(i) ? 0b1 : 0b0) << i;

        return integer;
    }

    QByteArray toByteArray(Endian::Endianness endianness = Endian::BE);

    void append(bool bit = false);
    void replace(const BitArray& bits, int start = 0, int length = -1);

    template<typename T, ENABLE_IF(std::is_integral<T>)>
    void replace(T integer, int start = 0, int length = -1)
    {
        BitArray converted = BitArray::fromInteger(integer);
        replace(converted, start, length);
    }

    BitArray extract(int start, int length = -1);

    BitArray operator<<(int n);
    void operator<<=(int n);
    BitArray operator>>(int n);
    void operator>>=(int n);
    BitArray operator+(BitArray rhs);
    void operator+=(const BitArray& rhs);
};

class Char
{
//-Class Functions----------------------------------------------------------------------------------------------
public:
    static bool isHexNumber(QChar hexNum);
};

#ifdef QT_GUI_LIB // Only enabled for GUI applications
class Color
{
//-Class Functions----------------------------------------------------------------------------------------------
public:
    static QColor textColorFromBackgroundColor(QColor bgColor);
};
#endif

class DateTime
{
//-Class Variables----------------------------------------------------------------------------------------------
private:
    static const qint64 FILETIME_EPOCH_OFFSET_MS = 11644473600000; // Milliseconds between FILETIME 0 and Unix Epoch 0
    static const qint64 EPOCH_MIN_MS = static_cast<qint64>(QDateTime::YearRange::First) * 31556952000; // Years to MS
    static const qint64 EPOCH_MAX_MS = std::numeric_limits<qint64>::max(); // The true max QDateTime can represent is above signed 64bit limit in ms

//-Class Functions----------------------------------------------------------------------------------------------
public:
    static QDateTime fromMSFileTime(qint64 fileTime);
};

template <typename T, ENABLE_IF(std::is_integral<T>)>
class FreeIndexTracker
{
//-Class Members-------------------------------------------------------------------------------------------------
public:
    static const int ABSOLUTE_MIN = 0;
    static const int TYPE_MAX = -1;

//-Instance Members----------------------------------------------------------------------------------------------
private:
    T mMinIndex;
    T mMaxIndex;
    QSet<T> mReservedIndicies;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    FreeIndexTracker(T minIndex = 0, T maxIndex = 0, QSet<T> reservedIndicies = QSet<T>()) : mMinIndex(minIndex), mMaxIndex(maxIndex), mReservedIndicies(reservedIndicies)
    {
        // Determine programatic limit if "type max" (-1) is specified
        if(maxIndex < 0)
            mMaxIndex = std::numeric_limits<T>::max();

        // Insure initial values are valid
        assert(mMinIndex >= 0 && mMinIndex <= mMaxIndex && (reservedIndicies.isEmpty() ||
               (*std::min_element(reservedIndicies.begin(), reservedIndicies.end())) >= 0));

        // Change bounds to match initial reserve list if they are mismatched
        if(!reservedIndicies.isEmpty())
        {
            T minElement = *std::min_element(reservedIndicies.begin(), reservedIndicies.end());
            if(minElement < minIndex)
                mMinIndex = minElement;

            T maxElement = *std::max_element(reservedIndicies.begin(), reservedIndicies.end());
            if(maxElement > mMaxIndex)
                mMaxIndex = maxElement;
        }
    }

//-Instance Functions----------------------------------------------------------------------------------------------
private:
    int reserveInternal(int index)
    {
        // Check for valid index
        assert(index == -1 || (index >= mMinIndex && index <= mMaxIndex));

        int indexAffected = -1;

        // Check if index is free and reserve if so
        if(index != -1 && !mReservedIndicies.contains(index))
        {
            mReservedIndicies.insert(index);
            indexAffected = index;
        }

        return indexAffected;
    }

    int releaseInternal(int index)
    {
        // Check for valid index
        assert(index == -1 || (index >= mMinIndex && index <= mMaxIndex));

        int indexAffected = -1;

        // Check if index is reserved and free if so
        if(index != -1 && mReservedIndicies.contains(index))
        {
            mReservedIndicies.remove(index);
            indexAffected = index;
        }

        return indexAffected;
    }

public:
    bool isReserved(T index) { return mReservedIndicies.contains(index); }
    T minimum() { return mMinIndex; }
    T maximum() { return mMaxIndex; }

    T firstReserved()
    {
        if(!mReservedIndicies.isEmpty())
            return (*std::min_element(mReservedIndicies.begin(), mReservedIndicies.end()));
        else
            return -1;
    }

    T lastReserved()
    {
        if(!mReservedIndicies.isEmpty())
            return (*std::max_element(mReservedIndicies.begin(), mReservedIndicies.end()));
        else
            return -1;
    }

    T firstFree()
    {
        // Quick check for all reserved
        if(mReservedIndicies.count() == rangeToLength(mMinIndex, mMaxIndex))
            return -1;

        // Full check for first available
        for(int i = mMinIndex; i <= mMaxIndex; i++)
            if(!mReservedIndicies.contains(i))
                return i;

        // Should never be reached, used to prevent warning (all control paths)
        return -1;
    }

    T lastFree()
    {
        // Quick check for all reserved
        if(mReservedIndicies.count() == rangeToLength(mMinIndex, mMaxIndex))
            return -1;

        // Full check for first available (backwards)
        for(int i = mMaxIndex; i >= mMinIndex ; i--)
            if(!mReservedIndicies.contains(i))
                return i;

        // Should never be reached, used to prevent warning (all control paths)
        return -1;
    }

    bool reserve(int index)
    {
        // Check for valid index
        assert(index >= mMinIndex && index <= mMaxIndex);

        return reserveInternal(index) == index;
    }

    T reserveFirstFree() { return reserveInternal(firstFree()); }

    T reserveLastFree() { return reserveInternal(lastFree()); }

    bool release(int index)
    {
        // Check for valid index
        assert(index >= mMinIndex && index <= mMaxIndex);

        return releaseInternal(index) == index;
    }

};

class GenericError //TODO - Have Qx functions that use this class return "default" error levels instead of undefined ones (document them once documentation starts
{
//-Class Enums-----------------------------------------------------------------------------------------------
public:
    enum ErrorLevel { Undefined, Warning, Error, Critical };

//-Class Members---------------------------------------------------------------------------------------------
private:
    static inline const QHash<ErrorLevel, QString> ERR_LVL_STRING_MAP = {
        {ErrorLevel::Undefined, "Undefined Severity"},
        {ErrorLevel::Warning, "Warning"},
        {ErrorLevel::Error, "Error"},
        {ErrorLevel::Critical, "Critical"},
    };

public:
    static const GenericError UNKNOWN_ERROR;

//-Instance Members------------------------------------------------------------------------------------------
private:
    ErrorLevel mErrorLevel;
    QString mCaption;
    QString mPrimaryInfo;
    QString mSecondaryInfo;
    QString mDetailedInfo;

//-Constructor----------------------------------------------------------------------------------------------
public:
    GenericError();
    GenericError(ErrorLevel errorLevel, QString primaryInfo,
                 QString secondaryInfo = QString(), QString detailedInfo = QString(), QString caption = QString());

//-Instance Functions----------------------------------------------------------------------------------------------
public:
    bool isValid() const;
    ErrorLevel errorLevel() const;
    QString errorLevelString(bool caps = true) const;
    QString caption() const;
    QString primaryInfo() const;
    QString secondaryInfo() const;
    QString detailedInfo() const;

    Qx::GenericError& setErrorLevel(ErrorLevel errorLevel);

#ifdef QT_WIDGETS_LIB // Only enabled for Widget applications
    int exec(QMessageBox::StandardButtons choices, QMessageBox::StandardButton defChoice = QMessageBox::NoButton) const;
#endif
};

class Integrity
{
//-Class Functions---------------------------------------------------------------------------------------------
public:
    static QString generateChecksum(QByteArray &data, QCryptographicHash::Algorithm hashAlgorithm);
};

class Json
{
//-Class Members-------------------------------------------------------------------------------------------------
public:
    // Type names
    static inline const QString JSON_TYPE_BOOL = "bool";
    static inline const QString JSON_TYPE_DOUBLE = "double";
    static inline const QString JSON_TYPE_STRING = "string";
    static inline const QString JSON_TYPE_ARRAY = "array";
    static inline const QString JSON_TYPE_OBJECT = "object";
    static inline const QString JSON_TYPE_NULL = "null";

private:
    // Errors
    static inline const QString ERR_RETRIEVING_VALUE = "JSON Error: Could not retrieve the %1 value from key '%2'.";
    static inline const QString ERR_KEY_DOESNT_EXIST = "The key '%1' does not exist.";
    static inline const QString ERR_KEY_TYPE_MISMATCH = "They key '%1' does not hold a %2 value.";

//-Class Functions-----------------------------------------------------------------------------------------------
public:
    static Qx::GenericError checkedKeyRetrieval(bool& valueBuffer, QJsonObject jObject, QString key);
    static Qx::GenericError checkedKeyRetrieval(double& valueBuffer, QJsonObject jObject, QString key);
    static Qx::GenericError checkedKeyRetrieval(QString& valueBuffer, QJsonObject jObject, QString key);
    static Qx::GenericError checkedKeyRetrieval(QJsonArray& valueBuffer, QJsonObject jObject, QString key);
    static Qx::GenericError checkedKeyRetrieval(QJsonObject& valueBuffer, QJsonObject jObject, QString key);
};

template <typename T, ENABLE_IF2(std::is_arithmetic<T>)>
class NII // Negative Is Infinity - Wrapper class (0 is minimum)
{
//-Class Members-------------------------------------------------------------------------------------------------
public:
    static const T INF = -1;
    static const T NULL_VAL = -2;

//-Instance Members----------------------------------------------------------------------------------------------
private:
    T mValue;

//-Constructor----------------------------------------------------------------------------------------------
public:
    NII() { mValue = NULL_VAL; }
    NII(T value, bool boundAtZero = false) : mValue(forceBounds(value, boundAtZero)) { }

//-Class Functions----------------------------------------------------------------------------------------------
private:
    T forceBounds(T checkValue, bool boundAtZero)
    {
        if(boundAtZero && checkValue < 0)
            return 0;
        else if(checkValue < 0 )
            return -1;

        return checkValue;
    }
//-Instance Functions----------------------------------------------------------------------------------------------
public:
    bool operator==(const NII& otherNII) const { return mValue = otherNII.mValue; }
    bool operator!=(const NII& otherNII) const { return !(*this == otherNII); }
    bool operator<(const NII& otherNII) const { return (!this->isInf() && otherNII.isInf()) || (!this->isInf() && (this->mValue < otherNII.mValue)); }
    bool operator<=(const NII& otherNII) const { return !(*this > otherNII); }
    bool operator>(const NII& otherNII) const { return otherNII < *this; }
    bool operator>=(const NII& otherNII) const { return !(*this < otherNII); }
    NII operator-(const NII& otherNII)
    {
        if(otherNII.isInf())
            return NII(0);
        else if(this->isInf())
            return NII(INF);
        else
            return forceBounds(this->mValue - otherNII.mValue, true);
    }
    NII operator+(const NII& otherNII)
    {
        if(otherNII.isInf() || this->isInf())
            return NII(INF);
        else
            return forceBounds(this->mValue + otherNII.mValue, true);
    }
    NII operator/(const NII& otherNII)
    {
        if(otherNII.isInf())
            return NII(0);
        else if(this->isInf())
            return NII(INF);
        else
            return forceBounds(this->mValue/otherNII.mValue, true);
    }
    NII operator*(const NII& otherNII)
    {
        if(otherNII.isInf() || this->isInf())
            return NII(INF);
        else
            return forceBounds(this->mValue*otherNII.mValue, true);
    }
    NII& operator++()
    {
        if(!this->isInf())
            this->mValue++;

        return *this;
    }
    NII operator++(int)
    {
        NII val = (*this);
        this->operator++();
        return val;
    }
    NII& operator--()
    {
        this->mValue = forceBounds(this->mValue - 1, true);
        return *this;
    }
    NII operator--(int)
    {
        NII val = (*this);
        this->operator--();
        return val;
    }

    void setInf() { mValue = INF; }
    void setNull() { mValue = NULL_VAL; }
    bool isInf() const { return mValue == INF; }
    T value() const { return mValue; }
};

class List
{
//-Class Functions----------------------------------------------------------------------------------------------
public:
    template<typename T>
    static QList<T>* getSubListThatContains(T element, QList<QList<T>*> listOfLists)
    {
        // Returns pointer to the first list that contains "element". Returns nullptr if none
        for(QList<T>* currentList : listOfLists)
           if(currentList->contains(element))
               return currentList;

        return nullptr;
    }

    template<typename T> static QList<T> subtractAB(QList<T> &listA, QList<T> &listB)
    {
        // Difference list to fill
        QList<T> differenceList;

        for(T entry : listA)
        {
            if(!listB.contains(entry))
                differenceList << entry;
        }
        return differenceList;
    }

#ifdef QT_WIDGETS_LIB // Only enabled for Widget applications
    static QWidgetList objectListToWidgetList(QObjectList list);
#endif
};

class MMRB
{
//-Class Variables---------------------------------------------------------------------------------------------
public:
    enum class StringFormat { Full, NoTrailZero, NoTrailRBZero };

//-Member Variables--------------------------------------------------------------------------------------------
private:
    int mMajor;
    int mMinor;
    int mRevision;
    int mBuild;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    MMRB();
    MMRB(int major, int minor, int revision, int build);

//-Member Functions--------------------------------------------------------------------------------------------
public:
    bool operator== (const MMRB &otherMMRB);
    bool operator!= (const MMRB &otherMMRB);
    bool operator> (const MMRB &otherMMRB);
    bool operator>= (const MMRB &otherMMRB);
    bool operator< (const MMRB &otherMMRB);
    bool operator<= (const MMRB &otherMMRB);

    bool isNull();
    QString toString(StringFormat format = StringFormat::Full);

    int getMajorVer();
    int getMinorVer();
    int getRevisionVer();
    int getBuildVer();

    void setMajorVer(int major);
    void setMinorVer(int minor);
    void setRevisionVer(int revision);
    void setBuildVer(int build);

    void incrementMajorVer();
    void incrementMinorVer();
    void incrementRevisionVer();
    void incrementBuildVer();

private:

//-Class Functions---------------------------------------------------------------------------------------------
public:
    static MMRB fromString(QString string);
};

class Number
{
//-Class Functions---------------------------------------------------------------------------------------------
public:
    template <typename T>
    static T typeLimitedAdd(T a, T b)
    {
        if(((b > 0) && (a > (std::numeric_limits<T>::max() - b))) ||
           ((b < 0) && (a < (std::numeric_limits<T>::min() - b))))
            return std::numeric_limits<T>::max();
        else
            return a + b;
    }

    template <typename T>
    static T typeLimitedSub(T a, T b)
    {
        if((b > 0 && a < std::numeric_limits<T>::min() + b) ||
           (b < 0 && a > std::numeric_limits<T>::max() + b))
            return std::numeric_limits<T>::min();
        else
            return a - b;
    }

    template <typename T, ENABLE_IF(std::is_integral<T>)>
    static T roundToNearestMultiple(T num, T mult)
    {
        // Ignore negative multiples
        mult = std::abs(mult);

        if(mult == 0)
            return 0;

        if(mult == 1)
            return num;

        T towardsZero = (num / mult) * mult;
        T awayFromZero = num < 0 ? typeLimitedSub(towardsZero, mult) : typeLimitedAdd(towardsZero, mult);

        // Return of closest the two directions
        return (abs(num) - abs(towardsZero) >= abs(awayFromZero) - abs(num))? awayFromZero : towardsZero;
    }

    template <typename T, ENABLE_IF(std::is_integral<T>)>
    static T ceilPowOfTwo(T num)
    {
        // Return if num already is power of 2
        if (num && !(num & (num - 1)))
            return num;

        T powOfTwo = 1;

        while(powOfTwo < num)
            powOfTwo <<= 1;

        return powOfTwo;
    }

    template <typename T, ENABLE_IF(std::is_integral<T>)>
    static T floorPowOfTwo(T num)
    {
        // Return if num already is power of 2
        if (num && !(num & (num - 1)))
            return num;

        // Start with largest possible power of T type can hold
        T powOfTwo = (std::numeric_limits<T>::max() >> 1) + 1;

        while(powOfTwo > num)
            powOfTwo >>= 1;

        return powOfTwo;
    }

    template <typename T, ENABLE_IF(std::is_integral<T>)>
    static T roundPowOfTwo(T num)
    {
       T above = ceilPowOfTwo(num);
       T below = floorPowOfTwo(num);

       return above - num <= num - below ? above : below;
    }
};

class RegEx
{
//-Class Variables---------------------------------------------------------------------------------------------
public:
    static inline const QRegularExpression hexOnly =  QRegularExpression("^[0-9A-F]+$", QRegularExpression::CaseInsensitiveOption);
    static inline const QRegularExpression anyNonHex = QRegularExpression("[^a-fA-F0-9 -]", QRegularExpression::CaseInsensitiveOption);
    static inline const QRegularExpression numbersOnly = QRegularExpression("^[0-9]*$", QRegularExpression::CaseInsensitiveOption); // a digit (\d)
    static inline const QRegularExpression alphanumericOnly = QRegularExpression("^[a-zA-Z0-9]*$", QRegularExpression::CaseInsensitiveOption);
    static inline const QRegularExpression lettersOnly = QRegularExpression("^[a-zA-Z]+$", QRegularExpression::CaseInsensitiveOption);
};

class String
{
//-Class Functions----------------------------------------------------------------------------------------------
public:
    static bool isOnlyNumbers(QString checkStr);
    static bool isValidMMRB(QString version);
    static bool isHexNumber(QString hexNum);
    static bool isValidChecksum(QString checksum, QCryptographicHash::Algorithm hashAlgorithm);
    static QString fromByteArrayDirectly(QByteArray data);
    static QString formattedHex(QByteArray data, QChar separator, Endian::Endianness endianness);
    static QString stripToHexOnly(QString string);

    template<typename T, typename F>
    static QString join(QList<T> list, F&& toStringFunc, QString separator = "", QString prefix = "")
    {
        QString conjuction;

        for(int i = 0; i < list.length(); ++i)
        {
            conjuction += prefix;
            conjuction += toStringFunc(list.at(i));
            if(i < list.length() - 1)
                conjuction += separator;
        }

        return conjuction;
    }

    static QString join(QList<QString> set, QString separator = "", QString prefix = ""); // Overload for T = QString

    template<typename T, typename F>
    static QString join(QSet<T> set, F&& toStringFunc, QString separator = "", QString prefix = "")
    {
        QString conjuction;

        typename QSet<T>::const_iterator i = set.constBegin();
        while(i != set.constEnd())
        {
            conjuction += prefix;
            conjuction += toStringFunc(*i);
            if(++i != set.constEnd())
                conjuction += separator;
        }

        return conjuction;
    }

    static QString join(QSet<QString> set, QString separator = "", QString prefix = ""); // Overload for T = QString
};

class StringTraverser
{
//-Instance Members----------------------------------------------------------------------------------------------------
private:
    int mIndex;
    QString::const_iterator mIterator;
    QString::const_iterator mEnd;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    StringTraverser(const QString& string);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    void advance(int count = 1);

    QChar currentChar();
    QChar currentIndex();
    QChar lookAhead(int posOffset = 1);
    bool atEnd();
};

}

//-Metatype declarations-------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(Qx::GenericError);

#endif // QX_H
