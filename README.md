# Il2Dump
[![Android CI status](https://github.com/MhmRdd/Il2Dump/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/MhmRdd/Il2Dump/actions/workflows/build.yml)
[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)

A Zygisk Module to dump il2cpp/unity games based on `GlobalMetadata` (optional) & `GlobalMetadataHeader` (optional) & `MetadataRegistration` & `CodeRegistration` offsets.

> [!WARNING]
> This module breaks SELinux policy of `untrusted_app` by allowing `write` on `unix_stream_socket` class to `zygote`.<br/>
> This beceause it's used to transact vectors of dump from game to an external folder in `/data/adb/il2dump/*/dump.cs`.

## Usage

1. Flash this module and reboot.
2. Create directory named as the package name of the targeted game in `/data/adb/il2dump/`.
3. Create & fill a `preset.prop` file in the directory.
4. Run the game and observe logs using command `logcat | grep 'Il2Dump'` for any errors.<br/>
   Your `dump.cs` will be generated under the directory.<br/>
   If `dump.cs` already exists, the module will automatically ignore future dumps & close itself until deleted.

**All configuration files & folders will take effect immediately.**

## Creating `preset.prop`
format:
```properties
library=libil2cpp.so
#s_GlobalMetadata=ABCD0D0
#s_GlobalMetadataHeader=ABCD0D0
s_Il2CppCodeRegistration=ABCD0B0
s_Il2CppMetadataRegistration=ABCD0B8
```



## Finding necessary offsets
To identify required offsets for working with **libil2cpp or libunity**, you can inspect the official Unity source implementation, precisely at [MetadataCache.cpp](https://github.com/dreamanlan/il2cpp_ref/blob/master/libil2cpp/vm/MetadataCache.cpp#L143), you can look for useful **patterns or strings** to identify these structures in the binary using reverse engineering tools like **IDA, Ghidra, ...**
```cpp
void MetadataCache::Register(const Il2CppCodeRegistration* const codeRegistration, const Il2CppMetadataRegistration* const metadataRegistration, const Il2CppCodeGenOptions* const codeGenOptions)
{
    s_Il2CppCodeRegistration = codeRegistration; /* Il2CppCodeRegistration */
    s_Il2CppMetadataRegistration = metadataRegistration; /* Il2CppMetadataRegistration */
    s_Il2CppCodeGenOptions = codeGenOptions;

    for (int32_t j = 0; j < metadataRegistration->genericClassesCount; j++)
        if (metadataRegistration->genericClasses[j]->typeDefinitionIndex != kTypeIndexInvalid)
            metadata::GenericMetadata::RegisterGenericClass(metadataRegistration->genericClasses[j]);

    for (int32_t i = 0; i < metadataRegistration->genericInstsCount; i++)
        s_GenericInstSet.insert(metadataRegistration->genericInsts[i]);

    s_InteropData.assign_external(codeRegistration->interopData, codeRegistration->interopDataCount);
}

static void* s_GlobalMetadata;
static const Il2CppGlobalMetadataHeader* s_GlobalMetadataHeader;

void MetadataCache::Initialize()
{
    s_GlobalMetadata = vm::MetadataLoader::LoadMetadataFile("global-metadata.dat"); /* GlobalMetadata */
    s_GlobalMetadataHeader = (const Il2CppGlobalMetadataHeader*)s_GlobalMetadata; /* GlobalMetadataHeader */
    /* ... */
}
```
> [!TIP]
> `s_GlobalMetadata` can be obtained automatically by Il2Dump (if not inputted manually), whereas `s_GlobalMetadataHeader` is set to default (as `s_GlobalMetadata`) when not specified.

## Acknowledgement

- [Zygisk Il2CppDumper](https://github.com/Perfare/Zygisk-Il2CppDumper)
- Arcy
