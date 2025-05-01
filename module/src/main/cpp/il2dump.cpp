/* Copyright 2022-2023 John "topjohnwu" Wu
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>
#include <sys/types.h>
#include <memory>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <elf.h>
#include <thread>

#include "external/il2cpp/blob.h"
#include "external/il2cpp/class-internals.h"
#include "external/il2cpp/il2cpp-api-types.h"
#include "external/il2cpp/il2cpp-config.h"
#include "external/il2cpp/il2cpp-metadata.h"
#include "external/il2cpp/il2cpp-string-types.h"
#include "external/il2cpp/il2cpp-vm-support.h"
#include "external/il2cpp/metadata.h"
#include "external/il2cpp/normalization-tables.h"
#include "external/il2cpp/number-formatter.h"
#include "external/il2cpp/object-internals.h"
#include "external/il2cpp/tabledefs.h"
#include "MountGuard.cpp"
#include <sys/syscall.h>

#include "zygisk.hpp"

#include "utils.cpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class Il2Dump : public zygisk::ModuleBase {
public:
    void onLoad(Api *_api, JNIEnv *_env) override {
        this->api = _api;
        this->env = _env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        preSpecialize(process);
        env->ReleaseStringUTFChars(args->nice_name, process);
    }

	void postAppSpecialize(const AppSpecializeArgs *args) override {
		const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
		postSpecialize(process);
		env->ReleaseStringUTFChars(args->nice_name, process);
	}

    void preServerSpecialize(ServerSpecializeArgs *args) override {
		api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api *api{};
    JNIEnv *env{};
	int fd = -1;

    void preSpecialize(const char* process) {
		fd = api->connectCompanion();
		xwrite(fd, process);
		auto pstatus = xread<bool>(fd);
		if (pstatus && *pstatus) {
			auto il2NamePtr = xread<std::string>(fd);
			if (il2NamePtr) {
				il2Name = *il2NamePtr;
				if (il2Name.empty()) {
					il2Name = "libil2cpp.so";
				}
				auto uintPtr = xread<uintptr_t>(fd);
				if (uintPtr)
					s_GlobalMetadata = (void*) *uintPtr;
				else {
					LOGE("Unexpected error during the parse of `s_GlobalMetadata`!");
					goto closing;
				}
				uintPtr = xread<uintptr_t>(fd);
				if (uintPtr)
					s_GlobalMetadataHeader = (Il2CppGlobalMetadataHeader *) *uintPtr;
				else {
					LOGE("Unexpected error during the parse of `s_GlobalMetadataHeader`!");
					goto closing;
				}
				uintPtr = xread<uintptr_t>(fd);
				if (uintPtr)
					s_Il2CppCodeRegistration = (Il2CppCodeRegistration *) *uintPtr;
				else {
					LOGE("Unexpected error during the parse of `s_Il2CppCodeRegistration`!");
					goto closing;
				}
				uintPtr = xread<uintptr_t>(fd);
				if (uintPtr)
					s_Il2CppMetadataRegistration = (Il2CppMetadataRegistration *) *uintPtr;
				else {
					LOGE("Unexpected error during the parse of `s_Il2CppMetadataRegistration`!");
					goto closing;
				}
				return;
			} else {
				LOGE("Unexpected error during the parse of library name!");
			}
		}
		closing:
		close(fd);
		fd = -1;
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

	void postSpecialize(const char *process) {
		if (fd == -1) {
			api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
			return;
		} else if (!isfd(fd)) {
			LOGE("Invalid file descriptor for companion(%d), connection is lost!", fd);
			api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
			return;
		}
		il2Package = std::string(process);
		LOGD("library=[%s]", il2Name.c_str());
		LOGD("GlobalMetadata=[%lx]", (uintptr_t) s_GlobalMetadata);
		LOGD("GlobalMetadataHeader=[%lx]", (uintptr_t) s_GlobalMetadataHeader);
		LOGD("Il2CppCodeRegistration=[%lx]", (uintptr_t) s_Il2CppCodeRegistration);
		LOGD("Il2CppMetadataRegistration=[%lx]", (uintptr_t) s_Il2CppMetadataRegistration);
		std::thread(dumpIl2, fd, api).detach();
	}

};

static void Il2Comp(int fd) {
	bool status = false;
	auto pkgName = xread<std::string>(fd);
	const char *packageName;
	if (!pkgName || access("/data/adb/modules/zygisk_il2dump/disable", F_OK) == 0) {
		error:
		xwrite(fd, status);
		close(fd);
		return;
	}
	packageName = pkgName->c_str();
	DIR* il2dump = opendir("/data/adb/il2dump");
	if (!il2dump) {
		LOGE("Unable to open `/data/adb/il2dump` (%d:%s)", errno, strerror(errno));
		goto error;
	}
	struct dirent* entry;
	while ((entry = readdir(il2dump)) != nullptr) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		if (entry->d_type == DT_DIR) {
			std::string il2Pkg(entry->d_name);
			if (il2Pkg != *pkgName)
				continue;
			std::string filePreset("/data/adb/il2dump/" + il2Pkg + "/preset.prop");
			std::string dumpCS("/data/adb/il2dump/" + il2Pkg + "/dump.cs");
			if (access(filePreset.c_str(), F_OK) == 0) {
				if (access(dumpCS.c_str(), F_OK) == 0) {
					LOGE("Dump already exists for `%s`, ignore.", il2Pkg.c_str());
					closedir(il2dump);
					goto error;
				}
				std::ifstream presetFile(filePreset);
				if (!presetFile) {
					LOGE("Unable to open preset file for `%s`, abort.", filePreset.c_str());
					closedir(il2dump);
					goto error;
				}
				std::string prop;
				std::string library;
				uintptr_t GlobalMetadata = 0, GlobalMetadataHeader = 0, Il2CppCodeRegistration = 0, Il2CppMetadataRegistration = 0;
				while (std::getline(presetFile, prop)) {
					if (prop.starts_with('#'))
						continue;
					size_t eqpos = prop.find('=');
					if (eqpos == std::string::npos)
						continue;
					std::string key = prop.substr(0, eqpos);
					std::string value = prop.substr(eqpos + 1);
					if (key == "library") {
						library = value;
					} else if (key == "s_GlobalMetadata") {
						GlobalMetadata = std::stoul(value, nullptr, 16);
						if (!GlobalMetadata) {
							GlobalMetadata = std::stoul(value, nullptr, 0);
							if (!GlobalMetadata) {
								LOGE("Invalid `s_GlobalMetadata` offset, abort!");
								closedir(il2dump);
								goto error;
							}
						}
					} else if (key == "s_GlobalMetadataHeader") {
						GlobalMetadataHeader = std::stoul(value, nullptr, 16);
						if (!GlobalMetadataHeader) {
							GlobalMetadataHeader = std::stoul(value, nullptr, 0);
							if (!GlobalMetadataHeader) {
								LOGE("Invalid `s_GlobalMetadataHeader` offset, abort!");
								closedir(il2dump);
								goto error;
							}
						}
					} else if (key == "s_Il2CppCodeRegistration") {
						Il2CppCodeRegistration = std::stoul(value, nullptr, 16);
						if (!Il2CppCodeRegistration) {
							Il2CppCodeRegistration = std::stoul(value, nullptr, 0);
							if (!Il2CppCodeRegistration) {
								LOGE("Invalid `s_Il2CppCodeRegistration` offset, abort!");
								closedir(il2dump);
								goto error;
							}
						}
					} else if (key == "s_Il2CppMetadataRegistration") {
						Il2CppMetadataRegistration = std::stoul(value, nullptr, 16);
						if (!Il2CppMetadataRegistration) {
							Il2CppMetadataRegistration = std::stoul(value, nullptr, 0);
							if (!Il2CppMetadataRegistration) {
								LOGE("Invalid `s_Il2CppMetadataRegistration` offset, abort!");
								closedir(il2dump);
								goto error;
							}
						}
					}
				}
				if (!GlobalMetadataHeader)
					GlobalMetadataHeader = GlobalMetadata;
				if (Il2CppCodeRegistration && Il2CppMetadataRegistration) {
					status = true;
					xwrite(fd, status);
					xwrite(fd, library);
					xwrite(fd, GlobalMetadata);
					xwrite(fd, GlobalMetadataHeader);
					xwrite(fd, Il2CppCodeRegistration);
					xwrite(fd, Il2CppMetadataRegistration);
					auto imagePtr = xread<std::string>(fd);
					auto countPtr = xread<std::size_t>(fd);
					if (imagePtr && countPtr) {
						auto image = *imagePtr;
						auto count = *countPtr;
						std::ofstream dump(dumpCS);
						if (!dump)
							goto abort;
						dump << image;
						for (int i = 0; i < count; ++i) {
							auto dumpPtr = xread<std::string>(fd);
							if (dumpPtr) {
								dump << *dumpPtr;
							} else {
								dump << std::endl << "// [Il2Dump]: Failed to transact vector[" << i << "]!";
							}
						}
						dump.close();
					} else {
						LOGE("Invalid pointer dereference for either (imagePtr=[%p] or countPtr=[%p])", imagePtr.get(), countPtr.get());
					}
					abort:
					closedir(il2dump);
					close(fd);
					return;
				} else {
					LOGE("Missing property of s_???, abort.");
					closedir(il2dump);
					goto error;
				}
			} else {
				LOGE("No preset exists for `%s`, abort.", packageName);
				closedir(il2dump);
				goto error;
			}
		}
	}
	closedir(il2dump);
	goto error;
}

REGISTER_ZYGISK_MODULE(Il2Dump)
REGISTER_ZYGISK_COMPANION(Il2Comp)
