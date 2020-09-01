# CLIFp (Command-line Interface for Flashpoint)
CLIFp (pronounced "Cliff-P") is a pseudo-command-line interface for [BlueMaxima's Flashpoint](https://bluemaxima.org/flashpoint/) project that allows you to start games/animations from within the collection via your system's command parser and contextual arguments. The utility is a "pseudo" CLI because it does not directly interface directly with any Flashpoint AP; instead, it emulates the environment 
present while the Flashpoint launcher is running each time it is used to provide close to the same functionality that a native CLI would. Other than a few pop-up dialogs used for alerts and errors, CLIFp runs completely in the background so that the only windows seen during use are the same ones present while running standard Flashpoint. It automatically terminates once the target application has exited, requiring no manual tasks or clean-up by the user. 

## Compatability
### General
While testing for 100% compatibility is infeasible given the size of Flashpoint, CLIFp was designed with full compatibility in mind; however, the focus of the project is perfecting its use in playing games from the Flashpoint collection so design goals and testing when it comes to the animations is of lower priority. Additionally, accessing the "Extras" content of an entry is currently not supported.

### Version Matching
Each release of this application targets a specific version or versions of BlueMaxima's Flashpoint and while newer releases will sometimes contain general improvements to functionality, they will largely be created to match the changes made between each Flashpoint release and therefore maintain compatibility. These matches are shown below:
| CLIFp Version | Target Flashpoint Version |
|--|--|
| 0.1 | 8.1 ("Spirit of Adventure") |
| 0.1.1 | 8.2 ("Approaching Planet Nine") |

Using a version of CLIFp that does not target the version of Flashpoint you wish to use it with is highly discouraged as some features may not work correctly or at all and in some cases the utility may fail to function entirely or even damage the Flashpoint install it is used with.

## Usage
### Target Usage
Although perfectly reasonable, CLIFp was not developed with the intentions of using it as a standalone application and instead was primarily created for use with its sister project [OFILb (Obby's Flashpoint Importer for LaunchBox)](https://github.com/oblivioncth/OFILb) to facilitate the inclusion of Flashpoint in to LaunchBox user's collections. The operation of CLIFp is completely automated when used in this manner.

### General
Typical usage of CLIFp as a command-line interface involves starting the utility with an "application" (--app) and "parameters" (--param) argument as shown in the following example: 
![Example usage](https://i.imgur.com/VawCM5Q.png)

The application argument is the full or relative path to the target Flashpoint application while the parameters argument is the list of arguments that are to be passed to the application when it is started. Generally these are taken directly from the specific entry in the Flashpoint database "flashpoint.sqlite" that holds the metadata for a particular game of interest.

### All Arguments

 - -h | --help | -?: Prints usage information
 -  -v | --version: Prints the current version of the tool
 -  -a | --app: Path of primary application to launch
 -  -p | --param: Command-line parameters to use when starting the primary application
 -  -m | --msg: Displays an pop-up dialog with the supplied message. Used primarily for some additional apps

The Application and Parameter options are required (unless using help, version, or msg). If the help or version options are specified, all other options are ignored.

## Limitations

 - Although general compatibility is quite high, compatibility with every single title cannot be assured. Issues with a title or group of titles will be fixed as they are discovered
 - The "Extras" feature for some games/animation is currently not support. It is currently unknown if it will be possible to support them at some point since no research into how they are loaded has been performed

## Source
This tool was written in C++ 17 along with Qt 5 and currently only targets Windows Vista and above; however, this tool can easily be ported to Linux with minimal changes, though to what end I am not sure since this is for a Windows application. The source includes an easy-to-use .pro file if you wish to build the application in Qt Creator and the available latest release was compiled in Qt Creator using MSVC 2019 and a static compilation of Qt 5.15.0. Other than a C++ 17 capable compiler and Qt 5.15.x+ all files required to compile this software are included, with the exception of a standard make file.

All functions/variables under the "Qx" (QExtended) namespace belong to a small, personal library I maintain to always have access to frequently used functionality in my projects. A pre-compiled static version of this library is provided with the source for this tool. If anyone truly needs it, I can provide the source for this library as well.
