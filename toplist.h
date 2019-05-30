#ifndef TOPLIST_H_
#define TOPLIST_H_

#define TOPLIST_VERSION "0.9.4"

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <regex>

// source engine
#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <igameevents.h>
#include <convar.h>
#include <iplayerinfo.h>
#include <bitbuf.h>

// boost
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "Include/pluginusermessages.hpp"
#include "Include/HTTPRequest.hpp"
#include "Include/server.hpp"
#include "Include/plog/Log.h"
#include "Include/Colors.hpp"

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

typedef struct Players {
	edict_t *Client;
	int userID;
	const char *playername;
} s_Players;

struct userMessageInfo
{
	char msgName[255];
	int messageID;
};

static bf_write *g_pMsgBuffer = nullptr;

class Toplist : public ISmmPlugin, public IMetamodListener, public IGameEventListener2 {

public:

	Toplist() = default;

	const char *GetAuthor() { return "Menko"; }
	const char *GetName() { return "Toplist"; }
	const char *GetDescription() { return "KZMOD Toplist and rank system."; }
	const char *GetURL() { return m_ToplistIP.c_str(); }
	const char *GetLicense() { return "GPLv3"; }
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

	// test
	CRecipientFilter filter;

	// helpers
	char *concat(const char *s1, const char *s2);
	const char *getCourse(IGameEvent *event);
	std::string getToplistIP();
	void sendDataToGame(std::string data);
	void UserMessageBegin(IRecipientFilter &filter, const char *messagename);
	void ClientPrint(IRecipientFilter& filter, int msg_dest, const char *msg_name, const char *param1 = nullptr, const char *param2 = nullptr, const char *param3 = nullptr, const char *param4 = nullptr);
	void ColorClientPrint(IRecipientFilter& filter, int msg_dest, int bChat, const char *msg_name, const char *param1 = nullptr, const char *param2 = nullptr, const char *param3 = nullptr, const char *param4 = nullptr);

	///// chat message functions
	// bitwise
	void MessageWriteBits(const void *pIn, int nBits);
	void MessageWriteSBitLong(int data, int numbits);
	void MessageWriteUBitLong(unsigned int data, int numbits);
	void MessageWriteBool(bool bValue);
	// rest
	void MessageWriteEHandle(CBaseEntity *pEntity);
	void MessageWriteEntity(int iValue);
	void MessageWriteString(const char *sz);
	void MessageWriteAngles(const QAngle& rgflValue);
	void MessageWriteVec3Normal(const Vector& rgflValue);
	void MessageWriteVec3Coord(const Vector& rgflValue);
	void MessageWriteCoord(float flValue);
	void MessageWriteAngle(float flValue);
	void MessageWriteFloat(float flValue);
	void MessageWriteLong(int iValue);
	void MessageWriteWord(int iValue);
	void MessageWriteShort(int iValue);
	void MessageWriteChar(int iValue);
	void MessageWriteByte(int iValue);
	void MessageEnd(void);
	/////

	// metamod listener
	bool OnLevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);
	
	// WebRequests
	bool registerMap();
	bool registerPlayer(std::string steamid, std::string player);
	bool addRecord(std::string mapName, const char *courseName, std::string steamID,std::string playerName, int seconds, int miliseconds, int checkpoints, int teleports);

	 
	void Hook_ClientCommand(edict_t *pEntity, const CCommand &args);

public:
	int GetApiVersion() { return METAMOD_PLAPI_VERSION; }

	static Toplist *pToplist;

private:

	// source engine
	IGameEventManager2 * m_GameEventManager;
	IVEngineServer *m_Engine;
	IServerGameDLL *m_ServerDll;
	IServerGameClients *m_ServerClients;
	IPlayerInfoManager *m_PlayerInfoManager;
	IConVar *p_ConVar;

	// my variables
	std::string m_ToplistIP;
	std::string currentMap;
	int m_CurrentPlayers = 0;
	std::vector<s_Players> m_Players;
	bool loadCustomCvar;

	// server
	boost::asio::io_service m_Service;
	std::shared_ptr<server> m_Server;


	void startServer(std::string ip);
};



class ToplistListener : public IMetamodListener
{
public:
	virtual void *OnMetamodQuery(const char *iface, int *ret);
};

extern Toplist g_Toplist;

#endif