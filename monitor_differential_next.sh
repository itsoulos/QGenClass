#!/usr/bin/env python3
"""Live QGenClass experiment monitor. Usage: ./monitor_differential_next.sh DATASET."""
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


def experiment(dataset, method, directory):
    candidates = list(Path(directory).glob(f'{glob.escape(dataset)}_*_summary_*.tsv'))
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


def process_running(dataset, method, directory):
    # Only infer activity from matching runmodel command lines, not log age alone.
    try:
        for proc in Path('/proc').iterdir():
            if not proc.name.isdigit() or int(proc.name) == os.getpid():
                continue
            try:
                cmd = (proc / 'cmdline').read_bytes().replace(b'\0', b' ').decode(errors='replace')
            except (OSError, PermissionError):
                continue
            if 'runmodel.sh' in cmd and re.search(r'--results-dir(?:=|\s+)(?:\./)?'+re.escape(directory)+r'/?(?:\s|$)', cmd) and re.search(r'--dataset(?:=|\s+)'+re.escape(dataset)+r'(?:\s|$)', cmd):
                if not method or '--mutation-method ' + method in cmd or '--mutation-method=' + method in cmd or '--pop_mutationmethod ' + method in cmd:
                    return True
    except OSError:
        pass
    return False


VARIANTS = [('0.50', '0.25', 'results_diff_f050_p025'),
            ('0.50', '0.75', 'results_diff_f050_p075'),
            ('0.25', '0.50', 'results_diff_f025_p050'),
            ('1.00', '0.50', 'results_diff_f100_p050')]


def display(dataset):
    lines = []
    total = 0
    for f, fraction, directory in VARIANTS:
        summary = experiment(dataset, 'differential', directory)
        rows = [r for r in read_rows(summary) if r.get('f1score') and r.get('class_error_percent')] if summary else []
        done = len(rows)
        total += done
        running = process_running(dataset, 'differential', directory)
        status = 'RUNNING' if running else 'COMPLETED' if done >= 100 else 'WAITING / NOT RUNNING'
        folds = {r.get('fold') for r in rows if r.get('fold')}
        lines.extend([f'F={f} | Fraction={fraction} | {status}',
                      f'  Runs: {done}/100 | Folds started: {len(folds)}/10'])
        for label, field in [('Class error (%)','class_error_percent'), ('Accuracy (%)','accuracy_percent'), ('Precision','precision'), ('Recall','recall'), ('F1 score','f1score')]:
            lines.append(f'  {label:16}: {stats(rows, field)}')
        if summary:
            prefix = summary.name.split('_summary_')[0]
            latest = max(Path(directory).glob(glob.escape(prefix)+'_fold*_seed*.log'), key=lambda p:p.stat().st_mtime, default=None)
            if latest:
                match = FOLD_SEED.search(latest.name)
                try:
                    with latest.open('rb') as stream:
                        stream.seek(max(0, latest.stat().st_size-150000))
                        content = stream.read().decode(errors='replace')
                    generations = list(GEN.finditer(content))
                    if match:
                        lines.append(f'  Latest fold / seed: {match.group(1)} / {match.group(2)}')
                    if generations:
                        g = generations[-1]
                        lines.append(f'  Generation: {g.group(1)} | Fitness: {g.group(2)} | Test error: {g.group(3) or "—"}')
                except OSError:
                    pass
        lines.append('')
    return '\n'.join([f'QGenClass Differential | {dataset} | Total: {total}/400 ({total/4:.1f}%)', '='*65, *lines, 'Ctrl+C: stop monitor only | Refresh: 2s'])


def main():
    if len(sys.argv) != 2 or sys.argv[1] in ('-h','--help'):
        print('Usage: ./monitor_differential_next.sh DATASET')
        return
    os.chdir(Path(__file__).resolve().parent)
    try:
        while True:
            if sys.stdout.isatty():
                print('\033[2J\033[H', end='')
            print(display(sys.argv[1]), flush=True)
            time.sleep(2)
    except KeyboardInterrupt:
        print('\nMonitor stopped.')

if __name__ == '__main__':
    main()
