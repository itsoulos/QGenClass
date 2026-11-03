#!/usr/bin/env python3
"""Reproduce figures from trajectory.tsv (Python 3 + matplotlib only)."""
import csv
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

root = Path(__file__).resolve().parent
methods = [
    ('standard', 'Standard'),
    ('standardElite', 'Standard+Elite'),
    ('standardBlock', 'Standard+Block'),
    ('targeted', 'Targeted'),
    ('targetedFull', 'Targeted+Full'),
    ('targetedWorst', 'TargetedWorst'),
]
rows = {}
with (root / 'trajectory.tsv').open(newline='') as f:
    for row in csv.DictReader(f, delimiter='\t'):
        rows.setdefault(row['method'], []).append(row)

for kind, ylabel, name in [
    ('count', 'Mean accepted improvements per 10,000-evaluation bin', 'accepted_improvements_by_budget'),
    ('rate', 'Accepted improvements per 1,000 crossover evaluations', 'acceptance_rate_by_budget'),
]:
    fig, ax = plt.subplots(figsize=(9.2, 5.3), layout='constrained')
    for method, label in methods:
        dat = sorted(rows[method], key=lambda r: int(r['fitness_evals_end']))
        x = [int(r['fitness_evals_end']) / 1000 for r in dat]
        if kind == 'count':
            y = [float(r['mean_accepted_in_bin']) for r in dat]
        else:
            y = [1000 * float(r['mean_accepted_in_bin']) / float(r['mean_crossover_evals_in_bin'])
                 if float(r['mean_crossover_evals_in_bin']) else float('nan') for r in dat]
        ax.plot(x, y, marker='o', markersize=3.5, linewidth=1.8, label=label)
    ax.set_xlabel('Fitness evaluations at bin endpoint (thousands)')
    ax.set_ylabel(ylabel)
    ax.set_xticks(range(10, 101, 10))
    ax.set_xlim(8, 102)
    ax.grid(True, alpha=0.25)
    ax.legend(loc='best', fontsize=8, ncol=2)
    for ext in ('pdf', 'png'):
        fig.savefig(root / f'{name}.{ext}', dpi=220)
    plt.close(fig)
print('Generated four figure files in', root)
