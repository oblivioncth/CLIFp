#ifndef QX_H
#define QX_H

//#define REQUIRES(...) std::enable_if_t<__VA_ARGS__> // enable_if Macro; allows REQUIRES(std::is_arithmetic_v<T>) for example
//TODO: Possibly remove this

#include <QHash>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QtEndian>
#include <QWidget>
#include <QSet>
#include "assert.h"

namespace Qx
{
//-Class Forward Declarations---------------------------------------------------------------------------------------------
template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
class NII;

class Endian;

//-Traits-------------------------------------------------------------------------------------------------------
template <class T, template <class...> class Template>
struct is_specialization : std::false_type {};

template <template <class...> class Template, class... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};

//-Functions----------------------------------------------------------------------------------------------------
template <typename T>
struct typeIdentifier {typedef T type; }; // Forces compiler to deduce the type of T from only one argument so that implicit conversions can be used for the others

template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T rangeToLength(T start, T end)
{
    // Returns the length from start to end including start, primarily for support of NII (Negative Is Infinity)
    T length = end - start;
    length++;
    return length;
}

template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
static bool isOdd(T num) { return num % 2; }

template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
static bool isEven(T num) { return !isOdd(num); }

//-Classes------------------------------------------------------------------------------------------------------
class Endian
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
    template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
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

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
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

    template<typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
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

    static QByteArray RAWFromString(QString str);
    static QByteArray RAWFromStringHex(QString str);
};

class Char
{
//-Class Functions----------------------------------------------------------------------------------------------
public:
    static bool isHexNumber(QChar hexNum);
};

template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
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
            maxIndex = std::numeric_limits<T>::max();

        // Insure initial values are valid
        assert(minIndex >= 0 && minIndex <= maxIndex && (reservedIndicies.isEmpty() ||
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

class Integrity
{
//-Class Functions---------------------------------------------------------------------------------------------
public:
    static QByteArray generateChecksum(QByteArray &data, QCryptographicHash::Algorithm hashAlgorithm);
};

template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
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

    static QWidgetList objectListToWidgetList(QObjectList list);

};

class MMRB
{
//-Class Variables---------------------------------------------------------------------------------------------
private:
    //static inline const QString MMRB_FORMAT = "%1.%2.%3.%4";

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

    QString toString();
    int getMajorVer();
    int getMinorVer();
    int getRevisionVer();
    int getBuildVer();
    bool isNull();
private:

//-Class Functions---------------------------------------------------------------------------------------------
public:
    static MMRB fromString(QString string);
};

class RegEx
{
//-Class Variables---------------------------------------------------------------------------------------------
public:
    static inline const QRegularExpression hexOnly =  QRegularExpression("^[0-9A-F]+$", QRegularExpression::CaseInsensitiveOption);
    static inline const QRegularExpression nonHexOnly = QRegularExpression("[^a-fA-F0-9 -]", QRegularExpression::CaseInsensitiveOption);
    static inline const QRegularExpression numbersOnly = QRegularExpression("\\d*", QRegularExpression::CaseInsensitiveOption); // a digit (\d)
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
    static QString fromByteArrayHex(QByteArray data);
    static QString fromByteArrayHex(QByteArray data, QChar separator, Endian::Endianness endianness);
    static QString stripToHexOnly(QString string);
};

}
#endif // QX_H
