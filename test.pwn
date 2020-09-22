#include "ac-l0gic.inc"

main(){}

public OnPlayerCheating(playerid, cheatid){
    switch(cheatid){
        case CHEAT_TYPE_AIRBREAK:{
            SendClientMessage(playerid, -1, "You are probably airbreaking!");
        }
        case CHEAT_TYPE_WARP:{
            SendClientMessage(playerid, -1, "You are probably vehicle warping!");
        }
    }
    return 1;
}