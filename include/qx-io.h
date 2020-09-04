#ifndef QXIO_H
#define QXIO_H

#include <QString>
#include <QList>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QTextStream>
#include <QDirIterator>

namespace Qx
{

//-Types------------------------------------------------------------------------------------------------------

enum IOOpType { IO_OP_READ, IO_OP_WRITE, IO_OP_ENUMERATE, IO_OP_INSPECT };
enum IOOpResultType { IO_SUCCESS, IO_ERR_UNKNOWN, IO_ERR_ACCESS_DENIED, IO_ERR_NOT_A_FILE, IO_ERR_NOT_A_DIR, IO_ERR_OUT_OF_RES,
                      IO_ERR_READ, IO_ERR_WRITE, IO_ERR_FATAL, IO_ERR_OPEN, IO_ERR_ABORT,
                      IO_ERR_TIMEOUT, IO_ERR_REMOVE, IO_ERR_RENAME, IO_ERR_REPOSITION,
                      IO_ERR_RESIZE, IO_ERR_COPY, IO_ERR_FILE_DNE, IO_ERR_DIR_DNE,
                      IO_ERR_FILE_EXISTS, IO_ERR_CANT_MAKE_DIR, IO_ERR_FILE_SIZE_MISMATCH, IO_ERR_CURSOR_OOB};
enum IOOpTargetType { IO_FILE, IO_DIR };

//-Classes--------------------------------------------------------------------------------------------
class IOOpReport
{
//-Class Members----------------------------------------------------------------------------------------------------
public:
    static const inline QStringList TARGET_TYPES  = {"file", "directory"};
    static const inline QString SUCCESS_TEMPLATE = R"(Succesfully %1 %2 "%3")";
    static const inline QString ERROR_TEMPLATE = R"(Error while %1 %2 "%3")";
    static const inline QStringList SUCCESS_VERBS = {"read", "wrote", "enumerated", "inspected"};
    static const inline QStringList ERROR_VERBS = {"reading", "writing", "enumerating", "inspecting"};
    static const inline QStringList ERROR_INFO = {"An unknown error has occured.", "Access denied.", "Target is not a file.", "Target is not a directory.", "Out of resources.",
                                                  "General read error.", "General write error.", "A fatal error has occured.", "Could not open file.", "The opperation was aborted.",
                                                  "Request timed out.", "The file could not be removed.", "The file could not be renamed.", "The file could not be moved.",
                                                  "The file could not be resized.", "The file could not be copied.", "File does not exist.", "Directory does not exist.",
                                                  "The file already exists.", "The directory could not be created.", "File size mismatch.", "File data cursor has gone out of bounds."};

//-Instance Members-------------------------------------------------------------------------------------------------
private:
    IOOpType mOperation;
    IOOpResultType mResult;
    IOOpTargetType mTargetType;
    QString mTarget = QString();
    QString mOutcome = QString();
    QString mOutcomeInfo = QString();
//-Constructor-------------------------------------------------------------------------------------------------------
public:
    IOOpReport();
    IOOpReport(IOOpType op, IOOpResultType res, QFile& tar);
    IOOpReport(IOOpType op, IOOpResultType res, QDir& tar);

//-Instance Functions----------------------------------------------------------------------------------------------
public:
    IOOpType getOperation() const;
    IOOpResultType getResult() const;
    IOOpTargetType getTargetType() const;
    QString getTarget() const;
    QString getOutcome() const;
    QString getOutcomeInfo() const;
    bool wasSuccessful() const;
private:
    void parseOutcome();
};

class TextPos
{
//-Class Types------------------------------------------------------------------------------------------------------
public:
    static const TextPos START;
    static const TextPos END;
//-Instance Variables------------------------------------------------------------------------------------------------
private:
    int mLineNum;
    int mCharNum;
//-Constructor-------------------------------------------------------------------------------------------------------
public:
    TextPos();
    TextPos(int lineNum, int charNum);

//-Instance Functions------------------------------------------------------------------------------------------------
public:
    int getLineNum();
    int getCharNum();
    void setLineNum(int lineNum);
    void setCharNum(int charNum);
    void setNull();
    bool isNull();

    bool operator== (const TextPos &otherTextPos);
    bool operator!= (const TextPos &otherTextPos);
    bool operator> (const TextPos &otherTextPos);
    bool operator>= (const TextPos &otherTextPos);
    bool operator< (const TextPos &otherTextPos);
    bool operator<= (const TextPos &otherTextPos);
};

//-Variables------------------------------------------------------------------------------------------------------------

static inline QString ENDL = "\r\n"; //NOTE: Currently this is windows only

//-Functions-------------------------------------------------------------------------------------------------------------

// General:
    bool fileIsEmpty(QFile& file);
    bool fileIsEmpty(QFile& file, IOOpReport& reportBuffer);
    QString kosherizeFileName(QString fileName);
// Text Based:
    IOOpReport getLineCountOfFile(long long& returnBuffer, QFile& textFile);
    IOOpReport findStringInFile(TextPos& returnBuffer, QFile& textFile, const QString& query, int hitsToSkip = 0, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);
    IOOpReport readTextFromFile(QString& returnBuffer, QFile& textFile, TextPos textPos, int characters = -1);
    IOOpReport readTextRangeFromFile(QString& returnBuffer, QFile& textFile, TextPos startPos, TextPos endPos = TextPos::END);
    IOOpReport readTextFromFileByLine(QStringList& returnBuffer, QFile &textFile, int startLine = 0, int endLine = -1);
    IOOpReport readAllTextFromFile(QString& returnBuffer, QFile& textFile);
    IOOpReport writeStringAsFile(QFile &textFile, const QString& text, bool overwriteIfExist = false, bool createDirs = true);
    IOOpReport writeStringToEndOfFile(QFile &textFile, const QString& text, bool ensureNewLine = false, bool createIfDNE = false, bool createDirs = true); // Consider making function just writeStringToFile and use TextPos with bool for overwrite vs insert
    IOOpReport deleteTextRangeFromFile(QFile &textFile, TextPos startPos, TextPos endPos);
// Directory Based:
    IOOpReport getDirFileList(QStringList& returnBuffer, QDir directory, QDirIterator::IteratorFlag traversalFlags = QDirIterator::NoIteratorFlags, QStringList extFilter = QStringList(),
                              bool leadingDotSensitive = true, Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);
    bool dirContainsFiles(QDir directory, bool includeSubdirectories = false);
    bool dirContainsFiles(QDir directory, IOOpReport& reportBuffer, bool includeSubdirectories = false);
// Integrity Based
    IOOpReport calculateFileChecksum(QByteArray& returnBuffer, QFile& file, QCryptographicHash::Algorithm hashAlgorithm);
// Raw Based
    IOOpReport readAllBytesFromFile(QByteArray& returnBuffer, QFile &file);
    IOOpReport readBytesFromFile(QByteArray& returnBuffer, QFile &file, long long start, long long end = -1);
    IOOpReport writeBytesAsFile(QFile &file, const QByteArray &byteArray, bool overwriteIfExist = false, bool createDirs = true);
}

#endif // IO_H
