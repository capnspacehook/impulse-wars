#ifndef IMPULSE_WARS_SCRIPTED_BOT_H
#define IMPULSE_WARS_SCRIPTED_BOT_H

#include "game.h"
#include "types.h"

agentActions scriptedBotActions(const env *e, droneEntity *drone) {
    agentActions actions = {0};
    const b2Vec2 pos = getCachedPos(drone->bodyID, &drone->pos);

    // TODO: handle multiple agent drones
    droneEntity *agentDrone = (droneEntity *)safe_array_get_at(e->drones, 0);
    const b2Vec2 agentDronePos = getCachedPos(agentDrone->bodyID, &agentDrone->pos);

    const b2Vec2 agentDroneRelNormPos = b2Normalize(b2Sub(agentDronePos, pos));
    actions.move = agentDroneRelNormPos;
    actions.aim = agentDroneRelNormPos;
    actions.shoot = true;

    return actions;
}

#endif
