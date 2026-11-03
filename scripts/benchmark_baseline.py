"""Measure Approach 1; standard library only. Official runs belong on Ubuntu."""
import argparse
import csv
import datetime as dt
import json
import math
from pathlib import Path
import platform
import statistics
import subprocess
import sys

FIELDS = ['approach', 'threads', 'k', 'points', 'inside', 'pi', 'time_sec']


def nonnegative_long_long(text):
    if not text.isascii() or not text.isdecimal() or int(text) > 2**63 - 1:
        raise argparse.ArgumentTypeError('expected a nonnegative decimal long long (max 9223372036854775807)')
    return int(text)


def inspect_command(command):
    try:
        result = subprocess.run(command, text=True, capture_output=True,
                                timeout=10, check=False)
        return result.stdout.strip() if result.returncode == 0 else 'unavailable'
    except (OSError, subprocess.TimeoutExpired):
        return 'unavailable'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', default='./pi_single')
    parser.add_argument('--points', type=nonnegative_long_long, default=1_000_000_000)
    parser.add_argument('--seed', type=nonnegative_long_long, default=42)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--warmups', type=int, default=1)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--build-flags', default='-O2 -std=c11 -Wall -Wextra -Wpedantic -pthread')
    args = parser.parse_args()
    if args.points == 0 or args.seed > 2**32 - 1 or args.runs < 5 or args.warmups < 0:
        parser.error('points > 0; seed in [0, 4294967295]; runs >= 5; warmups >= 0')
    binary = Path(args.binary).resolve()
    if not binary.is_file():
        parser.error(f'binary not found: {binary}; run make first')

    stamp = dt.datetime.now().astimezone()
    output = args.output or Path('results') / stamp.strftime('baseline_%Y%m%d_%H%M%S_%f')
    output.mkdir(parents=True, exist_ok=False)
    metadata = {
        'started_at': stamp.isoformat(), 'platform': platform.platform(),
        'python': sys.version, 'binary': str(binary),
        'command': [str(binary), str(args.points), str(args.seed)],
        'points': args.points, 'seed': args.seed, 'runs': args.runs,
        'discarded_warmups': args.warmups,
        'rng': 'POSIX rand_r; private unsigned int seed; reset for every run',
        'geometry': 'full circle; x,y in [-1,1]',
        'timed_scope': 'sample generation + circle test + local counting only',
        'compiler_available': inspect_command(['gcc', '--version']),
        'build_flags_declared': args.build_flags,
        'cpu': inspect_command(['lscpu']), 'kernel': inspect_command(['uname', '-a']),
        'status': 'running',
    }
    meta_path = output / 'machine.json'

    def save_meta():
        meta_path.write_text(json.dumps(metadata, indent=2, ensure_ascii=False), encoding='utf-8')

    save_meta()
    expected_inside = None
    measured = []
    try:
        with (output / 'raw.csv').open('w', newline='', encoding='utf-8') as stream:
            writer = csv.DictWriter(stream, fieldnames=['run', 'seed'] + FIELDS)
            writer.writeheader()
            for index in range(args.warmups + args.runs):
                process = subprocess.run(metadata['command'], text=True,
                                         capture_output=True, check=True)
                rows = list(csv.reader(process.stdout.splitlines()))
                if len(rows) != 1 or len(rows[0]) != len(FIELDS):
                    raise ValueError(f'unexpected CSV output: {process.stdout!r}')
                row = dict(zip(FIELDS, rows[0]))
                inside, seconds = int(row['inside']), float(row['time_sec'])
                estimate = float(row['pi'])
                if (row['approach'] != 'single' or int(row['threads']) != 1
                        or int(row['k']) != 0 or int(row['points']) != args.points
                        or not 0 <= inside <= args.points
                        or not math.isfinite(seconds) or seconds <= 0
                        or not math.isfinite(estimate)
                        or not math.isclose(estimate, 4 * inside / args.points, abs_tol=1e-11)):
                    raise ValueError(f'invalid result: {row}; use more points if time is zero')
                if expected_inside is not None and inside != expected_inside:
                    raise ValueError('same seed/configuration produced different inside counts')
                expected_inside = inside
                if index < args.warmups:
                    continue
                row.update(run=index - args.warmups + 1, seed=args.seed)
                writer.writerow(row)
                stream.flush()
                measured.append(seconds)
        summary = {
            'approach': 'single', 'threads': 1, 'points': args.points,
            'seed': args.seed, 'runs': args.runs, 'inside': expected_inside,
            'pi': 4.0 * expected_inside / args.points,
            'mean_seconds': statistics.mean(measured),
            'stdev_seconds': statistics.stdev(measured),
            'min_seconds': min(measured), 'max_seconds': max(measured),
            'speedup': 1.0,
        }
        with (output / 'summary.csv').open('w', newline='', encoding='utf-8') as stream:
            writer = csv.DictWriter(stream, fieldnames=list(summary))
            writer.writeheader()
            writer.writerow(summary)
        metadata['status'] = 'complete'
        save_meta()
        print(f'Results: {output.resolve()}')
        print(f'T_single = {summary["mean_seconds"]:.9f} s; '
              f'sample stdev = {summary["stdev_seconds"]:.9f} s')
        print(f'pi = {summary["pi"]:.12f}; runs = {args.runs}')
    except Exception as error:
        metadata['status'] = 'failed'
        metadata['error'] = str(error)
        save_meta()
        raise


if __name__ == '__main__':
    main()
