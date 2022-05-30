#ifndef FLASHPOINT_JSON_H
#define FLASHPOINT_JSON_H

// Qt Includes
#include <QString>
#include <QSet>
#include <QFile>

// Qx Includes
#include <qx/core/qx-genericerror.h>

// Project Includes
#include "fp-macro.h"

namespace Fp
{

class Json
{
//-Inner Classes-------------------------------------------------------------------------------------------------
public:
    class Object_Config
    {
    public:
        static inline const QString KEY_FLASHPOINT_PATH = "flashpointPath"; // Reading this value is current redundant and unused, but this may change in the future
        static inline const QString KEY_START_SERVER = "startServer";
        static inline const QString KEY_SERVER = "server";
    };

    class Object_Preferences
    {
    public:
        static inline const QString KEY_IMAGE_FOLDER_PATH = "imageFolderPath";
        static inline const QString KEY_JSON_FOLDER_PATH = "jsonFolderPath";
        static inline const QString KEY_DATA_PACKS_FOLDER_PATH = "dataPacksFolderPath";
        static inline const QString KEY_ON_DEMAND_IMAGES = "onDemandImages";
        static inline const QString KEY_ON_DEMAND_BASE_URL = "onDemandBaseUrl";
    };

    class Object_Server
    {
    public:
        static inline const QString KEY_NAME = "name";
        static inline const QString KEY_PATH = "path";
        static inline const QString KEY_FILENAME = "filename";
        static inline const QString KEY_ARGUMENTS = "arguments";
        static inline const QString KEY_KILL = "kill";
    };

    class Object_StartStop
    {
    public:
        static inline const QString KEY_PATH = "path";
        static inline const QString KEY_FILENAME = "filename";
        static inline const QString KEY_ARGUMENTS = "arguments";
    };

    class Object_Daemon
    {
    public:
        static inline const QString KEY_NAME = "name";
        static inline const QString KEY_PATH = "path";
        static inline const QString KEY_FILENAME = "filename";
        static inline const QString KEY_ARGUMENTS = "arguments";
        static inline const QString KEY_KILL = "kill";
    };

    class Object_Services
    {
    public:
        static inline const QString KEY_WATCH = "watch";
        static inline const QString KEY_SERVER = "server";
        static inline const QString KEY_DAEMON = "daemon";
        static inline const QString KEY_START = "start";
        static inline const QString KEY_STOP = "stop";
    };

    struct ServerDaemon
    {
        QString name;
        QString path;
        QString filename;
        QStringList arguments;
        bool kill;
    };

    struct StartStop
    {
        QString path;
        QString filename;
        QStringList arguments;

        friend bool operator== (const StartStop& lhs, const StartStop& rhs) noexcept;
        friend size_t qHash(const StartStop& key, size_t seed) noexcept;
    };

    struct Settings {};

    struct Config : public Settings
    {
        QString flashpointPath;
        bool startServer;
        QString server;
    };

    struct Preferences : public Settings
    {
        QString imageFolderPath;
        QString jsonFolderPath;
        QString dataPacksFolderPath;
        bool onDemandImages;
        QString onDemandBaseUrl;
    };

    struct Services : public Settings
    {
        //QSet<Watch> watches;
        QHash<QString, ServerDaemon> servers;
        QHash<QString, ServerDaemon> daemons;
        QSet<StartStop> starts;
        QSet<StartStop> stops;
    };

    class SettingsReader
    {
    //-Class variables-----------------------------------------------------------------------------------------------------
    public:
        static inline const QString ERR_PARSING_JSON_DOC = "Error parsing JSON Document: %1";
        static inline const QString ERR_JSON_UNEXP_FORMAT = "Unexpected document format";

    //-Instance Variables--------------------------------------------------------------------------------------------------
    protected:
        Settings* mTargetSettings;
        std::shared_ptr<QFile> mSourceJsonFile;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        SettingsReader(Settings* targetSettings, std::shared_ptr<QFile> sourceJsonFile);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        virtual Qx::GenericError parseDocument(const QJsonDocument& jsonDoc) = 0;

    public:
        Qx::GenericError readInto();

    };

    class ConfigReader : public SettingsReader
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        ConfigReader(Config* targetConfig, std::shared_ptr<QFile> sourceJsonFile);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        Qx::GenericError parseDocument(const QJsonDocument& configDoc);
    };

    class PreferencesReader : public SettingsReader
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        PreferencesReader(Preferences* targetPreferences, std::shared_ptr<QFile> sourceJsonFile);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        Qx::GenericError parseDocument(const QJsonDocument& prefDoc);
    };

    class ServicesReader : public SettingsReader
    {
    //-Instance Variables--------------------------------------------------------------------------------------------------
    private:
        const MacroResolver* mHostMacroResolver;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        ServicesReader(Services* targetServices, std::shared_ptr<QFile> sourceJsonFile, const MacroResolver* macroResolver);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        Qx::GenericError parseDocument(const QJsonDocument& servicesDoc);
        Qx::GenericError parseServerDaemon(ServerDaemon& serverBuffer, const QJsonValue& jvServer);
        Qx::GenericError parseStartStop(StartStop& startStopBuffer, const QJsonValue& jvStartStop);
    };
};
}

#endif // FLASHPOINT_JSON_H
