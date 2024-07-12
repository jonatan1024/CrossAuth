#include "memory.h"

#ifdef _WIN32

#include <Windows.h>

inline bool IsPageRW(const MEMORY_BASIC_INFORMATION* pPage) {
    return pPage->Protect == PAGE_READWRITE && pPage->State == MEM_COMMIT && pPage->Type == MEM_PRIVATE;
}

void EnumeratePages(void (*pageCallback) (void* address, int size)) {
	MEMORY_BASIC_INFORMATION mbi = {};
	LPCVOID lpAddress = NULL;
	while(VirtualQuery(lpAddress, &mbi, sizeof(mbi))) {
		if(IsPageRW(&mbi))
			pageCallback(mbi.BaseAddress, mbi.RegionSize);

		lpAddress = ((char*)mbi.BaseAddress) + mbi.RegionSize;
	}
}

bool IsAddressValid(const void* address) {
    MEMORY_BASIC_INFORMATION mbi = {};
    if(!VirtualQuery(address, &mbi, sizeof(mbi)))
        return false;
    return address >= mbi.BaseAddress && address < (char*)mbi.BaseAddress + mbi.RegionSize && IsPageRW(&mbi);
}

#else

#include <stdio.h>

inline FILE* OpenMaps() {
    return fopen("/proc/self/maps", "r");
}

inline bool GetRWPage(const char* line, unsigned int* start, unsigned int* end) {
    char r, w, x, p;
    if(sscanf(line, "%x-%x %c%c%c%c", start, end, &r, &w, &x, &p) != 6)
        return false;

    return r == 'r' && w == 'w' && x == '-' && p == 'p';
}

void EnumeratePages(void (*pageCallback) (void* address, int size)) {
    FILE* maps = OpenMaps();
    if(!maps)
        return;

    static char line[8192];
    while(fgets(line, sizeof(line), maps)) {
        unsigned int start, end;
        if(GetRWPage(line, &start, &end))
            pageCallback((void*)start, end - start);
    }

    fclose(maps);
}

bool IsAddressValid(const void* address) {
    bool retVal = false;

    FILE* maps = OpenMaps();
    if(!maps)
        return retVal;

    static char line[8192];
    while(fgets(line, sizeof(line), maps)) {
        unsigned int start, end;
        if(GetRWPage(line, &start, &end) && address >= (const void*)start && address < (const void*)end) {
            retVal = true;
            break;
        }
    }

    fclose(maps);
    return retVal;
}

#endif //_WIN32
