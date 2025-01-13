from typing import Tuple

from gymnasium import spaces
import numpy as np
import torch as th
from torch import nn
from torch.distributions.normal import Normal
from torch.nn.functional import one_hot

import pufferlib
from pufferlib.models import LSTMWrapper
from pufferlib.pytorch import layer_init, _nativize_dtype, nativize_tensor

from cy_impulse_wars import obsConstants


cnnChannels = 64
weaponTypeEmbeddingDims = 2
floatingWallOutputSize = 64
encoderOutputSize = 256
lstmOutputSize = 256


class Recurrent(LSTMWrapper):
    def __init__(self, env: pufferlib.PufferEnv, policy: nn.Module):
        super().__init__(env, policy, input_size=encoderOutputSize, hidden_size=lstmOutputSize)


class Policy(nn.Module):
    def __init__(
        self,
        env: pufferlib.PufferEnv,
        numDrones: int,
        discretizeActions: bool = False,
        isTraining: bool = True,
    ):
        super().__init__()

        self.is_continuous = not discretizeActions

        self.numDrones = numDrones
        self.isTraining = isTraining
        self.obsInfo = obsConstants(numDrones)

        # most of the observation is a 2D array of bytes, but the end
        # contains around 200 floats; this allows us to treat the end
        # of the observation as a float array
        _, *self.dtype = _nativize_dtype(
            np.dtype((np.uint8, (self.obsInfo.scalarObsBytes,))),
            np.dtype((np.float32, (self.obsInfo.scalarObsSize,))),
        )
        self.dtype = tuple(self.dtype)

        self.weaponTypeEmbedding = nn.Embedding(self.obsInfo.weaponTypes, weaponTypeEmbeddingDims)

        # each byte in the map observation contains 3 values:
        # - 2 bits for wall type
        # - 1 bit for is floating wall
        # - 1 bit for is weapon pickup
        # - 3 bits for drone index
        self.register_buffer(
            "unpackMask",
            th.tensor([0x60, 0x10, 0x08, 0x07], dtype=th.uint8),
            persistent=False,
        )
        self.register_buffer("unpackShift", th.tensor([5, 4, 3, 0], dtype=th.uint8), persistent=False)

        self.mapObsInputChannels = (self.obsInfo.wallTypes + 1) + 1 + 1 + (numDrones + 1)
        self.mapCNN = nn.Sequential(
            layer_init(
                nn.Conv2d(
                    self.mapObsInputChannels,
                    cnnChannels,
                    kernel_size=5,
                    stride=3,
                )
            ),
            nn.ReLU(),
            layer_init(nn.Conv2d(cnnChannels, cnnChannels, kernel_size=3, stride=1)),
            nn.ReLU(),
            nn.Flatten(),
        )
        cnnOutputSize = self._computeCNNShape()

        self.floatingWallEncoder = nn.Sequential(
            layer_init(
                nn.Linear(
                    self.obsInfo.wallTypes + 1 + self.obsInfo.floatingWallInfoObsSize, floatingWallOutputSize
                )
            ),
            nn.ReLU(),
        )

        featuresSize = (
            cnnOutputSize
            + (self.obsInfo.numNearWallObs * (self.obsInfo.wallTypes + self.obsInfo.nearWallPosObsSize))
            + floatingWallOutputSize
            + (
                self.obsInfo.numWeaponPickupObs
                * (weaponTypeEmbeddingDims + self.obsInfo.weaponPickupPosObsSize)
            )
            + (
                self.obsInfo.numProjectileObs
                * (weaponTypeEmbeddingDims + self.obsInfo.projectileInfoObsSize - 1 + self.numDrones + 1)
            )
            + ((self.numDrones - 1) * (weaponTypeEmbeddingDims + self.obsInfo.enemyDroneObsSize - 1))
            + (self.obsInfo.droneObsSize - 1 + weaponTypeEmbeddingDims)
            + self.obsInfo.miscObsSize
        )

        self.encoder = nn.Sequential(
            layer_init(nn.Linear(featuresSize, encoderOutputSize)),
            nn.ReLU(),
        )

        if self.is_continuous:
            self.actorMean = layer_init(nn.Linear(lstmOutputSize, env.single_action_space.shape[0]), std=0.01)
            self.actorLogStd = nn.Parameter(th.zeros(1, env.single_action_space.shape[0]))
        else:
            self.actionDim = env.single_action_space.nvec.tolist()
            self.actor = layer_init(nn.Linear(lstmOutputSize, sum(self.actionDim)), std=0.01)

        self.critic = layer_init(nn.Linear(lstmOutputSize, 1), std=1.0)

    def forward(self, obs: th.Tensor) -> Tuple[th.Tensor, th.Tensor]:
        hidden = self.encode_observations(obs)
        actions, value = self.decode_actions(hidden)
        return actions, value

    @th.compiler.disable
    def unpack(self, batchSize: int, obs: th.Tensor) -> th.Tensor:
        # prepare map obs to be unpacked
        mapObs = obs[:, : self.obsInfo.mapObsSize].reshape((batchSize, -1, 1))
        # unpack wall types, weapon pickup types, and drone indexes
        mapObs = (mapObs & self.unpackMask) >> self.unpackShift
        # reshape to 3D map
        return mapObs.permute(0, 2, 1).reshape(
            (batchSize, 4, self.obsInfo.mapObsRows, self.obsInfo.mapObsColumns)
        )

    def encode_observations(self, obs: th.Tensor) -> th.Tensor:
        batchSize = obs.shape[0]

        mapObs = self.unpack(batchSize, obs)

        # one hot encode wall types
        wallTypeObs = mapObs[:, 0, :, :].long()
        wallTypes = one_hot(wallTypeObs, self.obsInfo.wallTypes + 1).permute(0, 3, 1, 2).float()

        # unsqueeze floating wall booleans (is wall a floating wall)
        floatingWallObs = mapObs[:, 1, :, :].unsqueeze(1)

        # unsqueeze map pickup booleans (does map tile contain a weapon pickup)
        mapPickupObs = mapObs[:, 2, :, :].unsqueeze(1)

        # one hot drone indexes
        droneIndexObs = mapObs[:, 3, :, :].long()
        droneIndexes = one_hot(droneIndexObs, self.numDrones + 1).permute(0, 3, 1, 2).float()

        # combine all map observations and feed through CNN
        mapObs = th.cat((wallTypes, floatingWallObs, mapPickupObs, droneIndexes), dim=1)
        map = self.mapCNN(mapObs)

        # process scalar observations
        scalarObs = nativize_tensor(obs[:, self.obsInfo.scalarObsOffset :], self.dtype)

        # process N nearest wall types and positions
        nearWallTypeObs = scalarObs[
            :, self.obsInfo.nearWallTypesObsOffset : self.obsInfo.nearWallPosObsOffset
        ].long()
        nearWallTypes = one_hot(nearWallTypeObs, self.obsInfo.wallTypes).float()
        nearWallPosObs = scalarObs[
            :, self.obsInfo.nearWallPosObsOffset : self.obsInfo.floatingWallTypesObsOffset
        ]
        nearWallPosObs = nearWallPosObs.view(
            batchSize, self.obsInfo.numNearWallObs, self.obsInfo.nearWallPosObsSize
        )
        nearWalls = th.cat((nearWallTypes, nearWallPosObs), dim=-1)
        nearWalls = th.flatten(nearWalls, start_dim=1, end_dim=-1)

        # process floating wall types and positions
        floatingWallTypeObs = scalarObs[
            :, self.obsInfo.floatingWallTypesObsOffset : self.obsInfo.floatingWallInfoObsOffset
        ].long()
        floatingWallTypes = one_hot(floatingWallTypeObs, self.obsInfo.wallTypes + 1).float()
        floatingWallInfoObs = scalarObs[
            :, self.obsInfo.floatingWallInfoObsOffset : self.obsInfo.weaponPickupTypesObsOffset
        ]
        floatingWallInfoObs = floatingWallInfoObs.view(
            batchSize, self.obsInfo.numFloatingWallObs, self.obsInfo.floatingWallInfoObsSize
        )
        floatingWallObs = th.cat((floatingWallTypes, floatingWallInfoObs), dim=-1)
        floatingWalls = self.floatingWallEncoder(floatingWallObs)
        floatingWalls = th.max(floatingWalls, dim=1).values

        # process weapon pickup types and positions
        pickupTypeObs = scalarObs[
            :, self.obsInfo.weaponPickupTypesObsOffset : self.obsInfo.weaponPickupPosObsOffset
        ].int()
        pickupTypes = self.weaponTypeEmbedding(pickupTypeObs).float()
        pickupPosObs = scalarObs[
            :, self.obsInfo.weaponPickupPosObsOffset : self.obsInfo.projectileTypesObsOffset
        ]
        pickupPosObs = pickupPosObs.view(
            batchSize, self.obsInfo.numWeaponPickupObs, self.obsInfo.weaponPickupPosObsSize
        )
        pickups = th.cat((pickupTypes, pickupPosObs), dim=-1)
        pickups = th.flatten(pickups, start_dim=1, end_dim=-1)

        # process projectile types, drone creator, and positions
        projTypeObs = scalarObs[
            :,
            self.obsInfo.projectileTypesObsOffset : self.obsInfo.projectileTypesObsOffset
            + self.obsInfo.numProjectileObs,
        ].int()
        projTypes = self.weaponTypeEmbedding(projTypeObs).float()
        projInfoObs = scalarObs[
            :,
            self.obsInfo.projectilePosObsOffset : self.obsInfo.enemyDroneObsOffset,
        ]
        projInfoObs = projInfoObs.view(
            batchSize, self.obsInfo.numProjectileObs, self.obsInfo.projectileInfoObsSize
        )
        projDroneIdxObs = projInfoObs[:, :, 0].long()
        projInfoObs = projInfoObs[:, :, 1:]
        projDroneIdxes = one_hot(projDroneIdxObs, self.numDrones + 1).float()
        projectiles = th.cat((projTypes, projDroneIdxes, projInfoObs), dim=-1)
        projectiles = th.flatten(projectiles, start_dim=1, end_dim=-1)

        # process enemy drone observations
        enemyDroneWeaponObs = scalarObs[
            :, self.obsInfo.enemyDroneObsOffset : self.obsInfo.enemyDroneObsOffset + self.numDrones - 1
        ].int()
        enemyDroneWeapon = self.weaponTypeEmbedding(enemyDroneWeaponObs).squeeze(1).float()
        enemyDroneInfoObs = scalarObs[
            :, self.obsInfo.enemyDroneObsOffset + self.numDrones - 1 : self.obsInfo.droneObsOffset
        ]
        enemyDroneInfoObs = enemyDroneInfoObs.view(
            batchSize, self.numDrones - 1, self.obsInfo.enemyDroneObsSize - 1
        )
        # if there are 2 drones there will only be 1 enemy drone
        # observation and one less dimension than if there are 2+
        # enemy drones
        if self.numDrones == 2:
            enemyDroneWeapon = enemyDroneWeapon.unsqueeze(1)
        enemyDroneObs = th.cat((enemyDroneWeapon, enemyDroneInfoObs), dim=-1)
        enemyDroneObs = th.flatten(enemyDroneObs, start_dim=1, end_dim=-1)

        # process agent drone observations
        droneWeaponObs = scalarObs[:, self.obsInfo.droneObsOffset : self.obsInfo.droneObsOffset + 1].int()
        droneWeapon = self.weaponTypeEmbedding(droneWeaponObs).squeeze(1).float()
        droneInfoObs = scalarObs[:, self.obsInfo.droneObsOffset + 1 : self.obsInfo.miscObsOffset]
        droneObs = th.cat((droneWeapon, droneInfoObs), dim=-1)

        allDronesObs = th.cat((enemyDroneObs, droneObs), dim=1)

        # process misc observations
        miscObs = scalarObs[:, self.obsInfo.miscObsOffset :]

        # combine all observations and feed through final linear encoder
        features = th.cat(
            (map, nearWalls, floatingWalls, pickups, projectiles, allDronesObs, miscObs), dim=-1
        )

        return self.encoder(features), None

    def decode_actions(self, hidden: th.Tensor, lookup=None):
        if self.is_continuous:
            actionMean = self.actorMean(hidden)
            if self.isTraining:
                actionLogStd = self.actorLogStd.expand_as(actionMean)
                actionStd = th.exp(actionLogStd)
                action = Normal(actionMean, actionStd)
            else:
                action = actionMean
        else:
            action = self.actor(hidden)
            action = th.split(action, self.actionDim, dim=1)

        value = self.critic(hidden)

        return action, value

    def _computeCNNShape(self) -> int:
        mapSpace = spaces.Box(
            low=0,
            high=1,
            shape=(self.mapObsInputChannels, self.obsInfo.mapObsRows, self.obsInfo.mapObsColumns),
            dtype=np.float32,
        )

        with th.no_grad():
            t = th.as_tensor(mapSpace.sample()[None])
            return self.mapCNN(t).shape[1]
