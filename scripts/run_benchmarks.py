"""
scripts/run_benchmarks.py
Запускает все бенчмарки последовательно, затем строит графики.
Использование:
    python scripts/run_benchmarks.py             # все тесты
    python scripts/run_benchmarks.py --only B1   # только группа B1 / B2 / B3
    python scripts/run_benchmarks.py --dry-run   # только вывод команд без запуска
"""

import subprocess
import sys
import argparse
from pathlib import Path

# ── Пути ────────────────────────────────────────────────────────────────────
_ROOT = Path(__file__).resolve().parent.parent
EXE = _ROOT / "build" / "src" / "Release" / "Simulation.exe"
OUT_DIR = _ROOT / "results" / "raw"
OUT_DIR.mkdir(parents=True, exist_ok=True)

# ── B1: Производительность от числа частиц ──────────────────────────────────
PERF_PARTICLES = []
for backend, counts in [
    ("cpu", [1_000, 2_500, 5_000, 10_000, 25_000, 50_000, 100_000, 250_000, 500_000, 1_000_000]),
    ("cuda", [1_000, 2_500, 5_000, 10_000, 25_000, 50_000, 100_000, 250_000, 500_000, 1_000_000]),
]:
    for n in counts:
        PERF_PARTICLES.append({
            "group": "B1",
            "test": "perf_particles",
            "backend": backend,
            "particles": n,
            "iterations": 2,
            "scene": "benchmark_box",
            "frames": 500,
        })

# ── B1.5: Стабильность FPS (Dam Break, покадровый CSV) ──────────────────────
PERF_STABILITY = []
for backend, n in [
    ("cpu", 5_000),
    ("cpu", 10_000),
    ("cpu", 20_000),
    ("cuda", 20_000),
    ("cuda", 40_000),
    ("cuda", 80_000),
]:
    PERF_STABILITY.append({
        "group": "B2",
        "test": "perf_stability",
        "backend": backend,
        "particles": n,
        "iterations": 2,
        "scene": "benchmark_dambreak",
        "frames": 30000,
    })

# ── B2: Зависимость от числа итераций ────────────────────────────────────────
PERF_ITERATIONS = []
for backend, counts in [("cpu", [5_000, 20_000]), ("cuda", [5_000, 20_000])]:
    for n in counts:
        for iters in [2, 4, 8, 16]:
            PERF_ITERATIONS.append({
                "group": "B3",
                "test": "perf_iterations",
                "backend": backend,
                "particles": n,
                "iterations": iters,
                "scene": "benchmark_box",
                "frames": 500,
            })

ALL_BENCHMARKS = PERF_PARTICLES + PERF_STABILITY + PERF_ITERATIONS


def output_path(cfg: dict) -> Path:
    parts = [cfg["test"]]
    if "backend" in cfg:
        parts.append(cfg["backend"])
    parts.append(str(cfg.get("particles", 0)))
    if cfg["test"] == "perf_iterations":
        parts.append(f"iter{cfg.get('iterations', 2)}")
    parts.append(cfg.get("scene", ""))
    return OUT_DIR / ("_".join(parts) + ".csv")


def build_cmd(cfg: dict) -> list[str]:
    out = output_path(cfg)
    # perf_stability пишет каждый кадр отдельно — повторы не нужны
    repeats = 1 if cfg["test"] == "perf_stability" else 3
    cmd = [
        str(EXE),
        "--benchmark", cfg["test"],
        "--scene", cfg.get("scene", "benchmark_box"),
        "--output", str(out),
        "--warmup", "50",
        "--frames", str(cfg.get("frames", 500)),
        "--repeats", str(repeats),
    ]
    if "backend" in cfg: cmd += ["--backend", cfg["backend"]]
    if "particles" in cfg: cmd += ["--particles", str(cfg["particles"])]
    if "iterations" in cfg: cmd += ["--iterations", str(cfg["iterations"])]
    return cmd


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", help="Запустить только группу: B1 | B2 | B3")
    ap.add_argument("--dry-run", action="store_true", help="Только вывод команд")
    args = ap.parse_args()

    benchmarks = ALL_BENCHMARKS
    if args.only:
        benchmarks = [b for b in ALL_BENCHMARKS if b["group"] == args.only]
        if not benchmarks:
            print(f"Группа '{args.only}' не найдена. Доступны: B1, B2, B3")
            sys.exit(1)

    errors = []
    for i, cfg in enumerate(benchmarks):
        cmd = build_cmd(cfg)
        label = f"[{i+1}/{len(benchmarks)}] {cfg['group']} | {cfg['test']} | " \
                f"{cfg.get('backend','both')} | {cfg.get('particles',0)} ptcl " \
                f"| iter={cfg.get('iterations',2)} | {cfg.get('scene','')}"
        print(label)
        print("  $", " ".join(cmd))

        if args.dry_run:
            continue

        result = subprocess.run(cmd)
        if result.returncode != 0:
            msg = f"  ERROR: exit code {result.returncode} — {label}"
            print(msg, file=sys.stderr)
            errors.append(msg)

    if args.dry_run:
        print(f"\nDry-run: {len(benchmarks)} команд.")
        return

    if errors:
        print(f"\n{len(errors)} тест(ов) завершились с ошибкой:")
        for e in errors:
            print(" ", e)
    else:
        print("\nВсе бенчмарки завершены успешно.")

    print("Строю графики...")
    subprocess.run([sys.executable, "scripts/plot_benchmarks.py"])


if __name__ == "__main__":
    main()
