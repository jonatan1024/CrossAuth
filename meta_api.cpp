#include <extdll.h>
#include <meta_api.h>

#pragma push_macro("_DEBUG")
#undef _DEBUG
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#include <utlvector.h>
#pragma pop_macro("_DEBUG")

#include "ex_rehlds_api.h"
#include "memory.h"

meta_globals_t *gpMetaGlobals;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;
enginefuncs_t *g_pengfuncsTable;

plugin_info_t Plugin_info =
{
	META_INTERFACE_VERSION,			// ifvers
	"CrossAuth",					// name
	"0.1",							// version
	__DATE__,						// date
	"jonatan1024",					// author
	"http://",						// url
	"XAUTH",						// logtag
	PT_CHANGELEVEL,					// (when) loadable
	PT_ANYTIME,						// (when) unloadable
};

C_DLLEXPORT int Meta_Query(char *interfaceVersion, plugin_info_t **plinfo, mutil_funcs_t *pMetaUtilFuncs)
{
	*plinfo = &Plugin_info;
	gpMetaUtilFuncs = pMetaUtilFuncs;
	return TRUE;
}

META_FUNCTIONS gMetaFunctionTable =
{
	NULL,						// pfnGetEntityAPI		HL SDK; called before game DLL
	NULL,						// pfnGetEntityAPI_Post		META; called after game DLL
	GetEntityAPI2,				// pfnGetEntityAPI2		HL SDK2; called before game DLL
	GetEntityAPI2_Post,			// pfnGetEntityAPI2_Post	META; called after game DLL
	GetNewDLLFunctions,			// pfnGetNewDLLFunctions	HL SDK2; called before game DLL
	GetNewDLLFunctions_Post,	// pfnGetNewDLLFunctions_Post	META; called after game DLL
	GetEngineFunctions,			// pfnGetEngineFunctions	META; called before HL engine
	GetEngineFunctions_Post,	// pfnGetEngineFunctions_Post	META; called after HL engine
};

int gOriginalAppId = 0;
int gPort = 0;
CUtlVector<int*> gAppIds;

void FindAppIdInPage(void* address, int size) {
	const int portOffset = 4;
	const int patternSize = sizeof(int) * portOffset;
	const void* pEnd = (const char*)address + size - patternSize;

	for(int* pCurrent = (int*)address; pCurrent < pEnd; pCurrent++) {
		if(*pCurrent != gOriginalAppId)
			continue;
		if(pCurrent == &gOriginalAppId)
			continue;
		if(*(pCurrent + portOffset) != gPort)
			continue;

		gAppIds.AddToTail(pCurrent);
	}
}

void SV_ActivateServer_hook(IRehldsHook_SV_ActivateServer* chain, int runPhysics) {
	chain->callNext(runPhysics);
	
	gAppIds.RemoveAll();

	auto appIdFile = fopen("steam_appid.txt", "rt");
	if(!appIdFile) {
		SERVER_PRINT("steam_appid.txt not found!\n");
		return;
	}

	char appIdString[16];
	fgets(appIdString, 16, appIdFile);
	gOriginalAppId = atoi(appIdString);
	fclose(appIdFile);
	gPort = (int)CVAR_GET_FLOAT("ip_hostport");
	gPort = gPort ? gPort : (int)CVAR_GET_FLOAT("hostport");
	gPort = gPort ? gPort : (int)CVAR_GET_FLOAT("port");
	if(!gPort) {
		SERVER_PRINT("unknown port!\n");
		return;
	}

	SERVER_PRINT("Searching for SteamClient AppId structure...\n");

	EnumeratePages(&FindAppIdInPage);

	if(!gAppIds.Count()) {
		SERVER_PRINT("\tSteamClient AppId structure not found! CrossAuth will be disabled!\n");
		return;
	}

	if(gAppIds.Count() > 1) {
		SERVER_PRINT("\tFound multiple SteamClient AppId structures!\n");
		return;
	}

	SERVER_PRINT("\tSuccessfully found!\n");
}

void RescanAppIds() {
	gAppIds.RemoveAll();
	EnumeratePages(&FindAppIdInPage);	
}

qboolean Steam_NotifyClientConnect_hook(IRehldsHook_Steam_NotifyClientConnect* chain, IGameClient* client, const void* pvSteam2Key, unsigned int ucbSteam2Key) {
	int clientAppId = gOriginalAppId;
	if(ucbSteam2Key >= 0x30)
		clientAppId = *(const int*)((const char*)pvSteam2Key + 0x2C);

	if(gAppIds.Count() > 1)
		RescanAppIds();
	
	for(int iAppId = 0; iAppId < gAppIds.Count(); iAppId++)
		*(gAppIds[iAppId]) = clientAppId;

	auto retVal = chain->callNext(client, pvSteam2Key, ucbSteam2Key);

	for(int iAppId = 0; iAppId < gAppIds.Count(); iAppId++)
		*(gAppIds[iAppId]) = gOriginalAppId;

	return retVal;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs)
{
	gpMetaGlobals = pMGlobals;
	gpGamedllFuncs = pGamedllFuncs;

	if(!meta_init_rehlds_api()) {
		SERVER_PRINT("ReHLDS API failed to initialize!\n");
		return FALSE;
	}

	g_RehldsHookchains->SV_ActivateServer()->registerHook(&SV_ActivateServer_hook, HC_PRIORITY_MEDIUM - 1);
	g_RehldsHookchains->Steam_NotifyClientConnect()->registerHook(&Steam_NotifyClientConnect_hook, HC_PRIORITY_MEDIUM - 1);

	memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
	return TRUE;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
	g_RehldsHookchains->SV_ActivateServer()->unregisterHook(&SV_ActivateServer_hook);
	g_RehldsHookchains->Steam_NotifyClientConnect()->unregisterHook(&Steam_NotifyClientConnect_hook);

	return TRUE;
}
