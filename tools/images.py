#!/usr/bin/env python3
"""The translated machine code of Stunt Racer 2000, and how it's made.

    images.py unpack    tools/cload (the game's DeComp module) on the CLOAD
                        files -> unpacked/, assembled into unpacked/core: the
                        memory from &8F00 as the BASIC lays the blobs out
    images.py analyse   SuperSpider on core (superspider analyse --full), its
                        roots the addresses the BASIC CALLs (from Control's and
                        Code's address tables) -> analysis/core.json
    images.py gen       its C translation (superspider gen) -> recomp/gen/
    images.py roots LOG...   gap entries from --trace-gaps runs -> analysis/core.roots

The BASIC (Code.3d_data, translated by hand into recomp/basic.c) loads each
blob at b%(n) from its DATA; all of them are contiguous from PAGE's default
&8F00 to &75C9C.
"""
import os
import struct
import subprocess
import sys

TOP = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))
UNP = os.path.join(TOP, 'unpacked')
AN = os.path.join(TOP, 'analysis')
SSBIN = os.path.join(SS, '_build/default/bin/main.exe')
BASE = 0x8F00

# name, b%() index, address (DATA 36608,143400,397400,429124,0,371868,0,450132,442964)
BLOBS = [('Poly_Code', 0, 0x8F00), ('Code', 1, 0x23028), ('DashBd1', 2, 0x61058),
         ('DashBd2', 3, 0x68C44), ('Car_Code', 5, 0x5AC9C), ('Control', 7, 0x6DE54),
         ('Messages', 8, 0x6C254)]
# CALL targets: Control's table words (b%(7)!k), Code's (b%(1)!k), Car_Code+48
CONTROL_CALLS = [0, 8, 12, 16, 28, 40, 44, 56, 156, 184, 188, 200, 224, 384, 392, 416, 424, 484, 488, 512]
CODE_CALLS = [56, 72, 80, 84]


def run(cmd):
    print('+', ' '.join(cmd))
    subprocess.run(cmd, check=True)


def unpack():
    run(['make', '-s', '-C', os.path.join(TOP, 'tools')])
    os.makedirs(UNP, exist_ok=True)
    # cload's appdir is relative to tools/; it ends through roger_finish
    # (a non-zero status), so its output files are what's checked
    for n, _, _ in BLOBS:
        if os.path.exists(os.path.join(UNP, n)):
            os.remove(os.path.join(UNP, n))
    subprocess.run([os.path.join(TOP, 'tools', 'cload'), '--headless', '--dir=' + UNP, 'Code.Names=90000']
                   + ['Code.%s=%x' % (n, a) for n, _, a in BLOBS], cwd=os.path.join(TOP, 'tools'),
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    end = max(a + os.path.getsize(os.path.join(UNP, n)) for n, _, a in BLOBS)
    core = bytearray(end - BASE)
    for n, _, a in BLOBS:
        d = open(os.path.join(UNP, n), 'rb').read()
        core[a - BASE:a - BASE + len(d)] = d
    open(os.path.join(UNP, 'core'), 'wb').write(core)
    print('core: &%x-&%x' % (BASE, end))


def word(core, a):
    return struct.unpack_from('<I', core, a - BASE)[0]


def call_roots():
    core = open(os.path.join(UNP, 'core'), 'rb').read()
    ctl, code = dict((n, a) for n, _, a in BLOBS)['Control'], dict((n, a) for n, _, a in BLOBS)['Code']
    roots = {word(core, ctl + k) for k in CONTROL_CALLS} | {word(core, code + k) for k in CODE_CALLS}
    roots.add(dict((n, a) for n, _, a in BLOBS)['Car_Code'] + 48)
    return sorted(roots)


def analyse():
    roots = os.path.join(AN, 'core.roots')
    calls = os.path.join(AN, 'calls.roots')
    with open(calls, 'w') as f:
        f.write('# the addresses the BASIC CALLs (images.py call_roots)\n')
        f.writelines('%x\n' % a for a in call_roots())
    if not os.path.exists(roots):
        open(roots, 'w').write('# gap entries from --trace-gaps runs (images.py roots)\n')
    merged = os.path.join(AN, 'all.roots')
    open(merged, 'w').write(open(calls).read() + open(roots).read())
    subprocess.run(['dune', 'build'], cwd=SS, check=True)
    run([SSBIN, 'analyse', os.path.join(UNP, 'core'), '%x' % BASE, '--full', '--roots', merged,
         '-o', os.path.join(AN, 'core.json')])


def gen():
    rc = os.path.join(TOP, 'recomp')
    run([SSBIN, 'gen', os.path.join(UNP, 'core'), '%x' % BASE, os.path.join(AN, 'core.json'),
         '--c', os.path.join(rc, 'gen')])


def roots(logs):
    run([sys.executable, os.path.join(SS, 'tools/gap_roots.py'), os.path.join(UNP, 'core'), '%x' % BASE,
         os.path.join(AN, 'core.roots')] + logs)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if sys.argv[1] == 'roots':
        roots(sys.argv[2:])
    else:
        for step in sys.argv[1:]:
            {'unpack': unpack, 'analyse': analyse, 'gen': gen}[step]()
