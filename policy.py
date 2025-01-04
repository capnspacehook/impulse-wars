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


cnnChannels = 32
weaponTypeEmbeddingDims = 3
enemyDroneEncOutputSize = 32
droneEncOutputSize = 64
encoderOutputSize = 128
lstmOutputSize = 128


class Recurrent(LSTMWrapper):
    def __init__(self, env: pufferlib.PufferEnv, policy: nn.Module):
        super().__init__(env, policy, input_size=encoderOutputSize, hidden_size=lstmOutputSize)


class Policy(nn.Module):
    def __init__(self, env: pufferlib.PufferEnv, numDrones: int, discretizeActions: bool = False):
        super().__init__()

        self.is_continuous = not discretizeActions

        self.numDrones = numDrones
        self.obsInfo = obsConstants(numDrones)

        # most of the observation is a 2D array of bytes, but the end
        # contains around 100 floats; this allows us to treat the end
        # of the observation as a float array
        _, *self.dtype = _nativize_dtype(
            np.dtype((np.uint8, (self.obsInfo.scalarObsBytes,))),
            np.dtype((np.float32, (self.obsInfo.scalarObsSize,))),
        )
        self.dtype = tuple(self.dtype)

        self.weaponTypeEmbedding = nn.Embedding(self.obsInfo.weaponTypes, weaponTypeEmbeddingDims)

        # each byte in the map observation contains 3 values:
        # - 2 bits for wall type
        # - 1 bit for weapon pickup
        # - 3 bits for drone index
        self.register_buffer(
            "unpackMask",
            th.tensor([0x30, 0x08, 0x07], dtype=th.uint8),
            persistent=False,
        )
        self.register_buffer("unpackShift", th.tensor([4, 3, 0], dtype=th.uint8), persistent=False)

        self.mapObsInputChannels = self.obsInfo.wallTypes + 2 + numDrones + 1
        self.mapCNN = nn.Sequential(
            layer_init(
                nn.Conv2d(
                    self.mapObsInputChannels,
                    cnnChannels,
                    kernel_size=5,
                    stride=2,
                )
            ),
            nn.LeakyReLU(),
            layer_init(nn.Conv2d(cnnChannels, cnnChannels, kernel_size=3, stride=2)),
            nn.LeakyReLU(),
            nn.Flatten(),
        )
        cnnOutputSize = self._computeCNNShape()

        self.enemyDroneEncoder = nn.Sequential(
            layer_init(nn.Linear(self.obsInfo.enemyDroneObsSize, enemyDroneEncOutputSize)),
            nn.LeakyReLU(),
        )

        self.droneEncoder = nn.Sequential(
            layer_init(
                nn.Linear(self.obsInfo.droneObsSize - 1 + weaponTypeEmbeddingDims, droneEncOutputSize)
            ),
            nn.LeakyReLU(),
        )

        featuresSize = (
            cnnOutputSize
            + (self.obsInfo.numWeaponPickupObs * (weaponTypeEmbeddingDims + 2))
            + (self.obsInfo.numProjectileObs * (weaponTypeEmbeddingDims + 2))
            + enemyDroneEncOutputSize
            + droneEncOutputSize
        )

        self.encoder = nn.Sequential(
            layer_init(nn.Linear(featuresSize, encoderOutputSize)),
            nn.LeakyReLU(),
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
            (batchSize, 3, self.obsInfo.maxMapColumns, self.obsInfo.maxMapRows)
        )

    def encode_observations(self, obs: th.Tensor) -> th.Tensor:
        batchSize = obs.shape[0]

        mapObs = self.unpack(batchSize, obs)

        # one hot encode wall types
        wallTypeObs = mapObs[:, 0, :, :].long()
        wallTypes = droneIndexes = one_hot(wallTypeObs, self.obsInfo.wallTypes).permute(0, 3, 1, 2).float()

        # one hot weapon pickups
        mapPickupObs = mapObs[:, 1, :, :].long()
        mapPickups = one_hot(mapPickupObs, 2).permute(0, 3, 1, 2).float()

        # one hot drone indexes
        droneIndexObs = mapObs[:, 2, :, :].long()
        droneIndexes = one_hot(droneIndexObs, self.numDrones + 1).permute(0, 3, 1, 2).float()

        # combine all map observations and feed through CNN
        mapObs = th.cat((wallTypes, mapPickups, droneIndexes), dim=1)
        map = self.mapCNN(mapObs)

        # process scalar observations
        scalarObs = nativize_tensor(obs[:, self.obsInfo.scalarObsOffset :], self.dtype)

        # process weapon pickup types and positions
        nearPickupTypes = scalarObs[
            :, self.obsInfo.weaponPickupTypesObsOffset : self.obsInfo.weaponPickupPosObsOffset
        ].int()
        nearPickupTypes = self.weaponTypeEmbedding(nearPickupTypes).float()
        nearPickupPos = scalarObs[
            :, self.obsInfo.weaponPickupPosObsOffset : self.obsInfo.projectileTypesObsOffset
        ]
        nearPickupPos = nearPickupPos.view(batchSize, self.obsInfo.numWeaponPickupObs, 2)
        nearPickupObs = th.cat((nearPickupTypes, nearPickupPos), dim=-1)
        nearPickupObs = th.flatten(nearPickupObs, start_dim=1, end_dim=-1)

        # process projectile types and positions
        projTypes = scalarObs[
            :,
            self.obsInfo.projectileTypesObsOffset : self.obsInfo.projectileTypesObsOffset
            + self.obsInfo.numProjectileObs,
        ].int()
        projTypes = self.weaponTypeEmbedding(projTypes).float()
        projPos = scalarObs[
            :,
            self.obsInfo.projectilePosObsOffset : self.obsInfo.projectilePosObsOffset
            + (self.obsInfo.numProjectileObs * 2),
        ]
        projPos = projPos.view(batchSize, self.obsInfo.numProjectileObs, 2)
        projObs = th.cat((projTypes, projPos), dim=-1)
        projObs = th.flatten(projObs, start_dim=1, end_dim=-1)

        # process enemy drone observations
        enemyDroneObs = scalarObs[
            :,
            self.obsInfo.enemyDroneObsOffset : self.obsInfo.enemyDroneObsOffset
            + self.obsInfo.enemyDroneObsSize,
        ]
        enemyDrone = self.enemyDroneEncoder(enemyDroneObs)

        # process agent drone observations
        droneObs = scalarObs[:, self.obsInfo.droneObsOffset : -1]
        droneWeapon = scalarObs[:, -1].int()
        droneWeapon = self.weaponTypeEmbedding(droneWeapon).float()
        droneObs = th.cat((droneObs, droneWeapon), dim=-1)
        drone = self.droneEncoder(droneObs)

        # combine all observations and feed through final linear encoder
        features = th.cat((map, nearPickupObs, projObs, enemyDrone, drone), dim=-1)

        return self.encoder(features), None

    def decode_actions(self, hidden: th.Tensor, lookup=None):
        if self.is_continuous:
            actionMean = self.actorMean(hidden)
            actionLogStd = self.actorLogStd.expand_as(actionMean)
            actionStd = th.exp(actionLogStd)
            action = Normal(actionMean, actionStd)
        else:
            action = self.actor(hidden)
            action = th.split(action, self.actionDim, dim=1)

        value = self.critic(hidden)

        return action, value

    def _computeCNNShape(self) -> int:
        mapSpace = spaces.Box(
            low=0,
            high=1,
            shape=(self.mapObsInputChannels, self.obsInfo.maxMapColumns, self.obsInfo.maxMapRows),
            dtype=np.float32,
        )

        with th.no_grad():
            t = th.as_tensor(mapSpace.sample()[None])
            return self.mapCNN(t).shape[1]
