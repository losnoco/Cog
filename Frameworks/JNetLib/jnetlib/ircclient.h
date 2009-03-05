#ifndef _JNL_IRCCLIENT_H_
#define _JNL_IRCCLIENT_H_
#ifdef WIN32
#include <malloc.h>
#endif
#include "connection.h"
#include <string>
#include <list>
#include <ctype.h>
/*
**  ERRORS
*/
#define ERR_NOSUCHNICK       401
#define ERR_NOSUCHSERVER     402
#define ERR_NOSUCHCHANNEL    403
#define ERR_CANNOTSENDTOCHAN 404
#define ERR_TOOMANYCHANNELS  405
#define ERR_WASNOSUCHNICK    406
#define ERR_TOOMANYTARGETS   407
#define ERR_NOORIGIN         409
#define ERR_NORECIPIENT      411
#define ERR_NOTEXTTOSEND     412
#define ERR_NOTOPLEVEL       413
#define ERR_WILDTOPLEVEL     414
#define ERR_UNKNOWNCOMMAND   421
#define ERR_NOMOTD           422
#define ERR_NOADMININFO      423
#define ERR_FILEERROR        424
#define ERR_NONICKNAMEGIVEN  431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE    433
#define ERR_NICKCOLLISION    436
#define ERR_USERNOTINCHANNEL 441
#define ERR_NOTONCHANNEL     442
#define ERR_USERONCHANNEL    443
#define ERR_NOLOGIN          444
#define ERR_SUMMONDISABLED   445
#define ERR_USERSDISABLED    446
#define ERR_NOTREGISTERED    451
#define ERR_NEEDMOREPARAMS   461
#define ERR_ALREADYREGISTRED 462
#define ERR_NOPERMFORHOST    463
#define ERR_PASSWDMISMATCH   464
#define ERR_YOUREBANNEDCREEP 465
#define ERR_KEYSET           467
#define ERR_CHANNELISFULL    471
#define ERR_UNKNOWNMODE      472
#define ERR_INVITEONLYCHAN   473
#define ERR_BANNEDFROMCHAN   474
#define ERR_BADCHANNELKEY    475
#define ERR_NOPRIVILEGES     481
#define ERR_CHANOPRIVSNEEDED 482
#define ERR_CANTKILLSERVER   483
#define ERR_NOOPERHOST       491
#define ERR_UMODEUNKNOWNFLAG 501
#define ERR_USERSDONTMATCH   502

/*
**  Replies
*/
#define RPL_NONE             300
#define RPL_USERHOST         302
#define RPL_ISON             303
#define RPL_AWAY             301
#define RPL_UNAWAY           305
#define RPL_NOWAWAY          306
#define RPL_WHOISUSER        311
#define RPL_WHOISSERVER      312
#define RPL_WHOISOPERATOR    313
#define RPL_WHOISIDLE        317
#define RPL_ENDOFWHOIS       318
#define RPL_WHOISCHANNELS    319
#define RPL_WHOWASUSER       314
#define RPL_ENDOFWHOWAS      369
#define RPL_LISTSTART        321
#define RPL_LIST             322
#define RPL_LISTEND          323
#define RPL_CHANNELMODEIS    324
#define RPL_NOTOPIC          331
#define RPL_TOPIC            332
#define RPL_WHOISREALIP      338
#define RPL_INVITING         341
#define RPL_SUMMONING        342
#define RPL_VERSION          351
#define RPL_WHOREPLY         352
#define RPL_ENDOFWHO         315
#define RPL_NAMREPLY         353
#define RPL_ENDOFNAMES       366
#define RPL_LINKS            364
#define RPL_ENDOFLINKS       365
#define RPL_BANLIST          367
#define RPL_ENDOFBANLIST     368
#define RPL_INFO             371
#define RPL_ENDOFINFO        374
#define RPL_MOTDSTART        375
#define RPL_MOTD             372
#define RPL_ENDOFMOTD        376
#define RPL_YOUREOPER        381
#define RPL_REHASHING        382
#define RPL_TIME             391
#define RPL_USERSSTART       392
#define RPL_USERS            393
#define RPL_ENDOFUSERS       394
#define RPL_NOUSERS          395
#define RPL_TRACELINK        200
#define RPL_TRACECONNECTING  201
#define RPL_TRACEHANDSHAKE   202
#define RPL_TRACEUNKNOWN     203
#define RPL_TRACEOPERATOR    204
#define RPL_TRACEUSER        205
#define RPL_TRACESERVER      206
#define RPL_TRACENEWTYPE     208
#define RPL_TRACELOG         261
#define RPL_STATSLINKINFO    211
#define RPL_STATSCOMMANDS    212
#define RPL_STATSCLINE       213
#define RPL_STATSNLINE       214
#define RPL_STATSILINE       215
#define RPL_STATSKLINE       216
#define RPL_STATSYLINE       218
#define RPL_ENDOFSTATS       219
#define RPL_STATSLLINE       241
#define RPL_STATSUPTIME      242
#define RPL_STATSOLINE       243
#define RPL_STATSHLINE       244
#define RPL_STATSULINE       246
#define RPL_UMODEIS          221
#define RPL_LUSERCLIENT      251
#define RPL_LUSEROP          252
#define RPL_LUSERUNKNOWN     253
#define RPL_LUSERCHANNELS    254
#define RPL_LUSERME          255
#define RPL_ADMINME          256
#define RPL_ADMINLOC1        257
#define RPL_ADMINLOC2        258
#define RPL_ADMINEMAIL       259
#define RPL_LUSERLOCAL       265
#define RPL_LUSERGLOBAL      266

typedef enum
{
	JNL_IRC_RETVAL_OK = 0,
	JNL_IRC_RETVAL_FAIL
}JNL_IRC_RETVAL;

class JNL_IRCParams
{
public:
	std::string middle;
	std::string trailing;
	JNL_IRCParams *m_params;

	JNL_IRCParams::JNL_IRCParams()
	{
		m_params = NULL;
	}
	
	~JNL_IRCParams()
	{
		if(m_params)
		{
			/*
			**  delete children first.
			*/
			delete m_params;
			m_params = NULL;
		}
	}
};

class JNL_IRCMessage
{
public:
	JNL_IRCMessage()
	{
	}
	~JNL_IRCMessage()
	{
	}
public:
	std::string m_prefix;
	std::string m_command;
	JNL_IRCParams m_params;

	const char *command()
	{
		return m_command.c_str();
	}
	int messagenum()
	{
		if(isdigit(m_command[0]))
		{
			return atoi(m_command.c_str());
		}
		return 0;
	}
};

/*
**  JNL_IRCConnection:
**  sends and receives IRC messages
**  When run is called, it stores IRC messages in its queue.
**  After run, you may then call get_message() until there are none (null).
**  
**  The JNL_IRCClient exemplifies this in its version 
**  of run.
*/
class JNL_IRCConnection
{
protected:
	JNL_Connection *m_con;
	std::list<JNL_IRCMessage*> m_messagequeue;
public:
	JNL_IRCConnection();
	virtual ~JNL_IRCConnection();
	bool connect(char *szHost, unsigned short nPort=6667);
	void close()
	{
		m_con->close(1);
	}
	bool send_message(const char *fmt,...);
	bool run(void);
	JNL_IRCMessage *get_message();
protected:
	bool _process_line(const char *szline);
	int _parse_paramlist(const char *szline,JNL_IRCParams &p);
};

/*
**  JNL_IRCClient
**  Very basic, and woefully incomplete client
**  Here's what it can do, and what you probably should use from this:
**  Use the 'sending' block of commands
**  They are, sendpass, nick, oper, quit....etc.
**  
**  Next, there are quite a few virtual methods at your disposal to override.
**  They will be called if this, the base class, JNL_IRCClient's process_message is called.
**  If you want to short circuit those events, then override process_message and do your own
**  event dispatching.
**  
**  Else, just let JNL_IRCClient's  process_message run, and you override the on*** methods.
*/
class JNL_IRCClient
{
protected:
	JNL_IRCConnection m_con;
public:
	JNL_IRCClient()
	{
	}
	virtual ~JNL_IRCClient()
	{
		m_con.close();
	}

	void close()
	{
		m_con.close();
	}
	
	bool connect(char *szHost, unsigned short nPort=6667)
	{
		return m_con.connect(szHost,nPort);
	}
	bool run(void);

	virtual JNL_IRC_RETVAL process_message(JNL_IRCMessage &m);

	bool sendpass(char *password);
	bool nick(char *nick);
	bool oper(char *user, char *password);
	bool quit(char *swansong);
	bool part(char *swansong);
	bool join(char *channel, char *key=NULL);
	bool setmode(char *channel, int isadd, char mode, char *param=NULL);
	bool topic(char *channel, char *topic=NULL);
	bool userregistration(char *username="poopoo", char *hostname="hostname", char *servername="servername", char *realname="poopoo platter");
	bool opuser(char *channel, char *nick)
	{
		return setmode(channel,1,'o',nick);
	}
	bool deopuser(char *channel, char *nick)
	{
		return setmode(channel,0,'o',nick);
	}
	bool voiceuser(char *channel, char *nick)
	{
		return setmode(channel,1,'v',nick);
	}
	bool devoiceuser(char *channel, char *nick)
	{
		return setmode(channel,0,'v',nick);
	}
	bool banuser(char *channel, char *banstring)
	{
		return setmode(channel,1,'b',banstring);
	}
	bool unbanuser(char *channel, char *banstring)
	{
		return setmode(channel,0,'b',banstring);
	}
	bool names(char *channel=NULL);
	bool list(char *channel=NULL);
	bool invite(char *nick, char *channel);
	bool kick(char *channel, char *user, char *swansong=NULL);
	bool version(char *server=NULL);
	bool stats(char query = 0, char *server=NULL);
	bool links(char *remote=NULL, char *mask=NULL);
	bool time(char *server=NULL);
	/*
	**  Missing trace and connect
	*/
	bool admin(char *server=NULL);
	bool info(char *server=NULL);
	bool privmsg(char *recipient, char *msg, bool baction=false);
	bool notice(char *recipient, char *msg);
	bool who(char *nick, char o=0);
	bool whois(char *server,char *nick);
	bool ping(char *server);
	bool pong(char *server);

	/*
	**  Missing whowas and kill, and error
	*/
protected:

	virtual JNL_IRC_RETVAL onErrNoSuckNick(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoSuchServer(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoSuchChannel(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrCannotSendToChan(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrTooManyChannels(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrWasNoSuchNick(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrTooManyTargets(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoOrigin(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoRecipient(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoTextToSend(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoTopLevel(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrWildTopLevel(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrUnknownCommand(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoMOTD(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoAdminInfo(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrFileError(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoNickNameGiven(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrOneUSNickName(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNickNameInUse(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNickCollision(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNotOnChannel(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrUserNotInChannel(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrUserOnChannel(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoLogin(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrSummonDisabled(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrUserDisabled(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNotRegistered(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNeedMoreParams(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrAlreadyRegistered(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoPermForHost(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrPasswdMismatch(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrYourBannedCreep(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrKeySet(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrChannelIsFull(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrUnknownMode(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrInviteOnlyChan(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrBannedFromChan(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrBadChannelKey(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoPrivileges(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrChanOpPrivsNeeded(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrCantKillServer(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrNoOperHost(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrUModeUnknownFlag(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onErrUsersDontMatch(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	/*
	**  REPLY BLOCK
	*/
	virtual JNL_IRC_RETVAL onRplNone(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplUserHost(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplIsOn(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplAway(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplNoAway(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplUnAway(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoIsUser(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoIsServer(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoIsIdle(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoIsOperator(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfWhoIs(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoIsChannels(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoWasUser(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfWhoWas(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplListStart(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplList(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplListEnd(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplChannelModeIs(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplNoTopic(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTopic(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoIsRealIP(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplInviting(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplSummoning(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplVersion(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplWhoReply(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndWho(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplNameReply(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfNames(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLinks(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfLinks(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplBanList(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfBanList(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplInfo(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEnofOfInfo(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplMOTDStart(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplMOTD(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfMOTD(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplYoureOper(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplRehasing(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTime(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplUserStart(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplUser(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfUsers(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplNoUsers(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceLink(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceConnecting(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceHandleShake(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceUnknown(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceOperator(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceUser(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceServer(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceNewType(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplTraceLog(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsLinkInfo(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsCommands(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsCLine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsNLine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsILine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsKLine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsYLine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplEndOfStats(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsLLine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsUptime(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsOLine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsHLine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplStatsULine(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplUModeIs(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLUserClient(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLUserOp(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLUserUknown(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLUserChannels(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLUserMe(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplAdminMe(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplAdminLoc1(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplAdminLoc2(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplAdminEmail(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLUserLocal(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onRplLUserGlobal(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};		

	/*
	**  Messages from server
	*/
	virtual JNL_IRC_RETVAL onJoin(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onPart(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onMode(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onTopic(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onInvite(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onKick(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onStats(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onPrivMsg(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	virtual JNL_IRC_RETVAL onNotice(JNL_IRCMessage &m){m.messagenum();return JNL_IRC_RETVAL_OK;};
	/*
	**  Don't override this one (onPing), or if you do...make sure you call the base class.
	*/
	virtual JNL_IRC_RETVAL onPing(JNL_IRCMessage &m){pong((char*)m.m_params.trailing.c_str());return JNL_IRC_RETVAL_OK;};
		

};
#endif
