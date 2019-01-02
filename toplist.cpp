

#include "toplist.h"

#define	FIND_IFACE(func, assn_var, num_var, name, type) \
	do { \
		if ( (assn_var=(type)((ismm->func())(name, NULL))) != NULL ) { \
			num = 0; \
			break; \
		} \
		if (num >= 999) \
			break; \
	} while ( num_var=ismm->FormatIface(name, sizeof(name)-1) ); \
	if (!assn_var) { \
		if (error) \
			printf(error, maxlen, "Could not find interface %s", name); \
		return false; \
}

#define LOADINTERFACE(_module_, _version_, _out_) Sys_LoadInterface(_module_, _version_, NULL, reinterpret_cast<void**>(& _out_ ))

Toplist g_Toplist;
ToplistListener g_Listener;

PLUGIN_EXPOSE(Toplist, g_Toplist);

// Hook Declaration
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t*, const CCommand &);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);

bool Toplist::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) {

	m_CurrentPlayers = 0;

	PLUGIN_SAVEVARS();

	char iface_buffer[255];
	int num = 0;

		strcpy(iface_buffer, INTERFACEVERSION_SERVERGAMEDLL);
	FIND_IFACE(GetServerFactory, m_ServerDll, num, iface_buffer, IServerGameDLL *)
		strcpy(iface_buffer, INTERFACEVERSION_VENGINESERVER);
	FIND_IFACE(GetEngineFactory, m_Engine, num, iface_buffer, IVEngineServer *)
		strcpy(iface_buffer, INTERFACEVERSION_SERVERGAMECLIENTS);
	FIND_IFACE(GetServerFactory, m_ServerClients, num, iface_buffer, IServerGameClients *)
		strcpy(iface_buffer, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	FIND_IFACE(GetEngineFactory, m_GameEventManager, num, iface_buffer, IGameEventManager2 *);

	META_LOG(g_PLAPI, "Plugin Loaded");

	//Hook 
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, m_ServerClients, &g_Toplist, &Toplist::ClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, m_ServerClients, &g_Toplist, &Toplist::ClientDisconnect, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, m_ServerClients, &g_Toplist, &Toplist::Hook_ClientCommand, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, m_ServerDll, &g_Toplist, &Toplist::LevelShutdown, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_Toplist , &Toplist::OnLevelInit, false);
	
	ismm->AddListener(this, &g_Listener);

	return true;

}
bool Toplist::Unload(char *error, size_t maxlen) {
	m_CurrentPlayers = 0;
	m_GameEventManager->RemoveListener(this);
	return true;
}
void Toplist::AllPluginsLoaded() {
	META_LOG(g_PLAPI, "Trying to add Kreedz Climbing events...");
	if (!m_GameEventManager->LoadEventsFromFile("../kz/resource/modevents.res")) {
		META_LOG(g_PLAPI, "Failed.");
	}
	else {
		META_LOG(g_PLAPI, "Success.");
	}

	m_GameEventManager->AddListener(this, "player_starttimer", true);
	m_GameEventManager->AddListener(this, "player_starttimer2", true);
	m_GameEventManager->AddListener(this, "player_stoptimer", true);
	m_GameEventManager->AddListener(this, "player_stoptimer2", true);

}

bool Toplist::OnLevelInit(char const *pMapName,char const *pMapEntities,char const *pOldLevel,char const *pLandmarkName,bool loadGame,bool background) {
	currentMap = pMapName;

	std::thread mRegister( &Toplist::registerMap , this);
	mRegister.detach();
	return true;
}

bool Toplist::registerMap() {
	try {
		http::Request request("http://nagarus.net/toplist/mapregistration.php");
		http::Response response;

		// send a post request
		std::string prData = "map=" + currentMap;

		response = request.send("POST", prData, {
			"Content-Type: application/x-www-form-urlencoded"
			});

		ConMsg("[Toplist POST] %s\n", response.body.data()); // print the result
	}
	catch (const std::exception& e)
	{
		ConMsg("[Toplist ERROR] %s\n", e.what());
		return false;
	}
	return true;
}

bool Toplist::registerPlayer(std::string steamid, std::string player) {
	try {
		http::Request request("http://nagarus.net/toplist/registerplayer.php");
		http::Response response;

		// send a post request
		std::string prData = "steamid=" + steamid + "&player=" + player;

		response = request.send("POST", prData, {
			"Content-Type: application/x-www-form-urlencoded"
			});

		ConMsg("[Toplist POST] %s\n", response.body.data()); // print the result
	}
	catch (const std::exception& e)
	{
		ConMsg("[Toplist ERROR] %s\n", e.what());
		return false;
	}
	return true;
}

void Toplist::ClientPutInServer(edict_t *pEntity, char const *playername) {


	m_Players.push_back(s_Players());
	
	m_Players[m_CurrentPlayers].Client = pEntity;
	m_Players[m_CurrentPlayers].userID = m_Engine->GetPlayerUserId(pEntity);
	m_Players[m_CurrentPlayers].playername = playername;
	
	ConMsg("\n[TOPLIST DEBUG]Registrating player: %s, userID: %d, SteamID: %s\n", m_Players[m_CurrentPlayers].playername, m_Players[m_CurrentPlayers].userID, m_Engine->GetPlayerNetworkIDString(m_Players[m_CurrentPlayers].Client));
	ConMsg("Current Players: %d\n", m_CurrentPlayers);
	
	for (int j = 0; j != m_Players.size(); j++) {
		ConMsg("UserID: %d, Player: %s, SteamID: %s\n", m_Players[j].userID, m_Players[j].playername, m_Engine->GetPlayerNetworkIDString(m_Players[m_CurrentPlayers].Client));
	}
	
	std::thread pRegister(&Toplist::registerPlayer, this, m_Engine->GetPlayerNetworkIDString(m_Players[m_CurrentPlayers].Client), m_Players[m_CurrentPlayers].playername);
	pRegister.detach();
	
	++m_CurrentPlayers;

}

void Toplist::FireGameEvent(IGameEvent* event) {
	if (!event || event->IsEmpty())
		return;

	const char *course = getCourse(event);

	if ((Q_strcmp(event->GetName(), "player_starttimer") == 0) || Q_strcmp(event->GetName(), "player_starttimer2") == 0) {
		for(int i = 0; i != m_Players.size(); ++i){

		}
	}

	if ((Q_strcmp(event->GetName(), "player_stoptimer") == 0) || Q_strcmp(event->GetName(), "player_stoptimer2") == 0) {
		for (int i = 0; i != m_Players.size(); i++) {
			if ((m_Players[i].userID) == event->GetInt("userid")) {

				auto it = std::find_if(m_Players.begin(),m_Players.end(),[&](const s_Players &player) -> bool { return player.userID == event->GetInt("userid"); });

				ConMsg("\n[TOPLIST DEBUG] From my lambda: %UserID %d, steamID: %s, Player: %s EventUID: %d\n",it->userID , m_Engine->GetPlayerNetworkIDString(it->Client), it->playername, event->GetInt("userid"));
				ConMsg("\n Printing All Player tables: \n");

				int seconds = (int)event->GetFloat("time");
				int miliseconds = event->GetInt("milliseconds");
				ConMsg("[TOPLIST] Sending add record data: PlayerID - %s, Player - %s", m_Engine->GetPlayerNetworkIDString(m_Players[i].Client), m_Players[i].playername);
				std::thread rRegister( &Toplist::addRecord, this, currentMap, course, m_Engine->GetPlayerNetworkIDString(m_Players[i].Client), m_Players[i].playername, seconds, miliseconds, event->GetInt("checkpoints"), event->GetInt("teleports"));
				rRegister.detach();
			}
		}
	}
}

bool Toplist::addRecord(std::string mapName, std::string courseName, std::string steamID, std::string playerName, int seconds, int miliseconds, int checkpoints, int teleports) {

	try {
		http::Request request("http://nagarus.net/toplist/addrecord.php");
		http::Response response;

		// send a post request

		std::string prData = "map=" + mapName + "&course=" + courseName + "&steamid=" + steamID + "&player=" + playerName + "&seconds=" + std::to_string(seconds) + "&ms=" + std::to_string(miliseconds) + "&checkpoints=" + std::to_string(checkpoints) + "&teleports=" + std::to_string(teleports);

		response = request.send("POST", prData, {
			"Content-Type: application/x-www-form-urlencoded"
			});
		ConMsg("[Toplist POST] %s\n", response.body.data()); // print the result

	}catch (const std::exception& e)
	{
		ConMsg("[Toplist ERROR] %s\n",e.what());
		return false;
	}

	return true;
}


void Toplist::ClientDisconnect(edict_t *pEntity) {
	
	for (int i = 0; i < m_CurrentPlayers; i++) {
		if ((m_Players[i].userID) == m_Engine->GetPlayerUserId(pEntity)) {
			ConMsg("[Toplist] Player with id [%d] : %s ", m_Players[i].Client, m_Players[i].playername);
			ConMsg("Left Server\n");
			m_Players.erase(m_Players.begin() + i);
			--m_CurrentPlayers;
		}
	}

	

	RETURN_META(MRES_IGNORED);
}
void Toplist::LevelShutdown(void) {
	m_CurrentPlayers = 0;
	m_Players.clear();
}

void *ToplistListener::OnMetamodQuery(const char *iface, int *ret)
{
	if (strcmp(iface, "Toplist") == 0)
	{
		if (ret)
			*ret = IFACE_OK;
		return static_cast<void *>(&g_Toplist);
	}
	if (ret)
		*ret = IFACE_FAILED;

	return NULL;
}

void Toplist::Hook_ClientCommand(edict_t *pEntity, const CCommand &args) {
	const char *cmd = args.Arg(0);
}

char* Toplist::concat(const char *s1, const char *s2)
{
	char *result = (char*)malloc(strlen(s1) + strlen(s2) + 1);//+1 for the null-terminator
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

const char *Toplist::getCourse(IGameEvent *event) {

	int i_ID = event->GetInt("courseid");
	char buf[2];
	const char *courseName;
	const char *searchSource = "Course";
	itoa(i_ID, buf, 10);
	courseName = concat(searchSource, buf);
	ConMsg("[Toplist] Trying to find Consle Var: %s\n", courseName);

	ConVar* foundCvar;
	bool loadCustomCvar = !!(LOADINTERFACE("vstdlib.dll", CVAR_INTERFACE_VERSION, cvar)); // get the interface for customCvar. 

	if (loadCustomCvar) // check if it found the interface
	{
		foundCvar = cvar->FindVar(courseName); // find the variable

		if (foundCvar) {
			courseName = foundCvar->GetString();
		}
		else {
			courseName = event->GetString("coursename");
		}
	}

	return courseName;
}