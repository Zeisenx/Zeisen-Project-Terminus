
#include <sourcemod>

public Plugin myinfo =
{
	name = "Ability",
	author = "Zeisen",
	description = "Ability system for RPG, other feature",
	version = "1.0",
	url = ""
};

GlobalForward g_fwOnReset;
GlobalForward g_fwOnGetValue;
GlobalForward g_fwOnRegister;

methodmap AbilityMap < StringMap
{
    public AbilityMap(int client)
    {
        AbilityMap map = view_as<AbilityMap>(new StringMap());

        map.SetValue("ability_client", client);

        return map;
    }

    property int client
    {
        public get()
        {
            int client = 0;
            this.GetValue("ability_client", client);

            return client;
        }
    }

    public void Register(const char[] keyName, any defaultValue, bool replace=true)
    {
        this.SetValue(keyName, defaultValue, replace);
    }

    public void OnReset()
    {
        this.Clear();
        Call_StartForward(g_fwOnReset);
        Call_PushCell(this.client);
        Call_Finish();
    }
}

AbilityMap g_playerAbility[MAXPLAYERS + 1];

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
    RegPluginLibrary("ability");

    CreateNative("Ability_Register", Native_Register);
    CreateNative("Ability_HasAbility", Native_HasAbility);
    CreateNative("Ability_GetValue", Native_GetValue);

    g_fwOnReset     = CreateGlobalForward("Ability_OnReset", ET_Hook, Param_Cell);
    g_fwOnGetValue  = CreateGlobalForward("Ability_OnGetValue", ET_Ignore, Param_Cell, Param_String, Param_CellByRef);
    g_fwOnRegister  = CreateGlobalForward("Ability_OnRegister", ET_Ignore, Param_Cell, Param_String, Param_Cell);

    return APLRes_Success;    
}

public void OnClientPutInServer(int client)
{
    if (g_playerAbility[client] != null)
        delete g_playerAbility[client];

    g_playerAbility[client] = new AbilityMap(client);

    Call_StartForward(g_fwOnReset);
    Call_PushCell(client);
    Call_Finish();
}

public void OnClientDisconnect(int client)
{
    delete g_playerAbility[client];
}

int Native_Register(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    char keyName[64];
    GetNativeString(2, keyName, sizeof(keyName));
    any dftValue = 0.0;
    if (numParams >= 3)
        dftValue = GetNativeCell(3);

    g_playerAbility[client].Register(keyName, dftValue, numParams >= 3);
    Call_StartForward(g_fwOnRegister);
    Call_PushCell(client);
    Call_PushString(keyName);
    Call_PushCell(dftValue);
    Call_Finish();

    return 0;
}

any Native_HasAbility(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    char keyName[64];
    GetNativeString(2, keyName, sizeof(keyName));

    return g_playerAbility[client].ContainsKey(keyName);
}

any Native_GetValue(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    char keyName[64];
    GetNativeString(2, keyName, sizeof(keyName));

    any value;
    g_playerAbility[client].GetValue(keyName, value); // get default value
    Call_StartForward(g_fwOnGetValue);
    Call_PushCell(client);
    Call_PushString(keyName);
    Call_PushCellRef(value);
    Call_Finish();

    return value;
}