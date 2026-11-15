#!/usr/bin/env python3
"""Live QGenClass experiment monitor. Usage: ./monitor DATASET [METHOD]."""
import csv
import glob
import os
import re
import statistics
import sys
import time
from pathlib import Path

GEN = re.compile(r'GENERATION\s*=\s*(\d+).*?FITNESS\s*=\s*([^\s%]+)%?(?:.*?TEST_ERROR\s*=\s*([^\s%]+)%?)?')
FOLD_SEED = re.compile(r'_fold(\d+)_seed(\d+)_')


def read_rows(path):
    try:
        with open(path, newline='', encoding='utf-8', errors='replace') as f:
            return list(csv.DictReader(f, delimiter='\t'))
    except (OSError, csv.Error):
        return []


def experiment(dataset, method):
    candidates = list(Path('results').glob(f'{glob.escape(dataset)}_*_summary_*.tsv'))
    if method:
        candidates = [p for p in candidates if p.name.startswith(dataset + '_' + method + '_')]
    return max(candidates, key=lambda p: p.stat().st_ctime, default=None)


def stats(rows, field):
    values = []
    for row in rows:
        try:
            values.append(float(row[field]))
        except (KeyError, ValueError, TypeError):
            pass
    if not values:
        return '—'
    return f'{statistics.mean(values):.5f} ± {statistics.stdev(values) if len(values)>1 else 0:.5f}'


def process_running(dataset, method):
    # Only infer activity from matching runmodel command lines, not log age alone.
    try:
        for proc in Path('/proc').iterdir():
            if not proc.name.isdigit() or int(proc.name) == os.getpid():
                continue
            try:
                cmd = (proc / 'cmdline').read_bytes().replace(b'\0', b' ').decode(errors='replace')
            except (OSError, PermissionError):
                continue
            if 'runmodel.sh' in cmd and ('--dataset ' + dataset in cmd or '--dataset=' + dataset in cmd):
                if not method or '--mutation-method ' + method in cmd or '--mutation-method=' + method in cmd or '--pop_mutationmethod ' + method in cmd:
                    return True
    except OSError:
        pass
    return False


def display(dataset, method):
    summary = experiment(dataset, method)
    if not summary:
        return f'QGenClass | {dataset}\nWaiting for results/{dataset}_*_summary_*.tsv ...'
    prefix = summary.name.split('_summary_')[0]
    logs = list(Path('results').glob(glob.escape(prefix) + '_fold*_seed*.log'))
    latest = max(logs, key=lambda p: p.stat().st_mtime, default=None)
    rows = read_rows(summary)
    done = len(rows)
    folds = {row.get('fold') for row in rows if row.get('fold')}
    running = process_running(dataset, method)
    lines = [f'QGenClass experiment | {dataset}',
             f'Experiment: {prefix}',
             f'Status: {"RUNNING" if running else "NO MATCHING SCRIPT PROCESS"} | Completed runs: {done}',
             f'Completed folds (at least one run): {len(folds)}',
             '',
             'Results so far (mean ± sample std, completed runs only):',
             f'  Class error (%) : {stats(rows, "class_error_percent")}',
             f'  Accuracy (%)    : {stats(rows, "accuracy_percent")}',
             f'  Precision       : {stats(rows, "precision")}',
             f'  Recall          : {stats(rows, "recall")}',
             f'  F1 score        : {stats(rows, "f1score")}']
    if latest:
        match = FOLD_SEED.search(latest.name)
        try:
            with open(latest, encoding='utf-8', errors='replace') as f:
                content = f.read()[-150000:]
            generations = list(GEN.finditer(content))
        except OSError:
            generations = []
        lines.extend(['', 'Latest updated run:',
                      f'  Fold / seed: {match.group(1)} / {match.group(2)}' if match else f'  Log: {latest.name}',
                      f'  Generation: {generations[-1].group(1)} | Fitness: {generations[-1].group(2)} | Test error: {generations[-1].group(3) or "—"}' if generations else '  Waiting for generation output',
                      f'  Last log update: {int(time.time()-latest.stat().st_mtime)} seconds ago'])
    lines.extend(['', f'Summary: {summary}', 'Ctrl+C: stop monitor only | Refresh: 2s'])
    return '\n'.join(lines)


def main():
    if len(sys.argv) not in (2, 3) or sys.argv[1] in ('-h', '--help'):
        print('Usage: ./monitor DATASET [standard|active|targeted]')
        sys.exit(0 if len(sys.argv)>1 and sys.argv[1] in ('-h','--help') else 2)
    dataset = sys.argv[1]
    method = sys.argv[2] if len(sys.argv) == 3 else None
    try:
        while True:
            out = display(dataset, method)
            if sys.stdout.isatty():
                print('\033[2J\033[H', end='')
            print(out, flush=True)
            time.sleep(2)
    except KeyboardInterrupt:
        print('\nMonitor stopped.')

if __name__ == '__main__':
    main()
