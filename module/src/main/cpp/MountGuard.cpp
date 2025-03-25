//
// Created by riad8 on 08/02/2025.
//

#include <cstddef>
#include <sys/mman.h>
#include <cstdio>

class MountGuard {
private:
	bool stat;
	void* addr{};
	size_t length{};
	int prot{};

	static inline int toprot(const char *perms) {
		int prot = 0;
		if (perms[0] == 'r') prot |= PROT_READ;
		if (perms[1] == 'w') prot |= PROT_WRITE;
		if (perms[2] == 'x') prot |= PROT_EXEC;
		return prot;
	}

	bool stub(void* ptr) {
		FILE *file = fopen("/proc/self/maps", "r");
		if (!file) {
			perror("Failed to open `/proc/self/maps`.");
			return false;
		}
		char line[40];
		while (fgets(line, sizeof(line), file)) {
			uintptr_t start, end;
			char perms[5];
			if (sscanf(line, "%lx-%lx %4s",
					   &start, &end, perms) >= 3) {
				if (start <= (uintptr_t) ptr && end > (uintptr_t) ptr) {
					addr = (void*) start;
					prot = toprot(perms);
					length = end - start;
					fclose(file);
					return true;
				}
			}
		}
		fclose(file);
		return false;
	}
public:
	explicit MountGuard(void* ptr, int mount = PROT_WRITE) {
		if ((stat = stub(ptr)))
			mprotect(addr, length, prot | mount);
	}
	~MountGuard() {
		if (stat)
			mprotect(addr, length, prot);
	}
};
