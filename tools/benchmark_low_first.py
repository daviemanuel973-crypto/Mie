#!/usr/bin/env python3
"""Compare real scheduler/villager sources and isolated chunk ordering at one CPU.

The measurements are subsystem microbenchmarks, not FPS or GPU measurements.
Usage: python3 tools/benchmark_low_first.py --output /tmp/mie-performance.json
"""
import argparse
import json
import pathlib
import platform
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', default='v0.10.0')
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    baseline = subprocess.check_output(
        ['git', 'rev-parse', '--verify', args.baseline + '^{commit}'], cwd=root, text=True).strip()
    results = {}
    with tempfile.TemporaryDirectory(prefix='mie-low-first-') as work:
        work = pathlib.Path(work)
        for name in ('include/gameLayer/native/gameplayScheduler.h',
                     'include/gameLayer/native/villagerSociety.h',
                     'src/gameLayer/native/gameplayScheduler.cpp'):
            destination = work / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(subprocess.check_output(['git', 'show', baseline + ':' + name], cwd=root))
        for label, source_root in (('baseline', work), ('candidate', root)):
            binary = work / label
            command = [args.compiler, '-std=c++17', '-O2', '-DNDEBUG',
                       '-I' + str(source_root / 'include/gameLayer'),
                       '-I' + str(root / 'include/gameLayer'),
                       str(root / 'tools/lowFirstBenchmark.cpp'),
                       str(source_root / 'src/gameLayer/native/gameplayScheduler.cpp'),
                       '-o', str(binary)]
            if label == 'baseline':
                command.insert(1, '-DMIE_PERF_BASELINE')
            subprocess.run(command, cwd=root, check=True)
            output = subprocess.check_output([str(binary)], text=True)
            results[label] = [json.loads(row) for row in output.splitlines()]
    comparisons = []
    for old, new in zip(results['baseline'], results['candidate']):
        if old['case'] != new['case'] or old['checksum'] != new['checksum']:
            raise RuntimeError('Baseline/candidate result mismatch: ' + old['case'])
        comparisons.append({'case': old['case'], 'baseline_median_us': old['median_us'],
                            'candidate_median_us': new['median_us'],
                            'reduction_percent': round(100 * (1 - new['median_us'] / old['median_us']), 2)})
    report = {'baseline_commit': baseline, 'platform': platform.platform(),
              'compiler': subprocess.check_output([args.compiler, '--version'], text=True).splitlines()[0],
              'scope': 'Subsystem CPU microbenchmarks; excludes GPU, disk and target-hardware FPS.',
              'results': results, 'comparisons': comparisons}
    pathlib.Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(comparisons, indent=2))


if __name__ == '__main__':
    main()
