#!/usr/bin/env python3
"""Run real MuJoCo/DDS acceptance checks; retain sensor CSVs and process logs."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import subprocess
import time

UP = [0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763] * 2
DOWN = [0.0473455, 1.22187, -2.44375, -0.0473455, 1.22187, -2.44375] * 2


def summarize(rows, end, target):
    window = [r for r in rows if end - 0.65 <= r['time'] <= end - 0.15]
    if len(window) < 20:
        raise AssertionError(f'Insufficient fresh feedback: {len(window)} samples')
    values = [r[f'q{i}'] - target[i] for r in window for i in range(12)]
    result = dict(samples=len(window), rmse=math.sqrt(sum(x*x for x in values) / len(values)),
                  height=sum(r['height'] for r in window) / len(window),
                  max_tilt=max(max(abs(r['roll']), abs(r['pitch'])) for r in window))
    assert all(math.isfinite(v) for r in window for v in r.values()), 'Nonfinite sensor feedback'
    assert result['rmse'] <= 0.15, result
    assert result['max_tilt'] < 0.6, result
    return result


class Processes:
    def __init__(self, output):
        self.output = output
        self.owned = []
        self.commands = []

    def start(self, name, args):
        path = self.output / f'{name}.log'
        log = path.open('w')
        p = subprocess.Popen([str(a) for a in args], stdout=log, stderr=subprocess.STDOUT)
        self.owned.append((p, log))
        self.commands.append(dict(name=name, argv=[str(a) for a in args], pid=p.pid))
        return p

    def run(self, name, args, timeout=15, expected=0):
        p = self.start(name, args)
        code = p.wait(timeout=timeout)
        assert code == expected, (name, code, (self.output / f'{name}.log').read_text()[-2000:])
        return (self.output / f'{name}.log').read_text()

    def close(self):
        for p, log in reversed(self.owned):
            if p.poll() is None:
                p.terminate()
                try:
                    p.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    p.kill()
                    p.wait(timeout=5)
            log.close()


def physical(mode, args, procs, result):
    external = mode.startswith('external')
    if mode == 'external-original':
        sim_args = [args.original_simulator, '-r', 'go2', '-i', args.domain, '-n', args.network]
    else:
        sim_args = [args.simulator, *([] if args.gui else ['--headless']), '--control',
                    'sport' if mode == 'internal' else 'lowcmd', '-r', 'go2',
                    '-i', args.domain, '-n', args.network]
    sim = procs.start(f'{mode}-sim', sim_args)
    adapter = None
    time.sleep(0.8)
    assert sim.poll() is None, 'Simulator exited during startup'
    if external:
        adapter = procs.start(f'{mode}-adapter', [args.bin_dir / 'go2_sport_adapter',
                              '--domain', args.domain, '--network', args.network])
        time.sleep(0.5)
    probe_start = time.monotonic()
    probe_path = args.output / f'{mode}-sensors.csv'
    probe = procs.start(f'{mode}-probe', [args.bin_dir / 'go2_state_probe', args.domain, 14,
                                         probe_path, args.network])
    time.sleep(0.5)
    conflict = None
    if mode == 'internal':
        conflict = procs.start('internal-conflicting-lowcmd', [args.bin_dir / 'stand_go2', '--api',
                'lowcmd', '--action', 'standdown', '--auto', '--duration', 13,
                '--domain', args.domain, '--network', args.network])
    action_ends = []
    for pose in ('standup', 'standdown'):
        log = procs.run(f'{mode}-{pose}', [args.bin_dir / 'stand_go2', '--api',
            'lowcmd' if mode == 'direct' else 'sport', '--action', pose, '--auto',
            '--duration', 5, '--domain', args.domain, '--network', args.network], timeout=9)
        if mode != 'direct':
            assert 'result=0' in log, log
        action_ends.append(time.monotonic() - probe_start)
    if mode != 'direct':
        log = procs.run(f'{mode}-unsupported', [args.bin_dir / 'go2_rpc_probe', args.domain, 'sit'], expected=1)
        assert 'result=3203' in log, log
    assert probe.wait(timeout=6) == 0
    with probe_path.open() as f:
        rows = [{k: float(v) for k, v in row.items()} for row in csv.DictReader(f)]
    assert rows and rows[-1]['tick'] > rows[0]['tick'], 'Physics did not advance'
    assert all(math.isfinite(v) for r in rows for v in r.values()), 'Nonfinite sensor stream'
    assert max(max(abs(r['roll']), abs(r['pitch'])) for r in rows) < 1.0, 'Robot overturned'
    metrics = {pose: summarize(rows, end, target) for pose, end, target in
               zip(('up', 'down'), action_ends, (UP, DOWN))}
    metrics['height_difference'] = metrics['up']['height'] - metrics['down']['height']
    assert metrics['height_difference'] >= 0.08, metrics
    metrics['control_conflict_test'] = mode == 'internal'
    metrics['command_end_seconds'] = action_ends
    metrics['sensor_samples'] = len(rows)
    metrics['gui'] = mode == 'external-original' or args.gui
    result[mode] = metrics
    print(mode, json.dumps(metrics), flush=True)
    for p in (conflict, adapter, sim):
        if p and p.poll() is None:
            p.terminate()
            p.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--simulator', type=Path, required=True)
    parser.add_argument('--original-simulator', type=Path, help='Optional preserved original GUI executable')
    parser.add_argument('--bin-dir', type=Path, required=True)
    parser.add_argument('--network', default='lo')
    parser.add_argument('--domain', type=int, default=71)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--modes', nargs='+', default=['direct', 'external', 'internal'], choices=['direct', 'external', 'internal'])
    parser.add_argument('--skip-edges', action='store_true')
    parser.add_argument('--gui', action='store_true', help='Run physical checks through the GUI simulator')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    args.simulator = args.simulator.resolve()
    args.bin_dir = args.bin_dir.resolve()
    args.output = args.output.resolve()
    procs = Processes(args.output)
    result = dict(simulator=str(args.simulator), model='unitree_robots/go2/scene.xml',
                  gui=args.gui, network=args.network, domain=args.domain,
                  simulator_sha256=hashlib.sha256(args.simulator.read_bytes()).hexdigest())
    try:
        if not args.skip_edges:
            for action in ('up', 'down'):
                log = procs.run(f'no-server-{action}', [args.bin_dir / 'go2_rpc_probe', args.domain, action], expected=1)
                assert 'result=3104' in log
            result['no_server'] = 'StandUp and StandDown returned 3104'
            adapter = procs.start('contract-adapter', [args.bin_dir / 'go2_sport_adapter', '--domain', args.domain, '--network', args.network])
            time.sleep(0.5)
            procs.run('adapter-contract', [args.bin_dir / 'go2_adapter_test', args.domain])
            adapter.terminate(); assert adapter.wait(timeout=5) == 0
            result['adapter_contract'] = 'PASS'
            for robot in ('go2', 'g1'):
                procs.run(f'lowcmd-init-{robot}', [args.simulator, '--headless', '--duration', 0.6,
                    '--control', 'lowcmd', '-r', robot, '-i', args.domain, '-n', args.network])
            result['go_hg_initialization'] = 'PASS'
            for opts in (['--control', 'invalid'], ['--control', 'sport', '-r', 'g1']):
                procs.run('invalid-' + opts[-1], [args.simulator, '--headless', '--duration', 0.2,
                          '-i', args.domain, '-n', args.network, *opts], expected=2)
        for mode in args.modes:
            physical(mode, args, procs, result)
        if args.original_simulator:
            physical('external-original', args, procs, result)
        result['status'] = 'PASS'
    except Exception as e:
        result['status'] = 'FAIL'
        result['error'] = str(e)
        raise
    finally:
        procs.close()
        for (process, _), command in zip(procs.owned, procs.commands):
            command['exit_code'] = process.returncode
        result['commands'] = procs.commands
        (args.output / 'results.json').write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
