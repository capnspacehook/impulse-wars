from typing import Tuple

from gymnasium import spaces
import numpy as np
import torch as th
from torch import nn
from torch.distributions.normal import Normal

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

        _, *self.dtype = _nativize_dtype(
            np.dtype((np.uint8, (self.obsInfo.scalarObsBytes,))),
            np.dtype((np.float32, (self.obsInfo.scalarObsSize,))),
        )
        self.dtype = tuple(self.dtype)

        self.weaponTypeEmbedding = nn.Embedding(self.obsInfo.weaponTypes, weaponTypeEmbeddingDims)

        self.register_buffer(
            "unpackMask",
            th.tensor([0xC0, 0x3C, 0x03], dtype=th.uint8),
            persistent=False,
        )
        self.register_buffer("unpackShift", th.tensor([6, 2, 0], dtype=th.uint8), persistent=False)

        self.mapObsInputChannels = self.obsInfo.wallTypes + weaponTypeEmbeddingDims + numDrones + 1
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

    def encode_observations(self, obs: th.Tensor) -> th.Tensor:
        batchSize = obs.shape[0]

        # prepare map obs to be unpacked
        mapObs = obs[:, : self.obsInfo.mapObsSize].reshape((batchSize, -1, 1))
        # unpack wall types, weapon pickup types, and drone indexes
        mapObs = (mapObs & self.unpackMask) >> self.unpackShift
        # reshape to 3D map
        mapObs = mapObs.permute(0, 2, 1).reshape(
            (batchSize, 3, self.obsInfo.maxMapColumns, self.obsInfo.maxMapRows)
        )

        # one hot encode wall types
        wallTypeObs = mapObs[:, 0, :, :].unsqueeze(1).long()
        wallTypes = th.zeros(
            batchSize,
            self.obsInfo.wallTypes,
            self.obsInfo.maxMapColumns,
            self.obsInfo.maxMapRows,
            dtype=th.float32,
            device=obs.device,
        )
        wallTypes.scatter_(1, wallTypeObs, 1)

        # embed weapon pickup types
        mapPickupTypes = mapObs[:, 1, :, :].int()
        mapPickupTypes = self.weaponTypeEmbedding(mapPickupTypes).float()
        mapPickupTypes = mapPickupTypes.permute(0, 3, 1, 2)

        # one hot drone indexes
        droneIndexObs = mapObs[:, 2, :, :].unsqueeze(1).long()
        assert th.max(droneIndexObs) == 2
        droneIndexes = th.zeros(
            batchSize,
            self.numDrones + 1,
            self.obsInfo.maxMapColumns,
            self.obsInfo.maxMapRows,
            dtype=th.float32,
            device=obs.device,
        )
        droneIndexes.scatter_(1, droneIndexObs, 1)

        mapObs = th.cat((wallTypes.float(), mapPickupTypes, droneIndexes.float()), dim=1)
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
