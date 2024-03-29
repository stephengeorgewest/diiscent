#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#define PATCH12

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "descent.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "ipx.h"
#include "ipx_udp.h"
#include "menu.h"
#include "key.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "menu.h"
#include "text.h"
#include "byteswap.h"
#include "netmisc.h"
#include "kconfig.h"
#include "autodl.h"
#include "tracker.h"
#include "gamefont.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

extern int NetworkWhoIsMaster(), NetworkHowManyConnected(), GetMyNetRanking();
extern char bPauseableMenu;
//char *NetworkModeNames[]={"Anarchy", "Team Anarchy", "Robo Anarchy", "Cooperative", "Capture the Flag", "Hoard", "Team Hoard", "Unknown"};

void DoShowNetGameHelp (void)
{
	CMenu		m (30);
   char		szText [MENU_MAX_TEXTLEN];
	int		i, eff;
#if DBG
	int pl;
#endif
	//char *eff_strings[]={"trashing", "really hurting", "seriously affecting", "hurting", "affecting", "tarnishing"};

sprintf (szText, TXT_INFO_GAME, netGame.m_info.szGameName);
m.AddText (szText);
sprintf (szText, TXT_INFO_MISSION, netGame.m_info.szMissionTitle);
m.AddText (szText);
sprintf (szText, TXT_INFO_LEVEL, netGame.m_info.GetLevel ());
m.AddText (szText);
sprintf (szText, TXT_INFO_SKILL, MENU_DIFFICULTY_TEXT (netGame.m_info.difficulty));
m.AddText (szText);
sprintf (szText, TXT_INFO_MODE, GT (537 + netGame.m_info.gameMode));
m.AddText (szText);
sprintf (szText, TXT_INFO_SERVER, gameData.multiplayer.players [NetworkWhoIsMaster()].callsign);
m.AddText (szText);
sprintf (szText, TXT_INFO_PLRNUM, NetworkHowManyConnected (), netGame.m_info.nMaxPlayers);
m.AddText (szText);
sprintf (szText, TXT_INFO_PPS, netGame.GetPacketsPerSec ());
m.AddText (szText);
sprintf (szText, TXT_INFO_SHORTPKT, netGame.GetShortPackets () ? "Yes" : "No");
m.AddText (szText);
#if DBG
pl = (int) ((double (networkData.nTotalMissedPackets) / double (networkData.nTotalPacketsGot + networkData.nTotalMissedPackets)) * 100.0);
if (pl < 0)
	pl = 0;
sprintf (szText, TXT_INFO_LOSTPKT, networkData.nTotalMissedPackets, pl);
m.AddText (szText);
#endif
if (netGame.GetScoreGoal ())
	sprintf (szText, TXT_INFO_SCOREGOAL, netGame.GetScoreGoal () * 5);
sprintf (szText, " ");
sprintf (szText, TXT_INFO_PLRSCONN);
netPlayers.m_info.players [gameData.multiplayer.nLocalPlayer].rank = GetMyNetRanking ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected) {
		if (!gameOpts->multi.bNoRankings) {
			if (i == gameData.multiplayer.nLocalPlayer)
				sprintf (szText, "%s%s (%d/%d)",
							pszRankStrings [netPlayers.m_info.players [i].rank],
							gameData.multiplayer.players [i].callsign,
							networkData.nNetLifeKills,
							networkData.nNetLifeKilled);
			else
				sprintf (szText, "%s%s %d/%d",
							pszRankStrings[netPlayers.m_info.players [i].rank],
							gameData.multiplayer.players [i].callsign,
							gameData.multigame.kills.matrix[gameData.multiplayer.nLocalPlayer][i],
							gameData.multigame.kills.matrix[i][gameData.multiplayer.nLocalPlayer]);
			}
		else
			sprintf (szText, "%s", gameData.multiplayer.players [i].callsign);
		m.AddText (szText);
		}
	m.AddText ("");
	eff = (int)((double)((double) networkData.nNetLifeKills / ((double) networkData.nNetLifeKilled + (double) networkData.nNetLifeKills))*100.0);
	if (eff < 0)
		eff = 0;
	if (gameData.app.nGameMode & GM_HOARD) {
		if (gameData.score.nPhallicMan == -1)
			sprintf (szText, TXT_NO_RECORD2);
		else
			sprintf (szText, TXT_RECORD3, gameData.multiplayer.players [gameData.score.nPhallicMan].callsign, gameData.score.nPhallicLimit);
		}
	else if (!gameOpts->multi.bNoRankings) {
		sprintf (szText, TXT_EFF_LIFETIME, eff);
	m.AddText (szText);
	if (eff < 60)
		sprintf (szText, TXT_EFF_INFLUENCE, GT(546 + eff / 10));
	else
		sprintf (szText, TXT_EFF_SERVEWELL);
	m.AddText (szText);
	}
//paletteManager.SuspendEffect();
bPauseableMenu = 1;
m.TinyMenu (NULL, "NetGame Information");
//paletteManager.ResumeEffect ();
}

//------------------------------------------------------------------------------

