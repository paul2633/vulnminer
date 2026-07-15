#!/usr/bin/env python3

import argparse
import os

import matplotlib.pyplot as plt
import pandas as pd

import matplotlib.ticker as mticker


plt.rcParams.update({
    "font.size": 12,
})

parser = argparse.ArgumentParser()

parser.add_argument("-if", "--input-file", required=True)
parser.add_argument("-o", "--output")

parser.add_argument("-m", "--mode")
parser.add_argument("-s", "--source")
parser.add_argument("-g", "--granularity")
parser.add_argument("-v", "--variant")

args = parser.parse_args()

df = pd.read_csv(args.input_file, sep=";")

if args.mode:
    df = df[df["mode"] == args.mode]

if args.source:
    df = df[df["source"] == args.source]

if args.granularity:
    df = df[df["granularity"] == args.granularity]

if args.variant:
    df = df[df["variant"] == args.variant]

if df.empty:
    raise RuntimeError("No matching data.")


group_cols = ["mode", "source", "granularity", "variant"]

groups = []

for _, g in df.groupby(group_cols):

    g = g.sort_values("threads").copy()

    ref = g[g["threads"] == 1]

    if ref.empty:
        g["speedup"] = float("nan")
    else:
        ref_time = ref.iloc[0]["time"]
        g["speedup"] = ref_time / g["time"]

    groups.append(g)

df = pd.concat(groups)

# Moyenne des exécutions identiques
df = (
    df.groupby(
        ["mode", "source", "granularity", "variant", "threads"],
        as_index=False,
    )
    .agg(
        time=("time", "mean"),
        memory=("memory", "mean"),
        speedup=("speedup", "mean"),
        runs=("time", "size"),
    )
)

params = ["mode", "source", "granularity", "variant"]

common = []
varying = []

for p in params:
    if len(df[p].drop_duplicates()) == 1:
        common.append(p)
    else:
        varying.append(p)

print("common :", common)
print("varying:", varying)

fig, axes = plt.subplots(1, 3, figsize=(20, 6))

title = []

if "source" in common:
    title.append(f"Repository: {os.path.basename(df['source'].iloc[0])}")

for p in common:
    if p == "source":
        continue
    title.append(f"{p.capitalize()}: {df[p].iloc[0]}")

fig.suptitle(
    "    |    ".join(title),
    fontsize=20,
    fontweight="bold",
)

for key, g in df.groupby(varying):

    if not isinstance(key, tuple):
        key = (key,)

    label = " | ".join(
        str(g[col].iloc[0]) if col != "source"
        else os.path.basename(g[col].iloc[0])
        for col in varying
    )

    g = g.sort_values("threads")

    axes[0].plot(
        g["threads"],
        g["time"],
        marker="o",
        linewidth=3,
        markersize=7,
        label=label,
    )

    axes[1].plot(
        g["threads"],
        g["speedup"],
        marker="o",
        linewidth=3,
        markersize=7,
        label=label,
    )

    axes[2].plot(
        g["threads"],
        g["memory"],
        marker="o",
        linewidth=3,
        markersize=7,
        label=label,
    )

handles, labels = axes[0].get_legend_handles_labels()

fig.legend(
    handles,
    labels,
    loc="upper center",
    bbox_to_anchor=(0.5, 0.97),
    ncol=len(labels),
    fontsize=14,
    frameon=False,
)


axes[0].set_title("Execution time")
axes[0].set_xlabel("Threads")
axes[0].set_ylabel("Time (s)")

axes[1].set_title("Speedup")
axes[1].set_xlabel("Threads")
axes[1].set_ylabel("Speedup")

axes[2].set_title("Peak memory")
axes[2].set_xlabel("Threads")
axes[2].set_ylabel("Memory (GB)")


for ax in axes:
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.set_xticks(sorted(df["threads"].unique()))

plt.tight_layout(rect=[0, 0, 1, 0.88])

time_min = df["time"].min()
time_max = df["time"].max()

speedup_max = df["speedup"].max()

memory_var = (
    100
    * (df["memory"].max() - df["memory"].min())
    / df["memory"].min()
)

axes[0].set_title(f"Execution time\nMin = {time_min:.2f}s, Max = {time_max:.2f}s")

axes[1].set_title(
    f"Speedup\nMax = ×{speedup_max:.1f}"
)

axes[2].set_title(
    f"Peak memory usage\nΔ = {memory_var:.0f}%"
)

axes[0].set_yscale("log")

if args.output:
    plt.savefig(args.output, dpi=300)
else:
    plt.show()
