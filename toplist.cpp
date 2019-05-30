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

Toplist* Toplist::pToplist = nullptr;

static userMessageInfo g_userMessages[255];
static int g_iMaxUserMessages = 0;

static int UserMessageFromName(const char *pName)
{
	if (!pName)
		return -1;

	for (int i = 0; i < g_iMaxUserMessages; i++)
	{
		if (strcmp(pName, g_userMessages[i].msgName) == 0)
			return g_userMessages[i].messageID;
	}

	return -1;
}

bool Toplist::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) {

	m_CurrentPlayers = 0;

	PLUGIN_SAVEVARS();

	char iface_buffer[255];
	int num = 0;

	// logger
	plog::init(plog::debug, "kz/addons/metamod/toplist/log.txt");

	m_ToplistIP = getToplistIP();

	// server & server ip
	pToplist = this;

	std::thread srv(&Toplist::startServer, this, m_ToplistIP);
	srv.detach();


	// load interface
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

	while (true)
	{
		int dummy;
		if (!m_ServerDll->GetUserMessageInfo(g_iMaxUserMessages, g_userMessages[g_iMaxUserMessages].msgName, sizeof(g_userMessages[g_iMaxUserMessages].msgName), dummy))
			break;

			g_userMessages[g_iMaxUserMessages].messageID = g_iMaxUserMessages;

			g_iMaxUserMessages++;
	}


	return true;

}
bool Toplist::Unload(char *error, size_t maxlen) {

	filter.RemoveAllRecipients();
	m_Service.stop();
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
		http::Request request("http://" + m_ToplistIP + "/toplist/mapregistration.php");
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
		http::Request request("http://" + m_ToplistIP + "/toplist/registerplayer.php");
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
		http::Request request("http://" + m_ToplistIP + "/toplist/addrecord.php");
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

	filter.AddRecipient(m_CurrentPlayers);

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

	filter.RemoveRecipient(m_CurrentPlayers);

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

std::string Toplist::getToplistIP() {
	std::ifstream info;
	std::string line;
	std::smatch match;
	std::regex rgx("TOPLIST_IP ([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})");
	std::string ip;

	info.open("kz/addons/metamod/toplist/info.txt");
	if (info.good())
	{
		while (getline(info, line))
		{
			if (std::regex_search(line, match, rgx))
			{
				ip = match.str(1);
			}
			else {
				ip = "127.0.0.1";
			}
		}
	}
	else {
		ConMsg("\n[TOPLIST ERROR] Failed to open info file\n");
	}
	info.close();

	return ip;
}

void Toplist::sendDataToGame(std::string data) {

	char msgText[2048];
	*msgText = 0;

	int n = data.length();
	char *recMsg = new char[n + 1];
	strcpy(recMsg, data.c_str());

	strcat_s(msgText, recMsg);

	ColorClientPrint(filter, HUD_PRINTTALK, 1, msgText);

	delete recMsg;
}

void Toplist::startServer(std::string ip) {

	boost::asio::io_service::work serviceWork(m_Service);

	PLOGD << "[TOPLIST] - Running socket server...\n";
	m_Server = std::make_shared<server>(m_Service, "127.0.0.1", 5000);

	m_Service.run();
}


void Toplist::UserMessageBegin(IRecipientFilter &filter, const char *messagename) {

	if (g_pMsgBuffer)
	{
		Warning("Tried to start a new usermessage while one hasn't been sent. This has caused a memory leak.");
	}

	g_pMsgBuffer = NULL;

	int id = UserMessageFromName(messagename);

	if (id == -1)
	{
		Warning("Specified unknown usermessage name %s, can't send usermessage.\n", messagename);
	}

	g_pMsgBuffer = m_Engine->UserMessageBegin(&filter, id);
}

void Toplist::MessageEnd(void)
{
	m_Engine->MessageEnd();

	g_pMsgBuffer = NULL;
}

void Toplist::MessageWriteByte(int iValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteByte(iValue);
}

void Toplist::MessageWriteChar(int iValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteChar(iValue);
}

void Toplist::MessageWriteShort(int iValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteShort(iValue);
}

void Toplist::MessageWriteWord(int iValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteWord(iValue);
}

void Toplist::MessageWriteLong(int iValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteLong(iValue);
}

void Toplist::MessageWriteFloat(float flValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteFloat(flValue);
}

void Toplist::MessageWriteAngle(float flValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteBitAngle(flValue, 8);
}

void Toplist::MessageWriteCoord(float flValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteBitCoord(flValue);
}

void Toplist::MessageWriteVec3Coord(const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteBitVec3Coord(rgflValue);
}

void Toplist::MessageWriteVec3Normal(const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteBitVec3Normal(rgflValue);
}

void Toplist::MessageWriteAngles(const QAngle& rgflValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteBitAngles(rgflValue);
}

void Toplist::MessageWriteString(const char *sz)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteString(sz);
}

void Toplist::MessageWriteEntity(int iValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteShort(iValue);
}

void Toplist::MessageWriteEHandle(CBaseEntity *pEntity)
{
	Warning("TODO MessageWriteEHandle - not implemented.\n");
}

// bitwise
void Toplist::MessageWriteBool(bool bValue)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteOneBit(bValue ? 1 : 0);
}

void Toplist::MessageWriteUBitLong(unsigned int data, int numbits)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteUBitLong(data, numbits);
}

void Toplist::MessageWriteSBitLong(int data, int numbits)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteSBitLong(data, numbits);
}

void Toplist::MessageWriteBits(const void *pIn, int nBits)
{
	if (!g_pMsgBuffer)
	{
		Warning("WRITE_ usermessage called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteBits(pIn, nBits);
}

// Sends a text message to a player (or players) specified by the recipient filter. To use:
// CRecipientFilter filter;
// filter.AddRecipient( player entity index ) as many as you want.
// Then do ClientPrint( filter, etc... )
//
// Example: ClientPrint( filter, HUD_PRINTTALK, "Hello, this is a chat message" );
void Toplist::ClientPrint(IRecipientFilter& filter, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4) {
	UserMessageBegin(filter, "TextMsg");
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);

	if (param1)
		WRITE_STRING(param1);
	else
		WRITE_STRING("");

	if (param2)
		WRITE_STRING(param2);
	else
		WRITE_STRING("");

	if (param3)
		WRITE_STRING(param3);
	else
		WRITE_STRING("");

	if (param4)
		WRITE_STRING(param4);
	else
		WRITE_STRING("");

	MessageEnd();
}

void Toplist::ColorClientPrint(IRecipientFilter& filter, int msg_dest, int bChat, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4) {
	
	UserMessageBegin(filter, "SayText2");

	WRITE_BYTE(0); // Entid of who's talking, 0 for world
	WRITE_BYTE(bChat); // Act like a chat message and play sound if true, otherwise silent message false
	WRITE_STRING(msg_name); // The message itself

	if (param1)
		WRITE_STRING(param1);
	else
		WRITE_STRING("");

	if (param2)
		WRITE_STRING(param2);
	else
		WRITE_STRING("");

	if (param3)
		WRITE_STRING(param3);
	else
		WRITE_STRING("");

	if (param4)
		WRITE_STRING(param4);
	else
		WRITE_STRING("");

	MessageEnd();
}