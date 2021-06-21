#ifndef QXIO_H
#define QXIO_H

#include <QString>
#include <QList>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QTextStream>
#include <QDataStream>
#include <QDirIterator>

namespace Qx
{

//-Types------------------------------------------------------------------------------------------------------
enum IOOpType { IO_OP_READ, IO_OP_WRITE, IO_OP_ENUMERATE, IO_OP_INSPECT };
enum IOOpResultType { IO_SUCCESS, IO_ERR_UNKNOWN, IO_ERR_ACCESS_DENIED, IO_ERR_NOT_A_FILE, IO_ERR_NOT_A_DIR, IO_ERR_OUT_OF_RES,
                      IO_ERR_READ, IO_ERR_WRITE, IO_ERR_FATAL, IO_ERR_OPEN, IO_ERR_ABORT,
                      IO_ERR_TIMEOUT, IO_ERR_REMOVE, IO_ERR_RENAME, IO_ERR_REPOSITION,
                      IO_ERR_RESIZE, IO_ERR_COPY, IO_ERR_FILE_DNE, IO_ERR_DIR_DNE,
                      IO_ERR_FILE_EXISTS, IO_ERR_CANT_MAKE_DIR, IO_ERR_FILE_SIZE_MISMATCH, IO_ERR_CURSOR_OOB,
                      IO_ERR_FILE_NOT_OPEN};
enum IOOpTargetType { IO_FILE, IO_DIR };
enum WriteMode { Append, Overwrite, NewOnly, ExistingOnly }; // TODO - Modify all classes in this unit to use this instead of overwrite bool

//-Classes--------------------------------------------------------------------------------------------
class IOOpReport
{
//-Class Members----------------------------------------------------------------------------------------------------
public:
    static const inline QStringList TARGET_TYPES  = {"file", "directory"};
    static const inline QString SUCCESS_TEMPLATE = R"(Succesfully %1 %2 "%3")";
    static const inline QString ERROR_TEMPLATE = R"(Error while %1 %2 "%3")";
    static const inline QHash<IOOpType, QString> SUCCESS_VERBS = {
        {IO_OP_READ, "read"},
        {IO_OP_WRITE, "wrote"},
        {IO_OP_ENUMERATE, "enumerated"},
        {IO_OP_INSPECT, "inspected"}
    };
    static const inline QHash<IOOpType, QString> ERROR_VERBS = {
        {IO_OP_READ, "reading"},
        {IO_OP_WRITE, "writing"},
        {IO_OP_ENUMERATE, "enumerating"},
        {IO_OP_INSPECT, "inspecting"}
    };
    static const inline QHash<IOOpResultType, QString> ERROR_INFO = {
        {IO_ERR_UNKNOWN, "An unknown error has occured."},
        {IO_ERR_ACCESS_DENIED, "Access denied."},
        {IO_ERR_NOT_A_FILE, "Target is not a file."},
        {IO_ERR_NOT_A_DIR, "Target is not a directory."},
        {IO_ERR_OUT_OF_RES, "Out of resources."},
        {IO_ERR_READ, "General read error."},
        {IO_ERR_WRITE, "General write error."},
        {IO_ERR_FATAL, "A fatal error has occured."},
        {IO_ERR_OPEN, "Could not open file."},
        {IO_ERR_ABORT, "The opperation was aborted."},
        {IO_ERR_TIMEOUT, "Request timed out."},
        {IO_ERR_REMOVE, "The file could not be removed."},
        {IO_ERR_RENAME, "The file could not be renamed."},
        {IO_ERR_REPOSITION, "The file could not be moved."},
        {IO_ERR_RESIZE, "The file could not be resized."},
        {IO_ERR_COPY, "The file could not be copied."},
        {IO_ERR_FILE_DNE, "File does not exist."},
        {IO_ERR_DIR_DNE, "Directory does not exist."},
        {IO_ERR_FILE_EXISTS, "The file already exists."},
        {IO_ERR_CANT_MAKE_DIR, "The directory could not be created."},
        {IO_ERR_FILE_SIZE_MISMATCH, "File size mismatch."},
        {IO_ERR_CURSOR_OOB, "File data cursor has gone out of bounds."},
        {IO_ERR_FILE_NOT_OPEN, "The file is not open."}
    };

//-Instance Members-------------------------------------------------------------------------------------------------
private:
    bool mNull;
    IOOpType mOperation;
    IOOpResultType mResult;
    IOOpTargetType mTargetType;
    QString mTarget = QString();
    QString mOutcome = QString();
    QString mOutcomeInfo = QString();

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    IOOpReport();
    IOOpReport(IOOpType op, IOOpResultType res, const QFile& tar);
    IOOpReport(IOOpType op, IOOpResultType res, const QDir& tar);

//-Instance Functions----------------------------------------------------------------------------------------------
private:
    void parseOutcome();

public:
    IOOpType getOperation() const;
    IOOpResultType getResult() const;
    IOOpTargetType getTargetType() const;
    QString getTarget() const;
    QString getOutcome() const;
    QString getOutcomeInfo() const;
    bool wasSuccessful() const;
    bool isNull() const;
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
    int getLineNum() const;
    int getCharNum() const;
    void setLineNum(int lineNum);
    void setCharNum(int charNum);
    void setNull();
    bool isNull() const;

    bool operator== (const TextPos &otherTextPos);
    bool operator!= (const TextPos &otherTextPos);
    bool operator> (const TextPos &otherTextPos);
    bool operator>= (const TextPos &otherTextPos);
    bool operator< (const TextPos &otherTextPos);
    bool operator<= (const TextPos &otherTextPos);
};

class FileStreamWriter
{
//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QDataStream mStreamWriter;
    QFile& mTargetFile;
    WriteMode mWriteMode;
    bool mCreateDirs;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    FileStreamWriter(QFile& file, WriteMode writeMode = Append, bool createDirs = true);

//-Instance Functions------------------------------------------------------------------------------------------------
public:
    IOOpReport openFile();
    IOOpReport writeData(QByteArray data);
    void closeFile();
};

class TextStreamWriter
{
//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QTextStream mStreamWriter;
    QFile& mTargetFile;
    WriteMode mWriteMode;
    bool mCreateDirs;
    bool mBuffered;
    bool mAtLineStart;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    TextStreamWriter(QFile& file, WriteMode writeMode = Append, bool createDirs = true, bool buffered = true);

//-Instance Functions------------------------------------------------------------------------------------------------
public:
    IOOpReport openFile();
    IOOpReport writeLine(QString line, bool ensureLineStart = true);
    IOOpReport writeText(QString text);
    void closeFile();
};


//-Variables------------------------------------------------------------------------------------------------------------
    const QString ENDL = "\n"; //Auto cross platform thanks to QIODevice::OpenMode Text
    const QString LIST_ITM_PRFX = "- ";

    const QHash<WriteMode, QIODevice::OpenMode> WRITE_OPEN_FLAGS_MAP = {
        {Append, QIODevice::Append},
        {Overwrite, QIODevice::Truncate},
        {NewOnly, QIODevice::NewOnly},
        {ExistingOnly, QIODevice::ExistingOnly | QIODevice::Append}
    };

//-Functions-------------------------------------------------------------------------------------------------------------
// General:
    bool fileIsEmpty(const QFile& file);
    bool fileIsEmpty(const QFile& file, IOOpReport& reportBuffer);
    QString kosherizeFileName(QString fileName);

// Text Based:
    IOOpReport getLineCountOfFile(long long& returnBuffer, QFile& textFile);
    IOOpReport findStringInFile(TextPos& returnBuffer, QFile& textFile, const QString& query, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive, int hitsToSkip = 0 );
    IOOpReport findStringInFile(QList<TextPos>& returnBuffer, QFile& textFile, const QString& query, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive, int hitLimit = -1);
    IOOpReport fileContainsString(bool& returnBuffer, QFile& textFile, const QString& query, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);
    IOOpReport readTextFromFile(QString& returnBuffer, QFile& textFile, TextPos textPos, int characters = -1);
    IOOpReport readTextRangeFromFile(QString& returnBuffer, QFile& textFile, TextPos startPos, TextPos endPos = TextPos::END);
    IOOpReport readTextFromFileByLine(QStringList& returnBuffer, QFile &textFile, int startLine = 0, int endLine = -1);
    IOOpReport readAllTextFromFile(QString& returnBuffer, QFile& textFile);
    IOOpReport writeStringAsFile(QFile &textFile, const QString& text, bool overwriteIfExist = false, bool createDirs = true);
    IOOpReport writeStringToEndOfFile(QFile &textFile, const QString& text, bool ensureNewLine = false, bool createIfDNE = false, bool createDirs = true); // Consider making function just writeStringToFile and use TextPos with bool for overwrite vs insert
    IOOpReport deleteTextRangeFromFile(QFile &textFile, TextPos startPos, TextPos endPos);

// Directory Based:
    IOOpReport getDirFileList(QStringList& returnBuffer, QDir directory, QStringList extFilter = QStringList(), QDirIterator::IteratorFlag traversalFlags = QDirIterator::NoIteratorFlags,
                              Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);
    bool dirContainsFiles(QDir directory, bool includeSubdirectories = false);
    bool dirContainsFiles(QDir directory, IOOpReport& reportBuffer, bool includeSubdirectories = false);

// Integrity Based
    IOOpReport calculateFileChecksum(QString& returnBuffer, QFile& file, QCryptographicHash::Algorithm hashAlgorithm);
    IOOpReport fileMatchesChecksum(bool& returnBuffer, QFile& file, QString checksum, QCryptographicHash::Algorithm hashAlgorithm);

// Raw Based
    IOOpReport readAllBytesFromFile(QByteArray& returnBuffer, QFile &file);
    IOOpReport readBytesFromFile(QByteArray& returnBuffer, QFile &file, long long start, long long end = -1);
    IOOpReport writeBytesAsFile(QFile &file, const QByteArray &byteArray, bool overwriteIfExist = false, bool createDirs = true);
}

#endif // IO_H
