Stunt Racer 2000 (Fednet / The Fourth Dimension, 1993) - static recompilation
============================================================================

Disc images: `Stunt Racer 2000 (1993) (The Fourth Dimension) (Disk 1 of 2).adf`
and `(Disk 2 of 2).adf`, the later, unprotected release (1.01, 1997, RiscPC
and StrongARM compatible - readme.txt). `Stunt Racer (19xx)(-)(Disk 2 of
2).adf` is a played copy, one track short: not used.

    extracted/disc1, disc2   $SS/tools/adf_extract.py's output ($SS = ~/stuff/superspider)
    game/!SR2000             both discs merged: libroger's appdir (../game)
    basic/                   the BASIC programs, detokenised ($SS/tools/detok.py)
    tools/cload.c            the DeComp module run by libroger: unpacks the CLOAD files
    tools/images.py          unpack (-> unpacked/, assembled into unpacked/core: the
                             memory from &8F00 as the BASIC lays the blobs out),
                             analyse (superspider analyse --full -> analysis/core.json),
                             gen (superspider gen -> recomp/gen/), roots
    analysis/core.roots      gap entries from --trace-gaps runs
    recomp/                  make regen, make, ./stuntracer

The game is a BBC BASIC program, Code.3d_data, which CLOADs its machine
code (Poly_Code, Code, DashBd1/2, Car_Code, Control, Messages - back to back
from &8F00 to &75C9C) and CALLs entry points in it from Control's and
Code's address tables. The BASIC is translated to C by $SS/tools/bas2c.py
(recomp/basic.c, on the runtime in recomp/bas.c); the machine code is
analysed and translated as one image (the BASIC's CALL targets are its
roots). recomp/game.c is the libroger configuration: disc names
(OS_GBPB 5), Read_Disc, the one assembler patch the BASIC makes, and the
modules (DeComp, ReadDisc, RTFM, ExSound, the sound effects), which are
interpreted for now.

Status: headless, it runs to the title picture.
