import argparse
from collections import deque
import os
from typing import Any, Dict, Deque

import numpy as np
import pufferlib
import pufferlib.vector
import pufferlib.cleanrl
import torch as th
import wandb

from rich_argparse import RichHelpFormatter
from rich.console import Console
from rich.traceback import install

install(show_locals=False)

import clean_pufferl

from policy import Policy, Recurrent
from impulse_wars import ImpulseWars


def make_policy(env, config, isTraining: bool):
    """Make the policy for the environment"""
    policy = Policy(env, config.num_drones, config.discretize_actions, isTraining)
    policy = Recurrent(env, policy)
    return pufferlib.cleanrl.RecurrentPolicy(policy)


def train(args) -> Deque[Dict[str, Any]] | None:
    if args.track and args.mode != "sweep":
        args.wandb = init_wandb(args, args.wandb_name, id=args.train.exp_id)
        args.train.__dict__.update(dict(args.wandb.config.train))
    elif not args.track and args.train.exp_id is None:
        args.train.exp_id = wandb.util.generate_id()

    backend = None
    if args.vec.backend == "multiprocessing":
        backend = pufferlib.vector.Multiprocessing
    elif args.vec.backend == "native":
        backend = pufferlib.vector.PufferEnv

    vecenv = pufferlib.vector.make(
        ImpulseWars,
        num_envs=args.vec.num_envs,
        env_args=(args.train.num_internal_envs,),
        env_kwargs=dict(
            num_drones=args.env.num_drones,
            num_agents=args.env.num_agents,
            sitting_duck=args.env.sitting_duck,
            discretize_actions=args.env.discretize_actions,
            is_training=True,
            seed=args.seed,
            render=args.render,
        ),
        num_workers=args.vec.num_workers,
        batch_size=args.vec.env_batch_size,
        zero_copy=args.vec.zero_copy,
        backend=backend,
    )
    if args.render:
        vecenv.reset()

    if args.model_path is None:
        policy = make_policy(vecenv.driver_env, args.env, True).to(args.train.device)
    else:
        policy = th.load(args.model_path, map_location=args.train.device)

    data = clean_pufferl.create(args.train, vecenv, policy, wandb=args.wandb)

    try:
        stats = deque(maxlen=10)

        while data.global_step < args.train.total_timesteps:
            newStats, _ = clean_pufferl.evaluate(data)
            if newStats:
                stats.append(newStats)
            clean_pufferl.train(data)
    except KeyboardInterrupt:
        clean_pufferl.close(data)
        return None
    except Exception as e:
        Console().print_exception()
        clean_pufferl.close(data)
        raise e

    clean_pufferl.close(data)

    return stats


def init_wandb(args, name, id=None, resume=True):
    wandb.init(
        id=id or wandb.util.generate_id(),
        project=args.wandb_project,
        entity=args.wandb_entity,
        group=args.wandb_group,
        config={
            "env": dict(args.env),
            "train": dict(args.train),
            "vec": dict(args.vec),
        },
        name=name,
        save_code=True,
        resume=resume,
    )
    return wandb


def eval_policy(env: pufferlib.PufferEnv, policy, device, data=None, bestEval: float = None, printInfo=False):
    steps = 0
    totalReward = 0.0

    state = None
    ob, _ = env.reset()
    while True:
        with th.no_grad():
            ob = th.as_tensor(ob).to(device)
            if hasattr(policy, "lstm"):
                actions, _, state = policy.policy(ob, state)
            else:
                actions, _ = policy(ob)

            # TODO: only if discrete actions
            actions = tuple(e.argmax(dim=-1) for e in actions)
            actions = th.stack(actions, dim=-1)

            action = actions.cpu().numpy().reshape(env.action_space.shape)

        ob, reward, done, trunc, info = env.step(action)
        totalReward += reward
        if reward.any() != 0:
            print(f"Reward: {reward}")
        steps += 1

        if done.all() or trunc.all():
            break

    print(f"Totals: Steps: {steps}, Reward: {totalReward}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=f":blowfish: PufferLib [bright_cyan]{pufferlib.__version__}[/]"
        " demo options. Shows valid args for your env and policy",
        formatter_class=RichHelpFormatter,
        add_help=False,
    )
    parser.add_argument(
        "--mode",
        type=str,
        default="train",
        choices="train eval playtest autotune sweep".split(),
    )
    parser.add_argument("--sweep-child", action="store_true")
    parser.add_argument("--model-path", type=str, default=None, help="Path to model to evaluate or resume training")
    parser.add_argument("--seed", type=int, default=-1)
    parser.add_argument("--render", action="store_true", help="Enable rendering")
    parser.add_argument("--cell-id", type=int, default=0)
    parser.add_argument("--wandb-entity", type=str, default="capnspacehook", help="WandB entity")
    parser.add_argument("--wandb-project", type=str, default="Impulse Wars", help="WandB project")
    parser.add_argument("--wandb-group", type=str, default="", help="WandB group")
    parser.add_argument("--wandb-name", type=str, default="", help="WandB run name")
    parser.add_argument("--wandb-sweep", type=str, default="", help="Wandb sweep ID")
    parser.add_argument("--track", action="store_true", help="Track on WandB")

    parser.add_argument("--train.data-dir", type=str, default="checkpoints")
    parser.add_argument("--train.exp-id", type=str, default=None)
    parser.add_argument("--train.torch-deterministic", action="store_true")
    parser.add_argument("--train.cpu-offload", action="store_true")
    parser.add_argument("--train.device", type=str, default="cuda" if th.cuda.is_available() else "cpu")
    parser.add_argument("--train.total-timesteps", type=int, default=250_000_000)
    parser.add_argument("--train.checkpoint-interval", type=int, default=100)
    parser.add_argument("--train.eval-interval", type=int, default=1_000_000)
    parser.add_argument("--train.compile", action="store_true")
    parser.add_argument("--train.compile-mode", type=str, default="reduce-overhead")

    parser.add_argument("--train.num-internal-envs", type=int, default=256)
    parser.add_argument("--train.batch-size", type=int, default=131_072)
    parser.add_argument("--train.bptt-horizon", type=int, default=64)
    parser.add_argument("--train.clip-coef", type=float, default=0.2)
    parser.add_argument("--train.clip-vloss", action="store_false")
    parser.add_argument("--train.ent-coef", type=float, default=0.0013354181228161276)
    parser.add_argument("--train.gae-lambda", type=float, default=0.9341919288455168)
    parser.add_argument("--train.gamma", type=float, default=0.9911001539288388)
    parser.add_argument("--train.learning-rate", type=float, default=0.0001864609622641237)
    parser.add_argument("--train.anneal-lr", action="store_true")
    parser.add_argument("--train.max-grad-norm", type=float, default=0.5)
    parser.add_argument("--train.minibatch-size", type=int, default=32_768)
    parser.add_argument("--train.norm-adv", action="store_false")
    parser.add_argument("--train.update-epochs", type=int, default=1)
    parser.add_argument("--train.vf-clip-coef", type=float, default=0.1)
    parser.add_argument("--train.vf-coef", type=float, default=0.5)
    parser.add_argument("--train.target-kl", type=float, default=0.2)

    parser.add_argument("--env.discretize-actions", action="store_false")
    parser.add_argument("--env.num-drones", type=int, default=2, help="Number of drones in the environment")
    parser.add_argument(
        "--env.num-agents",
        type=int,
        default=1,
        help="Number of agents controlling drones, if this is less than --train.num-drones the other drones will be scripted",
    )
    parser.add_argument("--env.human-control", action="store_true", help="Enable human control by default")
    parser.add_argument("--env.sitting-duck", action="store_true", help="Scripted drones will do nothing")

    parser.add_argument("--vec.backend", type=str, default="multiprocessing")
    parser.add_argument("--vec.num-envs", type=int, default=8)
    parser.add_argument("--vec.num-workers", type=int, default=8)
    parser.add_argument("--vec.env-batch-size", type=int, default=4)
    parser.add_argument("--vec.zero-copy", action="store_false")
    parsed = parser.parse_args()

    args = {}
    for k, v in vars(parsed).items():
        if "." in k:
            group, name = k.split(".")
            if group not in args:
                args[group] = {}

            args[group][name] = v
        else:
            args[k] = v

    args["train"] = pufferlib.namespace(**args["train"])
    args["env"] = pufferlib.namespace(**args["env"])
    args["vec"] = pufferlib.namespace(**args["vec"])
    args = pufferlib.namespace(**args)

    args.train.env = "impulse_wars"

    if args.seed == -1:
        args.seed = np.random.randint(2**32 - 1, dtype=np.uint64).item()
    args.train.seed = args.seed
    print(f"Seed: {args.seed}")

    if args.mode == "train":
        try:
            args.wandb = None
            train(args)
            if args.track:
                wandb.finish()
        except KeyboardInterrupt:
            os._exit(0)
        except Exception:
            Console().print_exception()
            os._exit(0)
    elif args.mode == "eval":
        vecenv = pufferlib.vector.make(
            ImpulseWars,
            num_envs=1,
            env_args=(1,),
            env_kwargs=dict(
                num_drones=args.env.num_drones,
                num_agents=args.env.num_agents,
                sitting_duck=args.env.sitting_duck,
                discretize_actions=args.env.discretize_actions,
                is_training=False,
                human_control=args.env.human_control,
                render=True,
                seed=args.seed,
            ),
            num_workers=1,
            batch_size=1,
            backend=pufferlib.PufferEnv,
        )

        if args.model_path is None:
            policy = make_policy(vecenv, args.env, False).to(args.train.device)
        else:
            policy = th.load(args.model_path, map_location=args.train.device)
            policy.policy.policy.isTraining = False

        for _ in range(10):
            eval_policy(vecenv, policy, args.train.device)
    elif args.mode == "sweep":
        from sweep import sweep

        sweep(args, train)
