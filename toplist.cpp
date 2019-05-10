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

	loadCustomCvar = !!(LOADINTERFACE("vstdlib.dll", CVAR_INTERFACE_VERSION, cvar)); // get the interface for customCvar.
	if (loadCustomCvar) {
		META_LOG(g_PLAPI, "vstdlib interface loaded.");
	}else {
		META_LOG(g_PLAPI, "vstdlib interface failed to load.");
		return false;
	}


		strcpy(iface_buffer, INTERFACEVERSION_SERVERGAMEDLL);
	FIND_IFACE(GetServerFactory, m_ServerDll, num, iface_buffer, IServerGameDLL *)
		strcpy(iface_buffer, INTERFACEVERSION_VENGINESERVER);
	FIND_IFACE(GetEngineFactory, m_Engine, num, iface_buffer, IVEngineServer *)
		strcpy(iface_buffer, INTERFACEVERSION_SERVERGAMECLIENTS);
	FIND_IFACE(GetServerFactory, m_ServerClients, num, iface_buffer, IServerGameClients *)
		strcpy(iface_buffer, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	FIND_IFACE(GetEngineFactory, m_GameEventManager, num, iface_buffer, IGameEventManager2 *);

	META_LOG(g_PLAPI, "Toplist plugin Loaded");

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
		std::string s(TOPLIST_IP);
		http::Request request(s + "mapregistration.php");
		http::Response response;

		// send a post request
		std::string prData = "map=" + currentMap;

		response = request.send("POST", prData, {
			"Content-Type: application/x-www-form-urlencoded"
		});

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
		std::string s(TOPLIST_IP);
		http::Request request(s + "registerplayer.php");
		http::Response response;

		// send a post request
		std::string prData = "steamid=" + steamid + "&player=" + player;

		response = request.send("POST", prData, {
			"Content-Type: application/x-www-form-urlencoded"
			});

	}
	catch (const std::exception& e)
	{
		ConMsg("[Toplist ERROR] %s\n", e.what());
		return false;
	}
	return true;
}

bool Toplist::addRecord(std::string mapName, const char *courseName, std::string steamID, std::string playerName, int seconds, int miliseconds, int checkpoints, int teleports) {

	try {
		std::string s(TOPLIST_IP);
		http::Request request(s + "addrecord.php");
		http::Response response;

		// send a post request

		std::string prData = "map=" + mapName + "&course=" + courseName + "&steamid=" + steamID + "&player=" + playerName + "&seconds=" + std::to_string(seconds) + "&ms=" + std::to_string(miliseconds) + "&checkpoints=" + std::to_string(checkpoints) + "&teleports=" + std::to_string(teleports);

		response = request.send("POST", prData, {
			"Content-Type: application/x-www-form-urlencoded"
		});

		
		ConMsg("%s\n", response.body.data());

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
	
	std::thread pRegister(&Toplist::registerPlayer, this, m_Engine->GetPlayerNetworkIDString(m_Players[m_CurrentPlayers].Client), m_Players[m_CurrentPlayers].playername);
	pRegister.detach();
	
	++m_CurrentPlayers;

}

void Toplist::FireGameEvent(IGameEvent* event) {
	if (!event || event->IsEmpty())
		return;

	const char *course = getCourse(event);

	if ((Q_strcmp(event->GetName(), "player_stoptimer") == 0) || Q_strcmp(event->GetName(), "player_stoptimer2") == 0) {
		for (int i = 0; i != m_Players.size(); i++) {
			if ((m_Players[i].userID) == event->GetInt("userid")) {

				auto it = std::find_if(m_Players.begin(),m_Players.end(),[&](const s_Players &player) -> bool { return player.userID == event->GetInt("userid"); });

				int seconds = (int)event->GetFloat("time");
				int miliseconds = event->GetInt("milliseconds");
				std::thread rRegister( &Toplist::addRecord, this, currentMap, course, m_Engine->GetPlayerNetworkIDString(m_Players[i].Client), m_Players[i].playername, seconds, miliseconds, event->GetInt("checkpoints"), event->GetInt("teleports"));
				rRegister.detach();
			}
		}
	}
}


void Toplist::ClientDisconnect(edict_t *pEntity) {
	
	for (int i = 0; i < m_CurrentPlayers; i++) {
		if ((m_Players[i].userID) == m_Engine->GetPlayerUserId(pEntity)) {
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
	char *result = new char[(strlen(s1) + strlen(s2) + 1)];//+1 for the null-terminator
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}


const char *Toplist::getCourse(IGameEvent *event) {

	static char courseName[255];

	int i_ID = event->GetInt("courseid");
	char buf[2];
	const char *searchSource = "Course";
	itoa(i_ID, buf, 10);
	char *tmp = concat(searchSource, buf);
	strcpy(courseName,tmp);
	delete[] tmp;

	ConVar* foundCvar;
	 
	foundCvar = cvar->FindVar(courseName); // find the variable

	if (foundCvar) {
		strcpy(courseName, foundCvar->GetString());
	}
	else {
		strcpy(courseName, event->GetString("coursename"));
	}
	

	return courseName;
}