# CLIFp (Command-line Interface for Flashpoint)
CLIFp (pronounced "Cliff-P") is a command-line interface for [BlueMaxima's Flashpoint](https://bluemaxima.org/flashpoint/) project that allows you to start games/animations from within the collection via your system's command parser and contextual arguments. While it is a separate application, CLIFp functions closely to that of a native CLI by parsing the configuration of the Flashpoint install it is deployed into and therefore launches games/animation in the same manner as the standard GUI launcher.

Other than a few pop-up dialogs used for alerts and errors, CLIFp runs completely in the background so that the only windows seen during use are the same ones present while running standard Flashpoint. It automatically terminates once the target application has exited, requiring no manual tasks or clean-up by the user. 

## Compatability
### General
Since the 0.2 rewrite, the primary paradigm through which CLIFp functions has been significantly improved and should provide a perfect or near-perfect experience when compared to using the standard GUI method of launching games/animations. Additionally, the redesign is expected to be very resilient towards Flashpoint updates and will most likely only require compatibility patches when updates that make major changes are released.

All Flashpoint features are supported, other than editing your local database through the command-line. More specifically:

 - Games (with or without autorun-before additional apps)
 - Animations
 - Additional Apps (including messages)
 - Extras
 - Anything else I'm forgetting

While testing for complete compatibility is infeasible given the size of Flashpoint, CLIFp was designed with full compatibility in mind and theoretically is 100% compatible with the Flashpoint collection. Games are slightly prioritized when it comes to testing the application however.

### Version Matching
Each release of this application targets a specific version or versions of BlueMaxima's Flashpoint and while newer releases will sometimes contain general improvements to functionality, they will largely be created to match the changes made between each Flashpoint release and therefore maintain compatibility. These matches are shown below:
| CLIFp Version | Target Flashpoint Version |
|--|--|
| 0.1 | 8.1 ("Spirit of Adventure") |
| 0.1.1 | 8.2 ("Approaching Planet Nine") |
| 0.2 | 8.1, 8.2+ |

Using a version of CLIFp that does not target the version of Flashpoint you wish to use it with is highly discouraged as some features may not work correctly or at all and in some cases the utility may fail to function entirely; **however since 0.2 compatibility with newer versions is quite likely even if they aren't explicit listed yet** (usually because I haven't had time to check if an update is needed).

## Usage
### Target Usage
CLIFp  was primarily created for use with its sister project [OFILb (Obby's Flashpoint Importer for LaunchBox)](https://github.com/oblivioncth/OFILb) to facilitate the inclusion of Flashpoint in to LaunchBox user's collections. The operation of CLIFp is completely automated when used in this manner.

That being said, it is perfectly possible to use CLIFp with Flashpoint alone in any manner you see fit.

### General
While CLIFp has multiple functions, typical usage involves one of two main methods:

**Auto:**
This is the most straightforward and hassle free approach. Simply use the **--auto** or **-a** switch followed by the GUID/UUID of the game/animation you wish to start, as seen in the following example:
![Example usage](https://i.imgur.com/0vLToTi.png)

You can find a title's ID by right clicking on an entry in Flashpoint and selecting "Copy Game UUID". This switch also supports starting additional apps, though getting their UUID is more tricky as I currently know of no other way than opening [FP Install Dir]\Data\flashpoint.sqlite in a database browser and searching for the ID manually. I will look into if an easier way to obtain their IDs can be implemented.

If the ID belongs to a main game/animation entry, any additional apps that are marked as auto-run before will also be started ahead of time just as they would be when using the Flashpoint Launcher.

**App/Param:**
This method can only start one application at a time. Use the **-x/--exe** switch to specify the relative (from Flashpoint's directory) path to the application to launch and the **-p/--param** switch to provide the launch arguments that will be passed to the target application.

The applications and arguments that are used for each game/animation can be found within the Flashpoint database ([FP Install Dir]\Data\flashpoint.sqlite)

### All Switches/Arguments

 -  **-h | --help | -?:** Prints usage information
 -  **-v | --version:** Prints the current version of the tool
 -  **-x | --exe:** Relative (to Flashpoint Directory) of primary application to launch
 -  **-p | --param:** Command-line parameters to use when starting the primary application
 -  **-a | --auto:** Finds a game/additional-app by UUID and runs it if found, including run-before additional apps in the case of a game
 -  **-m | --msg:** Displays an pop-up dialog with the supplied message. Used primarily for some additional apps
 -  **-e | --extra"** Opens an explorer window to the specified extra. Used primarily for some additional apps

Use 'exe' and 'param' for normal operation, use 'auto' by itself for automatic operation, use 'msg' to display a popup message, use 'extra' to view an extra, or use 'help' and/or 'version' for information.

## Exit Codes
Once CLIFp has finished executing an exit code is reported that indicates the "error status" of the program, which can be useful for recording/determining issues. The exit code can be obtained by running the application in the following manner:

    start /wait CLIFp.exe [parameters]
    echo %errorlevel%

| Value | Code | Description |
|--|--|--|
| 0 | NO_ERR | The application completed successfully |
| 1 | INVALID_ARGS  | The arguments provided were not recognized or were formatted incorrectly |
| 2 | LAUNCHER_OPEN | The application could not start because the Flashpoint Launcher is currently open |
| 3 | INSTALL_INVALID | The Flashpoint install that CLIFp is deployed in is corrupted or not compatible with its current version  |
| 4 | CANT_PARSE_CONFIG | Failed to parse config.json |
| 5 | CANT_PARSE_SERVICES | Failed to parse services.json |
| 6 | CONFIG_SERVER_MISSING | The server entry specified in config.json was not found in services.json |
| 7 | AUTO_ID_NOT_VALID | The specified UUID for auto operation is not a valid 128-bit UUID |
| 8 | SQL_ERROR | An unexpected SQL error occured while reading flashpoint.sqlite |
| 9 | DB_MISSING_TABLES | flashpoint.sqlite is missing expected tables |
| 10 | DB_MISSING_COLUMNS | One or more tables in flashpoint.sqlite are missing expected columns |
| 11 | AUTO_NOT_FOUND | The specified UUID for auto operation is not associated with any title in the Flashpoint database |
| 12 | MORE_THAN_ONE_AUTO | The specified UUID for auto operation is associated with more than one title (possible collision) |
| 13 | EXTRA_NOT_FOUND | The specified or auto-determined extra was not found in the Extras folder  |
| 14 | EXECUTABLE_NOT_FOUND | An enqueued executable was not found at the specified path |
| 15 | EXECUTABLE_NOT_VALID | An file with the name of an enqueued executable was found but is not actually an executable|
| 16 | PROCESS_START_FAIL | An enqueued executable failed to start |
| 17 | BATCH_PROCESS_NOT_FOUND | The "wait-on" process for .bat based titles was not found (currently unused) |
| 18 | BATCH_PROCESS_NOT_HANDLED | A handle to the "wait-on" process for .bat based titles could not be obtained (currently unused) |
| 19 | BATCH_PROCESS_NOT_HOOKED | The "wait-on" process for .bat based titles returned before it finished executing (currently unused) |


## Limitations

 - Although general compatibility is quite high, compatibility with every single title cannot be assured. Issues with a title or group of titles will be fixed as they are discovered

## Source
This tool was written in C++ 17 along with Qt 5 and currently only targets Windows Vista and above; however, this tool can easily be ported to Linux with minimal changes, though to what end I am not sure since this is for a Windows application. The source includes an easy-to-use .pro file if you wish to build the application in Qt Creator and the available latest release was compiled in Qt Creator using MSVC 2019 and a static compilation of Qt 5.15.0. Other than a C++ 17 capable compiler and Qt 5.15.x+ all files required to compile this software are included, with the exception of a standard make file.

All functions/variables under the "Qx" (QExtended) namespace belong to a small, personal library I maintain to always have access to frequently used functionality in my projects. A pre-compiled static version of this library is provided with the source for this tool. If anyone truly needs it, I can provide the source for this library as well.
