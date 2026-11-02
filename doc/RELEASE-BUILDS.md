# Release Builds
For guidance on which [release](https://github.com/oblivioncth/CLIFp/releases) build is likely best for your system, compare your system to the table below and try whichever is most comparable to yours in the order shown. The compiler used is somewhat arbitrary, so equivalent compilers are combined in the table. If one build type with a given compiler does not work for you it's unlikely the other listed as part of the same line will, but you can try it if you want.

| System                                          | Builds                                                                   |
| ----------------------------------------------- | ------------------------------------------------------------------------ |
| Windows                                         | 1) Windows Static <br> 2) Windows Shared                                 |
| Arch, Steam OS, Other Frequently Updated Distro | 1) Linux Static Clang++18/GCC++14 <br> 2) Linux Shared Clang++18/GCC++14 |
| Ubuntu 24.04                                    | 1) Linux Static Clang++18/GCC++14 <br> 2) Linux Shared Clang++18/GCC++14 |
| Ubuntu 22.02                                    | 1) Linux Static Clang++15/GCC++12 <br> 2) Linux Shared Clang++15/GCC++12 |
| Ubuntu 20.04                                    | 1) Linux Static Clang++12 <br> 2) Linux Shared Clang++12                 |

> [!IMPORTANT]
> For shared builds, you must place the entire directory structure of the release in your Flashpoint folder and run CLIFp from within it's bin directory, like so:
> ```
> Flashpoint/
> ├── CLIFp/
> │   ├── bin/
> │   │   └── clifp.exe
> │   ├── cmake
> │   ├── LICENSE
> │   ├── README.md
> │   └── ...
> ├── version.txt
> ├── preferences.json
> └── ...
> ```
> For static builds you can simply take the CLIFp executable out of the bin directory and place it directly in your Flashpoint folder.

Remember these are just guidelines. Ultimately use whichever build you like, or build CLIFp yourself.
