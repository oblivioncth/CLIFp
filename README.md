# CLIFp (Command-line Interface for Flashpoint)
<img align="left" src="https://i.imgur.com/U6aDFSt.png" width=20%>

CLIFp (pronounced "Cliff-P") is a command-line interface for [Flashpoint Archive](https://flashpointarchive.org/) that allows starting games/animations from within the collection via a system's command parser and contextual arguments. While it is a separate application, CLIFp functions closely to that of a native CLI by parsing the configuration of the Flashpoint install it is deployed into and therefore launches games/animation in the same manner as the standard GUI launcher.

Other than a few pop-up dialogs used for alerts and errors, CLIFp runs completely in the background so that the only windows seen during use are the same ones present while running standard Flashpoint. It automatically terminates once the target application has exited, requiring no manual tasks or clean-up by the user. 

[![Dev Builds](https://github.com/oblivioncth/CLIFp/actions/workflows/push-reaction.yml/badge.svg?branch=dev)](https://github.com/oblivioncth/CLIFp/actions/workflows/push-reaction.yml)

## Compatability
### General
Because it directly mimics the actions of the GUI launcher, CLIFp should provide a perfect or near-perfect experience when compared to using the standard method of launching games/animations. Additionally, this makes it fairly resilient towards Flashpoint updates and will most likely only require compatibility patches when updates that make major changes are released.

All Flashpoint features are supported, other than editing the local database and querying title meta-data through the command-line. More specifically, one can launch:

 - Games (with or without autorun-before additional apps)
 - Animations
 - Additional Apps (including messages)
 - Extras
 - Anything else I'm forgetting

While testing for complete compatibility is infeasible given the size of Flashpoint, CLIFp was designed with full compatibility in mind and theoretically is 100% compatible with the Flashpoint collection. Games are slightly prioritized when it comes to testing the application however.

### Version Matching
Each release of this application targets a specific version series of Flashpoint Archive, which are composed of a major and minor version number, and are designed to work with all Flashpoint updates within that series. For example, a FIL release that targets Flashpoint 10.1 is intended to be used with any version of Flashpoint that fits the scheme `10.1.x.x`, such as `10.1`, `10.1.0.3`, `10.1.2`, etc, but **not**  `10.2`.

Using a version of CLIFp with a version of Flashpoint different than its target version is discouraged as some features may not work correctly or at all and in some cases the utility may fail to function entirely; **however**, given its design, CLIFp is likely to continue working with newer versions of FP that are released without requiring an update, unless that new version contains significant technical changes.

The title of each [release](https://stackedit.io/github.com/oblivioncth/CLIFp/releases) will indicate which version of Flashpoint it targets.

Updates will always be set to target the latest Flashpoint release, even if they were not created explicitly for compatibility reasons.

## Usage
### Target Usage
CLIFp was primarily created for use with its sister project [FIL (Flashpoint Importer for Launchers)](https://github.com/oblivioncth/FIL) to facilitate the inclusion of Flashpoint in to LaunchBox and other frontend collections. The operation of CLIFp is completely automated when used in this manner.

It was later refined to also be used directly with the Flashpoint project to allow users to create shortcuts and leverage other benefits of a CLI. 

That being said, it is perfectly possible to use CLIFp in any manner one sees fit.

It is recommended to place CLIFp in the root directory of Flashpoint (next to its shortcut), but CLIFp will search all parent directories in order for the root Flashpoint structure and therefore will work correctly in any Flashpoint sub-folder. However, this obviously won't work if CLIFp is behind a symlink/junction.

### Builds
**In most cases you should use the 'static' builds of CLIFp on Windows or Linux.**

### General
**Before using CLIFp, be sure to have ran Flashpoint through its regular launcher at least once. If using Infinity, it's also best to make sure your install is fully updated as well.**

**NOTE: Do not run CLIFp as an administrator/root as some titles may not work correctly or run at all**

CLIFp uses the following syntax scheme:

    CLIFp <global options> *command* <command options>
The order of switches within each options section does not matter.


**Primary Usage:**
The most common use case is the **play** command with the **-i** switch, followed by the UUID of a Flashpoint title:

    CLIFp play -i 37e5c215-9c39-4a3d-9912-b4343a17027e

This is the most straightforward and hassle free approach, as it will start that title exactly as if it had been launched from the Flashpoint GUI (including download/mounting of data packs, autorun before additional apps, etc.).

A title's ID can be found by right clicking on an entry in Flashpoint and selecting "Copy Game UUID". This command also supports starting additional apps, though getting their UUID is more tricky as I currently know of no other way than opening [FP Install Dir]\Data\flashpoint.sqlite in a database browser and searching for the ID manually. I will look into if an easier way to obtain their IDs can be implemented.

Alternatively, the **-t** switch can be used, followed by the exact title of an entry:

    CLIFp play -t "Interactive Buddy"

Or if feeling spontaneous, use the **-r** switch, followed by a library filter to select a title randomly:

    CLIFp play -r game

See the full command/options list for more information.

**Direct Execution:**
The legacy approach is to use the **run** command with the **--app** and **--param** switches. This will start Flashpoint's webserver and then start the application specified with the provided parameters:

    CLIFp run --app="FPSoftware\Flash\flashplayer_32_sa.exe" --param="http://www.mowa.org/work/buttons/buttons_art/basic.swf"

If the application needs to use files from a Data Pack that pack will need to be downloaded/mounted first using **prepare** or else it won't work.

The applications and arguments that are used for each game/animation can be found within the Flashpoint database ([FP Install Dir]\Data\flashpoint.sqlite)

### Flashpoint Protocol
CLIFp supports the "flashpoint" protocol, which means it can launch titles through URL with a custom scheme, followed by a title's UUID, like this:

    flashpoint://37e5c215-9c39-4a3d-9912-b4343a17027e

This makes it fast and easy to share games that you think your friends should try out.

To register CLIFp as the handler for these URLs, simply run:

    CLIFp share -c

To easily create a share link if you don't already know the UUID of a game, you can use the **share** command with the **-t** switch followed by a title:

    CLIFp share -t "Simple, Tasty Buttons"

This will create a share link for that title which will be displayed via a message box and automatically copied to the system clipboard.

If for whatever reason the service through which you wish to share a link does not support links with custom schemes, you can use the **-u** switch to generate a standard "https" link that utilizes a GitHub-hosted redirect page, enabling share links to be provided everywhere. 

[!IMPORTANT]
You will want to disable the "Register As Protocol Handler" option in the default launcher or else it will replace CLIFp as the "flashpoint" protocol handler every time it's started (**NOTE: This option is not currently available in a released version of Flashpoint**).

## All Commands/Options
The recommended way to use all switches is to use their short form when the value for the switch has no spaces:

    -i 95b149c2-7f57-4894-b980-6ef03192f79d

and the long form when the value does have spaces

    --msg="I am a message"
though this isn't required as long as quotation and space use is carefully employed.

### Global Options:
 -  **-h | --help | -?:** Prints usage information
 -  **-v | --version:** Prints the current version of the tool
 -  **-q | --quiet:** Silences all non-critical messages
 -  **-s | --silent:** Silences all messages (takes precedence over quiet mode)
 
### Commands:
**link** - Creates  a  shortcut  to  a  Flashpoint  title

Options:
 -  **-i | --id:** UUID  of  title  to  make a shortcut for
 -  **-t | --title:** Title to  make a shortcut for
 -  **-T | --title-strict:** Same as **-t**, but only exact matches are considered
 -  **-s | --subtitle:** Name of additional-app under the title to make a shortcut for. Must be used with **-t**/**-T**
 -  **-S | --subtitle-strict:** Same as **-s**, but only exact matches are considered
 -  **-p | --path:** Path to new shortcut. Path's ending with ".lnk" (Windows) or ".desktop" (Linux) will be interpreted as a named shortcut file. Any other path will be interpreted as a directory and the title will automatically be used as the filename
 -  **-h | --help | -?:** Prints command specific usage information

Requires:
**-i** or **-t** 

Notes: 

 - On Linux, when providing a full shortcut path via the **--path** switch, the filename component is re-interpreted as the shortcut's display name and the actual filename is set automatically.

   For example, when specifying:
   
       CLIFp link -p "~/Desktop/Cool Name.desktop" ...

   the display name of the desktop entry will be set to "Cool Name".
 - On some Linux desktop environments (i.e. GNOME) the shortcut might need to manually be set to "trusted" in order to be used and displayed correctly after it is created. This option is usually available in the file's right-click context menu.
 - See the **play** command notes for information regarding the **t**/**T** and **s**/**S** switches.

--------------------------------------------------------------------------------

**play** - Launch  a  title  and  all  of  it's  support  applications,  in  the  same  manner  as  using  the  GUI

Options:

 -  **-i | --id:** UUID  of  title  to  start
 -  **-t | --title:** Title  to  start
 -  **-T | --title-strict:** Same as **-t**, but only exact matches are considered
 -  **-s | --subtitle:** Name of additional-app under the title to start. Must be used with **-t**/**-T**
 -  **-S | --subtitle-strict:** Same as **-s**, but only exact matches are considered
 -  **-r | --random:** Select  a  random  title  from  the  database  to  start.  Must  be  followed  by  a  library  filter:  all/any,  game/arcade,  animation/theatre
 -  **-h | --help | -?:** Prints command specific usage information

Requires:
**-i** or **-t** or **-r**

Notes:

The **-t** and **-s** switches are case-insensitive and will match any title that contains the value provided. If more than one result is found, a dialog will be presented that allows for selected the desired title; however, there is a limit to the number of matches.

Using the **-T** and **-S** switches only consider exact matches and are performed slightly faster than their more flexible counterparts.

Tip: You can use **-s** with an empty string (i.e. `-s ""`) to see all of the additional-apps for a given title.

--------------------------------------------------------------------------------
 
 **prepare** - Initializes  Flashpoint  for  playing  the  provided  Data  Pack  based  title  by  UUID.  If  the  title  does  not  use  a  Data  Pack  this  command  has  no  effect.

Options:

 -  **-i | --id:** UUID  of  title  to  prepare
 -  **-t | --title:** Title  to  prepare
 -  **-h | --help | -?:** Prints command specific usage information

Requires:
**-i** or **-t** 

--------------------------------------------------------------------------------
 
 **run** - Start  Flashpoint's  webserver  and  then  execute  the  provided  application

Options:
 -  **-a | --app:** Relative (to Flashpoint Directory) path of  application to launch
 -  **-p | --param:** Command-line parameters to use when starting the application
 -  **-h | --help | -?:** Prints command specific usage information

Requires:
**-a** 

Notes: 

When using the **--exe** and **--param** switches all quotes that are part of the input itself must be escaped for the command to be passed correctly. For example, the launch command

    "http://website.com" -switch "value"
  must be specified as
  

    --param="\"http://website.com\" -switch \"value\""
   Additionally, any characters with special meaning to the Windows shell that are not within at least one level of quote pairs must also be escaped (this can be avoided as long as you wrap the entire value of the switch in quotes as shown above):

    http://website.com/foo&bar => --param=http://website.com/foo^&bar

See http://www.robvanderwoude.com/escapechars.php for more information.

--------------------------------------------------------------------------------

**share** - Generates a URL for starting a Flashpoint title that can be shared to other users.

Options:
 -  **-i | --id:** UUID  of  title  to  make a share link for
 -  **-t | --title:** Title to  make a share link for
 -  **-T | --title-strict:** Same as **-t**, but only exact matches are considered
 -  **-s | --subtitle:** Name of additional-app under the title to make a share link for. Must be used with **-t**/**-T**
 -  **-S | --subtitle-strict:** Same as **-s**, but only exact matches are considered
 -  **-u | --universal:** Creates a standard HTTPS link that utilizes a redirect page. May be easier to share on some platforms.
 - **-c | --configure:** Registers CLIFp as the default handler for "flashpoint" protocol links.
 - **-C | --unconfigure:** Removes CLIFp as the default handler for "flashpoint" protocol links.
 -  **-h | --help | -?:** Prints command specific usage information

Requires:
**-i**, **-t** or **-c** 

Notes: 

 - By default, the standard Flashpoint launcher is registered to handle share links; therefore, its "Register As Protocol Handler" option should likely be disabled if you intend to use CLIFp instead.
 - See the **play** command notes for information regarding the **t**/**T** and **s**/**S** switches.

--------------------------------------------------------------------------------

 **show** - Display  a  message  or  extra  folder

Options:
 -  **-m | --msg:** Displays an pop-up dialog with the supplied message. Used primarily for some additional apps
 -  **-e | --extra:** Opens an explorer window to the specified extra. Used primarily for some additional apps
 -  **-h | --help | -?:** Prints command specific usage information

Requires:
**-m** or **-e** 

### Remarks
With any use of the **--title**/**--subtitle** options for the commands that support them, the provided title should match as closely as possible to how it appears within Flashpoint, as checks for close matches are limited due to technical restrictions. If more than one entry is found, a dialog window with more information will be displayed so that the intended title can be selected.

## Other Features
CLIFp displays a system tray icon so that one can be sure it is still running. This icon also will display basic status messages when clicked on and features a context menu with an option to exit at any time.

The functionality of the tray icon may be expanded upon in future releases.

## Limitations

 - Although general compatibility is quite high, compatibility with every single title cannot be assured. Issues with a title or group of titles will be fixed as they are discovered

## Source

### Summary

 - C++20
 - CMake 3.24.0
 - Targets:
	 - Windows 10+
	 - Linux

### Dependencies
- Qt6
- [Qx](https://github.com/oblivioncth/Qx/)
- [libfp](https://github.com/oblivioncth/libfp/)
- [QI-QMP](https://github.com/oblivioncth/QI-QMP/)
- [QuaZip](https://github.com/stachenov/quazip)
- [Neargye's Magic Enum](https://github.com/Neargye/magic_enum)
- [OBCMake](https://github.com/oblivioncth/OBCmake)

### Details
The source for this project is managed by a sensible CMake configuration that allows for straightforward compilation and consumption of its target(s), either as a sub-project or as an imported package. All required dependencies except for Qt6 are automatically acquired via CMake's FetchContent mechanism.