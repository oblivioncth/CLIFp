// Unit Includes
#include "fp-json.h"

// Qt Includes
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Qx Includes
#include <qx/core/qx-json.h>
#include <qx/io/qx-common-io.h>

namespace Fp
{

//===============================================================================================================
// JSON::START_STOP
//===============================================================================================================

//-Operators----------------------------------------------------------------------------------------------------
//Public:
bool operator== (const Json::StartStop& lhs, const Json::StartStop& rhs) noexcept
{
    return lhs.path == rhs.path && lhs.filename == rhs.filename && lhs.arguments == rhs.arguments;
}

//-Hashing------------------------------------------------------------------------------------------------------
size_t qHash(const Json::StartStop& key, size_t seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.path);
    seed = hash(seed, key.filename);
    seed = hash(seed, key.arguments);

    return seed;
}

//===============================================================================================================
// JSON::SETTINGS_READER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Json::SettingsReader::SettingsReader(Settings* targetSettings, std::shared_ptr<QFile> sourceJsonFile) :
    mTargetSettings(targetSettings),
    mSourceJsonFile(sourceJsonFile)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError Json::SettingsReader::readInto()
{
    // Load original JSON file
    QByteArray settingsData;
    Qx::IoOpReport settingsLoadReport = Qx::readBytesFromFile(settingsData, *mSourceJsonFile);

    if(!settingsLoadReport.wasSuccessful())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), settingsLoadReport.outcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument settingsDocument = QJsonDocument::fromJson(settingsData, &parseError);

    if(settingsDocument.isNull())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), parseError.errorString());
    else
    {
        // Ensure top level container is object
        if(!settingsDocument.isObject())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        return parseDocument(settingsDocument);
    }
}

//===============================================================================================================
// JSON::CONFIG_READER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Json::ConfigReader::ConfigReader(Config* targetConfig, std::shared_ptr<QFile> sourceJsonFile) :
    SettingsReader(targetConfig, sourceJsonFile)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Json::ConfigReader::parseDocument(const QJsonDocument& configDoc)
{
    // Get derivation specific target
    Config* targetConfig = static_cast<Config*>(mTargetSettings);

    // Get values
    Qx::GenericError valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(targetConfig->flashpointPath, configDoc.object(), Object_Config::KEY_FLASHPOINT_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetConfig->startServer, configDoc.object(), Object_Config::KEY_START_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(targetConfig->server, configDoc.object(), Object_Config::KEY_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Return invalid error on success
    return Qx::GenericError();

}

//===============================================================================================================
// JSON::PREFERENCES_READER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Json::PreferencesReader::PreferencesReader(Preferences* targetPreferences, std::shared_ptr<QFile> sourceJsonFile) :
    SettingsReader(targetPreferences, sourceJsonFile)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Json::PreferencesReader::parseDocument(const QJsonDocument &prefDoc)
{
    // Get derivation specific target
    Preferences* targetPreferences = static_cast<Preferences*>(mTargetSettings);

    // Get values
    Qx::GenericError valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->imageFolderPath, prefDoc.object(), Object_Preferences::KEY_IMAGE_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->jsonFolderPath, prefDoc.object(), Object_Preferences::KEY_JSON_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->dataPacksFolderPath, prefDoc.object(), Object_Preferences::KEY_DATA_PACKS_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->onDemandImages, prefDoc.object(), Object_Preferences::KEY_ON_DEMAND_IMAGES)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->onDemandBaseUrl, prefDoc.object(), Object_Preferences::KEY_ON_DEMAND_BASE_URL)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    // Return invalid error on success
    return Qx::GenericError();
}

//===============================================================================================================
// JSON::SERVICES_READER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Json::ServicesReader::ServicesReader(Services* targetServices, std::shared_ptr<QFile> sourceJsonFile, const MacroResolver* macroResolver) :
    SettingsReader(targetServices, sourceJsonFile),
    mHostMacroResolver(macroResolver)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Json::ServicesReader::parseDocument(const QJsonDocument &servicesDoc)
{
    // Get derivation specific target
    Services* targetServices = static_cast<Services*>(mTargetSettings);

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get watches
    // TODO: include logs

    // Get servers
    QJsonArray jaServers;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaServers, servicesDoc.object(), Object_Services::KEY_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse servers
    for(const QJsonValue& jvServer : qAsConst(jaServers))
    {
        ServerDaemon serverBuffer;
        if((valueError = parseServerDaemon(serverBuffer, jvServer)).isValid())
            return valueError;

        targetServices->servers.insert(serverBuffer.name, serverBuffer);
    }

    // Get daemons
    QJsonArray jaDaemons;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaDaemons, servicesDoc.object(), Object_Services::KEY_DAEMON)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse daemons
    for(const QJsonValue& jvDaemon : qAsConst(jaDaemons))
    {
        ServerDaemon daemonBuffer;
        if((valueError = parseServerDaemon(daemonBuffer, jvDaemon)).isValid())
            return valueError;

        targetServices->daemons.insert(daemonBuffer.name, daemonBuffer);
    }

    // Get starts
    QJsonArray jaStarts;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStarts, servicesDoc.object(), Object_Services::KEY_START)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse starts
    for(const QJsonValue& jvStart : qAsConst(jaStarts))
    {
        StartStop startStopBuffer;
        if((valueError = parseStartStop(startStopBuffer, jvStart)).isValid())
            return valueError;

        targetServices->starts.insert(startStopBuffer);
    }

    // Get stops
    QJsonArray jaStops;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStops, servicesDoc.object(), Object_Services::KEY_STOP)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse starts
    for(const QJsonValue& jvStop : qAsConst(jaStops))
    {
        StartStop startStopBuffer;
        if((valueError = parseStartStop(startStopBuffer, jvStop)).isValid())
            return valueError;

        targetServices->stops.insert(startStopBuffer);
    }

    // Return invalid error on success
    return Qx::GenericError();
}

Qx::GenericError Json::ServicesReader::parseServerDaemon(ServerDaemon& serverBuffer, const QJsonValue& jvServer)
{
    // Ensure array element is Object
    if(!jvServer.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get server Object
    QJsonObject joServer = jvServer.toObject();

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get direct values
    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.name, joServer, Object_Server::KEY_NAME)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.path, joServer, Object_Server::KEY_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.filename, joServer, Object_Server::KEY_FILENAME)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.kill, joServer, Object_Server::KEY_KILL)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Get arguments
    QJsonArray jaArgs;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaArgs, joServer, Object_Server::KEY_ARGUMENTS)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    for(const QJsonValue& jvArg : qAsConst(jaArgs))
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        serverBuffer.arguments.append(jvArg.toString());
    }

    // Ensure filename has extension (assuming exe)
    if(QFileInfo(serverBuffer.filename).suffix().isEmpty())
        serverBuffer.filename += ".exe";

    // Resolve macros for relevant variables
    serverBuffer.path = mHostMacroResolver->resolve(serverBuffer.path);
    for(QString& arg : serverBuffer.arguments)
        arg = mHostMacroResolver->resolve(arg);

    // Return invalid error on success
    return Qx::GenericError();
}

Qx::GenericError Json::ServicesReader::parseStartStop(StartStop& startStopBuffer, const QJsonValue& jvStartStop)
{
    // Ensure return buffer is null
    startStopBuffer = StartStop();

    // Ensure array element is Object
    if(!jvStartStop.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get server Object
    QJsonObject joStartStop = jvStartStop.toObject();

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get direct values
    if((valueError = Qx::Json::checkedKeyRetrieval(startStopBuffer.path, joStartStop , Object_StartStop::KEY_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(startStopBuffer.filename, joStartStop, Object_StartStop::KEY_FILENAME)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Get arguments
    QJsonArray jaArgs;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaArgs, joStartStop, Object_StartStop::KEY_ARGUMENTS)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    for(const QJsonValue& jvArg : qAsConst(jaArgs))
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        startStopBuffer.arguments.append(jvArg.toString());
    }

    // Ensure filename has extension (assuming exe)
    if(QFileInfo(startStopBuffer.filename).suffix().isEmpty())
        startStopBuffer.filename += ".exe";

    // Resolve macros for relevant variables
    startStopBuffer.path = mHostMacroResolver->resolve(startStopBuffer.path);
    for(QString& arg : startStopBuffer.arguments)
        arg = mHostMacroResolver->resolve(arg);

    // Return invalid error on success
    return Qx::GenericError();
}
}
