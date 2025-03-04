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
encoderOutputSize = 256
lstmOutputSize = 256


class Recurrent(LSTMWrapper):
    def __init__(self, env: pufferlib.PufferEnv, policy: nn.Module):
        super().__init__(env, policy, input_size=encoderOutputSize, hidden_size=lstmOutputSize)


class Policy(nn.Module):
    def __init__(
        self,
        env: pufferlib.PufferEnv,
        batchSize: int,
        numDrones: int,
        discretizeActions: bool = False,
        isTraining: bool = True,
        device: str = "cuda",
    ):
        super().__init__()

        self.is_continuous = not discretizeActions

        self.numDrones = numDrones
        self.isTraining = isTraining
        self.obsInfo = obsConstants(numDrones)

        self.discreteFactors = np.array(
            [self.obsInfo.wallTypes] * self.obsInfo.numNearWallObs
            + [self.obsInfo.wallTypes + 1] * self.obsInfo.numFloatingWallObs
            + [self.numDrones + 1] * self.obsInfo.numProjectileObs,
        )
        discreteOffsets = th.tensor([0] + list(np.cumsum(self.discreteFactors)[:-1]), device=device).view(
            1, -1
        )
        self.register_buffer("discreteOffsets", discreteOffsets, persistent=False)
        self.discreteMultihotDim = self.discreteFactors.sum()

        multihotBuffer = th.zeros(batchSize, self.discreteMultihotDim, device=device)
        self.register_buffer("multihotOutput", multihotBuffer, persistent=False)

        # most of the observation is a 2D array of bytes, but the end
        # contains around 200 floats; this allows us to treat the end
        # of the observation as a float array
        _, *self.dtype = _nativize_dtype(
            np.dtype((np.uint8, (self.obsInfo.continuousObsBytes,))),
            np.dtype((np.float32, (self.obsInfo.continuousObsSize,))),
        )
        self.dtype = tuple(self.dtype)

        self.weaponTypeEmbedding = nn.Embedding(self.obsInfo.weaponTypes, weaponTypeEmbeddingDims)

        # each byte in the map observation contains 4 values:
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

        self.mapObsInputChannels = (self.obsInfo.wallTypes + 1) + 1 + 1 + numDrones
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

        featuresSize = (
            cnnOutputSize
            + (self.obsInfo.numNearWallObs * (self.obsInfo.wallTypes + self.obsInfo.nearWallPosObsSize))
            + (
                self.obsInfo.numFloatingWallObs
                * (self.obsInfo.wallTypes + 1 + self.obsInfo.floatingWallInfoObsSize)
            )
            + (
                self.obsInfo.numWeaponPickupObs
                * (weaponTypeEmbeddingDims + self.obsInfo.weaponPickupPosObsSize)
            )
            + (
                self.obsInfo.numProjectileObs
                * (weaponTypeEmbeddingDims + self.obsInfo.projectileInfoObsSize + self.numDrones + 1)
            )
            + ((self.numDrones - 1) * (weaponTypeEmbeddingDims + self.obsInfo.enemyDroneObsSize))
            + (self.obsInfo.droneObsSize + weaponTypeEmbeddingDims)
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

    def unpack(self, batchSize: int, obs: th.Tensor) -> th.Tensor:
        # prepare map obs to be unpacked
        mapObs = obs[:, : self.obsInfo.mapObsSize].reshape((batchSize, -1, 1))
        # unpack wall types, weapon pickup types, and drone indexes
        mapObs = (mapObs & self.unpackMask) >> self.unpackShift
        # reshape so channels are first, required for torch conv2d
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
        droneIndexes = one_hot(droneIndexObs, self.numDrones).permute(0, 3, 1, 2).float()

        # combine all map observations and feed through CNN
        mapObs = th.cat((wallTypes, floatingWallObs, mapPickupObs, droneIndexes), dim=1)
        map = self.mapCNN(mapObs)

        # process discrete observations
        multihotInput = (
            obs[:, self.obsInfo.nearWallTypesObsOffset : self.obsInfo.projectileTypesObsOffset]
            + self.discreteOffsets
        )
        multihotOutput = self.multihotOutput[:batchSize].zero_()
        multihotOutput.scatter_(1, multihotInput.long(), 1)

        weaponTypeObs = obs[:, self.obsInfo.projectileTypesObsOffset : self.obsInfo.discreteObsSize].int()
        weaponTypes = self.weaponTypeEmbedding(weaponTypeObs).float()
        weaponTypes = th.flatten(weaponTypes, start_dim=1, end_dim=-1)

        # process continuous observations
        continuousObs = nativize_tensor(obs[:, self.obsInfo.continuousObsOffset :], self.dtype)

        # combine all observations and feed through final linear encoder
        features = th.cat((map, multihotOutput, weaponTypes, continuousObs), dim=-1)

        return self.encoder(features), None

    def decode_actions(self, hidden: th.Tensor, lookup=None):
        if self.is_continuous:
            actionMean = self.actorMean(hidden)
            if self.isTraining:
                actionLogStd = self.actorLogStd.expand_as(actionMean)
                actionStd = th.exp(actionLogStd)
                actionMean += th.empty(
                    actionMean.shape, dtype=actionMean.dtype, device=actionMean.device
                ).uniform_(-0.25, 0.25)
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
