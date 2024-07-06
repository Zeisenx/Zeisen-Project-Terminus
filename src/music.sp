
#include <sourcemod>
#include <sdktools>

#undef REQUIRE_EXTENSIONS
#include <soundlib>
#define REQUIRE_EXTENSIONS

#include <colorvariables>
#include <zp_tools>

public Plugin myinfo = 
{
	name = "Zeisen Project | Music",
	author = "Zeisen",
	description = "",
	version = "1.01",
	url = "http://steamcommunity.com/profiles/76561198002384750"
}

#define MUSICINFO_PATH "addons/sourcemod/configs/zp/music.cfg"

ArrayList g_prevAlbumList;
char g_albumName[128];
char g_playingMusic[MAXPLAYERS + 1][256];
KeyValues g_kvMusicInfo;

bool g_isLoop[MAXPLAYERS + 1];
float g_playerVol[MAXPLAYERS + 1];

ConVar g_cvMusicPitch;
ConVar g_cvMusicForce;

EngineVersion g_engineVersion;

bool g_pluginSoundLib;

public void OnPluginStart()
{
	RegAdminCmd("zp_music_getalbumname", Cmd_GetAlbumName, ADMFLAG_ROOT);
	RegAdminCmd("zp_music_playall", Cmd_PlayAll, ADMFLAG_ROOT);
	
	RegConsoleCmd("sm_music", Cmd_MusicVolume);
	RegConsoleCmd("sm_mvol", Cmd_MusicVolume);
	RegConsoleCmd("sm_musicoff", Cmd_MusicOff);
	RegConsoleCmd("sm_offmusic", Cmd_MusicOff);
	
	HookEvent("round_start", OnRoundPreStart, EventHookMode_Pre);
	
	g_cvMusicPitch = CreateConVar("zp_music_pitch", "100");
	g_cvMusicForce = CreateConVar("zp_music_force", "");
	
	RegServerCmd("zp_music_forcecmd", Cmd_MusicForce);
	g_prevAlbumList = new ArrayList(64);

	CreateTimer(1.0, Timer_MusicRepeat, _, TIMER_REPEAT);

	g_engineVersion = GetEngineVersion();
}

public void OnAllPluginsLoaded()
{
	g_pluginSoundLib = LibraryExists("Sound Info Library");
}

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	RegPluginLibrary("zp_fakestar_music");
	CreateNative("ZMusic_PlayToClient", Native_PlayToClient);
	CreateNative("ZMusic_PlayToAll", Native_PlayToAll);
	
	return APLRes_Success;
}

Action Cmd_MusicForce(int args)
{
	GetCmdArg(1, g_albumName, sizeof(g_albumName));
	Music_PrepareMusic(g_albumName);

	return Plugin_Handled;
}

public void OnMapStart()
{
	char mapName[64];
	GetCurrentMap(mapName, sizeof(mapName));

	g_cvMusicForce.GetString(g_albumName, sizeof(g_albumName));
	
	delete g_kvMusicInfo;
	g_kvMusicInfo = new KeyValues("");
	g_kvMusicInfo.ImportFromFile(MUSICINFO_PATH);
	
	Music_PrepareMusic("generic");
	if (g_albumName[0] == EOS) {
		Music_GetRandomResult(g_albumName, sizeof(g_albumName));
	}
	Music_PrepareMusic(g_albumName);
	
	g_cvMusicForce.SetString("");
}

public void OnClientConnected(int client)
{
	g_playingMusic[client][0] = EOS;
}

public void OnClientPutInServer(int client)
{
	g_playerVol[client] = 0.1; 
}

Action Timer_MusicRepeat(Handle timer)
{
	for (int i=1; i<=MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
			continue;
		
		if (!g_isLoop[i])
			continue;
		
		if (g_playingMusic[i][0] == '\0')
			continue;
		
		EmitMusic(i);
	}

	return Plugin_Handled;
}

void OnRoundPreStart(Event event, const char[] name, bool dontBroadcast)
{
	for (int i=1; i<=MaxClients; i++)
		g_playingMusic[i][0] = '\0';
}

Action Cmd_MusicVolume(int client, int args)
{
	if (args == 0)
	{
		CPrintToChat(client, "{green}[Music]{default} 현재 음악 볼륨 : %.3f", g_playerVol[client]);
		CPrintToChat(client, "{green}[Music]{default} !mvol <0.0 ~ 1.0>");
		return Plugin_Handled;
	}

	char buffer[32];
	GetCmdArg(1, buffer, sizeof(buffer));
	
	float vol = StringToFloat(buffer);
	if (vol < 0.0 || vol > 1.0) {
		CPrintToChat(client, "{green}[Music]{default} 음악 볼륨은 0.0 ~ 1.0이여야 합니다.");
		return Plugin_Handled;
	}
	
	Music_SetClientVolume(client, vol);
	
	return Plugin_Handled;
}

public Action Cmd_MusicOff(int client, int args)
{
	Music_SetClientVolume(client, 0.0);
	return Plugin_Handled;
}

public Action Cmd_GetAlbumName(int client, int args)
{
	ReplyToCommand(client, "g_albumName : %s", g_albumName);
	return Plugin_Handled;
}

public Action Cmd_PlayAll(int client, int args)
{
	char keyName[128];
	GetCmdArg(1, keyName, sizeof(keyName));
	Music_PlayToAll(keyName);
	
	return Plugin_Handled;
}

public int Native_PlayToClient(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	
	char keyName[128];
	GetNativeString(2, keyName, sizeof(keyName));
	
	Music_Play(client, keyName);
	
	return 0;
}

public int Native_PlayToAll(Handle plugin, int numParams)
{
	char keyName[128];
	GetNativeString(1, keyName, sizeof(keyName));
	
	Music_PlayToAll(keyName);
	
	return 0;
}

void Music_SetClientVolume(int client, float vol = -1.0, bool notify = true)
{
	if (vol >= 0.0)
		g_playerVol[client] = vol;
	if (notify)
		CPrintToChat(client, "{green}[Music]{default} 음악 볼륨이 %.3f로 설정되었습니다.", vol);
	
	Music_Replay(client);
}

void Music_PrepareMusic(const char[] keyName)
{
	g_kvMusicInfo.JumpToKey("list");
	g_kvMusicInfo.JumpToKey(keyName);
	
	g_kvMusicInfo.GotoFirstSubKey(false);
	do {
		char musicPath[256];
		g_kvMusicInfo.GetString(NULL_STRING, musicPath, sizeof(musicPath));
		Music_ConvertPath(musicPath, sizeof(musicPath));
		
		if (musicPath[0] == EOS)
			continue;
		
		PrintToServer(musicPath);
		PrecacheSound(musicPath);
		Format(musicPath, sizeof(musicPath), "sound/%s", musicPath);
		AddFileToDownloadsTable(musicPath);
	}
	while (g_kvMusicInfo.GotoNextKey(false))
	
	g_kvMusicInfo.Rewind();
}

void Music_GetRandomResult(char[] buffer, int maxlength)
{
	ArrayList list = new ArrayList(64);
	g_kvMusicInfo.JumpToKey("random_list");
	g_kvMusicInfo.GotoFirstSubKey();
	do {
		char albumName[128];
		g_kvMusicInfo.GetSectionName(albumName, sizeof(albumName));
		LogMessage(albumName);
		
		list.PushString(albumName);
	}
	while (g_kvMusicInfo.GotoNextKey())
	g_kvMusicInfo.Rewind();
	
	if (list.Length == g_prevAlbumList.Length) {
		g_prevAlbumList.Clear();
	}
	else {
		for (int i=0; i<g_prevAlbumList.Length; i++) {
			char prevAlbumName[128];
			g_prevAlbumList.GetString(i, prevAlbumName, sizeof(prevAlbumName));
			int idx = list.FindString(prevAlbumName);
			if (idx != -1)
				list.Erase(idx);
		}
	}
	
	list.GetString(GetRandomInt(0, list.Length - 1), buffer, maxlength);
	g_prevAlbumList.PushString(buffer);
	delete list;
}

float Music_GetVolume(int client)
{
	return g_playerVol[client];
}

void Music_Play(int client, const char[] keyName)
{
	if (IsFakeClient(client))
		return;
	
	char musicPath[256];
	Music_GetPath(keyName, musicPath, sizeof(musicPath));
	if (musicPath[0] == '\0')
		return;
	
	bool isSameMusic = StrEqual(g_playingMusic[client], keyName);
	if (g_playingMusic[client][0] != EOS && !StrEqual(keyName, g_playingMusic[client])) {
		char playingMusicPath[256];
		Music_GetPath(g_playingMusic[client], playingMusicPath, sizeof(playingMusicPath));
		StopSound(client, SNDCHAN_STATIC, playingMusicPath);
	}
	
	g_isLoop[client] = false;
	strcopy(g_playingMusic[client], sizeof(g_playingMusic[]), keyName);
	EmitMusic(client);
	
	if (g_pluginSoundLib && Music_GetVolume(client) > 0.0 && !isSameMusic)
		Music_PrintSource(client, musicPath);
}

void Music_PrintSource(int client, const char[] musicPath)
{
	SoundFile musicInfo = new SoundFile(musicPath);
	if (musicInfo == null)
		return;
		
	char artist[128], album[128], title[128];
	musicInfo.GetArtist(artist, sizeof(artist));
	musicInfo.GetAlbum(album, sizeof(album));
	musicInfo.GetTitle(title, sizeof(title));

	if (artist[0] == '\0' && album[0] == '\0' && title[0] == '\0')
	{
		delete musicInfo;
		return;
	}

	g_isLoop[client] = GetSoundLengthFloat(musicInfo) > 10.0;

	char srcMessage[256];
	Format(srcMessage, sizeof(srcMessage), "{green}[Music]{default} %s - %s (%s)", title, artist, album);
	
	DataPack pack = new DataPack();
	CreateDataTimer(2.5, Timer_PrintSource, pack);
	pack.WriteCell(GetClientUserId(client));
	pack.WriteString(srcMessage);

	delete musicInfo;
}

void EmitMusic(int client)
{
	char musicPath[256];
	Music_GetPath(g_playingMusic[client], musicPath, sizeof(musicPath));

	float vol = Music_GetVolume(client);
	int pitch = g_cvMusicPitch.IntValue;

	// hack :: CSGO reduce volume to half when player is spectating someone on first person.
	// This hack makes more consistent like CS:S
	if (g_engineVersion == Engine_CSGO &&
		g_isLoop[client] && !IsPlayerAlive(client) && GetEntProp(client, Prop_Send, "m_iObserverMode") == 4)
		vol *= 2.0;

	if (vol > 1.0)
		vol = 1.0;
	
	EmitSoundToClient(client, musicPath, _, SNDCHAN_STATIC, SNDLEVEL_NONE, SND_CHANGEVOL, .pitch = pitch, .volume = vol);
}

public Action Timer_PrintSource(Handle timer, DataPack pack)
{
	pack.Reset();
	int client = GetClientOfUserId(pack.ReadCell());
	char srcMessage[256];
	pack.ReadString(srcMessage, sizeof(srcMessage));

	if (client == 0)
		return Plugin_Continue;

	CPrintToChat(client, srcMessage);
	return Plugin_Continue;
}

void Music_Replay(int client)
{
	Music_Play(client, g_playingMusic[client]);
}

void Music_PlayToAll(const char[] keyName)
{
	for (int client=1; client<=MaxClients; client++)
	{
		if (!IsClientInGame(client))
			continue;
		
		Music_Play(client, keyName);
	}
}

bool Music_GetPath(const char[] keyName, char[] buffer, int maxlength)
{
	char fmtBuffer[256];
	Format(fmtBuffer, sizeof(fmtBuffer), "list/%s/%s", g_albumName, keyName);
	g_kvMusicInfo.GetString(fmtBuffer, buffer, maxlength, "");
	if (buffer[0] != EOS) {
		Music_ConvertPath(buffer, maxlength);
		return true;
	}
	
	Format(fmtBuffer, sizeof(fmtBuffer), "list/generic/%s", keyName);
	g_kvMusicInfo.GetString(fmtBuffer, buffer, maxlength, "");
	if (buffer[0] != EOS) {
		Music_ConvertPath(buffer, maxlength);
		return true;
	}
	
	return false;
}

void Music_ConvertPath(char[] buffer, int maxlength)
{
	if (buffer[0] == '*')
		Format(buffer, maxlength, "zp/fakestar/bgm/%s%s", g_albumName, buffer[1]);
}
