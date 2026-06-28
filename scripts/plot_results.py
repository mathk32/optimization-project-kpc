import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

sns.set_theme(style="whitegrid")

plt.rcParams.update({
    "font.size": 11,
    "axes.titlesize": 14,
    "figure.titlesize": 16
})

COLOR_DEFAULT = "#4a5a6a"
COLOR_SCORE = "#2e8b57"

PRE_FILE = "src/pre_solver_results.csv"
POST_FILE = "src/post_solver_results.csv"

pre = pd.read_csv(PRE_FILE)
post = pd.read_csv(POST_FILE)

for df in [pre, post]:
    df["Density_Pct"] = (df["Density"] * 100).round().astype(int)
    df["Categoria"] = (
        df["Items"].astype(str)
        + " items (d="
        + df["Density_Pct"].astype(str)
        + "%)"
    )

def check_profit_integrity(df, name):
    divergences = df[np.abs(df["Default_Profit"] - df["Score_Profit"]) > 1e-3]
    if divergences.empty:
        print(f"[{name}] OK: Profits are identical across all instances.")
    else:
        print(f"[{name}] ALERT: {len(divergences)} profit divergence(s) found!")

print("=" * 90)
print("INTEGRITY CHECK")
print("=" * 90)
check_profit_integrity(pre, "Layer 1 - Isolated")
check_profit_integrity(post, "Layer 2 - Commercial")

def compute_layer_stats(df, label):
    agg = df.groupby("Categoria").agg(
        Default_Nodes=("Default_Nodes", "mean"),
        Score_Nodes=("Score_Nodes", "mean"),
        Default_Time=("Default_Time(s)", "mean"),
        Score_Time=("Score_Time(s)", "mean"),
        N=("Instance", "count"),
    ).reset_index()

    agg["Node_Reduction_%"] = (
        (agg["Default_Nodes"] - agg["Score_Nodes"]) / agg["Default_Nodes"]
    ) * 100
    agg["Time_Reduction_%"] = (
        (agg["Default_Time"] - agg["Score_Time"]) / agg["Default_Time"]
    ) * 100
    agg["Layer"] = label
    return agg

pre_stats = compute_layer_stats(pre, "Isolated (no presolve/cuts)")
post_stats = compute_layer_stats(post, "Commercial (Default CPLEX)")

def print_layer_report(stats, title):
    print("\n" + "=" * 90)
    print(title)
    print("=" * 90)
    for _, row in stats.iterrows():
        print(f"\n[{row['Categoria']}] (n={int(row['N'])})")
        print(f"  Nodes: Default={row['Default_Nodes']:>12,.1f} | Score={row['Score_Nodes']:>12,.1f} "
              f"-> reduction of {row['Node_Reduction_%']:>7.2f}%")
        print(f"  Time:  Default={row['Default_Time']:>10.4f}s | Score={row['Score_Time']:>10.4f}s "
              f"-> reduction of {row['Time_Reduction_%']:>7.2f}%")

print_layer_report(pre_stats, "LAYER 1 -- ISOLATED: MostFractional (Default) vs S_i (Score)")
print_layer_report(post_stats, "LAYER 2 -- COMMERCIAL: Default CPLEX vs S_i (Score)")

print("\n" + "=" * 90)
print("GLOBAL SUMMARY")
print("=" * 90)
print(f"\nLayer 1 (Isolated)  -> mean node reduction: {pre_stats['Node_Reduction_%'].mean():.2f}% "
      f"| mean time reduction: {pre_stats['Time_Reduction_%'].mean():.2f}%")
print(f"Layer 2 (Commercial) -> mean node reduction: {post_stats['Node_Reduction_%'].mean():.2f}% "
      f"| mean time reduction: {post_stats['Time_Reduction_%'].mean():.2f}%")

fig, ax = plt.subplots(figsize=(11, 6))
x = np.arange(len(pre_stats))
width = 0.35

ax.bar(x - width/2, pre_stats["Default_Nodes"], width,
       label="Most Fractional", color=COLOR_DEFAULT)
ax.bar(x + width/2, pre_stats["Score_Nodes"], width,
       label="$S_i$ (proposed)", color=COLOR_SCORE)

ax.set_yscale("log")
ax.set_ylabel("Nodes explored (mean, log scale)")
ax.set_title("Layer 1: Branch-and-Bound Tree Size")
ax.set_xticks(x)
ax.set_xticklabels(pre_stats["Categoria"], rotation=25, ha="right")
ax.legend()
plt.tight_layout()
plt.savefig("plot_1_isolated_nodes.png", dpi=300)
plt.close(fig)

fig, ax = plt.subplots(figsize=(11, 6))

ax.bar(x - width/2, pre_stats["Default_Time"], width,
       label="Most Fractional", color=COLOR_DEFAULT)
ax.bar(x + width/2, pre_stats["Score_Time"], width,
       label="$S_i$ (proposed)", color=COLOR_SCORE)

ax.set_yscale("log")
ax.set_ylabel("CPU Time in seconds (mean, log scale)")
ax.set_title("Layer 1: Execution Time")
ax.set_xticks(x)
ax.set_xticklabels(pre_stats["Categoria"], rotation=25, ha="right")
ax.legend()
plt.tight_layout()
plt.savefig("plot_2_isolated_time.png", dpi=300)
plt.close(fig)

fig, ax = plt.subplots(figsize=(11, 6))
x2 = np.arange(len(post_stats))

ax.bar(x2 - width/2, post_stats["Default_Nodes"], width,
       label="Default CPLEX", color=COLOR_DEFAULT)
ax.bar(x2 + width/2, post_stats["Score_Nodes"], width,
       label="$S_i$ (proposed)", color=COLOR_SCORE)

ax.set_ylabel("Nodes explored (mean)")
ax.set_title("Layer 2: Branch-and-Bound Tree Size")
ax.set_xticks(x2)
ax.set_xticklabels(post_stats["Categoria"], rotation=25, ha="right")
ax.legend()
plt.tight_layout()
plt.savefig("plot_3_commercial_nodes.png", dpi=300)
plt.close(fig)

fig, ax = plt.subplots(figsize=(11, 6))

ax.bar(x2 - width/2, post_stats["Default_Time"], width,
       label="Default CPLEX", color=COLOR_DEFAULT)
ax.bar(x2 + width/2, post_stats["Score_Time"], width,
       label="$S_i$ (proposed)", color=COLOR_SCORE)

ax.set_ylabel("CPU Time in seconds (mean)")
ax.set_title("Layer 2: Execution Time")
ax.set_xticks(x2)
ax.set_xticklabels(post_stats["Categoria"], rotation=25, ha="right")
ax.legend()
plt.tight_layout()
plt.savefig("plot_4_commercial_time.png", dpi=300)
plt.close(fig)

print("\n" + "=" * 90)
print("PLOTS GENERATED")
print("=" * 90)
print("plot_1_isolated_nodes.png")
print("plot_2_isolated_time.png")
print("plot_3_commercial_nodes.png")
print("plot_4_commercial_time.png")