#!/usr/bin/env python3
"""Live monitor for QGenClass runmodel.sh logs (read-only)."""
import argparse
import datetime as dt
import glob
import os
import re
import sys
import time

GEN = re.compile(r'GENERATION\s*=\s*(\d+)\s+FITNESS\s*=\s*([^\s%]+)%?(?:\s+TEST_ERROR\s*=\s*([^\s%]+)%?)?')
FINAL = re.compile(r'CLASS_ERROR:\s*([^\s]+)\s+PRECISION:\s*([^\s]+)\s+RECALL:\s*([^\s]+)\s+F1SCORE:\s*([^\s]+)')
SEED = re.compile(r'_seed(\d+)_')

def latest_log(folder, dataset, method, seed, explicit):
    if explicit:
        return explicit if os.path.isfile(explicit) else None
    files = glob.glob(os.path.join(glob.escape(folder), '*.log'))
    matches = []
    for p in files:
        name = os.path.basename(p)
        if dataset and not name.startswith(dataset + '_'):
            continue
        m = SEED.search(name)
        if not m or (seed is not None and int(m.group(1)) != seed):
            continue
        # The original runmodel.sh does not encode mutation method in filenames.
        # Check the command line printed by the script if present; otherwise warn.
        matches.append(p)
    if not matches:
        return None
    matches.sort(key=lambda p: os.stat(p).st_mtime_ns, reverse=True)
    if method:
        for p in matches:
            try:
                with open(p, encoding='utf-8', errors='replace') as f:
                    head = f.read(10000)
                if '--pop_mutationmethod=' + method in head:
                    return p
            except OSError:
                pass
        # Method cannot be determined reliably from the existing filename/log.
        return None
    return matches[0]

def fmt(v):
    return v if v is not None else '—'

def main():
    ap = argparse.ArgumentParser(description='Monitor one QGenClass run in real time.')
    ap.add_argument('--dataset', help='Dataset prefix, e.g. ionosphere')
    ap.add_argument('--seed', type=int, help='Seed of the run to monitor')
    ap.add_argument('--method', choices=['standard','active','targeted'], help='Only if mutation method is written in the log command')
    ap.add_argument('--results-dir', default='results')
    ap.add_argument('--log', help='Exact log file (recommended when multiple experiments run)')
    ap.add_argument('--generations', type=int, help='Expected maximum number of generations')
    ap.add_argument('--interval', type=float, default=2.0)
    args = ap.parse_args()
    if args.interval <= 0: ap.error('--interval must be positive')
    if not args.log and not args.dataset: ap.error('Specify --dataset or --log')
    last_display = None
    try:
        while True:
            path = latest_log(args.results_dir, args.dataset, args.method, args.seed, args.log)
            if not path:
                print('\rWaiting for matching log file... ', end='', flush=True)
                time.sleep(args.interval)
                continue
            try:
                with open(path, encoding='utf-8', errors='replace') as f:
                    content = f.read()
                stat = os.stat(path)
            except OSError:
                time.sleep(args.interval)
                continue
            generations = list(GEN.finditer(content))
            finals = list(FINAL.finditer(content))
            g = generations[-1] if generations else None
            final = finals[-1] if finals else None
            age = max(0, time.time() - stat.st_mtime)
            elapsed = max(0, time.time() - stat.st_ctime) # approximate; ctime is metadata change time
            generation = int(g.group(1)) if g else None
            progress = f'{generation}/{args.generations}' if generation is not None and args.generations else fmt(generation)
            status = 'FINAL' if final else ('WAITING FOR GENERATION' if not g else 'RUNNING / log may be idle')
            lines = [
                'QGenClass live run monitor',
                f'Log: {path}',
                f'Status: {status} | Last log update: {age:.0f}s ago',
                f'Generation: {progress} | Fitness: {fmt(g.group(2) if g else None)}% | Test error: {fmt(g.group(3) if g else None)}%',
            ]
            if final:
                lines.append(f'Final class error: {final.group(1)} | Precision: {final.group(2)} | Recall: {final.group(3)} | F1: {final.group(4)}')
            lines.append('Press Ctrl+C to stop monitoring (the experiment continues).')
            display = '\n'.join(lines)
            if display != last_display:
                sys.stdout.write('\033[2J\033[H' if sys.stdout.isatty() else '\n')
                print(display, flush=True)
                last_display = display
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print('\nMonitor stopped.')

if __name__ == '__main__':
    main()
