#!/usr/bin/env python3
"""The translated machine code of Stunt Racer 2000, and how it's made.

    images.py unpack    tools/cload (the game's DeComp module) on the CLOAD
                        files -> unpacked/, assembled into unpacked/core: the
                        memory from &8F00 as the BASIC lays the blobs out
    images.py analyse   SuperSpider (superspider analyse --full) on core, its
                        roots the addresses the BASIC CALLs (from Control's and
                        Code's address tables) -> analysis/core.json, and on
                        each module -> analysis/<name>.json
    images.py gen       their C translations (superspider gen) -> recomp/gen/<name>/,
                        plus recomp/gen/modules.inc (game.c's module list)
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

# The modules !Run3 and the BASIC load (in game/!SR2000/Code), in game.c's
# order, each translated for its own 64K slot in the RMA from &1840000.
# They are loaded when the program asks for them (on demand).
MODULES = ['DeComp,fffffa45-00000000', 'ReadDisc,fffffa47-00000000', 'RTFM,fffffa45-00000000',
           'ExSound,fffffa45-00000000'] + ['Sounds/%s,fffffa45-00000000' % n for n in
           ['Mod2', 'Engine', 'ExplodeMD2', 'CheckPt', 'Lap', 'Start', 'Finished', 'BottomOut',
            'SCrash', 'grass_skid', 'Transport', 'Zoom', 'Fuzz']]
CODE = os.path.join(TOP, 'game', '!SR2000', 'Code')

# Code.SUtil: 176 bytes the BASIC *LOADs into ka% (DIM ka% 176, the second
# block in the heap: the BASIC runtime's heap (bas_lomem, recomp/game.c) starts at &8FD60, after DIM ga% 8,
# which takes 9 bytes) and CALLs. Translated for that address, an overlay
# in game.c.
SUTIL = ('SUtil,fffffd45-00000000', 0x8FD6C)


def modules():
    """(name, path relative to game/!SR2000/Code, base)"""
    return [(m.split('/')[-1].split(',')[0].lower(), m, 0x1840000 + k * 0x10000) for k, m in enumerate(MODULES)]


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
    run([SSBIN, 'analyse', os.path.join(CODE, SUTIL[0]), '%x' % SUTIL[1], '--full',
         '-o', os.path.join(AN, 'sutil.json')])
    for name, path, base in modules():
        rf = os.path.join(AN, name + '.roots')
        run([SSBIN, 'analyse', os.path.join(CODE, path), '%x' % base, '--module', '--full',
             '-o', os.path.join(AN, name + '.json')] + (['--roots', rf] if os.path.exists(rf) else []))


def gen():
    rc = os.path.join(TOP, 'recomp')
    run([SSBIN, 'gen', os.path.join(UNP, 'core'), '%x' % BASE, os.path.join(AN, 'core.json'),
         '--c', os.path.join(rc, 'gen', 'core')])
    run([SSBIN, 'gen', os.path.join(CODE, SUTIL[0]), '%x' % SUTIL[1], os.path.join(AN, 'sutil.json'),
         '--c', os.path.join(rc, 'gen', 'sutil'), '--prefix', 'S', '--tabprefix', 'recomp_sutil', '--files', '1'])
    for k, (name, path, base) in enumerate(modules()):
        run([SSBIN, 'gen', os.path.join(CODE, path), '%x' % base, os.path.join(AN, name + '.json'),
             '--c', os.path.join(rc, 'gen', name), '--prefix', 'M%d' % k, '--tabprefix', 'recomp_' + name,
             '--files', '1', '--covtag', chr(ord('a') + k)])
    with open(os.path.join(rc, 'gen', 'modules.inc'), 'w') as f:
        f.write('/* generated by ../tools/images.py - the modules, for game.c */\n')
        for name, _, _ in modules():
            f.write('extern const struct roger_image recomp_%s_image;\n' % name)
        f.write('static const struct roger_module modules[] = {\n')
        for name, path, _ in modules():
            f.write('    {"../game/!SR2000/Code/%s", &recomp_%s_image, 1},\n' % (path, name))
        f.write('    {NULL, NULL, 0}};\n')


def roots(logs):
    run([sys.executable, os.path.join(SS, 'tools/gap_roots.py'), os.path.join(UNP, 'core'), '%x' % BASE,
         os.path.join(AN, 'core.roots')] + logs)
    for name, path, base in modules():
        run([sys.executable, os.path.join(SS, 'tools/gap_roots.py'), os.path.join(CODE, path), '%x' % base,
             os.path.join(AN, name + '.roots')] + logs)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if sys.argv[1] == 'roots':
        roots(sys.argv[2:])
    else:
        for step in sys.argv[1:]:
            {'unpack': unpack, 'analyse': analyse, 'gen': gen}[step]()
