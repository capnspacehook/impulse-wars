from typing import Dict

import gymnasium
import numpy as np

import pufferlib

from cy_impulse_wars import (
    maxDrones,
    obsConstants,
    continuousActionsSize,
    CyImpulseWars,
)


def transformRawLog(numDrones: int, rawLog: Dict[str, float]):
    log = {"length": rawLog["length"]}

    count = 0
    for i, stats in enumerate(rawLog["stats"]):
        log[f"drone_{i}_reward"] = stats["reward"]
        log[f"drone_{i}_wins"] = stats["wins"]
        log[f"drone_{i}_distance_traveled"] = stats["distanceTraveled"]
        log[f"drone_{i}_abs_distance_traveled"] = stats["absDistanceTraveled"]
        log[f"drone_{i}_shots_fired"] = sum(stats["shotsFired"])
        log[f"drone_{i}_shots_hit"] = sum(stats["shotsHit"])
        log[f"drone_{i}_shots_taken"] = sum(stats["shotsTaken"])
        log[f"drone_{i}_own_shots_taken"] = sum(stats["ownShotsTaken"])
        log[f"drone_{i}_weapons_picked_up"] = sum(stats["weaponsPickedUp"])
        log[f"drone_{i}_shots_distance"] = sum(stats["shotDistances"])
        count += 1
        if count >= numDrones:
            break

    return log


class ImpulseWars(pufferlib.PufferEnv):
    def __init__(
        self,
        num_envs: int,
        num_drones: int = 2,
        num_agents: int = 2,
        discretize_actions: bool = False,
        seed: int = 0,
        render: bool = False,
        report_interval: int = 16,
        buf=None,
    ):
        if num_drones > maxDrones() or num_drones <= 0:
            raise ValueError(f"num_drones must greater than 0 and less than or equal to {maxDrones()}")
        if num_agents > num_drones or num_agents <= 0:
            raise ValueError("num_agents must greater than 0 and less than or equal to num_drones")

        self.numDrones = num_drones
        self.num_agents = num_agents * num_envs
        self.obsInfo = obsConstants(num_drones)
        self.tick = 0

        self.single_observation_space = gymnasium.spaces.Box(
            low=0.0, high=255, shape=(self.obsInfo.obsSize,), dtype=np.uint8
        )

        if discretize_actions:
            self.single_action_space = gymnasium.spaces.MultiDiscrete(
                [
                    9,  # move, cardinal directions + diagonal directions + noop
                    9,  # aim, cardinal directions + diagonal directions + noop
                    2,  # shoot
                ]
            )
        else:
            # action space is actually bounded by (-1, 1) but pufferlib
            # will check that actions are within the bounds of the action
            # space before actions get to the env, and we ensure the actions
            # are bounded there; so set bounds to (-inf, inf) here so
            # action bounds checks pass
            self.single_action_space = gymnasium.spaces.Box(
                low=float("-inf"), high=float("inf"), shape=(continuousActionsSize(),), dtype=np.float32
            )

        self.report_interval = report_interval
        self.render_mode = "human" if render else None

        super().__init__(buf)

        # pass both the discrete and continuous actions to the env, the
        # continuous actions will always be used for human players
        discreteActions = self.actions
        continuousActions = self.actions
        if discretize_actions:
            continuousActions = np.zeros((self.num_agents, *self.single_action_space.shape), dtype=np.float32)
        else:
            discreteActions = np.zeros((self.num_agents, *self.single_action_space.shape), dtype=np.int32)

        self.c_envs = CyImpulseWars(
            num_envs,
            num_drones,
            num_agents,
            self.observations,
            discretize_actions,
            continuousActions,
            discreteActions,
            self.rewards,
            self.terminals,
            seed,
            render,
        )

    def reset(self, seed=None):
        self.c_envs.reset()
        self.tick = 0
        return self.observations, []

    def step(self, actions):
        self.actions[:] = actions
        self.c_envs.step()

        infos = []
        self.tick += 1
        if self.tick % self.report_interval == 0:
            rawLog = self.c_envs.log()
            if rawLog["length"] > 0:
                infos.append(transformRawLog(self.numDrones, rawLog))

        return self.observations, self.rewards, self.terminals, self.truncations, infos

    def render(self):
        pass

    def close(self):
        self.c_envs.close()


def testPerf(timeout, actionCache, numEnvs):
    env = ImpulseWars(numEnvs)

    import time

    np.random.seed(int(time.time()))
    actions = np.random.uniform(
        env.single_action_space.low[0],
        env.single_action_space.high[0],
        (actionCache, env.num_agents, continuousActionsSize()),
    )

    tick = 0
    start = time.time()
    while time.time() - start < timeout:
        action = actions[tick % actionCache]
        env.step(action)
        tick += 1

    sps = numEnvs * tick / (time.time() - start)
    print(f"SPS: {sps:,}")

    env.close()


if __name__ == "__main__":
    testPerf(timeout=5, actionCache=1024, numEnvs=1)
