#ifndef TOPLIST_H_
#define TOPLIST_H_

#define TOPLIST_VERSION "0.6.1"

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <igameevents.h>
#include <convar.h>
#include <iplayerinfo.h>

#include "Include/HTTPRequest.hpp"

typedef struct Players {
	edict_t *Client;
	int userID;
	const char *playername;
} s_Players;


class Toplist : public ISmmPlugin, public IMetamodListener, public IGameEventListener2 {

public:

	const char *GetAuthor() { return "Menko"; }
	const char *GetName() { return "Toplist"; }
	const char *GetDescription() { return "Global toplist."; }
	const char *GetURL() { return ""; }
	const char *GetLicense() { return "Closed source"; }
	const char *GetVersion() { return TOPLIST_VERSION; }
	const char *GetDate() { return __DATE__; }
	const char *GetLogTag() { return "TL"; }


	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();
	bool Pause(char *error, size_t maxlen) { return true; }
	bool Unpause(char *error, size_t maxlen) { return true; }
	void ClientPutInServer(edict_t *pEntity, char const *playername);
	void ClientDisconnect(edict_t *pEntity);
	void LevelShutdown(void);
	void FireGameEvent(IGameEvent* event);

	// helpers
	char *concat(const char *s1, const char *s2);
	const char *getCourse(IGameEvent *event);

	// metamod listener
	bool OnLevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);
	
	// WebRequests
	bool registerMap();
	bool registerPlayer(std::string steamid, std::string player);
	bool addRecord(std::string mapName, std::string courseName, std::string steamID,std::string playerName, int seconds, int miliseconds, int checkpoints, int teleports);


	void Hook_ClientCommand(edict_t *pEntity, const CCommand &args);

public:
	int GetApiVersion() { return METAMOD_PLAPI_VERSION; }

private:

	IGameEventManager2 * m_GameEventManager;
	IVEngineServer *m_Engine;
	IServerGameDLL *m_ServerDll;
	IServerGameClients *m_ServerClients;

	IPlayerInfoManager *m_PlayerInfoManager;

	IConVar *p_ConVar;

	std::string currentMap;
	int m_CurrentPlayers = 0;
	std::vector<s_Players> m_Players;

};

class ToplistListener : public IMetamodListener
{
public:
	virtual void *OnMetamodQuery(const char *iface, int *ret);
};

extern Toplist g_Toplist;

#endif