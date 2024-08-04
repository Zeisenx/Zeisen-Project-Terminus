
#include <sourcemod>
#include <sdktools>
#include <cstrike>

bool g_bWaitingForPlayers = true; 
ConVar g_cvWTPEnable, g_cvWTPTime;

public void OnPluginStart()
{
    HookEvent("round_start", OnRoundStart);
    HookEvent("round_freeze_end", OnRoundFreezeEnd);

    g_cvWTPEnable = CreateConVar("zp_wtp_enable", "1");
    g_cvWTPTime = CreateConVar("zp_wtp_time", "60.0");
}

public void OnMapStart()
{
    g_bWaitingForPlayers = true;
}

void OnRoundStart(Event event, const char[] name, bool dontBroadcast)
{
    if (!g_bWaitingForPlayers || !g_cvWTPEnable.BoolValue)
        return;
    
    GameRules_SetPropFloat("m_fRoundStartTime", GetGameTime() + g_cvWTPTime.FloatValue);
}

void OnRoundFreezeEnd(Event event, const char[] name, bool dontBroadcast)
{
    if (!g_cvWTPEnable.BoolValue)
        return;

    if (IsThereActiveHumanPlayers())
        g_bWaitingForPlayers = false;
}

bool IsThereActiveHumanPlayers()
{
    for (int i=1; i<=MaxClients; i++)
    {
        if (IsClientInGame(i) && !IsFakeClient(i) && GetClientTeam(i) >= CS_TEAM_T)
            return true;
    }

    return false;
}
