// Author: l0gic
// I think that anticheat should be seperate component of the gamemode.
// Gamemode should contain funcinality for players and anticheat should be seperate part of it.
// It's main focus should be background checks, that compare playerside information with serverside information.

// Includes
#include <a_samp>
#include <YSI_Coding\y_timers>  // Timer creation
#include <YSI_Data\y_iterate>   // Iterating over arrays
#include <YSI_Data\y_va>        // Formated text
#include <YSI_Coding\y_hooks>   // Public function hooking

// Parameters what can be calibrated
#define max_travel_onfoot       20.0
#define max_travel_vehicle      100.0
#define max_warning_count       3
#define max_warning_timespan    30

// Cheating types
#define CHEAT_TYPE_AIRBREAK     0
#define CHEAT_TYPE_WARP         1

// Callbacks
forward OnPlayerCheating(playerid, cheatid);

// Player related variables
enum ePlayer{
    // Aitbreak / Telepotation
    Float:LastPos[3],
    Warnings,
    LastWarningTime,
    // Teleporting between vehicles
    LastVehicle,
    LastVehicleTime
};
static Player[MAX_PLAYERS][ePlayer];

// Warnings should be reseted, when player connects
hook OnPlayerConnect(playerid){
    Player[playerid][Warnings] = 0;
    return 1;
}

// static in front of function, makes them visible only in this file

// Warning counter to avoid false positive results
static WarningCounter(playerid, cheatid){
    // Avoid false positivies, because cheater will use his cheat in some time range.
    // If current and last warning timespan is more than max_warning_timespan, warnings will be reseted
    new currentTime = gettime();
    if(currentTime - Player[playerid][LastWarningTime] > max_warning_timespan){
        Player[playerid][Warnings] = 0;
    }
    Player[playerid][LastWarningTime] = currentTime;
    Player[playerid][Warnings] += 1;
    // If warning count reaches max OnPlayerAirbreak will be called
    if(Player[playerid][Warnings] == max_warning_count){
        Player[playerid][Warnings] = 0;
        OnPlayerCheating(playerid, cheatid);
    }
    return 1;
}

// Checks if vehicle model is plane or helicopter
static bool:IsAircraft(vehicleid){
	switch(GetVehicleModel(vehicleid)){
        case 460,476,511,512,513,519,520,553,577,592,593,548,425,417,487,488,497,563,447,469: 
            return true;
    }
	return false;
}

// Just for debugging purpose
static DebugMsg(playerid, color, const fmat[], va_args<>){
	new str[145];
	va_format(str, sizeof (str), fmat, va_start<3>);
	return SendClientMessage(playerid, color, str);
}

ptask CheckForWarp[100](playerid){
    new vehicleid = GetPlayerVehicleID(playerid);
    if(vehicleid == 0)return 1;
    // Last vehicle doesn't match with current one
    if(Player[playerid][LastVehicle] != vehicleid){
        // Last vehicle detection and current vehicle detection delay is less then 200ms
        // Then player is warping between vehicles
        new currentTime = GetTickCount();
        if(currentTime - Player[playerid][LastVehicleTime] < 200){
            OnPlayerCheating(playerid, CHEAT_TYPE_WARP);
        // Store current vehicle
        }else{
            Player[playerid][LastVehicle] = vehicleid;
            Player[playerid][LastVehicleTime] = currentTime;
        }
    } 
    return 1;
}

// Checks players for using speed or teleport hacks
ptask CheckForAirbreak[1000](playerid) {
    new playerState = GetPlayerState(playerid);
    if(playerState == PLAYER_STATE_ONFOOT){

        // Get current player position
        new Float:x, Float:y, Float:z, Float:dis;
        GetPlayerPos(playerid, x, y, z);

        // Compare current player position, with last position
        new bool:suspicious = (dis = VectorSize(Player[playerid][LastPos][0] - x, Player[playerid][LastPos][1] - y, Player[playerid][LastPos][2] - z)) > max_travel_onfoot;
        
        // Checks possibility if player is falling
        new Float:radius = VectorSize(Player[playerid][LastPos][0] - x, Player[playerid][LastPos][1] - y, 0.0);
        if(radius < 10.0 && Player[playerid][LastPos][2] - z > 5.0){
            suspicious = false;
        }

        // Debugging
        #pragma unused dis
        //DebugMsg(playerid, -1, "ANTICHEAT_DEBUG: onfoot, %f", dis);

        // If suspicious movement is detected
        if(suspicious){
            WarningCounter(playerid, CHEAT_TYPE_AIRBREAK);
        }

        // Store current player position
        Player[playerid][LastPos][0] = x;
        Player[playerid][LastPos][1] = y;
        Player[playerid][LastPos][2] = z;

    }else if(playerState == PLAYER_STATE_DRIVER){

        // Get current players vehicle position
        new Float:x, Float:y, Float:z, vehicleid = GetPlayerVehicleID(playerid), Float:dis;
        GetVehiclePos(vehicleid, x, y, z);

        // Compare current players vehicle position, with last position
        new bool:suspicious = (dis = VectorSize(Player[playerid][LastPos][0] - x, Player[playerid][LastPos][1] - y, Player[playerid][LastPos][2] - z)) > max_travel_vehicle;

        // Checks if players vehicle is falling
        new Float:radius = VectorSize(Player[playerid][LastPos][0] - x, Player[playerid][LastPos][1] - y, 0.0);
        if(radius < 20.0 && Player[playerid][LastPos][2] - z > 5.0){
            suspicious = false;
        }

        // Checks if its aircraft, because there isn't big difference if its speedhacking or not
        if(IsAircraft(vehicleid)){
            suspicious = false;
        }
        
        // Debugging
        #pragma unused dis
        //DebugMsg(playerid, -1, "ANTICHEAT_DEBUG: vehicle, %f", dis);

        // If suspicious movement is detected
        if(suspicious){
            WarningCounter(playerid, CHEAT_TYPE_AIRBREAK);
        }

        // Store current players vehicle position
        Player[playerid][LastPos][0] = x;
        Player[playerid][LastPos][1] = y;
        Player[playerid][LastPos][2] = z;
    }
    return 1;
}

// --- FUNCTION REDEFINES --- //

// SetPlayerPos is redefined, because it's serverside player position changer
SetPlayerPosEx(playerid, Float:x, Float:y, Float:z){
    // Resets player warning count, to avoid false positives
    Player[playerid][Warnings] = 0;
    return SetPlayerPos(playerid, x, y, z);
}
#define SetPlayerPos SetPlayerPosEx

// SetVehiclePos is redefined, because it's serverside vehicle position changer
SetVehiclePosEx(vehicleid, Float:x, Float:y, Float:z){
    // Finds player who is currently driving this vehicle and resets warnings score
    foreach(new i : Player){
        if(GetPlayerState(i) == PLAYER_STATE_DRIVER && GetPlayerVehicleID(i) == vehicleid){
            Player[i][Warnings] = 0;
        }
    }
    return SetVehiclePos(vehicleid, x, y, z);
}
#define SetVehiclePos SetVehiclePosEx

// PutPlayerInVehicle is redefined, because it's serverside player seat switcher
PutPlayerInVehicleEx(playerid, vehicleid, seatid){
    Player[playerid][LastVehicle] = vehicleid;
    return PutPlayerInVehicle(playerid, vehicleid, seatid);
}
#define PutPlayerInVehicle PutPlayerInVehicleEx