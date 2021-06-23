#ifndef QXWINDOWS_H
#define QXWINDOWS_H
#include "Windows.h"
#include "qx.h"
#include <QString>

namespace Qx
{

//-Structs---------------------------------------------------------------------------------------------------------------
struct ShortcutProperties
{
    enum ShowMode {
        NORMAL = SW_SHOWNORMAL,
        MAXIMIZED = SW_SHOWMAXIMIZED,
        MINIMIZED = SW_SHOWMINIMIZED
    };

    QString target;
    QString targetArgs;
    QString startIn;
    QString comment;
    QString iconFilePath;
    int iconIndex = 0;
    ShowMode showMode = NORMAL;
};

//-Classes---------------------------------------------------------------------------------------------------------------
class FileDetails
{
//-Friends---------------------------------------------------------------------------------------------------------------
friend FileDetails getFileDetails(QString filePath);

//-Inner Enums-----------------------------------------------------------------------------------------------------------
public:
    enum FileFlag {
        FF_NONE = 0x00,
        FF_DEBUG = 0x01,
        FF_INFOINFERRED = 0x02,
        FF_PATCHED = 0x04,
        FF_PRERELEASE = 0x08,
        FF_PRIVATEBUILD = 0x10,
        FF_SPECIALBUILD = 0x20
    };
    Q_DECLARE_FLAGS(FileFlags, FileFlag)

    enum TargetSystem {
        TS_NONE = 0x000,
        TS_DOS = 0x001,
        TS_NT = 0x002,
        TS_WINDOWS16 = 0x004,
        TS_WINDOWS32 = 0x008,
        TS_OS216 = 0x010,
        TS_OS232 = 0x020,
        TS_PM16 = 0x040,
        TS_PM32 = 0x080,
        TS_UNK = 0x100,
        TS_DOS_WINDOWS16 = TS_DOS | TS_WINDOWS16,
        TS_DOS_WINDOWS32 = TS_DOS | TS_WINDOWS32,
        TS_NT_WINDOWS32 = TS_NT | TS_WINDOWS32,
        TS_OS216_PM16 = TS_OS216 | TS_PM16,
        TS_OS232_PM32 = TS_OS232 | TS_PM32
    };
    Q_DECLARE_FLAGS(TargetSystems, TargetSystem)

    enum FileType {
        FT_NONE = 0x00,
        FT_APP = 0x01,
        FT_DLL = 0x02,
        FT_DRV = 0x04,
        FT_FONT = 0x08,
        FT_STATIC_LIB = 0x10,
        FT_VXD = 0x20,
        FT_UNK = 0x40
    };
    Q_DECLARE_FLAGS(FileTypes, FileType)

    enum FileSubType {
        FST_NONE = 0x00,
        FST_DRV_COMM = 0x01,
        FST_DRV_DISPLAY = 0x02,
        FST_DRV_INSTALLABLE = 0x04,
        FST_DRV_KEYBOARD = 0x08,
        FST_DRV_LANGUAGE = 0x10,
        FST_DRV_MOUSE = 0x20,
        FST_DRV_NETWORK = 0x40,
        FST_DRV_PRINTER = 0x80,
        FST_DRV_SOUND = 0x100,
        FST_DRV_SYSTEM = 0x200,
        FST_DRV_VER_PRINTER = 0x400,
        FST_FONT_RASTER = 0x800,
        FST_FONT_TRUETYPE = 0x1000,
        FST_FONT_VECTOR = 0x2000,
        FST_VXD_ID = 0x4000,
        FST_UNK = 0x8000
    };
    Q_DECLARE_FLAGS(FileSubTypes, FileSubType)

//-Inner Structs---------------------------------------------------------------------------------------------------------
public:
    struct StringTable
    {
        QString metaLanguageID;
        QString metaCodePageID;
        QString comments;
        QString companyName;
        QString fileDescription;
        QString fileVersion;
        QString internalName;
        QString legalCopyright;
        QString legalTrademarks;
        QString originalFilename;
        QString productName;
        QString productVersion;
        QString privateBuild;
        QString specialBuild;
    };

//-Class Members----------------------------------------------------------------------------------------------------
private:
    static inline const QString LANG_CODE_PAGE_QUERY = "\\VarFileInfo\\Translation";
    static inline const QString SUB_BLOCK_BASE_TEMPLATE = "\\StringFileInfo\\%1%2\\";
    static inline const QString ST_COMMENTS_QUERY = "Comments";
    static inline const QString ST_COMPANY_NAME_QUERY = "CompanyName";
    static inline const QString ST_FILE_DESCRIPTION_QUERY = "FileDescription";
    static inline const QString ST_FILE_VERSION_QUERY = "FileVersion";
    static inline const QString ST_INTERNAL_NAME_QUERY = "InternalName";
    static inline const QString ST_LEGAL_COPYRIGHT_QUERY = "LegalCopyright";
    static inline const QString ST_LEGAL_TRADEMARKS_QUERY = "LegalTrademarks";
    static inline const QString ST_ORIGINAL_FILENAME_QUERY = "OriginalFilename";
    static inline const QString ST_PRODUCT_NAME_QUERY = "ProductName";
    static inline const QString ST_PRODUCT_VERSION_QUERY = "ProductVersion";
    static inline const QString ST_PRIVATE_BUILD_QUERY = "PrivateBuild";
    static inline const QString ST_SPECIAL_BUILD_QUERY = "SpecialBuild";

    static inline const QHash<DWORD, FileFlag> FILE_FLAG_MAP{{VS_FF_DEBUG, FF_DEBUG},
                                                             {VS_FF_INFOINFERRED, FF_INFOINFERRED},
                                                             {VS_FF_PATCHED, FF_PATCHED},
                                                             {VS_FF_PRERELEASE, FF_PRERELEASE},
                                                             {VS_FF_PRIVATEBUILD, FF_PRIVATEBUILD},
                                                             {VS_FF_SPECIALBUILD, FF_SPECIALBUILD}};

    static inline const QHash<DWORD, TargetSystem> TARGET_SYSTEM_MAP{{VOS_DOS, TS_DOS},
                                                                     {VOS_NT, TS_NT},
                                                                     {VOS__WINDOWS16, TS_WINDOWS16},
                                                                     {VOS__WINDOWS32, TS_WINDOWS32},
                                                                     {VOS_OS216, TS_OS216},
                                                                     {VOS_OS232, TS_OS232},
                                                                     {VOS__PM16, TS_PM16},
                                                                     {VOS__PM32, TS_PM32},
                                                                     {VOS_UNKNOWN, TS_UNK}};

    static inline const QHash<DWORD, FileType> FILE_TYPE_MAP{{VFT_APP, FT_APP},
                                                             {VFT_DLL, FT_DLL},
                                                             {VFT_DRV, FT_DRV},
                                                             {VFT_FONT, FT_FONT},
                                                             {VFT_STATIC_LIB, FT_STATIC_LIB},
                                                             {VFT_UNKNOWN, FT_VXD},
                                                             {VFT_VXD, FT_UNK}};

    static inline const QHash<QPair<DWORD, DWORD>, FileSubType> FILE_SUB_TYPE_MAP{{qMakePair(VFT_DRV, VFT2_DRV_COMM), FST_DRV_COMM},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_DISPLAY), FST_DRV_DISPLAY},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_INSTALLABLE), FST_DRV_INSTALLABLE},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_KEYBOARD), FST_DRV_KEYBOARD},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_LANGUAGE), FST_DRV_LANGUAGE},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_MOUSE), FST_DRV_MOUSE},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_NETWORK), FST_DRV_NETWORK},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_PRINTER), FST_DRV_PRINTER},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_SOUND), FST_DRV_SOUND},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_SYSTEM), FST_DRV_SYSTEM},
                                                                                  {qMakePair(VFT_DRV, VFT2_DRV_VERSIONED_PRINTER), FST_DRV_VER_PRINTER},
                                                                                  {qMakePair(VFT_DRV, VFT2_UNKNOWN), FST_UNK},
                                                                                  {qMakePair(VFT_FONT, VFT2_FONT_RASTER), FST_FONT_RASTER},
                                                                                  {qMakePair(VFT_FONT, VFT2_FONT_TRUETYPE), FST_FONT_TRUETYPE},
                                                                                  {qMakePair(VFT_FONT, VFT2_FONT_VECTOR), FST_FONT_VECTOR},
                                                                                  {qMakePair(VFT_FONT, VFT2_UNKNOWN), FST_UNK}};

//-Instance Members-------------------------------------------------------------------------------------------------
private:
    MMRB mMetaStructVersion;
    MMRB mFileVersion;
    MMRB mProductVersion;
    FileFlags mFileFlags = FF_NONE;
    TargetSystems mTargetSystems = TS_NONE;
    FileType mFileType = FT_NONE;
    FileSubType mFileSubType = FST_NONE;
    int mVirtualDeviceID = -1;
    QList<StringTable> mStringTables;
    QHash<QPair<QString, QString>, int> mLangCodePageMap;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    FileDetails();

//-Instance Functions----------------------------------------------------------------------------------------------
private:
    void addStringTable(StringTable stringTable);

public:
    bool isNull();
    int stringTableCount();
    QList<QPair<QString, QString>> availableLangCodePages();
    bool hasLangCodePage(QString lanuage, QString codePage);
    MMRB metaStructVersion();

    MMRB getFileVersion();
    MMRB getProductVersion();
    FileFlags getFileFlags();
    TargetSystems getTargetSystems();
    FileType getFileType();
    FileSubType getFileSubType();
    int getVirtualDeviceID();
    const StringTable getStringTable(int index = 0);
    const StringTable getStringTable(QString language, QString codePage);
};

//-Functions-------------------------------------------------------------------------------------------------------------

// Files
FileDetails getFileDetails(QString filePath);

// Processes
DWORD getProcessIDByName(QString processName);
QString getProcessNameByID(DWORD processID);
bool processIsRunning(QString processName);
bool processIsRunning(DWORD processID);
bool enforceSingleInstance();

// Error codes
Qx::GenericError translateHresult(HRESULT res);
Qx::GenericError translateNtstatus(NTSTATUS stat);

// Filesystem
Qx::GenericError createShortcut(QString shortcutPath, ShortcutProperties sp);

}

//-Qt Flag Operators-----------------------------------------------------------------------------------------------------
Q_DECLARE_OPERATORS_FOR_FLAGS(Qx::FileDetails::FileFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Qx::FileDetails::TargetSystems)
Q_DECLARE_OPERATORS_FOR_FLAGS(Qx::FileDetails::FileTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Qx::FileDetails::FileSubTypes)

#endif // QXWINDOWS_H
