# Usage

## Original Usage
CLIFp was originally created for use with its sister project [FIL (Flashpoint Importer for Launchers)](https://github.com/oblivioncth/FIL) to facilitate the inclusion of Flashpoint in to LaunchBox and other frontend collections. The operation of CLIFp is completely automated when used in this manner.

It was later refined to also be used directly with the Flashpoint project to allow users to create shortcuts and leverage other benefits of a CLI.

That being said, it is perfectly possible to use CLIFp in any manner one sees fit.

## General

> [!NOTE]
> In most cases you should use the 'static' builds of CLIFp on Windows or Linux.

It is recommended to place CLIFp in the root directory of Flashpoint (next to its shortcut), but CLIFp will search all parent directories in order for the root Flashpoint structure and therefore will work correctly in any Flashpoint sub-folder. However, this obviously won't work if CLIFp is behind a symlink/junction.

Alternative, you can use the `-f` [global switch](COMMANDS.md#global-options) to specify the root Flashpoint directory, which will work regardless of where CLIFp is located.

> [!IMPORTANT]
> Before using CLIFp, be sure to have ran Flashpoint through its regular launcher at least once. If using Infinity, it's also best to make sure your install is fully updated as well.
>
> Do not run CLIFp as an administrator/root as some titles may not work correctly or run at all.

CLIFp uses the following syntax scheme:

    CLIFp <global options> *command* <command options>

The order of switches within each options section does not matter.

**Primary Usage:**

The most common use case is the `play` command with a [Title Command](COMMANDS.md#title-commands) switch. For this example `-i` is used to indicate a title is being referenced by its UUID

    CLIFp play -i 37e5c215-9c39-4a3d-9912-b4343a17027e

This is the most straightforward and hassle free approach, as it will start that title exactly as if it had been launched from the Flashpoint GUI (including download/mounting of data packs, autorun before additional apps, etc.).

A title's ID can be found by right clicking on an entry in Flashpoint and selecting "Copy Game UUID". You can also get the ID for an additional application by right clicking it and selecting "Copy Additional App UUID". Otherwise, you can search [FP Install Dir]\Data\flashpoint.sqlite in a database browser manually.

Alternatively, the `-t` switch can be used, followed by the exact title of an entry:

    CLIFp play -t "Interactive Buddy"

Or if feeling spontaneous, use the `-r` switch, followed by a library filter to select a title randomly:

    CLIFp play -r game

See the [Full Commands + Options Documentation](doc/COMMANDS.md) section for more information.

**Direct Execution:**
The legacy approach is to use the **run** command with the **--app** and **--param** switches. This will start Flashpoint's services and then start the application specified with the provided parameters:

    CLIFp run --app="FPSoftware\Flash\flashplayer_32_sa.exe" --param="http://www.mowa.org/work/buttons/buttons_art/basic.swf"

If the application needs to use files from a Data Pack that pack will need to be downloaded/mounted first using **prepare** or else it won't work.

The applications and arguments that are used for each game/animation can be found within the Flashpoint database ([FP Install Dir]\Data\flashpoint.sqlite)

## Flashpoint Protocol

CLIFp supports the "flashpoint" protocol, which means it can launch titles through URL with a custom scheme, followed by a title's UUID, like this:

    flashpoint://37e5c215-9c39-4a3d-9912-b4343a17027e

This makes it fast and easy to share games that you think your friends should try out.

To register CLIFp as the handler for these URLs, simply run:

    CLIFp share -c

To easily create a share link if you don't already know the UUID of a game/animation, you can use the `share` command with the `-t` switch followed by a title:

    CLIFp share -t "Simple, Tasty Buttons"

This will create a share link for that title which will be displayed via a message box and automatically copied to the system clipboard.

If for whatever reason the service through which you wish to share a link does not support links with custom schemes, you can use the `-u` switch to generate a standard "https" link that utilizes a GitHub-hosted redirect page, enabling share links to be provided everywhere.

> [!IMPORTANT]
> You will want to disable the "Register As Protocol Handler" option in the default launcher or else it will replace CLIFp as the "flashpoint" protocol handler every time it's started.