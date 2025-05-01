# Il2Dump
[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)

A Zygisk Module to dump il2cpp/unity games based on `GlobalMetadata` & `GlobalMetadataHeader` & `MetadataRegistration` & `CodeRegistration` offsets.

> [!WARNING]
> This module breaks SELinux policy of `untrusted_app` by allowing `write` on `unix_stream_socket` class to `zygote`.<br/>
> This beceause it's used to transact vectors of dump from game to an external folder in `/data/adb/il2dump/*/dump.cs`.

## Usage

1. Flash this module and reboot.
2. Create directory named as the package name of the targeted game in `/data/adb/il2dump/`.
3. Create & fill a `preset.prop` file in the directory.
4. Run the game and observe logs for the tag `Il2Dump` for any errors.<br/>
   Your `dump.cs` will be generated under the directory.<br/>
   If `dump.cs` already exists, the module will automatically ignore future dumps & close itself until deleted.

**All configuration files & folders will take effect immediately.**

## Creating `preset.prop`
All offsets should be filled, format:
```properties
library=libil2cpp.so
s_GlobalMetadata=ABCD0D0
s_GlobalMetadataHeader=ABCD0D0
s_Il2CppCodeRegistration=ABCD0B0
s_Il2CppMetadataRegistration=ABCD0B8
```

## Acknowledgement

- [Zygisk Il2CppDumper](https://github.com/Perfare/Zygisk-Il2CppDumper)
- Arcy
