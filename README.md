# CLIFp (Command-line Interface for Flashpoint)
CLIFp (pronounced "Cliff-P") is a command-line interface for [BlueMaxima's Flashpoint](https://bluemaxima.org/flashpoint/) project that allows you to start games/animations from within the collection via your system's command parser and contextual arguments. While it is a separate application, CLIFp functions closely to that of a native CLI by parsing the configuration of the Flashpoint install it is deployed into and therefore launches games/animation in the same manner as the standard GUI launcher.

Other than a few pop-up dialogs used for alerts and errors, CLIFp runs completely in the background so that the only windows seen during use are the same ones present while running standard Flashpoint. It automatically terminates once the target application has exited, requiring no manual tasks or clean-up by the user. 

## Compatability
### General
Since the 0.2 rewrite, the primary paradigm through which CLIFp functions has been significantly improved and should provide a perfect or near-perfect experience when compared to using the standard GUI method of launching games/animations. Additionally, the redesign is expected to be very resilient towards Flashpoint updates and will most likely only require compatibility patches when updates that make major changes are released.

All Flashpoint features are supported, other than editing your local database (this includes playlists) and querying title meta-data through the command-line. More specifically, you can launch:

 - Games (with or without autorun-before additional apps)
 - Animations
 - Additional Apps (including messages)
 - Extras
 - Anything else I'm forgetting

While testing for complete compatibility is infeasible given the size of Flashpoint, CLIFp was designed with full compatibility in mind and theoretically is 100% compatible with the Flashpoint collection. Games are slightly prioritized when it comes to testing the application however.

### Version Matching
Each release of this application targets a specific version or versions of BlueMaxima's Flashpoint and while newer releases will sometimes contain general improvements to functionality, they will largely be created to match the changes made between each Flashpoint release and therefore maintain compatibility. These matches are shown below:
| CLIFp Version   | Target Flashpoint Version       |
|-----------------|---------------------------------|
| 0.1             | 8.1 ("Spirit of Adventure")     |
| 0.1.1           | 8.2 ("Approaching Planet Nine") |
| 0.2 - 0.3.1.1   | 8.1 - 8.2                       |
| 0.3.2 - 0.4.0.1 | 9.0 ("Glorious Sunset")         |
| 0.4.1 - 0.5     | 10.0 ("Absence")                |

Using a version of CLIFp that does not target the version of Flashpoint you wish to use it with is highly discouraged as some features may not work correctly or at all and in some cases the utility may fail to function entirely; **however since 0.2 compatibility with newer versions is quite likely even if they aren't explicit listed yet** (usually because I haven't had time to check if an update is needed).

## Usage
### Target Usage
CLIFp  was primarily created for use with its sister project [OFILb (Obby's Flashpoint Importer for LaunchBox)](https://github.com/oblivioncth/OFILb) to facilitate the inclusion of Flashpoint in to LaunchBox user's collections. The operation of CLIFp is completely automated when used in this manner.

It was later refined to also be used directly with the Flashpoint project to allow users to create shortcuts and leverage other benefits of a CLI. 

That being said, it is perfectly possible to use CLIFp in any manner you see fit.

It is recommended to place CLIFp in the root directory of Flashpoint (next to its shortcut), but CLIFp will search all parent directories in order for the root Flashpoint structure and therefore will work correctly in any Flashpoint sub-folder. However, this obviously won't work if CLIFp is behind a symlink/junction.

### General
**NOTE: Do not run CLIFp as an administrator as some titles may not work correctly or run at all**

While CLIFp has multiple functions, typical usage involves one of three main methods:

**Auto:**
This is the most straightforward and hassle free approach. Simply use the **--auto** or **-a** switch followed by the GUID/UUID of the game/animation you wish to start, as seen in the following example:
![Example usage](https://i.imgur.com/0vLToTi.png)

You can find a title's ID by right clicking on an entry in Flashpoint and selecting "Copy Game UUID". This switch also supports starting additional apps, though getting their UUID is more tricky as I currently know of no other way than opening [FP Install Dir]\Data\flashpoint.sqlite in a database browser and searching for the ID manually. I will look into if an easier way to obtain their IDs can be implemented.

If the ID belongs to a main game/animation entry, any additional apps that are marked as auto-run before will also be started ahead of time just as they would be when using the Flashpoint Launcher.

**Random:**
This mode works exactly the same as Auto mode except that the title ID is selected at random (uniformly) for you from the pool of all playable games/animations. A title qualifies as playable if it is a main title who's status is not "Not Working" or if it is an additional-app that is not a message, extra, or otherwise Autostart-before entry. You must specify a library filter: **all** or **any** for all titles to be considered, **game** or **arcade** for only games to be considered, or **animation** or **theatre** for only animations to be considered. Basic information of the game that gets selected will be displayed before it starts unless the **-s** or **-q** switches are specified.

    Example: CLIFp -r all

**App/Param:**
This method can only start one application at a time. Use the **-x/-\-exe** switch to specify the relative (from Flashpoint's directory) path to the application to launch and the **-p/-\-param** switch to provide the launch arguments that will be passed to the target application.

If the application needs to use files from a Data Pack that pack will need to be downloaded/mounted first using **-i/-\-prepare** or else it won't work.

The applications and arguments that are used for each game/animation can be found within the Flashpoint database ([FP Install Dir]\Data\flashpoint.sqlite)

### All Switches/Arguments

 -  **-h | --help | -?:** Prints usage information
 -  **-v | --version:** Prints the current version of the tool
 -  **-x | --exe:** Relative (to Flashpoint Directory) of primary application to launch
 -  **-p | --param:** Command-line parameters to use when starting the primary application
 -  **-a | --auto:** Finds a game/additional-app by UUID and runs it if found, including run-before additional apps in the case of a game
 - **-r | --random:** Selects  a  random  game  UUID  from  the  database  and  starts  it  in  the  same  manner  as  using  the  --auto switch. Required value options for this argument (filters):  all/any,  game/arcade,  animation/theatre
 - **-i | --prepare:** Initializes  Flashpoint  for  playing  the  provided  Data  Pack  based  title  by  UUID.  If  the  title  does  not  use  a  Data  Pack  this  option  has  no  effect
 -  **-m | --msg:** Displays an pop-up dialog with the supplied message. Used primarily for some additional apps
 -  **-e | --extra:** Opens an explorer window to the specified extra. Used primarily for some additional apps
 -  **-q | --quiet:** Silences all non-critical messages
 -  **-s | --silent:** Silences all messages (takes precedence over quiet mode)
 
Use **'exe'** and **'param'** for direct operation, use **'auto'** by itself for automatic operation, use **'random'** by itself for random operation, use **'msg'** to display a popup message, use **'extra'** to view an extra, use **'prepare'**  to  prepare  a  Data  Pack  game, or use **'help'** and/or **'version'** for information.

**NOTE:** When using the **--exe** and **--param** switches all quotes that are part of the input itself must be escaped for the command to be passed correctly. For example, the launch command

    "http://website.com" -switch "value"
  must be specified as
  

    --param="\"http://website.com\" -switch \"value\""
   Additionally, any characters with special meaning to the Windows shell that are not within at least one level of quote pairs must also be escaped (this can be avoided as long as you wrap the entire value of the switch in quotes as shown above):

    http://website.com/foo&bar => --param=http://website.com/foo^&bar

See http://www.robvanderwoude.com/escapechars.php for more information.

## Exit Codes
Once CLIFp has finished executing an exit code is reported that indicates the "error status" of the program, which can be useful for recording/determining issues. The exit code can be obtained by running the application in the following manner, or by examining CLIFp.log:

    start /wait CLIFp.exe [parameters]
    echo %errorlevel%

| Value | Code | Description |
|----|-------------------------|-----------------------------------------------------------------------------------------------------------|
| 0  | NO_ERR                  | The application completed successfully                                                                    |
| 1  | ALREADY_OPEN            | Another instance of CLIFp is already running                                                              |
| 2  | INVALID_ARGS            | The arguments provided were not recognized or were formatted incorrectly                                  |
| 3  | LAUNCHER_OPEN           | The application could not start because the Flashpoint Launcher is currently open                         |
| 4  | INSTALL_INVALID         | The Flashpoint install that CLIFp is deployed in is corrupted or not compatible with its current version  |
| 5  | CANT_PARSE_CONFIG       | Failed to parse config.json                                                                               |
| 6  | CANT_PARSE_PREF         | Failed to parse preferences.json                                                                          |
| 7  | CANT_PARSE_SERVICES     | Failed to parse services.json                                                                             |
| 8  | CONFIG_SERVER_MISSING   | The server entry specified in config.json was not found in services.json                                  |
| 9  | ID_NOT_VALID            | The specified string is not a valid 128-bit UUID                                                          |
| 10 | RAND_FILTER_NOT_VALID   | The  provided  string  for  random  operation  was  not  a  valid  filter                                 |
| 11 | SQL_ERROR               | An unexpected SQL error occurred while reading flashpoint.sqlite                                           |
| 12 | SQL_MISMATCH            | Received  a  different  form  of  result  from  an  SQL  query  than  expected                            |
| 13 | DB_MISSING_TABLES       | flashpoint.sqlite is missing expected tables                                                              |
| 14 | DB_MISSING_COLUMNS      | One or more tables in flashpoint.sqlite are missing expected columns                                      |
| 15 | ID_NOT_FOUND            | The specified UUID is not associated with any title in the Flashpoint database                            |
| 16 | MORE_THAN_ONE_AUTO      | The specified UUID for auto operation is associated with more than one title (possible collision)         |
| 17 | EXTRA_NOT_FOUND         | The specified or auto-determined extra was not found in the Extras folder                                 |
| 18 | EXECUTABLE_NOT_FOUND    | An enqueued executable was not found at the specified path                                                |
| 19 | EXECUTABLE_NOT_VALID    | An file with the name of an enqueued executable was found but is not actually an executable               |
| 20 | PROCESS_START_FAIL      | An enqueued executable failed to start                                                                    |
| 21 | WAIT_PROCESS_NOT_HANDLED| A handle to a "wait-on" process (usually for .bat based titles) could not be obtained                     |
| 22 | WAIT_PROCESS_NOT_HOOKED | A wait task returned before its "wait-on" process (usually for .bat based titles) finished executing      |
| 23 | CANT_READ_BAT_FILE      | Failed to read a batch script for checking if it contains a use of a "wait-on" process                    |
| 24 | PARENT_INVALID          | The parent ID of the target additional app is missing or invalid                                          |
| 25 | CANT_OBTAIN_DATA_PACK   | Failed to download the selected title's Data Pack                                                         |
| 26 | DATA_PACK_INVALID       | The selected title's Data Pack checksum did not match it's known value after download                     |

## Limitations

 - Although general compatibility is quite high, compatibility with every single title cannot be assured. Issues with a title or group of titles will be fixed as they are discovered

## Source
This tool was written in C++ 17 along with Qt 5 and currently only targets Windows Vista and above; however, this tool can easily be ported to Linux with minimal changes, though to what end I am not sure since this is for a Windows application. The source includes an easy-to-use .pro file if you wish to build the application in Qt Creator and the available latest release was compiled in Qt Creator using MSVC 2019 and a static compilation of Qt 5.15.2. Other than a C++ 17 capable compiler and Qt 5.15.x+ (compiled with some form of SSL support) all files required to compile this software are included, with the exception of a standard make file.

All functions/variables under the "Qx" (QExtended) namespace belong to a small, personal library I maintain to always have access to frequently used functionality in my projects. A pre-compiled static version of this library is provided with the source for this tool. If anyone truly needs it, I can provide the source for this library as well.

Additionally the source makes use of [Neargye's Magic Enum](https://github.com/Neargye/magic_enum) header for gaining static reflection of enumerated types, primarily to easily convert enum names to strings.