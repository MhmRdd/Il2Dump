

#ifdef DEBUG_BUILD
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Il2Dump", __VA_ARGS__)
#else
#define LOGD(...) ((void) 0)
#endif

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Il2Dump", __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, "Il2Dump", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Il2Dump", __VA_ARGS__)

constexpr int COUNTDOWN = 60;

std::string il2Package;
std::string il2Name;
uintptr_t il2Addr;
static void *s_GlobalMetadata;
static Il2CppGlobalMetadataHeader *s_GlobalMetadataHeader;
static Il2CppCodeRegistration *s_Il2CppCodeRegistration;
static Il2CppMetadataRegistration *s_Il2CppMetadataRegistration;

template <typename T>
T Read(uintptr_t address) {
	T data;
	memcpy(reinterpret_cast<void *>(&data), reinterpret_cast<void *>(address), sizeof(T));
	return data;
}

template <typename T>
T *ReadPointer(uintptr_t address) {
	if (!address) return nullptr;
	T *data = new T();
	memcpy(reinterpret_cast<void *>(data), reinterpret_cast<void *>(address), sizeof(T));
	return data;
}

static inline uintptr_t getPointer(uintptr_t address) {
	return Read<uintptr_t>(address);
}

const char *getStringFromIndex(StringIndex index) {
	IL2CPP_ASSERT(index <= s_GlobalMetadataHeader->stringCount);
	const char *strings = (static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->stringOffset) + index;
	return strings;
}

const Il2CppTypeDefinition *getTypeDefinitionFromIndex(TypeDefinitionIndex index) {
	if (index == kTypeDefinitionIndexInvalid) {
		return nullptr;
	}

	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) < s_GlobalMetadataHeader->typeDefinitionsCount / sizeof(Il2CppTypeDefinition));
	auto typeDefinitions = reinterpret_cast<const Il2CppTypeDefinition *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->typeDefinitionsOffset));
	return typeDefinitions + index;
}

const Il2CppType *getIl2CppTypeFromIndex(TypeIndex index) {
	if (index == kTypeIndexInvalid) {
		return nullptr;
	}

	IL2CPP_ASSERT(index < s_Il2CppMetadataRegistration->typesCount && "Invalid type index ");
	return s_Il2CppMetadataRegistration->types[index];
}

const Il2CppType *getInterfaceFromIndex(InterfacesIndex index) {
	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->interfacesCount / sizeof(TypeIndex));
	auto interfaceIndices = reinterpret_cast<const TypeIndex *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->interfacesOffset));
	return getIl2CppTypeFromIndex(interfaceIndices[index]);
}

const Il2CppGenericParameter *getGenericParameterFromIndex(GenericParameterIndex index) {
	if (index == kGenericParameterIndexInvalid) {
		return nullptr;
	}

	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->genericParametersCount / sizeof(Il2CppGenericParameter));
	auto genericParameters = reinterpret_cast<const Il2CppGenericParameter *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->genericParametersOffset));
	return genericParameters + index;
}

const Il2CppGenericContainer *getGenericContainerFromIndex(GenericContainerIndex index) {
	if (index == kGenericContainerIndexInvalid) {
		return nullptr;
	}

	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->genericContainersCount / sizeof(Il2CppGenericContainer));
	auto genericContainers = reinterpret_cast<const Il2CppGenericContainer *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->genericContainersOffset));
	return genericContainers + index;
}

const Il2CppFieldDefinition *getFieldDefinitionFromIndex(FieldIndex index) {
	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->fieldsCount / sizeof(Il2CppFieldDefinition));
	auto fields = reinterpret_cast<const Il2CppFieldDefinition *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->fieldsOffset));
	return fields + index;
}

const Il2CppFieldDefaultValue *getFieldDefaultValueFromIndex(FieldIndex index) {
	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->fieldDefaultValuesCount / sizeof(Il2CppFieldDefaultValue));
	auto defaultValues = reinterpret_cast<const Il2CppFieldDefaultValue *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->fieldDefaultValuesOffset));
	return defaultValues + index;
}

const uint8_t *getFieldDefaultValueDataFromIndex(FieldIndex index) {
	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->fieldAndParameterDefaultValueDataCount / sizeof(uint8_t));
	auto defaultValuesData = reinterpret_cast<const uint8_t *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->fieldAndParameterDefaultValueDataOffset));
	return defaultValuesData + index;
}

const Il2CppPropertyDefinition *getPropertyDefinitionFromIndex(PropertyIndex index) {
	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->propertiesCount / sizeof(Il2CppPropertyDefinition));
	auto properties = reinterpret_cast<const Il2CppPropertyDefinition *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->propertiesOffset));
	return properties + index;
}

const Il2CppMethodDefinition *getMethodDefinitionFromIndex(MethodIndex index) {
	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->methodsCount / sizeof(Il2CppMethodDefinition));
	auto methods = reinterpret_cast<const Il2CppMethodDefinition *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->methodsOffset));
	return methods + index;
}

Il2CppMethodPointer getMethodPointerFromIndex(MethodIndex index) {
	if (index == kMethodIndexInvalid) {
		return nullptr;
	}

	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) < s_Il2CppCodeRegistration->methodPointersCount);
	return s_Il2CppCodeRegistration->methodPointers[index];
}

const Il2CppParameterDefinition *getParameterDefinitionFromIndex(ParameterIndex index) {
	IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->parametersCount / sizeof(Il2CppParameterDefinition));
	auto parameters = reinterpret_cast<const Il2CppParameterDefinition *>((static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->parametersOffset));
	return parameters + index;
}

int32_t getFieldOffsetFromIndex(TypeIndex typeIndex, int32_t fieldIndexInType) {
	IL2CPP_ASSERT(typeIndex <= s_Il2CppMetadataRegistration->typeDefinitionsSizesCount);
	return s_Il2CppMetadataRegistration->fieldOffsets[typeIndex][fieldIndexInType];
}

std::string getModifiers(const Il2CppMethodDefinition *methodDef) {
	std::string outPut;
	auto access = methodDef->flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
	switch (access) {
		case METHOD_ATTRIBUTE_PRIVATE:
			outPut += "private ";
			break;
		case METHOD_ATTRIBUTE_PUBLIC:
			outPut += "public ";
			break;
		case METHOD_ATTRIBUTE_FAMILY:
			outPut += "protected ";
			break;
		case METHOD_ATTRIBUTE_ASSEM:
		case METHOD_ATTRIBUTE_FAM_AND_ASSEM:
			outPut += "internal ";
			break;
		case METHOD_ATTRIBUTE_FAM_OR_ASSEM:
			outPut += "protected internal ";
			break;
	}

	if ((methodDef->flags & METHOD_ATTRIBUTE_STATIC) != 0) {
		outPut += "static ";
	}

	if ((methodDef->flags & METHOD_ATTRIBUTE_ABSTRACT) != 0) {
		outPut += "abstract ";
		if ((methodDef->flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT) {
			outPut += "override ";
		}
	} else if ((methodDef->flags & METHOD_ATTRIBUTE_FINAL) != 0) {
		if ((methodDef->flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT) {
			outPut += "sealed override ";
		}
	} else if ((methodDef->flags & METHOD_ATTRIBUTE_VIRTUAL) != 0) {
		if ((methodDef->flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT) {
			outPut += "virtual ";
		} else {
			outPut += "override ";
		}
	}

	if ((methodDef->flags & METHOD_ATTRIBUTE_PINVOKE_IMPL) != 0) {
		outPut += "extern ";
	}
	return outPut;
}

std::string parseType(const Il2CppType *il2CppType) {
	switch (il2CppType->type) {
		case IL2CPP_TYPE_VOID:
			return "void";
		case IL2CPP_TYPE_BOOLEAN:
			return "bool";
		case IL2CPP_TYPE_CHAR:
			return "char";
		case IL2CPP_TYPE_I1:
			return "sbyte";
		case IL2CPP_TYPE_U1:
			return "byte";
		case IL2CPP_TYPE_I2:
			return "short";
		case IL2CPP_TYPE_U2:
			return "ushort";
		case IL2CPP_TYPE_I4:
			return "int";
		case IL2CPP_TYPE_U4:
			return "uint";
		case IL2CPP_TYPE_I8:
			return "long";
		case IL2CPP_TYPE_U8:
			return "ulong";
		case IL2CPP_TYPE_R4:
			return "float";
		case IL2CPP_TYPE_R8:
			return "double";
		case IL2CPP_TYPE_STRING:
			return "string";
		case IL2CPP_TYPE_TYPEDBYREF:
			return "TypedReference";
		case IL2CPP_TYPE_I:
			return "IntPtr";
		case IL2CPP_TYPE_U:
			return "UIntPtr";
		case IL2CPP_TYPE_OBJECT:
			return "object";
		case IL2CPP_TYPE_END:
			break;
		case IL2CPP_TYPE_PTR:
			break;
		case IL2CPP_TYPE_BYREF:
			break;
		case IL2CPP_TYPE_VALUETYPE:
			break;
		case IL2CPP_TYPE_CLASS:
			break;
		case IL2CPP_TYPE_VAR:
			break;
		case IL2CPP_TYPE_ARRAY:
			break;
		case IL2CPP_TYPE_GENERICINST:
			break;
		case IL2CPP_TYPE_FNPTR:
			break;
		case IL2CPP_TYPE_SZARRAY:
			break;
		case IL2CPP_TYPE_MVAR:
			break;
		case IL2CPP_TYPE_CMOD_REQD:
			break;
		case IL2CPP_TYPE_CMOD_OPT:
			break;
		case IL2CPP_TYPE_INTERNAL:
			break;
		case IL2CPP_TYPE_MODIFIER:
			break;
		case IL2CPP_TYPE_SENTINEL:
			break;
		case IL2CPP_TYPE_PINNED:
			break;
		case IL2CPP_TYPE_ENUM:
			break;
	}
	return "";
}

std::string stringJoin(const std::vector<std::string> &v, const std::string& delimiter) {
	std::string outPut;
	if (!v.empty() && !delimiter.empty()) {
		for (int i = 0; i < v.size(); i++) {
			if (v[i].empty()) {
				continue;
			}

			outPut += v[i];
			if (i < v.size() - 1) {
				outPut += delimiter;
			}
		}
		return outPut;
	}
	return "";
}

std::string getTypeName(const Il2CppType *il2CppType, bool addNamespace, bool isNested);
std::string getGenericInstParams(const Il2CppGenericInst *genericInst) {
	std::vector<std::string> genericParameterNames;
	for (uint32_t i = 0; i < genericInst->type_argc; i++) {
		auto il2CppType = genericInst->type_argv[i];
		genericParameterNames.push_back(getTypeName(il2CppType, false, false));
	}
	return std::string("<").append(stringJoin(genericParameterNames, ", ")).append(">");
}

std::string getGenericContainerParams(const Il2CppGenericContainer *genericContainer) {
	std::vector<std::string> genericParameterNames;
	for (int32_t i = 0; i < genericContainer->type_argc; i++) {
		auto genericParameterIndex = genericContainer->genericParameterStart + i;
		auto genericParameter = getGenericParameterFromIndex(genericParameterIndex);
		genericParameterNames.emplace_back(getStringFromIndex(genericParameter->nameIndex));
	}
	return std::string("<").append(stringJoin(genericParameterNames, ", ")).append(">");
}

std::string getTypeName(const Il2CppType *il2CppType, bool addNamespace, bool isNested) {
	switch (il2CppType->type) {
		case IL2CPP_TYPE_ARRAY: {
			auto arrayType = il2CppType->data.array;
			auto elementType = arrayType->etype;
			return getTypeName(elementType, addNamespace, false).append("[" + std::string(",", arrayType->rank - 1) + "]");
		}

		case IL2CPP_TYPE_SZARRAY: {
			auto elementType = il2CppType->data.type;
			return getTypeName(elementType, addNamespace, false).append("[]");
		}

		case IL2CPP_TYPE_PTR: {
			auto oriType = il2CppType->data.type;
			return getTypeName(oriType, addNamespace, false).append("*");
		}

		case IL2CPP_TYPE_VAR:
		case IL2CPP_TYPE_MVAR: {
			auto param = getGenericParameterFromIndex(il2CppType->data.genericParameterIndex);
			return getStringFromIndex(param->nameIndex);
		}

		case IL2CPP_TYPE_CLASS:
		case IL2CPP_TYPE_VALUETYPE:
		case IL2CPP_TYPE_GENERICINST: {
			std::string str;
			const Il2CppTypeDefinition *typeDefinition;
			const Il2CppGenericClass *genericClass = nullptr;
			if (il2CppType->type == IL2CPP_TYPE_GENERICINST) {
				genericClass = il2CppType->data.generic_class;
				typeDefinition = getTypeDefinitionFromIndex(genericClass->typeDefinitionIndex);
			} else {
				typeDefinition = getTypeDefinitionFromIndex(il2CppType->data.klassIndex);
			}

			if (typeDefinition->declaringTypeIndex != -1) {
				str += getTypeName(getIl2CppTypeFromIndex(typeDefinition->declaringTypeIndex), addNamespace, true);
				str += '.';
			} else if (addNamespace) {
				auto namespaceName = std::string(getStringFromIndex(typeDefinition->namespaceIndex));
				if (!namespaceName.empty()) {
					str += namespaceName + ".";
				}
			}

			auto typeName = std::string(getStringFromIndex(typeDefinition->nameIndex));
			auto index = typeName.find('`');
			if (index != std::string::npos) {
				str += typeName.substr(0, index);
			} else {
				str += typeName;
			}

			if (isNested) {
				return str;
			}

			if (genericClass != nullptr) {
				auto genericInst = genericClass->context.class_inst;
				str += getGenericInstParams(genericInst);
			} else if (typeDefinition->genericContainerIndex >= 0) {
				auto genericContainer = getGenericContainerFromIndex(typeDefinition->genericContainerIndex);
				str += getGenericContainerParams(genericContainer);
			}
			return str;
		} default: {
			return parseType(il2CppType);
		}
	}
}

std::string getTypeDefName(const Il2CppTypeDefinition *typeDefinition, bool addNamespace, bool genericParameter) {
	std::string prefix;
	if (typeDefinition->declaringTypeIndex != -1) {
		prefix = getTypeName(getIl2CppTypeFromIndex(typeDefinition->declaringTypeIndex), addNamespace, true) + ".";
	} else if (addNamespace) {
		auto namespaceStr = std::string(getStringFromIndex(typeDefinition->namespaceIndex));
		if (!namespaceStr.empty()) {
			prefix = namespaceStr + ".";
		}
	}

	auto typeName = std::string(getStringFromIndex(typeDefinition->nameIndex));
	if (typeDefinition->genericContainerIndex >= 0) {
		auto index = typeName.find('`');
		if (index != std::string::npos) {
			typeName = typeName.substr(0, index);
		}

		if (genericParameter) {
			auto genericContainer = getGenericContainerFromIndex(typeDefinition->genericContainerIndex);
			typeName += getGenericContainerParams(genericContainer);
		}
	}
	return prefix + typeName;
}

bool isfd(int fd) {
	return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

char* readstr(int fd) {
	char *result = NULL;
	size_t length = 0;
	size_t capacity = 16;
	result = (char*) malloc(capacity);
	if (!result) {
		LOGE("Memory allocation failed!");
		return nullptr;
	}
	char buffer;
	ssize_t bytesRead;
	while ((bytesRead = read(fd, &buffer, 1)) > 0) {
		if (buffer == '\0') {
			break;
		}
		if (length + 1 >= capacity) {
			capacity *= 2;
			char *temp = (char*) realloc(result, capacity);
			if (!temp) {
				LOGE("Memory reallocation failed!");
				free(result);
				return nullptr;
			}
			result = temp;
		}
		result[length++] = buffer;
	}
	if (bytesRead < 0) {
		LOGE("Read error!");
		free(result);
		return nullptr;
	}
	result[length] = '\0';
	return result;
}

void writestr(int fd, char* src) {
	write(fd, src, strlen(src));
	write(fd, "\0", sizeof(char));
}

template <typename T>
bool xwrite(int fd, const T& data) {
	uint64_t size = sizeof(T);
	if (write(fd, &size, sizeof(size)) != sizeof(size)) {
		return false;
	}
	if (write(fd, data.data(), size) != static_cast<ssize_t>(size)) {
		return false;
	}
	return true;
}

template<>
bool xwrite<std::string>(int fd, const std::string& data) {
	uint64_t size = data.size();
	if (write(fd, &size, sizeof(size)) != sizeof(size)) {
		return false;
	}
	if (write(fd, data.data(), size) != static_cast<ssize_t>(size)) {
		return false;
	}
	return true;
}

bool xwrite(int fd, const char* data) {
	if (!data) return false;
	return xwrite(fd, std::string(data));
}

template <>
bool xwrite<bool>(int fd, const bool& data) {
	uint64_t size = sizeof(bool);
	if (write(fd, &size, sizeof(size)) != sizeof(size)) {
		return false;
	}
	uint8_t byteData = data ? 1 : 0;
	if (write(fd, &byteData, sizeof(byteData)) != sizeof(byteData)) {
		return false;
	}
	return true;
}

template <>
bool xwrite<uintptr_t>(int fd, const uintptr_t& data) {
	uint64_t size = sizeof(uintptr_t);
	if (write(fd, &size, sizeof(size)) != sizeof(size)) {
		return false;
	}
	if (write(fd, &data, size) != size) {
		return false;
	}
	return true;
}

template <typename T>
std::unique_ptr<T> xread(int fd) {
	uint64_t size = 0;
	if (read(fd, &size, sizeof(size)) != sizeof(size)) {
		return nullptr;
	}
	if (size != sizeof(T)) {
		return nullptr;
	}
	auto data = std::make_unique<T>();
	if (read(fd, data.get(), size) != static_cast<ssize_t>(size)) {
		return nullptr;
	}
	return data;
}

template<>
std::unique_ptr<std::string> xread<std::string>(int fd) {
	uint64_t size = 0;
	if (read(fd, &size, sizeof(size)) != sizeof(size)) {
		return nullptr;
	}
	auto data = std::make_unique<std::string>(size, '\0');
	if (read(fd, data->data(), size) != static_cast<ssize_t>(size)) {
		return nullptr;
	}
	return data;
}

template <>
std::unique_ptr<bool> xread<bool>(int fd) {
	uint64_t size = 0;
	if (read(fd, &size, sizeof(size)) != sizeof(size)) {
		return nullptr;
	}
	if (size != sizeof(bool)) {
		return nullptr;
	}
	uint8_t byteData = 0;
	if (read(fd, &byteData, sizeof(byteData)) != sizeof(byteData)) {
		return nullptr;
	}
	return std::make_unique<bool>(byteData != 0);
}

template <>
std::unique_ptr<uintptr_t> xread<uintptr_t>(int fd) {
	uint64_t size = 0;
	if (read(fd, &size, sizeof(size)) != sizeof(size)) {
		return nullptr;
	}
	if (size != sizeof(uintptr_t)) {
		return nullptr;
	}
	uintptr_t data = 0;
	if (read(fd, &data, size) != size) {
		return nullptr;
	}
	return std::make_unique<uintptr_t>(data);
}

static bool monitorIl2Addr() {
	if (il2Name.empty()) {
		LOGE("Invalid library name to dump, abort!");
		return false;
	}
	for (int i = 0; i < COUNTDOWN; i++) {
		std::ifstream maps("/proc/self/maps");
		std::string line;
		if (!maps) {
			LOGF("Unable to open `/proc/self/maps`, abort!");
			return false;
		} else if (!maps.is_open()) {
			LOGE("Unable to open `/proc/self/maps`, abort!");
			return false;
		}
		while (std::getline(maps, line)) {
			std::istringstream iss(line);
			std::string addr_range, perms, offset, dev, inode, path;
			uintptr_t start;
			if (!(iss >> addr_range >> perms >> offset >> dev >> inode)) continue;
			std::getline(iss >> std::ws, path);
			if (perms.find('r') != std::string::npos && perms.find('x') != std::string::npos) {
				size_t dash = addr_range.find('-');
				if (dash == std::string::npos) continue;
				start = std::stoul(addr_range.substr(0, dash), nullptr, 16);
				if (path.starts_with("/data/app/") &&
					path.find(il2Package) != std::string::npos &&
					path.ends_with("/" + il2Name)
					) {
					if (!memcmp((void *) start, ELFMAG, SELFMAG)) {
						il2Addr = start;
						maps.close();
						return true;
					}
				}
			}
		}
		maps.close();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return false;
}

static void *getGlobalMetadata() {
	if (s_GlobalMetadata)
		return (void*) getPointer(il2Addr + (uintptr_t) s_GlobalMetadata);
	std::ifstream maps("/proc/self/maps");
	std::string line;
	if (!maps) {
		LOGF("Unable to open `/proc/self/maps`, abort!");
		return nullptr;
	} else if (!maps.is_open()) {
		LOGE("Unable to open `/proc/self/maps`, abort!");
		return nullptr;
	}
	while (std::getline(maps, line)) {
		std::istringstream iss(line);
		std::string addr_range, perms, offset, dev, inode, path;
		uintptr_t start;
		if (!(iss >> addr_range >> perms >> offset >> dev >> inode)) continue;
		std::getline(iss >> std::ws, path);
		if (perms[0] == 'r' || perms[1] == 'w') {
			size_t dash = addr_range.find('-');
			if (dash == std::string::npos) continue;
			start = std::stoul(addr_range.substr(0, dash), nullptr, 16);
			if (path.find(il2Package) != std::string::npos &&
				path.ends_with("/il2cpp/Metadata/global-metadata.dat")
				) {
				maps.close();
				return (void*) start;
			}
		}
	}
	maps.close();
	return nullptr;
}

void dumpIl2(int fd, zygisk::Api* api) {
	if (!monitorIl2Addr()) {
		error:
		LOGE("Failed to dump il2 for `%s:%s`!", il2Package.c_str(), il2Name.c_str());
		close(fd);
		api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
		return;
	} else if (!il2Addr) {
		LOGE("Invalid `%s` base address!", il2Name.c_str());
		goto error;
	}
	std::this_thread::sleep_for(std::chrono::seconds(2));
	MountGuard rodata((void*) (il2Addr + (uintptr_t) s_GlobalMetadata), PROT_READ);
	s_GlobalMetadata = getGlobalMetadata();
	if (!s_GlobalMetadata) {
		LOGE("Invalid pointer dereference at `GlobalMetadata`!");
		goto error;
	}
	s_GlobalMetadataHeader = s_GlobalMetadataHeader ?
			ReadPointer<Il2CppGlobalMetadataHeader>(
			 getPointer(il2Addr + (uintptr_t) s_GlobalMetadataHeader)
			 ) : ReadPointer<Il2CppGlobalMetadataHeader>(
			 (uintptr_t ) s_GlobalMetadata
			);
	if (!s_GlobalMetadataHeader) {
		LOGE("Invalid pointer dereference at `GlobalMetadataHeader`!");
		goto error;
	}
	s_Il2CppMetadataRegistration = ReadPointer<Il2CppMetadataRegistration>(
			getPointer(il2Addr + (uintptr_t) s_Il2CppMetadataRegistration));
	if (!s_Il2CppMetadataRegistration) {
		LOGE("Invalid pointer dereference at `Il2CppMetadataRegistration`!");
		goto error;
	}
	s_Il2CppCodeRegistration = ReadPointer<Il2CppCodeRegistration>(
			getPointer(il2Addr + (uintptr_t) s_Il2CppCodeRegistration));
	if (!s_Il2CppCodeRegistration) {
		LOGE("Invalid pointer dereference at `Il2CppCodeRegistration`!");
		goto error;
	}
	std::vector<std::string> outPuts;
	std::stringstream imageOutput;
	auto imagesDefinitions = reinterpret_cast<const Il2CppImageDefinition *>(static_cast<const char *>(s_GlobalMetadata) + s_GlobalMetadataHeader->imagesOffset);
	for (int32_t imageIndex = 0; imageIndex < s_GlobalMetadataHeader->imagesCount / sizeof(Il2CppImageDefinition); imageIndex++) {
		auto imageDefinition = imagesDefinitions + imageIndex;
		auto imageName = getStringFromIndex(imageDefinition->nameIndex);
		imageOutput << "// Image " << imageIndex << ": " << imageName << " - " << imageDefinition->typeStart << "\n";

		auto typeEnd = imageDefinition->typeStart + imageDefinition->typeCount;
		for (TypeDefinitionIndex typeDefinitionIndex = imageDefinition->typeStart; typeDefinitionIndex < typeEnd; typeDefinitionIndex++) {
			auto typeDefinition = getTypeDefinitionFromIndex(typeDefinitionIndex);
			auto isValueType = (typeDefinition->bitfield & 0x1) == 1;
			auto isEnum = ((typeDefinition->bitfield >> 1) & 0x1) == 1;

			std::vector<std::string> extends;
			if (typeDefinition->parentIndex >= 0) {
				auto parentName = getTypeName(getIl2CppTypeFromIndex(typeDefinition->parentIndex), false, false);
				if (!isValueType && !isEnum && parentName != "object") {
					extends.emplace_back(parentName);
				}
			}

			if (typeDefinition->interfaces_count > 0) {
				for (uint16_t i = 0; i < typeDefinition->interfaces_count; i++) {
					auto interface = getInterfaceFromIndex(typeDefinition->interfacesStart + i);
					extends.emplace_back(getTypeName(interface, false, false));
				}
			}

			outPuts.emplace_back(std::string("\n// Namespace: ") + getStringFromIndex(typeDefinition->namespaceIndex) + "\n");
			if ((typeDefinition->flags & TYPE_ATTRIBUTE_SERIALIZABLE) != 0) {
				outPuts.emplace_back("[Serializable]\n");
			}

			auto visibility = typeDefinition->flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
			switch (visibility) {
				case TYPE_ATTRIBUTE_PUBLIC:
				case TYPE_ATTRIBUTE_NESTED_PUBLIC:
					outPuts.emplace_back("public ");
					break;
				case TYPE_ATTRIBUTE_NOT_PUBLIC:
				case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
				case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:
					outPuts.emplace_back("internal ");
					break;
				case TYPE_ATTRIBUTE_NESTED_PRIVATE:
					outPuts.emplace_back("private ");
					break;
				case TYPE_ATTRIBUTE_NESTED_FAMILY:
					outPuts.emplace_back("protected ");
					break;
				case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:
					outPuts.emplace_back("protected internal ");
					break;
			}

			if ((typeDefinition->flags & TYPE_ATTRIBUTE_ABSTRACT) != 0 && (typeDefinition->flags & TYPE_ATTRIBUTE_SEALED) != 0) {
				outPuts.emplace_back("static ");
			} else if ((typeDefinition->flags & TYPE_ATTRIBUTE_INTERFACE) == 0 && (typeDefinition->flags & TYPE_ATTRIBUTE_ABSTRACT) != 0) {
				outPuts.emplace_back("abstract ");
			} else if (!isValueType && !isEnum && (typeDefinition->flags & TYPE_ATTRIBUTE_SEALED) != 0) {
				outPuts.emplace_back("sealed ");
			}

			if ((typeDefinition->flags & TYPE_ATTRIBUTE_INTERFACE) != 0) {
				outPuts.emplace_back("interface ");
			} else if (isEnum) {
				outPuts.emplace_back("enum ");
			} else if (isValueType) {
				outPuts.emplace_back("struct ");
			} else {
				outPuts.emplace_back("class ");
			}

			outPuts.emplace_back(getTypeDefName(typeDefinition, false, true));
			if (!extends.empty()) {
				outPuts.emplace_back(" : " + stringJoin(extends, ", "));
			}

			outPuts.emplace_back(" // TypeDefIndex: " + std::to_string(typeDefinitionIndex) + "\n{");
			if (typeDefinition->field_count > 0) {
				outPuts.emplace_back("\n\t// Fields\n");
				auto fieldEnd = typeDefinition->fieldStart + typeDefinition->field_count;
				for (auto fieldIndex = typeDefinition->fieldStart; fieldIndex < fieldEnd; fieldIndex++) {
					auto fieldDefinition = getFieldDefinitionFromIndex(fieldIndex);
					auto fieldType = getIl2CppTypeFromIndex(fieldDefinition->typeIndex);
					bool isStatic = false;
					bool isConst = false;
					outPuts.emplace_back("\t");

					auto access = fieldType->attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
					switch (access) {
						case FIELD_ATTRIBUTE_PRIVATE:
							outPuts.emplace_back("private ");
							break;
						case FIELD_ATTRIBUTE_PUBLIC:
							outPuts.emplace_back("public ");
							break;
						case FIELD_ATTRIBUTE_FAMILY:
							outPuts.emplace_back("protected ");
							break;
						case FIELD_ATTRIBUTE_ASSEMBLY:
						case FIELD_ATTRIBUTE_FAM_AND_ASSEM:
							outPuts.emplace_back("internal ");
							break;
						case FIELD_ATTRIBUTE_FAM_OR_ASSEM:
							outPuts.emplace_back("protected internal ");
							break;
					}

					if ((fieldType->attrs & FIELD_ATTRIBUTE_LITERAL) != 0) {
						isConst = true;
						outPuts.emplace_back("const ");
					} else {
						if ((fieldType->attrs & FIELD_ATTRIBUTE_STATIC) != 0) {
							isStatic = true;
							outPuts.emplace_back("static ");
						}
						if ((fieldType->attrs & FIELD_ATTRIBUTE_INIT_ONLY) != 0) {
							outPuts.emplace_back("readonly ");
						}
					}

					outPuts.emplace_back(getTypeName(fieldType, false, false).append(" ").append(getStringFromIndex(fieldDefinition->nameIndex)));
					auto fieldOffset = getFieldOffsetFromIndex(typeDefinitionIndex, (fieldIndex - typeDefinition->fieldStart));
					if (!isConst) {
						if (fieldOffset > 0) {
							if (isValueType && !isStatic) {
								fieldOffset -= 16;
							}
						}

						std::stringstream temp;
						temp << std::hex << fieldOffset;
						outPuts.emplace_back("; // 0x").append(temp.str()).append("\n");
					} else {
						outPuts.emplace_back(";\n");
					}
				}
			}

			if (typeDefinition->property_count > 0) {
				outPuts.emplace_back("\n\t// Properties\n");
				uint16_t propertyEnd = typeDefinition->propertyStart + typeDefinition->property_count;
				for (uint16_t i = typeDefinition->propertyStart; i < propertyEnd; i++) {
					auto propertyDefinition = getPropertyDefinitionFromIndex(i);
					outPuts.emplace_back("\t");
					if (propertyDefinition->get >= 0) {
						auto methodDefinition = getMethodDefinitionFromIndex(typeDefinition->methodStart + propertyDefinition->get);
						outPuts.emplace_back(getModifiers(methodDefinition));
						auto propertyType = getIl2CppTypeFromIndex(methodDefinition->returnType);
						outPuts.emplace_back(getTypeName(propertyType, false, false)).append(" ").append(getStringFromIndex(propertyDefinition->nameIndex)).append(" { ");
					} else if (propertyDefinition->set >= 0) {
						auto methodDefinition = getMethodDefinitionFromIndex(typeDefinition->methodStart + propertyDefinition->set);
						outPuts.emplace_back(getModifiers(methodDefinition));
						auto parameterDefinition = getParameterDefinitionFromIndex(methodDefinition->parameterStart);
						auto propertyType = getIl2CppTypeFromIndex(parameterDefinition->typeIndex);
						outPuts.emplace_back(getTypeName(propertyType, false, false)).append(" ").append(getStringFromIndex(propertyDefinition->nameIndex)).append(" { ");
					}

					if (propertyDefinition->get >= 0) {
						outPuts.emplace_back("get; ");
					}

					if (propertyDefinition->set >= 0) {
						outPuts.emplace_back("set; ");
					}

					outPuts.emplace_back("}");
					outPuts.emplace_back("\n");
				}
			}

			if (typeDefinition->method_count > 0) {
				outPuts.emplace_back("\n\t// Methods\n");
				auto methodEnd = typeDefinition->methodStart + typeDefinition->method_count;
				for (auto methodIndex = typeDefinition->methodStart; methodIndex < methodEnd; methodIndex++) {
					auto methodDefinition = getMethodDefinitionFromIndex(methodIndex);
					auto isAbstract = (methodDefinition->flags & METHOD_ATTRIBUTE_ABSTRACT) != 0;
					auto methodPointer = getMethodPointerFromIndex(methodDefinition->methodIndex);
					if (!isAbstract && reinterpret_cast<uintptr_t>(methodPointer) > 0) {
						std::stringstream temp;
						temp << "\t// RVA: 0x";
						temp << std::hex << reinterpret_cast<uintptr_t>(methodPointer) - il2Addr;
						temp << " VA: 0x";
						temp << std::hex << reinterpret_cast<uintptr_t>(methodPointer);
						outPuts.emplace_back(temp.str());
					} else {
						outPuts.emplace_back("\t// RVA: -1 VA: -1");
					}

					if (methodDefinition->slot != USHRT_MAX) {
						outPuts.emplace_back(" Slot: ");
						outPuts.emplace_back(std::to_string(methodDefinition->slot));
					}

					outPuts.emplace_back("\n");
					outPuts.emplace_back("\t");
					outPuts.emplace_back(getModifiers(methodDefinition));

					auto methodReturnType = getIl2CppTypeFromIndex(methodDefinition->returnType);
					auto methodName = std::string(getStringFromIndex(methodDefinition->nameIndex));
					if (methodDefinition->genericContainerIndex >= 0) {
						auto genericContainer = getGenericContainerFromIndex(methodDefinition->genericContainerIndex);
						methodName += getGenericContainerParams(genericContainer);
					}

					if (methodReturnType->byref == 1) {
						outPuts.emplace_back("ref ");
					}

					outPuts.emplace_back(getTypeName(methodReturnType, false, false).append(" ").append(methodName).append("("));
					std::vector<std::string> parameterStrs;
					for (auto j = 0; j < methodDefinition->parameterCount; j++) {
						std::string parameterStr;
						auto parameterDefinition = getParameterDefinitionFromIndex(methodDefinition->parameterStart + j);
						auto parameterName = getStringFromIndex(parameterDefinition->nameIndex);
						auto parameterType = getIl2CppTypeFromIndex(parameterDefinition->typeIndex);
						auto parameterTypeName = getTypeName(parameterType, false, false);

						if (parameterType->byref == 1) {
							if ((parameterType->attrs & PARAM_ATTRIBUTE_OUT) != 0 && (parameterType->attrs & PARAM_ATTRIBUTE_IN) == 0) {
								parameterStr += "out ";
							} else if ((parameterType->attrs & PARAM_ATTRIBUTE_OUT) == 0 && (parameterType->attrs & PARAM_ATTRIBUTE_IN) != 0) {
								parameterStr += "in ";
							} else {
								parameterStr += "ref ";
							}
						} else {
							if ((parameterType->attrs & PARAM_ATTRIBUTE_IN) != 0) {
								parameterStr += "[In] ";
							}
							if ((parameterType->attrs & PARAM_ATTRIBUTE_OUT) != 0) {
								parameterStr += "[Out] ";
							}
						}

						parameterStr += parameterTypeName.append(" ").append(parameterName);
						parameterStrs.emplace_back(parameterStr);
					}

					outPuts.emplace_back(stringJoin(parameterStrs, ", "));
					if (isAbstract) {
						outPuts.emplace_back(");\n");
					} else {
						outPuts.emplace_back(") { }\n");
					}
				}
			}
			outPuts.emplace_back("}\n");
		}
	}
	xwrite(fd, imageOutput.str());
	xwrite(fd, outPuts.size());
	auto count = outPuts.size();
	for (int i = 0; i < count; ++i) {
		xwrite(fd, outPuts[i]);
	}
	close(fd);
	api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
}