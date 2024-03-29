/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _PLAYER_H
#define _PLAYER_H

#include "fix.h"
#include "vecmat.h"
#include "weapon.h"

#define MAX_PLAYERS_D2		8
#define MAX_PLAYERS_D2X		16
#define MAX_PLAYERS			16
#define MAX_COOP_PLAYERS	3
#define MAX_MULTI_PLAYERS (MAX_PLAYERS + MAX_COOP_PLAYERS)
#define MAX_PLAYER_COLORS	8

// Initial CPlayerData stat values
#define INITIAL_ENERGY  I2X(100)    // 100% energy to start
#define INITIAL_SHIELDS I2X(100)    // 100% shields to start

#define MAX_ENERGY      I2X(200)    // go up to 200
#define MAX_SHIELDS     I2X(200)

#define INITIAL_LIVES               3   // start off with 3 lives

// Values for special flags
#define PLAYER_FLAGS_INVULNERABLE   1       // Player is invulnerable
#define PLAYER_FLAGS_BLUE_KEY       2       // Player has blue key
#define PLAYER_FLAGS_RED_KEY        4       // Player has red key
#define PLAYER_FLAGS_GOLD_KEY       8       // Player has gold key
#define PLAYER_FLAGS_FLAG           16      // Player has his team's flag
#define PLAYER_FLAGS_UNUSED         32      //
#define PLAYER_FLAGS_FULLMAP        64      // Player can see unvisited areas on map
#define PLAYER_FLAGS_AMMO_RACK      128     // Player has ammo rack
#define PLAYER_FLAGS_CONVERTER      256     // Player has energy->shield converter
#define PLAYER_FLAGS_FULLMAP_CHEAT  512     // Player can see unvisited areas on map normally
#define PLAYER_FLAGS_QUAD_LASERS    1024    // Player shoots 4 at once
#define PLAYER_FLAGS_CLOAKED        2048    // Player is cloaked for awhile
#define PLAYER_FLAGS_AFTERBURNER    4096    // Player's afterburner is engaged
#define PLAYER_FLAGS_HEADLIGHT      8192    // Player has headlight boost
#define PLAYER_FLAGS_HEADLIGHT_ON   16384   // is headlight on or off?
#define PLAYER_FLAGS_SLOWMOTION		32768
#define PLAYER_FLAGS_BULLETTIME		65536

#define PLAYER_FLAGS_ALL_KEYS			(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY)

#define AFTERBURNER_MAX_TIME    		I2X (5)    // Max time afterburner can be on.
#define CALLSIGN_LEN                8       // so can use as filename (was: 12)

// Amount of time CPlayerData is cloaked.
#define CLOAK_TIME_MAX          I2X (30)
#define INVULNERABLE_TIME_MAX   I2X (30)

#define PLAYER_STRUCT_VERSION   17  // increment this every time CPlayerData struct changes

// defines for teams
#define TEAM_BLUE   0
#define TEAM_RED    1

typedef struct tPlayerHostages {
	ushort  nRescued;		// Total number of hostages rescued.
	ushort  nTotal;      // Total number of hostages.
	ubyte   nOnBoard;    // Number of hostages on ship.
	ubyte   nLevel;      // Number of hostages on this level.
} __pack__ tPlayerHostages;
// When this structure changes, increment the constant
// SAVE_FILE_VERSION in playsave.c
class __pack__ CPlayerData {
	public:
		// Who am I data
		char    callsign[CALLSIGN_LEN+1];   // The callsign of this CPlayerData, for net purposes.
		ubyte   netAddress[6];					// The network address of the CPlayerData.
		sbyte   connected;						// Is the CPlayerData connected or not?
		int     nObject;						   // What CObject number this CPlayerData is. (made an int by mk because it's very often referenced)
		int     nPacketsGot;					   // How many packets we got from them
		int     nPacketsSent;					// How many packets we sent to them

		//  -- make sure you're 4 byte aligned now!

		// Game data
		uint    flags;							// Powerup flags, see below...
		fix     energy;						// Amount of energy remaining.
		fix     shields;						// shields remaining (protection)
		ubyte   lives;							// Lives remaining, 0 = game over.
		sbyte   level;							// Current level CPlayerData is playing. (must be signed for secret levels)
		ubyte   laserLevel;					// Current level of the laser.
		sbyte   startingLevel;				// What level the CPlayerData started on.
		short   nKillerObj;					// Who killed me.... (-1 if no one)
		ushort  primaryWeaponFlags;		// bit set indicates the CPlayerData has this weapon.
		ushort  secondaryWeaponFlags;		// bit set indicates the CPlayerData has this weapon.
		ushort  primaryAmmo[MAX_PRIMARY_WEAPONS]; // How much ammo of each nType.
		ushort  secondaryAmmo[MAX_SECONDARY_WEAPONS]; // How much ammo of each nType.
	#if 1 //for inventory system
		ubyte	  nInvuls;
		ubyte   nCloaks;
	#else
		ushort  pad; // Pad because increased weaponFlags from byte to short -YW 3/22/95
	#endif
		//  -- make sure you're 4 byte aligned now

		// Statistics...
		int     lastScore;            // Score at beginning of current level.
		int     score;                // Current score.
		fix     timeLevel;            // Level time played
		fix     timeTotal;            // Game time played (high word = seconds)

		fix     cloakTime;            // Time cloaked
		fix     invulnerableTime;     // Time invulnerable

		short   nScoreGoalCount;       // Num of players killed this level
		short   netKilledTotal;			// Number of times killed total
		short   netKillsTotal;        // Number of net kills total
		short   numKillsLevel;        // Number of kills this level
		short   numKillsTotal;        // Number of kills total
		short   numRobotsLevel;       // Number of initial robots this level
		short   numRobotsTotal;       // Number of robots total
		tPlayerHostages	hostages;
		fix     homingObjectDist;     // Distance of nearest homing CObject.
		sbyte   hoursLevel;           // Hours played (since timeTotal can only go up to 9 hours)
		sbyte   hoursTotal;           // Hours played (since timeTotal can only go up to 9 hours)
	public:
		CPlayerData () { memset (this, 0, sizeof (*this)); }
};


//version 16 structure

#define MAX_PRIMARY_WEAPONS16   5
#define MAX_SECONDARY_WEAPONS16 5

typedef struct player16 {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1]; // The callsign of this CPlayerData, for net purposes.
	ubyte   netAddress[6];         // The network address of the CPlayerData.
	sbyte   connected;              // Is the CPlayerData connected or not?
	int     nObject;                 // What CObject number this CPlayerData is. (made an int by mk because it's very often referenced)
	int     nPacketsGot;          // How many packets we got from them
	int     nPacketsSent;         // How many packets we sent to them

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shields;                // shields remaining (protection)
	ubyte   lives;                  // Lives remaining, 0 = game over.
	sbyte   level;                  // Current level CPlayerData is playing. (must be signed for secret levels)
	ubyte   laserLevel;            // Current level of the laser.
	sbyte   startingLevel;         // What level the CPlayerData started on.
	short   nKillerObj;          // Who killed me.... (-1 if no one)
	ubyte   primaryWeaponFlags;   // bit set indicates the CPlayerData has this weapon.
	ubyte   secondaryWeaponFlags; // bit set indicates the CPlayerData has this weapon.
	ushort  primaryAmmo[MAX_PRIMARY_WEAPONS16];    // How much ammo of each nType.
	ushort  secondaryAmmo[MAX_SECONDARY_WEAPONS16];// How much ammo of each nType.

	//  -- make sure you're 4 byte aligned now

	// Statistics...
	int     last_score;             // Score at beginning of current level.
	int     score;                  // Current score.
	fix     timeLevel;             // Level time played
	fix     timeTotal;             // Game time played (high word = seconds)

	fix     cloakTime;             // Time cloaked
	fix     invulnerableTime;      // Time invulnerable

	short   netKilledTotal;       // Number of times killed total
	short   netKillsTotal;        // Number of net kills total
	short   numKillsLevel;        // Number of kills this level
	short   numKillsTotal;        // Number of kills total
	short   numRobotsLevel;       // Number of initial robots this level
	short   numRobotsTotal;       // Number of robots total
	tPlayerHostages	hostages;
	fix     homingObjectDist;     // Distance of nearest homing CObject.
	sbyte   hoursLevel;            // Hours played (since timeTotal can only go up to 9 hours)
	sbyte   hoursTotal;            // Hours played (since timeTotal can only go up to 9 hours)
} __pack__ player16;

//------------------------------------------------------------------------------

#define N_PLAYER_GUNS 8

class __pack__ CPlayerShip {
	public:
		int			nModel;
		int			nExplVClip;
		fix			mass;
		fix			drag;
		fix			maxThrust;
		fix			reverseThrust;
		fix			brakes;
		fix			wiggle;
		fix			maxRotThrust;
		CFixVector	gunPoints [N_PLAYER_GUNS];

	public:
		CPlayerShip () { memset (this, 0, sizeof (*this)); }
};

/*
 * reads a CPlayerShip structure from a CFILE
 */
void PlayerShipRead(CPlayerShip *ps, CFile& cf);
int EquippedPlayerGun (CObject *objP);
int EquippedPlayerBomb (CObject *objP);
int EquippedPlayerMissile (CObject *objP, int *nMissiles);
void UpdatePlayerWeaponInfo (void);

#endif
