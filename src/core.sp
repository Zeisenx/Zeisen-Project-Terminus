
#include <sourcemod>
#include <cstrike>
#include <colorvariables>

public Plugin myinfo =
{
	name = "Zeisen Project Core",
	author = "Zeisen",
	description = "Core plugin for Zeisen Project",
	version = "0.1",
	url = ""
};

ConVar g_cvGameMode, g_cvGameType, g_cvGameTypeNext;
ConVar g_cvDifficulty;

ConVar g_cvZombieTeam, g_cvHumanTeam;

int g_humanTeam, g_zombieTeam;

public void OnPluginStart()
{
    g_cvGameMode = CreateConVar("zp_gamemode", "Quick Casual", "");
    g_cvGameType = CreateConVar("zp_gametype", "Default", "");
    g_cvGameTypeNext = CreateConVar("zp_gametype_next", "Default", "");
    g_cvDifficulty = CreateConVar("zp_difficulty", "1", "", .hasMin=true, .min = 1.0);

    g_cvZombieTeam = CreateConVar("zp_zombieteam", "2", "Zombie team", .hasMin=true, .min = 2.0, .hasMax=true, .max=3.0);
    g_cvHumanTeam = CreateConVar("zp_humanteam", "3", "Human team", .hasMin=true, .min = 2.0, .hasMax=true, .max=3.0);

    RegServerCmd("bot_add_human", Cmd_AddBots);
    RegServerCmd("bot_add_zombie", Cmd_AddBots);

    // Prehook for announce the gamemode earlier, could be split to other plugin
    HookEvent("round_freeze_end", OnFreezeEnd, EventHookMode_Pre);
}

void OnFreezeEnd(Event event, const char[] name, bool dontBroadcast)
{
    char gameMode[64];
    g_cvGameMode.GetString(gameMode, sizeof(gameMode));

    CPrintToChatAll("{green}[Game Mode]{white} %s", gameMode);
}

Action Cmd_AddBots(int args)
{
    char cmdName[64];
    GetCmdArg(0, cmdName, sizeof(cmdName));

    int targetTeam = StrEqual(cmdName, "bot_add_human") ? g_humanTeam : g_zombieTeam;

    char botName[64];
    GetCmdArg(1, botName, sizeof(botName));

    int botCount = args <= 1 ? 0 : GetCmdArgInt(2);

    if (botCount == 0)
        ServerCommand("bot_add_%s \"%s\"", targetTeam == CS_TEAM_T ? "t" : "ct", botName);

    if (botCount > 0)
    {
        for (int cnt=1; cnt<=botCount; cnt++)
            ServerCommand("bot_add_%s \"%s %d\"", targetTeam == CS_TEAM_T ? "t" : "ct", botName, cnt);
    }
    return Plugin_Handled;
}

public void OnMapStart()
{
    char mapName[128];
    GetCurrentMap(mapName, sizeof(mapName));

    KeyValues configKV = new KeyValues("");

    char filePath[256];
    Format(filePath, sizeof(filePath), "addons/sourcemod/configs/zp_fakestar/maps/%s.cfg", mapName);
    if (!configKV.ImportFromFile(filePath))
    {
        ServerCommand("exec \"sourcemod/zp_fakestar/bots_default\"");
        LogError("Failed to find config %s", filePath);
        return;
    }


    char gameType[64];
    g_cvGameTypeNext.GetString(gameType, sizeof(gameType));

    if (!configKV.JumpToKey(gameType))
    {
        LogError("Failed to find config %s->%s", mapName, gameType);
        configKV.Rewind();
        g_cvGameTypeNext.RestoreDefault();
        return;
    }

    char gameMode[64];
    configKV.GetString("gamemode", gameMode, sizeof(gameMode));
    g_cvGameMode.SetString(gameMode);
    ServerCommand("exec \"sourcemod/zp_fakestar/gamemode/%s\"", gameMode);

    g_zombieTeam = configKV.GetNum("zombieteam", CS_TEAM_T);
    g_humanTeam = configKV.GetNum("humanteam", CS_TEAM_CT);

    g_cvHumanTeam.IntValue = g_humanTeam;
    g_cvZombieTeam.IntValue = g_zombieTeam;

    FindConVar("bot_join_team").SetString(g_zombieTeam == CS_TEAM_T ? "t" : "ct")
    FindConVar("mp_humanteam").SetString(g_humanTeam == CS_TEAM_T ? "t" : "ct")
    
    if (configKV.JumpToKey("Configs"))
    {
        if (configKV.GotoFirstSubKey())
        {
            do
            {
                char configPath[256];
                configKV.GetSectionName(configPath, sizeof(configPath));

                ServerCommand("exec %s", configPath);
                PrintToServer("[Fakestar Core] Execing %s", configPath);
            }
            while (configKV.GotoNextKey())
            configKV.GoBack();
        }
        configKV.GoBack();
    }
    else
    {
        char sectionName[256];
        configKV.GetSectionName(sectionName, sizeof(sectionName));

        LogError("failed to find configs : %s", sectionName);
    }

    configKV.Rewind();
    g_cvGameTypeNext.RestoreDefault();
}