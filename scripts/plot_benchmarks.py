"""
scripts/plot_benchmarks.py
Читает CSV из results/raw/, строит 7 графиков в results/plots/.

Ожидаемые схемы CSV
───────────────────
perf_particles / perf_iterations / perf_stability:
  testname, backend, scene, actualparticles, iterations, repeatid,
  avgstepms, medianstepms, p95stepms, stdstepms, physicsfps,
  avgpredictms, avgneighborms, avgsolverms, avgvelocitycorrectms

  (perf_stability: покадровый CSV с теми же колонками, но repeatid = номер кадра)

consistency_cpu_cuda (генерируется самим приложением отдельно):
  scene, actualparticles, frame,
  position_rmse, position_max_error,
  kinetic_energy_cpu, kinetic_energy_cuda, kinetic_energy_relative_error
"""

import sys
import warnings
from pathlib import Path

import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

warnings.filterwarnings("ignore")

_ROOT = Path(__file__).resolve().parent.parent  # scripts/ -> корень проекта
RAW_DIR  = _ROOT / "results" / "raw"
PLOTS_DIR = _ROOT / "results" / "plots"
PLOTS_DIR.mkdir(parents=True, exist_ok=True)

# ── Цветовая схема ────────────────────────────────────────────────────────────
COLOR_CPU = "#4e88d4"
COLOR_CUDA = "#e05a3a"
STAGE_COLORS = {
    "Predict":       "#5b9bd5",
    "Neighbor":      "#ed7d31",
    "Solver":        "#70ad47",
    "VelocityCorr":  "#ffc000",
}
ALPHA_FILL = 0.15
FIG_DPI = 150

# ── Утилиты ───────────────────────────────────────────────────────────────────

def load_glob(pattern: str) -> pd.DataFrame:
    """Загружает все CSV по glob-паттерну и объединяет в один DataFrame."""
    files = list(RAW_DIR.glob(pattern))
    if not files:
        return pd.DataFrame()
    frames = []
    for f in files:
        try:
            df = pd.read_csv(f, skipinitialspace=True)
            df.columns = df.columns.str.strip().str.lower().str.replace(" ", "").str.replace("_", "")
            frames.append(df)
        except Exception as e:
            print(f"  Не удалось прочитать {f}: {e}", file=sys.stderr)
    return pd.concat(frames, ignore_index=True) if frames else pd.DataFrame()


def save(fig: plt.Figure, name: str):
    path = PLOTS_DIR / name
    fig.savefig(path, dpi=FIG_DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"  Сохранён: {path}")


def setup_ax(ax: plt.Axes, title: str, xlabel: str, ylabel: str):
    ax.set_title(title, fontsize=12, fontweight="bold", pad=10)
    ax.set_xlabel(xlabel, fontsize=10)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.grid(axis="y", linestyle="--", alpha=0.5)
    ax.spines[["top", "right"]].set_visible(False)


def aggregate_repeats(df: pd.DataFrame) -> pd.DataFrame:
    """Среднее по повторам (repeat_id) для каждой комбинации backend/particles/iterations."""
    key_cols = [c for c in ["backend", "scene", "actualparticles", "iterations"]
                if c in df.columns]
    num_cols = [c for c in df.columns if c not in key_cols + ["testname", "repeatid"]]
    return df.groupby(key_cols, as_index=False)[num_cols].mean()


def required_columns(df: pd.DataFrame, cols: list, chart_name: str) -> bool:
    missing = [c for c in cols if c not in df.columns]
    if missing:
        print(f"  [SKIP] {chart_name}: отсутствуют колонки {missing}", file=sys.stderr)
        return False
    return True

# ── График 1: Время шага от числа частиц, CPU vs CUDA ─────────────────────────

def plot_particles_vs_step_ms():
    df = load_glob("perf_particles_*.csv")
    if df.empty:
        print("  [SKIP] G1: нет данных perf_particles_*.csv"); return
    need = ["backend", "actualparticles", "avgstepms"]
    if not required_columns(df, need, "G1"): return

    df = aggregate_repeats(df)
    fig, ax = plt.subplots(figsize=(8, 5))
    setup_ax(ax, "Время шага симуляции vs. Число частиц",
             "Число частиц", "Среднее время шага, мс")

    for backend, color, label in [("cpu", COLOR_CPU, "CPU"), ("cuda", COLOR_CUDA, "CUDA")]:
        sub = df[df["backend"] == backend].sort_values("actualparticles")
        if sub.empty: continue
        x, y = sub["actualparticles"], sub["avgstepms"]
        ax.plot(x, y, "o-", color=color, label=label, linewidth=2, markersize=6)
        if "stdstepms" in sub.columns:
            ax.fill_between(x, y - sub["stdstepms"], y + sub["stdstepms"],
                            alpha=ALPHA_FILL, color=color)

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.xaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{int(v):,}"))
    ax.legend()
    save(fig, "particles_vs_step_ms.png")


# ── График 2: Ускорение CUDA / CPU от числа частиц ────────────────────────────

def plot_cuda_speedup():
    df = load_glob("perf_particles_*.csv")
    if df.empty:
        print("  [SKIP] G2: нет данных perf_particles_*.csv"); return
    need = ["backend", "actualparticles", "avgstepms"]
    if not required_columns(df, need, "G2"): return

    df = aggregate_repeats(df)
    cpu  = df[df["backend"] == "cpu"][["actualparticles", "avgstepms"]].rename(
               columns={"avgstepms": "cpu_ms"})
    cuda = df[df["backend"] == "cuda"][["actualparticles", "avgstepms"]].rename(
               columns={"avgstepms": "cuda_ms"})
    merged = pd.merge(cpu, cuda, on="actualparticles").sort_values("actualparticles")
    if merged.empty:
        print("  [SKIP] G2: нет пересечения CPU/CUDA"); return

    merged["speedup"] = merged["cpu_ms"] / merged["cuda_ms"]

    fig, ax = plt.subplots(figsize=(8, 5))
    setup_ax(ax, "Ускорение CUDA относительно CPU",
             "Число частиц", "Ускорение (×)")
    ax.axhline(1.0, color="grey", linestyle="--", linewidth=1, label="1× (паритет)")
    ax.plot(merged["actualparticles"], merged["speedup"],
            "o-", color=COLOR_CUDA, linewidth=2, markersize=6, label="CUDA / CPU")
    for _, row in merged.iterrows():
        ax.annotate(f"{row['speedup']:.1f}×",
                    xy=(row["actualparticles"], row["speedup"]),
                    xytext=(0, 8), textcoords="offset points",
                    ha="center", fontsize=8, color=COLOR_CUDA)

    ax.set_xscale("log")
    ax.xaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{int(v):,}"))
    ax.legend()
    save(fig, "cuda_speedup_vs_particles.png")


# ── График 3: Stacked Bar — доли этапов при росте числа частиц ────────────────

def plot_stages_stacked():
    df = load_glob("perf_particles_*.csv")
    if df.empty:
        print("  [SKIP] G3: нет данных perf_particles_*.csv"); return
    stage_cols = ["avgpredictms", "avgneighborms", "avgsolverms", "avgvelocitycorrectms"]
    if not required_columns(df, stage_cols + ["backend", "actualparticles"], "G3"): return

    df = aggregate_repeats(df)
    backends = [b for b in ["cpu", "cuda"] if b in df["backend"].values]
    fig, axes = plt.subplots(1, len(backends), figsize=(6 * len(backends), 5),
                             sharey=True)   # sharey=True — шкала одна для всех
    if len(backends) == 1:
        axes = [axes]

    stage_labels = ["Predict", "Neighbor", "Solver", "VelocityCorr"]
    colors = [STAGE_COLORS[s] for s in stage_labels]

    for ax, backend in zip(axes, backends):
        sub = df[df["backend"] == backend].sort_values("actualparticles")
        if sub.empty: continue

        x      = np.arange(len(sub))
        xlabs  = [f"{int(v):,}" for v in sub["actualparticles"]]
        totals = sub[stage_cols].sum(axis=1).to_numpy()  # сумма по строке
        bottom = np.zeros(len(sub))

        for col, label, color in zip(stage_cols, stage_labels, colors):
            vals = sub[col].to_numpy() / totals * 100     # нормировка → %
            ax.bar(x, vals, bottom=bottom, label=label, color=color, width=0.6)

            # Подпись доли внутри сегмента (только если сегмент достаточно широк)
            for xi, (v, b) in enumerate(zip(vals, bottom)):
                if v >= 5:   # не рисуем подпись если сегмент < 5%
                    ax.text(xi, b + v / 2, f"{v:.1f}%",
                            ha="center", va="center",
                            fontsize=6.5, color="white", fontweight="bold")
            bottom += vals

        ax.set_ylim(0, 100)
        ax.yaxis.set_major_formatter(
            plt.FuncFormatter(lambda val, _: f"{val:.0f}%")
        )
        ax.set_xticks(x)
        ax.set_xticklabels(xlabs, rotation=30, ha="right", fontsize=8)
        ax.set_title(f"Этапы шага — {backend.upper()}", fontsize=11, fontweight="bold")
        ax.set_xlabel("Число частиц")
        ax.set_ylabel("Доля от суммарного времени, %")
        ax.spines[["top", "right"]].set_visible(False)
        ax.grid(axis="y", linestyle="--", alpha=0.4)
        ax.legend(fontsize=8)

    fig.tight_layout()
    save(fig, "particles_vs_stages_stacked.png")


# ── График 4: Стабильность — время шага по кадрам (Dam Break) ────────────────

def plot_stability():
    df = load_glob("perf_stability_*.csv")
    if df.empty:
        print("  [SKIP] G4: нет данных perf_stability_*.csv"); return
    if not required_columns(df, ["backend", "actualparticles", "avgstepms", "repeatid"], "G4"):
        return

    frame_col = "repeatid"   # номер кадра
    ms_col = "avgstepms"  # время кадра, мс
    STRIDE = 10           # каждый 10-й кадр
    SMOOTH = 50           # окно скользящего среднего (в выборках после stride)

    cpu_shades  = ["#4e88d4", "#2c5fa8", "#1a3d7a"]
    cuda_shades = ["#e05a3a", "#b83010", "#7a1e08"]

    fig, axes = plt.subplots(2, 1, figsize=(13, 10), sharex=False)
    ax_cpu, ax_cuda = axes

    for ax, backend, shades, label_prefix in [
        (ax_cpu,  "cpu",  cpu_shades,  "CPU"),
        (ax_cuda, "cuda", cuda_shades, "CUDA"),
    ]:
        sub_backend = df[df["backend"] == backend]
        if sub_backend.empty:
            ax.text(0.5, 0.5, f"Нет данных {backend.upper()}",
                    ha="center", va="center", transform=ax.transAxes,
                    fontsize=11, color="grey")
            ax.set_title(f"Стабильность времени шага — {label_prefix}",
                         fontsize=12, fontweight="bold", pad=10)
            ax.spines[["top", "right"]].set_visible(False)
            continue

        particle_counts = sorted(sub_backend["actualparticles"].unique())

        for idx, n in enumerate(particle_counts):
            chunk = (sub_backend[sub_backend["actualparticles"] == n]
                     .sort_values(frame_col)
                     .reset_index(drop=True))
            if chunk.empty:
                continue

            # Каждый 10-й кадр
            sampled = chunk.iloc[::STRIDE].copy()
            frames_x = sampled[frame_col].to_numpy()
            step_y  = sampled[ms_col].to_numpy()

            Q1, Q3  = np.percentile(step_y, 25), np.percentile(step_y, 75)
            IQR = Q3 - Q1
            mask = (step_y >= Q1 - 3.0 * IQR) & (step_y <= Q3 + 3.0 * IQR)
            frames_x = frames_x[mask]
            step_y = step_y[mask]

            color = shades[idx % len(shades)]
            lbl = f"{int(n):,} частиц"

            # Сырые точки + тонкая линия
            ax.scatter(frames_x, step_y, color=color, s=5, alpha=0.40, zorder=2)
            ax.plot(frames_x, step_y,    color=color, linewidth=0.7, alpha=0.30, zorder=3)

            # Скользящее среднее — тренд
            smoothed = (pd.Series(step_y)
                        .rolling(SMOOTH, min_periods=1, center=True)
                        .mean()
                        .to_numpy())
            ax.plot(frames_x, smoothed,
                    color=color, linewidth=2.2, alpha=0.95,
                    label=lbl, zorder=4)

        ax.set_title(
            f"Стабильность времени шага — {label_prefix}"
            f"  (точки: каждый {STRIDE}-й кадр · линия: скользящее среднее, окно={SMOOTH})",
            fontsize=11, fontweight="bold", pad=10)
        ax.set_xlabel("Номер кадра", fontsize=10)
        ax.set_ylabel("Время шага, мс", fontsize=10)
        ax.grid(axis="y", linestyle="--", alpha=0.45)
        ax.grid(axis="x", linestyle=":", alpha=0.20)
        ax.spines[["top", "right"]].set_visible(False)
        ax.xaxis.set_major_formatter(
            mticker.FuncFormatter(lambda v, _: f"{int(v):,}"))
        ax.legend(fontsize=9, title=label_prefix, title_fontsize=9,
                  loc="upper right", framealpha=0.85)

    fig.suptitle(
        "B2 — Динамика времени шага симуляции (Dam Break, 30 000 кадров)\n"
        "Точки = каждый 10-й кадр  ·  Линия = скользящее среднее (окно 50 выборок)",
        fontsize=11, y=1.01)
    fig.tight_layout()
    save(fig, "stability_step_ms_over_time.png")


# ── График 5: Время шага от числа PBF-итераций ────────────────────────────────

def plot_iterations_vs_step_ms():
    df = load_glob("perf_iterations_*.csv")
    if df.empty:
        print("  [SKIP] G5: нет данных perf_iterations_*.csv"); return
    need = ["backend", "actualparticles", "iterations", "avgstepms"]
    if not required_columns(df, need, "G5"): return

    df = aggregate_repeats(df)
    fig, ax = plt.subplots(figsize=(8, 5))
    setup_ax(ax, "Время шага vs. Число PBF-итераций",
             "Число итераций", "Среднее время шага, мс")

    for backend, color in [("cpu", COLOR_CPU), ("cuda", COLOR_CUDA)]:
        sub_b = df[df["backend"] == backend]
        for n in sorted(sub_b["actualparticles"].unique()):
            sub = sub_b[sub_b["actualparticles"] == n].sort_values("iterations")
            if sub.empty: continue
            style = "-o" if n == sorted(sub_b["actualparticles"].unique())[0] else "-s"
            ax.plot(sub["iterations"], sub["avgstepms"],
                    style, color=color, linewidth=2, markersize=6,
                    label=f"{backend.upper()} {int(n):,} ptcl")

    ax.set_xticks([2, 4, 8, 16])
    ax.legend(fontsize=8, ncol=2)
    save(fig, "iterations_vs_step_ms.png")

# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    print(f"Читаю CSV из {RAW_DIR}/")
    print(f"Сохраняю графики в {PLOTS_DIR}/\n")

    charts = [
        ("G1 — Время шага vs. Число частиц", plot_particles_vs_step_ms),
        ("G2 — Ускорение CUDA / CPU", plot_cuda_speedup),
        ("G3 — Stacked Bar этапы", plot_stages_stacked),
        ("G4 — Стабильность (Dam Break)", plot_stability),
        ("G5 — Время шага vs. Итерации", plot_iterations_vs_step_ms),
    ]

    for name, fn in charts:
        print(f"{name}")
        try:
            fn()
        except Exception as e:
            print(f"  ОШИБКА: {e}", file=sys.stderr)

    print("\nГотово.")


if __name__ == "__main__":
    main()
