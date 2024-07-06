
#include <sourcemod>
#include <sdktools>

public Plugin myinfo =
{
	name = "CSBot custom behavior",
	author = "Zeisen",
	description = "Easy setup custom bot behavior setups via edit convars",
	version = "1.01",
	url = ""
};

#define CLIENT_INVALID 0

int g_hasVisitedEnemySpawnOffset;
ConVar g_cvRushToSpawn;

public void OnPluginStart()
{
    GameData gc = new GameData("csbot.games");
    if (gc == null)
    {
        SetFailState("Failed to find csbot.games gamedata");
        return;
    }

    g_hasVisitedEnemySpawnOffset = gc.GetOffset("m_hasVisitedEnemySpawn");
    delete gc;

    g_cvRushToSpawn = CreateConVar("bot_huntstate_rush_to_spawn", "1", .hasMin=true, .min = 0.0, .hasMax = true, .max = 1.0);

    HookEvent("player_spawn", OnPlayerSpawn);
}

void OnPlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(event.GetInt("userid"));
    if (IsFakeClient(client))
        RequestFrame(RF_OnBotSpawn, GetClientUserId(client));
}

void RF_OnBotSpawn(int userid)
{
    int client = GetClientOfUserId(userid);
    if (client == CLIENT_INVALID)
        return;

    if (!g_cvRushToSpawn.BoolValue)
        SetEntData(client, g_hasVisitedEnemySpawnOffset, 1, 1);
}