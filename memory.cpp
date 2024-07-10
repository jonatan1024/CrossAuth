#include "memory.h"

#ifdef _WIN32

#include <Windows.h>

void EnumeratePages(void (*pageCallback) (void* address, int size)) {
	MEMORY_BASIC_INFORMATION mbi = {};
	LPCVOID lpAddress = NULL;
	while(VirtualQuery(lpAddress, &mbi, sizeof(mbi))) {
		if(mbi.Protect == PAGE_READWRITE && mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE) {
			pageCallback(mbi.BaseAddress, mbi.RegionSize);
		}
		lpAddress = ((char*)mbi.BaseAddress) + mbi.RegionSize;
	}
}

#else

#include <stdio.h>

void EnumeratePages(void (*pageCallback) (void* address, int size)) {
    FILE* maps = fopen("/proc/self/maps", "r");
    if(!maps)
        return;

    static char line[8192];
    while(fgets(line, sizeof(line), maps)) {
        int start, end;
        char r, w, x, p;
        if(sscanf(line, "%x-%x %c%c%c%c", &start, &end, &r, &w, &x, &p) != 6)
            continue;
        
        if(r != 'r' || w != 'w' || x != '-' || p != 'p')
            continue;

        pageCallback((void*)start, end - start);
    }

    fclose(maps);
}

#endif //_WIN32
