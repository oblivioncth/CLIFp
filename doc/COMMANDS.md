# Full Commands + Options Documentation

### Forms

Most options have short and long forms (shown separated by a pipe `|` below), which are interchangeable. For options that take a value, a space or **=** can be used between the option and its value, e.g.

    # Short/Long
    -s
    --silent

    # Short/Long + Argument
    -i 95b149c2-7f57-4894-b980-6ef03192f79d
    --id 95b149c2-7f57-4894-b980-6ef03192f79d
    -m "I am a message"
    --msg="I am a message"


### Global **Options**:
- `-h` | `--help` | `-?` - Prints usage information
- `-v` | `--version` - Prints the current version of the tool
- `-q` | `--quiet` - Silences all non-critical messages
- `-s` | `--silent` - Silences all messages (takes precedence over quiet mode)
- `-f` | `--flashpoint` - Manually specify the Flashpoint root directory. Allows use of CLIFp from anywhere

These options pertain directly to CLIFp itself and not any one particular command. As such they are specified immediately following the executable name, but **before** any command:

    CLIFp -h

Command specific options are to be placed **after** the command, like so:

    CLIFp play -r

### Common **Options**:

> [!NOTE]
> Every command shown below also has it's own specific `-h` | `--help` | `-?` option you can use to see usage information specific to that command.


### Title Commands:
Many of CLIFp's commands require a game/animation to be specified, which can be done in several ways. These commands, are known as *title commands* and are noted as such below. The explanation for these common options are shown here instead of being repeated under each individually.

Title Comamnd Shared **Options**:
- `-i` | `--id` - UUID  of a game
- `-t` | `--title` - The title of a game.
- `-T` | `--title-strict:` - Same as `-t`, but only exact matches are considered
- `-s` | `--subtitle` - Name of an additional-app under a game. Must be used with `-t`/`-T` or `-i` (when `-i` refers to a parent game ID)
- `-S` | `--subtitle-strict` - Same as `-s`, but only exact matches are considered
- `-r` | `--random` - Selects  a  random  title  from  the  database.  Must  be  followed  by  a  library  filter:  `all`/`any`,  `game`/`arcade`,  or `animation`/`theatre`

The `--title` and `--subtitle` options are case-insensitive and will match any title that contains the value provided; however, the provided title should match as closely as possible to how it appears within Flashpoint, as checks for close matches are limited due to technical restrictions. If more than one entry is found, a dialog window with more information will be displayed so that the intended title can be selected, though there is a limit to the number of matches.

The `--title-strict` and `--subtitle-strict` options only consider exact matches and are performed slightly faster than their more flexible counterparts.

Tip: You can use `-subtitle` with an empty string (i.e. `-s ""`) to see all of the additional-apps for a given title, if more than 1; otherwise this will launch the only additional-app (if any).


## Command List:

 ### `download` - Downloads data packs for games that require them in bulk

**Options**:
- `-p` | `--playlist` - Name of the playlist to download games for.

--------------------------------------------------------------------------------

`link` - Creates  a  shortcut  to  a  Flashpoint  title

**Options**:
- [Title Command](#title-commands) options
- `-p` | `--path` - Path to directory in which to place the shortcut. Prompts if not provided.
- `-n` | `--name` - Name of the shortcut. Defaults to the name of the title

**Notes**:
 - On some Linux desktop environments (i.e. GNOME) the shortcut might need to manually be set to "trusted" in order to be used and displayed correctly after it is created. This option is usually available in the file's right-click context menu.

--------------------------------------------------------------------------------

### `play` - Launch  a game/animation. The bread and butter of this application.

**Options**:

- [Title Command](#title-commands) options
- `-f` | `--fullscreen` - Forces the title to run in full screen if the underlying executable supports it.
- `--ruffle` - Forces the use of Ruffle for applicable Flash games. Takes precedence over `--flash`.
- `--flash` - Forces the use of the standard app (usually Flash Player) for applicable Flash games.

**Notes**:
 - You can use `--` to pass arguments directly to the title's underlying executable/script, which can be useful for testing or customizing your experience:

       CLIFp play -t "One Stick Man!" -- -monitor 2

--------------------------------------------------------------------------------

 ### `prepare` - Initializes  Flashpoint  for  playing  the  provided  Data  Pack  based  title  by  UUID

**Options**:

- [Title Command](#title-commands) options

**Notes:**

If  the  title  does  not  use  a  Data  Pack  this  command  has  no  effect.

--------------------------------------------------------------------------------

 ### `run` - Start  Flashpoint's  services and  then  execute  the  provided  application

**Options**:
- `-a` | `--app` - Relative (to Flashpoint Directory) path of  application to launch
- `-p` | `--param` - Command-line parameters to use when starting the application

**Requires:**
`-a`

**Notes**:

When using the `--exe` and `--param` switches all quotes that are part of the input itself must be escaped for the command to be passed correctly. For example, the launch command

    "http://website.com" -switch "value"
  must be specified as


    --param="\"http://website.com\" -switch \"value\""
   Additionally, any characters with special meaning to the Windows shell that are not within at least one level of quote pairs must also be escaped (this can be avoided as long as you wrap the entire value of the switch in quotes as shown above):

    http://website.com/foo&bar => --param=http://website.com/foo^&bar

See http://www.robvanderwoude.com/escapechars.php for more information.

--------------------------------------------------------------------------------

### `share` - Generates a URL for starting a Flashpoint title that can be shared to other users

**Options**:
 - [Title Command](#title-commands) options
 - `-u` | `--universal` - Creates a standard HTTPS link that utilizes a redirect page. May be easier to share on some platforms.
 - `-c` | `--configure` - Registers CLIFp as the default handler for "flashpoint" protocol links. Warns if it already was.
 - `-C` | `--configured` - Ensures CLIFp is registered as the default handler for "flashpoint" protocol links (idempotent).
`--unconfigure` - Removes CLIFp as the default handler for "flashpoint" protocol links. Warns if it wasn't.
`--unconfigured` - Ensures CLIFp is not the default handler for "flashpoint" protocol links (idempotent).

**Notes**:

 - No title is required when using any of the 'configure'-type options
 - By default, the standard Flashpoint launcher is registered to handle share links; therefore, its "Register As Protocol Handler" option should likely be disabled if you intend to use CLIFp instead.

--------------------------------------------------------------------------------

 ### `show` - Display  a  message  or  extra  folder

**Options**:
 - `-m` | `--msg` - Displays an pop-up dialog with the supplied message. Used primarily for some additional apps
 - `-e` | `--extra` - Opens an explorer window to the specified extra. Used primarily for some additional apps

**Requires:**
`-m` or `-e`

--------------------------------------------------------------------------------

 ### `update` - Check for and optionally apply the latest update

**Options**: NONE