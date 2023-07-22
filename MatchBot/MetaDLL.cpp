#include "precompiled.h"

// DLL PRE Functions Table
DLL_FUNCTIONS gDLL_FunctionTable_Pre;

// DLL POST Functions Table
DLL_FUNCTIONS gDLL_FunctionTable_Post;

#pragma region DLL_PRE
C_DLLEXPORT int GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	memset(&gDLL_FunctionTable_Pre, 0, sizeof(DLL_FUNCTIONS));

	gDLL_FunctionTable_Pre.pfnClientConnect = DLL_PRE_ClientConnect;
	gDLL_FunctionTable_Pre.pfnInconsistentFile = DLL_PRE_InconsistentFile;

	memcpy(pFunctionTable, &gDLL_FunctionTable_Pre, sizeof(DLL_FUNCTIONS));

	return 1;
}

BOOL DLL_PRE_ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	if (!gMatchAdmin.PlayerConnect(pEntity, pszName, pszAddress, szRejectReason))
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, FALSE);
		return FALSE;
	}

	if (!gMatchBot.PlayerConnect(pEntity, pszName, pszAddress, szRejectReason))
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, FALSE);
		return FALSE;
	}

	if (!gMatchStats.PlayerConnect(pEntity, pszName, pszAddress, szRejectReason))
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, FALSE);
		return FALSE;
	}

	RETURN_META_VALUE(MRES_IGNORED, TRUE);
	return TRUE;
}

BOOL DLL_PRE_InconsistentFile(const edict_t* pEntity, const char* pszFilename, char* szDisconnectMessage)
{
	// Notify each admin that someone tried to connect with a modified file
	for (int i = 0; i <= gpGlobals->maxClients; ++i)
	{
		auto pEdict  = INDEXENT(i);

		// Do not notify the client himself
		if (!pEdict || pEdict == pEntity)
		{
			continue;
		}

		auto pPlayer = reinterpret_cast< CBasePlayer* >(GET_PRIVATE(pEdict));

		// Do not notify if admin is dormant
		if (!pPlayer || pPlayer->IsDormant())
		{
			continue;
		}

		// Only an admin with rcon access level should be notified
		if (gMatchAdmin.Access(i, ADMIN_RCON))
		{
			// If the user who will be disconnected is an admin
			// tell this to the other connected admins
			const bool bIsAdmin = gMatchAdmin.GetFlags(ENTINDEX(pEntity));

			gMatchUtil.ClientPrint(
				pEdict,
				PRINT_CONSOLE,
				bIsAdmin ? "[ADMIN][%s] %s: %s" : "[%s] %s: %s",
				GET_USER_AUTH(((edict_t*)pEntity)),
				_T( "Modified file" ),
				pszFilename
			);
		}
	}

	// Now remove the player from the server
	gMatchUtil.DropClient(ENTINDEX(pEntity), _T("Reinstall the file: %s"), pszFilename);

	RETURN_META_VALUE(MRES_SUPERCEDE, FALSE);
}
#pragma endregion

#pragma region DLL_POST
C_DLLEXPORT int GetEntityAPI2_Post(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	memset(&gDLL_FunctionTable_Post, 0, sizeof(DLL_FUNCTIONS));

	gDLL_FunctionTable_Post.pfnServerActivate = DLL_POST_ServerActivate;

	gDLL_FunctionTable_Post.pfnServerDeactivate = DLL_POST_ServerDeactivate;

	gDLL_FunctionTable_Post.pfnStartFrame = DLL_POST_StartFrame;

	memcpy(pFunctionTable, &gDLL_FunctionTable_Post, sizeof(DLL_FUNCTIONS));

	return 1;
}

void DLL_POST_ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
	// TODO: Add support for checking multiple files
	PRECACHE_MODEL("sprites/gas_puff_01.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, nullptr, nullptr, "sprites/gas_puff_01.spr");

	gMatchTask.ServerActivate();

	gMatchCurl.ServerActivate();

	gMatchBot.ServerActivate();

	gMatchAdmin.ServerActivate();

	gMatchVoteMenu.ServerActivate();

	gMatchStats.ServerActivate();

	gMatchReport.ServerActivate();

	RETURN_META(MRES_IGNORED);
}

void DLL_POST_ServerDeactivate(void)
{
	gMatchTask.ServerDeactivate();

	gMatchCurl.ServerDeactivate();

	gMatchBot.ServerDeactivate();

	RETURN_META(MRES_IGNORED);
}

void DLL_POST_StartFrame(void)
{
	gMatchTask.ServerFrame();

	gMatchCurl.ServerFrame();

	RETURN_META(MRES_IGNORED);
}
#pragma endregion