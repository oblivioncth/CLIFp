# Frontends

> [!NOTE]
> This document is a stub and will be fleshed out more at a later time.

CLIFp features two different frontends.


## GUI

The GUI-based frontend is the standard CLIFp experience. It fully supports command-line arguments for starting the application, but displays all information via pop-up dialogs and feautres a tray icon while running.

You use this frontend by using `CLIFp.exe` on Windows and `clifp` on Linux.

This frontend sports an "Assisted Mode" feature that can be activated by simply double-clicking the executable, or running it from the command-line with no command.

## Console

The console-based frontend is a fully bi-direction CLI that handles I/O entirely through the console with no GUI features whatsoever (aside from placing content in the clipboard in one particular case).

You use this frontend by using `CLIFP-c.exe` on Windows and `clifp-c` on Linux.