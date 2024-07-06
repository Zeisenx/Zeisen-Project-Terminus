
#if defined _zp_tools_included_
  #endinput
#endif
#define _zp_tools_included_

#include <sdktools>
#include <sdkhooks>
#include <cstrike>

#define ZT_MIN(%0,%1) ((%0) > (%1) ? (%1) : (%0))
#define ZT_MAX(%0,%1) ((%0) < (%1) ? (%1) : (%0))
#define ZT_LoopValidClients(%1) for (int %1=1; %1<=MaxClients; %1++) if (IsClientInGame(%1))
#define ZT_IsClient(%1) (%1 > 0 && %1 <= MAXPLAYERS)

// IsValidHandle() is deprecated, let's do a real check then...
// By Thraaawn
stock bool IsValidPlugin(Handle hPlugin) {
	if(hPlugin == null)
		return false;

	Handle hIterator = GetPluginIterator();

	bool bPluginValid = false;
	while(MorePlugins(hIterator)) {
		Handle hLoadedPlugin = ReadPlugin(hIterator);
		if(hLoadedPlugin == hPlugin) {
			bPluginValid = GetPluginStatus(hLoadedPlugin) == Plugin_Running;
			break;
		}
	}

	delete hIterator;

	return bPluginValid;
}

stock bool MenuLoadConfig(Menu menu, KeyValues kv)
{
	bool onlySort = !!kv.GetNum("config/only_sort", 1);
	
	kv.Rewind();
	if (!kv.JumpToKey("items"))
		return false;
		
	if (!kv.GotoFirstSubKey()) {
		kv.Rewind();
		return false;
	}
	
	KeyValues dummyKV = new KeyValues("dummy_kv");
	
	do {
		char keyName[64], display[256];
		int style;
		
		kv.GetString("display", display, sizeof(display));
		style = kv.GetNum("style", ITEMDRAW_DEFAULT);
		
		dummyKV.JumpToKey(keyName, true);
		if (display[0] != EOS)
			dummyKV.SetString("display", display);
		if (style != ITEMDRAW_DEFAULT)
			dummyKV.SetNum("style", ITEMDRAW_DEFAULT);
		dummyKV.SetNum("from_config", 1);
		dummyKV.Rewind();
	}
	while (kv.GotoNextKey())
	kv.Rewind();
	
	int itemCount = menu.ItemCount;
	for (int i=0; i<itemCount; i++) {
		char keyName[64], display[256];
		int style;
		menu.GetItem(i, keyName, sizeof(keyName), style, display, sizeof(display));
		
		dummyKV.JumpToKey(keyName, true);
		dummyKV.SetString("display", display);
		dummyKV.SetNum("style", style);
		dummyKV.SetNum("from_config", 0);
		dummyKV.Rewind();
	}
	
	menu.RemoveAllItems();
	if (dummyKV.GotoFirstSubKey()) {
		do {
			if (onlySort && dummyKV.GetNum("from_config", 1))
				continue;
			
			char keyName[64], display[256];
			int style;
			
			dummyKV.GetSectionName(keyName, sizeof(keyName));
			dummyKV.GetString("display", display, sizeof(display));
			style = dummyKV.GetNum("style", ITEMDRAW_DEFAULT);
			
			menu.AddItem(keyName, display, style);
		}
		while (dummyKV.GotoNextKey())
	}
	
	delete dummyKV;
	return true;
}

stock ZT_CopyArray(any[] fArray, int size, any[] sArray)
{
	for (int i=0; i<size; i++)
		fArray[i] = sArray[i];
}

stock void ZT_AddCash(int client, int value)
{
	int cash = GetEntProp(client, Prop_Send, "m_iAccount");
	cash += value;
	
	ConVar maxMoney = FindConVar("mp_maxmoney");
	if (maxMoney != null && cash > maxMoney.IntValue)
		cash = maxMoney.IntValue;
	
	SetEntProp(client, Prop_Send, "m_iAccount", cash);
}

stock void ZT_SetModelScale(int entity, float scale)
{
	float oldScale = GetEntPropFloat(entity, Prop_Send, "m_flModelScale");
	float scaleRatio = scale / oldScale;
	
	float min[3], max[3], scaledMin[3], scaledMax[3];
	GetEntPropVector(entity, Prop_Data, "m_vecMins", min);
	GetEntPropVector(entity, Prop_Data, "m_vecMaxs", max);
	
	ZT_CopyArray(scaledMin, sizeof(scaledMin), min);
	ZT_CopyArray(scaledMax, sizeof(scaledMax), max);
	ScaleVector(scaledMin, scaleRatio);
	ScaleVector(scaledMax, scaleRatio);
	
	SetEntPropFloat(entity, Prop_Send, "m_flModelScale", scale);
	SetEntPropVector(entity, Prop_Send, "m_vecSpecifiedSurroundingMins", scaledMin);
	SetEntPropVector(entity, Prop_Send, "m_vecSpecifiedSurroundingMaxs", scaledMax);
}


stock int ZT_CreateProjectile(const char[] entName, int owner, const char[] modelName)
{
	int ent = CreateEntityByName(entName);
	
	DispatchSpawn(ent);
	SetEntPropEnt(ent, Prop_Send, "m_hOwnerEntity", owner);
	SetEntityMoveType(ent, MOVETYPE_FLY);
	
	SetEntProp(ent, Prop_Send, "m_usSolidFlags", 12);
	SetEntProp(ent, Prop_Send, "m_CollisionGroup", 1);
	
	SetEntityModel(ent, modelName);
	
	return ent;
}

stock ZT_GetKnockbackVelocity(const int client, const float pos[3], float velocity[3])
{
	float client_pos[3];
	GetClientAbsOrigin(client, client_pos);
	
	float vec_attacker_to_client[3];
	MakeVectorFromPoints(pos, client_pos, vec_attacker_to_client);
	
	NormalizeVector(vec_attacker_to_client, velocity);
}

stock ZT_GetKnockbackVelocity2(const float firstPos[3], const float pos[3], float velocity[3])
{
	float vec_attacker_to_client[3];
	MakeVectorFromPoints(firstPos, pos, vec_attacker_to_client);
	
	NormalizeVector(vec_attacker_to_client, velocity);
}

stock ZT_KnockbackPlayer(const int client, const float pos[3], const float knockback, const bool is2D = false)
{
	float client_pos[3];
	GetClientAbsOrigin(client, client_pos);
	
	float vec_knockback[3];
	ZT_GetKnockbackVelocity(client, pos, vec_knockback);
	ScaleVector(vec_knockback, knockback);
	
	if (is2D)
		vec_knockback[2] = 0.0;
	
	float clientVec[3];
	GetEntPropVector(client, Prop_Data, "m_vecVelocity", clientVec);
	
	AddVectors(clientVec, vec_knockback, vec_knockback);

	TeleportEntity(client, NULL_VECTOR, NULL_VECTOR, vec_knockback);
}

stock void ZT_KnockbackPlayerByVelocity(const int client, const float vel[3], const float knockback, const bool is2D = false)
{
	float vec_knockback[3];
	NormalizeVector(vel, vec_knockback);
	ScaleVector(vec_knockback, knockback);
	
	if (is2D)
		vec_knockback[2] = 0.0;
	
	TeleportEntity(client, NULL_VECTOR, NULL_VECTOR, vec_knockback);
}

stock bool ZT_DLPrecacheSound(const char[] filePath)
{
	PrecacheSound(filePath);
	
	char buffer[256];
	Format(buffer, sizeof(buffer), "sound/%s", filePath);
	
	AddFileToDownloadsTable(buffer);
	
	return true;
}

stock bool ZT_AddDownloadTableFromFile(char[] filePath)
{
	File inputFile = OpenFile(filePath, "r");
	if (inputFile == null)
		return false;
	
	char buffer[256];
	int length;
	while (inputFile.EndOfFile() == false)
	{
		inputFile.ReadLine(buffer, sizeof(buffer));
		
		length = strlen(buffer);
		if (buffer[length-1] == '\n')
			buffer[--length] = EOS;
		
		TrimString(buffer);
		
		if (buffer[0] == EOS)
			continue;
		
		if (StrContains(buffer, "//") == 0)
			continue;
		
		AddFileToDownloadsTable(buffer);
	}

	delete inputFile;
	return true;
}

stock void ZT_PrecacheDLFromFile(char[] filePath)
{
	File inputFile = OpenFile(filePath, "r");
	char buffer[256];
	int length;
	while (inputFile.EndOfFile() == false)
	{
		inputFile.ReadLine(buffer, sizeof(buffer));
		
		length = strlen(buffer);
		if (buffer[length-1] == '\n')
			buffer[--length] = EOS;
		
		TrimString(buffer);
		
		if (buffer[0] == EOS)
			continue;
		
		if (StrContains(buffer, "//") == 0)
			continue;
		
		AddFileToDownloadsTable(buffer);
		if (StrContains(buffer, ".mdl") != -1)
			PrecacheModel(buffer);
	}

	delete inputFile;
}

stock float ZT_AbsFloat(float value)
{
	if (value < 0.0)
		return value * -1.0;	
	return value;
}

stock bool ZT_GetWorkshopID(const char[] mapName, char[] buffer, int maxlength)
{
	if (StrContains(mapName, "workshop]") == -1 &&
		StrContains(mapName, "workshop/") == -1)
		return false;
	
	strcopy(buffer, maxlength, mapName);
	ReplaceString(buffer, maxlength, "workshop]", "");
	ReplaceString(buffer, maxlength, "workshop/", "");
	
	int strPos = StrContains(buffer, "/");
	buffer[strPos] = '\0';
	
	return true;
}

stock float ZT_GetSpeed(int client, bool withoutZ = false)
{
	float velocity[3];
	GetEntPropVector(client, Prop_Data, "m_vecVelocity", velocity);
	if (withoutZ)
		velocity[2] = 0.0;
	
	return GetVectorLength(velocity);
}


stock ZT_GetWeaponClassname(int weapon, char[] buffer, int maxlength)
{
	GetEntityClassname(weapon, buffer, maxlength);
	
	CSWeaponID wpId = ZT_GetWeaponType(weapon);
	
	switch (wpId)
	{
		case CSWeapon_USP_SILENCER:
			strcopy(buffer, maxlength, "weapon_usp_silencer");
		case CSWeapon_CZ75A:
			strcopy(buffer, maxlength, "weapon_cz75a");
		case CSWeapon_REVOLVER:
			strcopy(buffer, maxlength, "weapon_revolver");
		case CSWeapon_MP5NAVY: // it doesn't exists!
			strcopy(buffer, maxlength, "weapon_mp5sd");
		case CSWeapon_M4A1_SILENCER:
			strcopy(buffer, maxlength, "weapon_m4a1_silencer");
	}
}

stock CSWeaponID ZT_GetWeaponType(int weaponIndex)
{
	return CS_ItemDefIndexToID(GetEntProp(weaponIndex, Prop_Send, "m_iItemDefinitionIndex"));
}

stock int ZT_GetWeaponSlot(int weapon)
{
	CSWeaponID id = ZT_GetWeaponType(weapon);
	if (ZT_IsPrimaryWeapon(id))
		return CS_SLOT_PRIMARY;
	
	if (ZT_IsPistol(id))
		return CS_SLOT_SECONDARY;
	
	char weaponName[32];
	GetEntityClassname(weapon, weaponName, sizeof(weaponName));
	if (StrEqual(weaponName, "weapon_knife"))
		return CS_SLOT_KNIFE;

	if (StrEqual(weaponName, "weapon_c4"))
		return CS_SLOT_C4;
	
	return CS_SLOT_GRENADE;
}

stock ZT_IsGrenade(CSWeaponID id)
{
	if (id == CSWeapon_HEGRENADE ||
		id == CSWeapon_FLASHBANG ||
		id == CSWeapon_SMOKEGRENADE ||
		id == CSWeapon_MOLOTOV ||
		id == CSWeapon_INCGRENADE ||
		id == CSWeapon_DECOY ||
		id == CSWeapon_TAGGRENADE)
		return true;
	
	return false;
}

stock ZT_IsPistol(CSWeaponID id)
{
	if (id == CSWeapon_GLOCK ||
		id == CSWeapon_USP ||
		id == CSWeapon_P228 ||
		id == CSWeapon_DEAGLE ||
		id == CSWeapon_FIVESEVEN ||
		id == CSWeapon_ELITE ||
		id == CSWeapon_HKP2000 ||
		id == CSWeapon_USP_SILENCER ||
		id == CSWeapon_P250 ||
		id == CSWeapon_TEC9 ||
		id == CSWeapon_CZ75A ||
		id == CSWeapon_REVOLVER)
		return true;
	
	return false;
}

stock ZT_IsPrimaryWeapon(CSWeaponID id)
{
	return ZT_IsShotgun(id) || ZT_IsSMG(id) || ZT_IsRifle(id) || ZT_IsSniperRifle(id) || ZT_IsMachineGun(id);
}

stock bool ZT_IsShotgun(CSWeaponID id)
{
	if (id == CSWeapon_NOVA ||
		id == CSWeapon_XM1014 ||
		id == CSWeapon_SAWEDOFF ||
		id == CSWeapon_MAG7)
		return true;
	
	return false;
}

stock ZT_IsSMG(CSWeaponID id)
{
	return id == CSWeapon_MAC10 ||
			id == CSWeapon_MP9 ||
			id == CSWeapon_MP7 ||
			id == CSWeapon_MP5NAVY ||
			id == CSWeapon_UMP45 ||
			id == CSWeapon_P90 ||
			id == CSWeapon_BIZON
	;
}

stock ZT_IsKnife(CSWeaponID id)
{
	if (id == CSWeapon_KNIFE ||
		id == CSWeapon_KNIFE_T ||
		id == CSWeapon_BAYONET ||
		id == CSWeapon_KNIFE_FLIP ||
		id == CSWeapon_KNIFE_KARAMBIT ||
		id == CSWeapon_KNIFE_M9_BAYONET ||
		id == CSWeapon_KNIFE_TATICAL ||
		id == CSWeapon_KNIFE_FALCHION ||
		id == CSWeapon_KNIFE_SURVIVAL_BOWIE ||
		id == CSWeapon_KNIFE_BUTTERFLY ||
		id == CSWeapon_KNIFE_PUSH)
		return true;
	
	return false;
}

stock ZT_IsRifle(CSWeaponID id)
{
	if (id == CSWeapon_SG552 ||
		id == CSWeapon_SG556 ||
		id == CSWeapon_AK47 ||
		id == CSWeapon_M4A1 ||
		id == CSWeapon_M4A1_SILENCER ||
		id == CSWeapon_FAMAS ||
		id == CSWeapon_GALIL ||
		id == CSWeapon_GALILAR ||
		id == CSWeapon_AUG)
		return true;
	
	return false;
}

stock ZT_IsZoomWeapon(CSWeaponID id)
{
	if (ZT_IsSniperRifle(id) ||
		id == CSWeapon_SG552 ||
		id == CSWeapon_SG556 ||
		id == CSWeapon_AUG)
		return true;
	
	return false;
}

stock ZT_IsSniperRifle(CSWeaponID id)
{
	if (id == CSWeapon_SCOUT ||
		id == CSWeapon_AWP ||
		id == CSWeapon_SSG08 ||
		id == CSWeapon_SCAR17 ||
		id == CSWeapon_SCAR20 ||
		id == CSWeapon_G3SG1 ||
		id == CSWeapon_SG550)
		return true;
	
	return false;
}

stock ZT_IsMachineGun(CSWeaponID id)
{
	return id == CSWeapon_M249 || id == CSWeapon_NEGEV;
}

public Action ZT_EmptyTimer(Handle a, Handle &timer)
{
	timer = INVALID_HANDLE;
	
	return Plugin_Continue;
}

stock int ZT_GetFOV(int client)
{
	int fov = GetEntProp(client, Prop_Data, "m_iFOV");
	if (fov == 0 || fov == 90)
		return 106;
	
	return fov;
}

stock ZT_CreateDirectory(char[] path)
{
	if (!DirExists(path))
		CreateDirectory(path, 0); // 0 ignore on windows
}

stock bool ZT_IsWarmUp()
{
	return GameRules_GetProp("m_bWarmupPeriod") == 1;
}

stock ZT_CopyFile(char[] copy_path, char[] target_path)
{
	File copy_file = OpenFile(copy_path, "rb");
	if (copy_file != null)
	{
		File target_file = OpenFile(target_path, "wb");
		while (!copy_file.EndOfFile())
		{
			int buffer[1];
			int data = copy_file.Read(buffer, sizeof(buffer), 1);
			target_file.Write(buffer, data, 1);
		}
		target_file.Close();
	}
	copy_file.Close();
}

stock int ZT_WeaponCleanUp()
{
	int killCount = 0;
	
	int index = -1;
	while ((index = FindEntityByClassname(index, "weapon_*")) != -1) {
		int owner = GetEntPropEnt(index, Prop_Data, "m_hOwner");
		if (owner <= 0) {
			AcceptEntityInput(index, "Kill");
			killCount++;
		}
	}
	
	return killCount;
}

typedef Strip_OnRemoveWeapon = function bool (int client, int weapon);
stock ZT_StripPlayer(int client, Strip_OnRemoveWeapon func = INVALID_FUNCTION)
{
	int m_hMyWeapons_size = GetEntPropArraySize(client, Prop_Send, "m_hMyWeapons"); // array size 
	int item; 

	for (int index = 0; index < m_hMyWeapons_size; index++) 
	{ 
		item = GetEntPropEnt(client, Prop_Send, "m_hMyWeapons", index); 
		if (item == -1)
			continue;
		
		if (func != INVALID_FUNCTION)
		{
			bool result = true;
			Call_StartFunction(INVALID_HANDLE, func);
			Call_PushCell(client);
			Call_PushCell(item);
			Call_Finish(result);
			
			if (!result)
				continue;
		}
		
		ZT_RemovePlayerItem(client, item);
	}
}

stock bool ZT_CreateParticle(int client, char[] particle, bool parent, float deletetime = -1.0)
{
	int ent = CreateEntityByName("info_particle_system");
	
	float particleOrigin[3];
	
	GetEntPropVector(client, Prop_Send, "m_vecOrigin", particleOrigin);
	/*if (parent)
	{
		GetEntPropVector(userid, Prop_Send, "m_vecOrigin", propOrigin);
	}
	else
		GetClientEyePosition(client, particleOrigin);*/

	DispatchKeyValue(ent , "start_active", "0");
	DispatchKeyValue(ent, "effect_name", particle);
	DispatchSpawn(ent);
	
	TeleportEntity(ent, particleOrigin, NULL_VECTOR,NULL_VECTOR);
	
	
	if (parent)
	{
		SetVariantString("!activator");
		AcceptEntityInput(ent, "SetParent", client, ent, 0);
	}
	
	ActivateEntity(ent);
	AcceptEntityInput(ent, "Start");
	
	if (deletetime >= 0.0)
		CreateTimer(deletetime, CreateParticle_Delete, EntIndexToEntRef(ent));
	
	return true;
}

public Action CreateParticle_Delete(Handle timer, int refent)
{
	int ent = EntRefToEntIndex(refent);
	if (ent && ent != INVALID_ENT_REFERENCE)
		AcceptEntityInput(ent, "Kill");

	return Plugin_Continue
}

stock ZT_RemovePlayerItemByName(int client, const char[] removeName)
{
	int m_hMyWeapons_size = GetEntPropArraySize(client, Prop_Send, "m_hMyWeapons"); // array size 
	int item; 

	for (int index = 0; index < m_hMyWeapons_size; index++) 
	{ 
		item = GetEntPropEnt(client, Prop_Send, "m_hMyWeapons", index); 
		if (item == -1)
			continue;
		
		char name[64];
		ZT_GetWeaponClassname(item, name, sizeof(name));
		if (!StrEqual(name, removeName))
			continue;
			
		ZT_RemovePlayerItem(client, item);
	}
}

stock ZT_RemovePlayerItem(int client, int index)
{
	RemovePlayerItem(client, index);
	if (IsValidEdict(index))
		RemoveEdict(index);
}

stock void ZT_GetCurrentMapName(char[] sBuffer, int iSize)
{
	GetCurrentMap(sBuffer, iSize);

	char workshopID[128];
	if (ZT_GetWorkshopID(sBuffer, workshopID, sizeof(workshopID)))
		strcopy(sBuffer, iSize, workshopID);
}

stock ZT_GameOver()
{
	int entity = CreateEntityByName("game_end");
	DispatchSpawn(entity);
	AcceptEntityInput(entity, "EndGame");
}

stock int ZT_GetBotCount()
{
	int counts;
	ZT_LoopValidClients(i)
	{
		if (!IsFakeClient(i))
			continue;
		
		counts++;
	}
	
	return counts;
}

stock int ZT_GetActivePlayers()
{
	int counts;
	ZT_LoopValidClients(i)
	{
		if (IsFakeClient(i) || GetClientTeam(i) < CS_TEAM_T)
			continue;
		
		counts++;
	}
	
	return counts;
}

stock int ZT_GetEntityHealth(int entity)
{
	return GetEntProp(entity, Prop_Data, "m_iHealth");
}

stock ZT_SetEntityHealth(int entity, int health)
{
	SetEntProp(entity, Prop_Data, "m_iHealth", health);
}

stock void ZT_ClearHandle(Handle &hd)
{
	if (hd != INVALID_HANDLE)
	{
		CloseHandle(hd);
		hd = INVALID_HANDLE;
	}
}

stock void ZT_ClearTimer(Handle &timer)
{
	if (timer != INVALID_HANDLE)
	{
		KillTimer(timer);
		timer = INVALID_HANDLE;
	}
}

stock ZT_ClearWeapons()
{
	int entity = -1;
	while ((entity = FindEntityByClassname(entity, "weapon_*")) != -1) {
		int owner = GetEntPropEnt(entity, Prop_Data, "m_hOwnerEntity"); 
		if (owner > 0)
			continue;
		
		AcceptEntityInput(entity, "Kill");
	}
}

stock ZT_RemoveAllEntities(char[] name)
{
	int entity = -1;
	while ((entity = FindEntityByClassname(entity, name)) != -1) {
		AcceptEntityInput(entity, "Kill");
	}
}

stock int ZT_GetTotalRounds()
{
	return 1 + GetTeamScore(CS_TEAM_T) + GetTeamScore(CS_TEAM_CT);
}

stock int ZT_GetKVNodeName(KeyValues kv, char[] name, int size)
{
	char buffer[512];
	int node_counts = kv.NodesInStack();
	if (node_counts == 0)
		return node_counts;
	
	for (int i=1; i<=node_counts; i++)
	{
		kv.GetSectionName(buffer, sizeof(buffer));
		if (i == 1)
			Format(name, size, "%s", buffer);
		else
			Format(name, size, "%s/%s", buffer, name);
		kv.GoBack();
	}
	
	return node_counts;
}

stock bool ZT_GetKVMapName(char[] map_name, char[] game_type, char[] buffer, int size)
{
	KeyValues kv_generic_config = new KeyValues("");
	if (!kv_generic_config.ImportFromFile("addons/sourcemod/configs/zeisenproject_-1/generic_config.cfg"))
		LogError("Failed to Import generic_config.cfg");
	
	if (kv_generic_config.JumpToKey("Map") && kv_generic_config.JumpToKey(map_name))
	{
		if (!kv_generic_config.JumpToKey(game_type))
			kv_generic_config.JumpToKey("Default");
		
		kv_generic_config.GetString("name", buffer, size);
	}
	
	delete kv_generic_config;
	return true;
}

stock ZT_UnloadMyPlugin()
{
	char sPluginName[64];
	GetPluginFilename(INVALID_HANDLE, sPluginName, sizeof(sPluginName));
	
	ServerCommand("sm plugins unload %s", sPluginName);
}

stock ZT_GetEntityOrigin(int entity, float origin[3])
{
	GetEntPropVector(entity, Prop_Send, "m_vecOrigin", origin);
}

stock void ZT_AddInFrontOf(float vecOrigin[3], float vecAngle[3], float units, float output[3])
{
	float vecAngVectors[3];
	vecAngVectors = vecAngle; //Don't change input
	GetAngleVectors(vecAngVectors, vecAngVectors, NULL_VECTOR, NULL_VECTOR);
	for (int i; i < 3; i++)
		output[i] = vecOrigin[i] + (vecAngVectors[i] * units);
}

stock void ZT_GetAttachmentPos(int entity, const char[] name, float pos[3])
{
	int infentity = CreateEntityByName("info_target");
	DispatchSpawn(infentity);
	
	float entityPos[3];
	ZT_GetEntityOrigin(entity, entityPos);
	TeleportEntity(infentity, entityPos, NULL_VECTOR, NULL_VECTOR);
	
	SetVariantString("!activator"); 
	AcceptEntityInput(infentity, "SetParent", entity, infentity, 0); 
	SetVariantString(name);
	AcceptEntityInput(infentity, "SetParentAttachment", infentity, infentity, 0);
	
	GetEntPropVector(infentity, Prop_Data, "m_vecAbsOrigin", pos);
	RemoveEdict(infentity);
}

stock ZT_GetEyePosition(int client, float buffer[3])
{
	#if defined _ZT_SOURCEMOD_EYELOC
	{
		GetClientEyePosition(client, buffer);
	}
	#else
	{
		char client_model[128];
		GetClientModel(client, client_model, sizeof(client_model));
		
		if (StrEqual(client_model, "models/player/zombie.mdl") || StrEqual(client_model, "models/player/slow/eve/slow.mdl") ||
			StrContains(client_model, "models/player/kuristaja/zombies/") == 0)
		{
			GetClientEyePosition(client, buffer);
		}
		else
		{
			int infentity = CreateEntityByName("info_target");
			DispatchSpawn(infentity);
			
			float client_pos[3];
			ZT_GetEntityOrigin(client, client_pos);
			TeleportEntity(infentity, client_pos, NULL_VECTOR, NULL_VECTOR);
			
			SetVariantString("!activator"); 
			AcceptEntityInput(infentity, "SetParent", client, infentity, 0); 
			SetVariantString("facemask");
			AcceptEntityInput(infentity, "SetParentAttachment", infentity, infentity, 0);
			
			GetEntPropVector(infentity, Prop_Data, "m_vecAbsOrigin", buffer);
			RemoveEdict(infentity);
		}
	}
	#endif
}

stock ZT_DealDamage(nClientVictim, nDamage, nClientAttacker = 0, nDamageType = DMG_GENERIC, char sWeapon[64] = "")
{
	if (nClientVictim > 0 && IsValidEntity(nClientVictim) && IsClientInGame(nClientVictim) && nDamage > 0)
	{
		int EntityPointHurt = CreateEntityByName("point_hurt");
		if (EntityPointHurt != 0)
		{
			char sDamage[16];
			IntToString(nDamage, sDamage, sizeof(sDamage));

			char sDamageType[32];
			IntToString(nDamageType, sDamageType, sizeof(sDamageType));

			char targetName[32];
			GetEntPropString(nClientVictim, Prop_Data, "m_iName", targetName, sizeof(targetName));  

			DispatchKeyValue(nClientVictim,                 "targetname",           "war3_hurtme");
			DispatchKeyValue(EntityPointHurt,               "DamageTarget", "war3_hurtme");
			DispatchKeyValue(EntityPointHurt,               "Damage",                               sDamage);
			DispatchKeyValue(EntityPointHurt,               "DamageType",           sDamageType);
			if(!StrEqual(sWeapon, ""))
				DispatchKeyValue(EntityPointHurt,       "classname",            sWeapon);

			DispatchSpawn(EntityPointHurt);

			AcceptEntityInput(EntityPointHurt,      "Hurt",                                 (nClientAttacker != 0) ? nClientAttacker : -1);
			DispatchKeyValue(EntityPointHurt,               "classname",            "point_hurt");
			DispatchKeyValue(nClientVictim,                 "targetname",           targetName);

			RemoveEdict(EntityPointHurt);
		}
	}
}

enum { BODYPART_HEAD = 1, BODYPART_GUT }
stock ZT_GetClientBodyPartPosition(int client, int part, float buffer[3])
{
	switch (part)
	{
		case BODYPART_HEAD:
		{
			ZT_GetEyePosition(client, buffer);
			
			float client_angle[3];
			GetClientEyeAngles(client, client_angle);
			
			float vecView[3];
			ZT_GetViewVector(client_angle, vecView);

			buffer[0] = buffer[0] - (vecView[0] * 4.0);
			buffer[1] = buffer[1] - (vecView[1] * 4.0);
			buffer[2] = buffer[2] - (vecView[2] * 4.0);
			//buffer[2] -= 3.0;
		}
		case BODYPART_GUT:
		{
			GetClientAbsOrigin(client, buffer);
			buffer[2] += 35.0;
		}
	}
}

public ZT_GetViewVector(float vecAngle[3], float output[3])
{
    output[0] = Cosine(vecAngle[1] / (180 / FLOAT_PI));
    output[1] = Sine(vecAngle[1] / (180 / FLOAT_PI));
    output[2] = -Sine(vecAngle[0] / (180 / FLOAT_PI));
}

stock ZT_PosToAngle(int client, const float pos[3], float aimAngle[3])
{
	float client_eyepos[3];
	GetClientEyePosition(client, client_eyepos);
	
	float aim_vector[3];
	MakeVectorFromPoints(client_eyepos, pos, aim_vector);
	
	GetVectorAngles(aim_vector, aimAngle);
	ZT_FixAngle(aimAngle);
}

stock ZT_SetPlayerAngle(int client, const float angle[3], bool care_recoil = false, float aimOffset[3] = {0.0, 0.0, 0.0})
{
	float setAngle[3];
	setAngle = angle;
	
	if (care_recoil)
	{
		float client_punch_angle[3];
		GetEntPropVector(client, Prop_Send, "m_aimPunchAngle", client_punch_angle);
		
		for (int i=0; i<=1; i++)
			setAngle[i] -= client_punch_angle[i] * FindConVar("weapon_recoil_scale").FloatValue;
	}
	
	for (int i=0; i<3; i++)
		setAngle[i] += aimOffset[i];
	
	TeleportEntity(client, NULL_VECTOR, setAngle, NULL_VECTOR);
}

stock ZT_SetPlayerAim(int client, const float pos_to_aim[3], bool care_recoil = false, float aimOffset[3] = {0.0, 0.0, 0.0})
{
	float aim_angle[3];
	ZT_PosToAngle(client, pos_to_aim, aim_angle);
	
	ZT_SetPlayerAngle(client, aim_angle, care_recoil, aimOffset);
}

stock float ZT_GetPlayerDistance(int client, int target, bool is2D = false)
{
	float clientPos[3], targetPos[3];
	GetClientAbsOrigin(client, clientPos);
	GetClientAbsOrigin(target, targetPos);
	if (is2D)
		clientPos[2] = targetPos[2] = 0.0;
	
	return GetVectorDistance(clientPos, targetPos);
}

stock void ZT_FixAngle(float angle[3])
{
	while (angle[0] > 89.0)  angle[0] -= 360.0;
	while (angle[0] < -89.0) angle[0] += 360.0;
	while (angle[1] > 180.0) angle[1] -= 360.0;
	while (angle[1] <-180.0) angle[1] += 360.0;
}

stock float ZT_GetTargetSightRange(int client, int target)
{
	float clientpos[3];
	float targetpos[3];
	float anglevector[3];
	float targetvector[3];
	float resultangle;
	
	GetClientEyeAngles(client, anglevector);
	anglevector[0] = anglevector[2] = 0.0;
	GetAngleVectors(anglevector, anglevector, NULL_VECTOR, NULL_VECTOR);
	NormalizeVector(anglevector, anglevector);
	
	GetClientAbsOrigin(client, clientpos);
	GetClientAbsOrigin(target, targetpos);
	
	clientpos[2] = targetpos[2] = 0.0;
	MakeVectorFromPoints(clientpos, targetpos, targetvector);
	NormalizeVector(targetvector, targetvector);
	
	resultangle = RadToDeg(ArcCosine(GetVectorDotProduct(targetvector, anglevector)));
	
	return (resultangle * 2.0);
}

stock bool ZT_IsAbleToSee(const float pos[3], const float secondPos[3], float offset[3] = {0.0, 0.0, 0.0})
{
	float temp[3]; temp = secondPos;
	for (int i=0; i<sizeof(temp); i++)
		temp[i] += offset[i];
	
	Handle trace = TR_TraceRayFilterEx(pos,
									   temp,
									   MASK_VISIBLE_AND_NPCS,
									   RayType_EndPoint,
									   ZT_TRF_WorldOnlyFilter);
	
	if (TR_GetFraction(trace) == 1.0)
	{
		CloseHandle(trace);
		return true;
	}
	
	CloseHandle(trace);
	return false;
}

typedef RandomPlayerFilter = function bool (int client);

stock ArrayList ZT_GetRandomPlayer(int count, RandomPlayerFilter func)
{
	bool result;
	ArrayList playerList = new ArrayList(1);
	for (int i=1; i<=MaxClients; i++)
	{
		if (!IsClientInGame(i))
			continue;
		
		Call_StartFunction(INVALID_HANDLE, func);
		Call_PushCell(i);
		Call_Finish(result);
		
		if (!result)
			continue;
		
		playerList.Push(i);
	}

	ArrayList resultList = new ArrayList(1);
	if (playerList.Length == 0)
		return resultList;

	for (int i=1; i<=count; i++)
	{
		int randIndex = GetRandomInt(0, playerList.Length - 1);
		int randPlayer = playerList.Get(randIndex);
		
		playerList.Erase(randIndex);
		resultList.Push(randPlayer);
	}

	return resultList;
}

stock bool ZT_IsAbleToSeeTarget(int client, int target)
{
	float client_eyepos[3];
	GetClientEyePosition(client, client_eyepos);
	
	float pos[3];
	
	GetClientEyePosition(target, pos);
	if (ZT_IsAbleToSee(client_eyepos, pos))
		return true;
	
	GetClientAbsOrigin(target, pos);
	if (ZT_IsAbleToSee(client_eyepos, pos))
		return true;
	
	if (ZT_IsAbleToSee(client_eyepos, pos, view_as<float>({0.0, 0.0, 30.0})))
		return true;
	
	if (ZT_IsAbleToSee(client_eyepos, pos, view_as<float>({0.0, 0.0, 64.0})))
		return true;
	
	return false;
}

stock bool ZT_IsAbleToSeePos(int client, float pos[3], float offset[3] = {0.0, 0.0, 0.0})
{
	float temp[3]; temp = pos;
	for (int i=0; i<sizeof(temp); i++)
		temp[i] += offset[i];
	
	float client_eyepos[3];
	GetClientEyePosition(client, client_eyepos);
	
	Handle trace = TR_TraceRayFilterEx(client_eyepos,
									   temp,
									   MASK_VISIBLE_AND_NPCS,
									   RayType_EndPoint,
									   ZT_TRF_WorldOnlyFilter);

	bool is_able_to_see = TR_GetFraction(trace) == 1.0;
	
	CloseHandle(trace);
	return is_able_to_see;
}

stock int ZT_IsAbleToSeePosEx(float client_eyepos[3], float pos[3])
{
	Handle trace = TR_TraceRayFilterEx(client_eyepos,
									   pos,
									   MASK_VISIBLE_AND_NPCS,
									   RayType_EndPoint,
									   ZT_TRF_WorldOnlyFilter);

	bool is_able_to_see = TR_GetFraction(trace) == 1.0;
	
	CloseHandle(trace);
	
	return is_able_to_see;
}

public bool ZT_TRF_AbleToSee(int entity, int contentsMask, int data)
{
	return entity == data || entity == 0;
}

public bool ZT_TRF_WorldOnlyFilter(int entity, int contentsMask)
{
	return entity == 0;
}

stock int ZT_IsAimingTarget(int client, int target, bool able_through_wall = false)
{
	float client_eyepos[3];
	GetClientEyePosition(client, client_eyepos);
	
	float client_angle[3];
	GetClientEyeAngles(client, client_angle);
	
	DataPack pack = new DataPack();
	pack.WriteCell(client);
	pack.WriteCell(target);
	pack.WriteCell(able_through_wall);
	
	Handle trace = TR_TraceRayFilterEx(client_eyepos,
									   client_angle,
									   MASK_VISIBLE_AND_NPCS,
									   RayType_Infinite,
									   ZT_IsAimingTargetFilter,
									   pack);
	
	int return_entity = -1;
	if (TR_DidHit(trace))
		return_entity = TR_GetEntityIndex(trace);
	
	CloseHandle(pack);
	CloseHandle(trace);
	
	return return_entity == target;
}

stock int ZT_GetAimTarget(int client, bool able_through_wall = false, bool only_enemy = false)
{
	float client_eyepos[3];
	GetClientEyePosition(client, client_eyepos);
	
	float client_angle[3];
	GetClientEyeAngles(client, client_angle);
	
	DataPack pack = new DataPack();
	pack.WriteCell(client);
	pack.WriteCell(able_through_wall);
	pack.WriteCell(only_enemy);
	
	Handle trace = TR_TraceRayFilterEx(client_eyepos,
									   client_angle,
									   MASK_VISIBLE_AND_NPCS,
									   RayType_Infinite,
									   ZT_GetAimTargetFilter,
									   pack);
	
	int return_entity = -1;
	if (TR_DidHit(trace))
		return_entity = TR_GetEntityIndex(trace);
	
	CloseHandle(pack);
	CloseHandle(trace);
	
	return return_entity;
}

public bool ZT_GetAimTargetFilter(int entity, int contentsMask, DataPack pack)
{
	pack.Reset();
	int client = pack.ReadCell();
	bool able_through_wall = pack.ReadCell();
	bool only_enemy = pack.ReadCell();
	
	if (entity == client || entity > MaxClients)
		return false;
	
	if (able_through_wall && entity == 0)
		return false;
	
	if (entity != 0 && only_enemy == false)
		return true;
	
	return GetClientTeam(client) != GetClientTeam(entity);
}

public bool ZT_IsAimingTargetFilter(int entity, int contentsMask, DataPack pack)
{
	pack.Reset();
	int client = pack.ReadCell();
	int target = pack.ReadCell();
	bool able_through_wall = pack.ReadCell();
	
	if (entity == client || entity != target)
		return false;
	
	if (able_through_wall && entity == 0)
		return false;
	
	return true;
}

stock bool ZT_GetClientLookingAt(int client, float output[3])
{
	float client_eyepos[3];
	GetClientEyePosition(client, client_eyepos);
	float client_angle[3];
	GetClientEyeAngles(client, client_angle);
	
	TR_TraceRayFilter(client_eyepos, client_angle, MASK_SOLID, RayType_Infinite, Filter_GetClientLookingAt, client);
	
	bool bRayHit = TR_DidHit(INVALID_HANDLE);
	if (bRayHit)
		TR_GetEndPosition(output, INVALID_HANDLE);
	
	return bRayHit;
}

public bool Filter_GetClientLookingAt(int entity, int contestsmask, int client)
{
	return entity != client;
}

stock ZT_ForceRoundEnd(int winner)
{
	int entity = CreateEntityByName("game_round_end");
	
	if (winner == CS_TEAM_T || winner == CS_TEAM_CT)
	{
		SetVariantFloat(FindConVar("mp_round_restart_delay").FloatValue);
		if (winner == CS_TEAM_T)
			AcceptEntityInput(entity, "EndRound_TerroristsWin");
		else
			AcceptEntityInput(entity, "EndRound_CounterTerroristsWin");
	}
	
	RemoveEdict(entity);
}

stock bool ZT_LockedFastQuery(Database database, const char[] query, int len=-1)
{
	SQL_LockDatabase(database);
	bool bSuccess = SQL_FastQuery(database, query, len);
	SQL_UnlockDatabase(database);
	return bSuccess;
}