#!/bin/sh
# Build Stunt Racer 2000 from your own copy of the game:
#     ./build.sh "Stunt Racer 2000 (1993) (The Fourth Dimension).zip"     (or the two .adf files)
# then: cd recomp && ./stuntracer
# The steps are recipe.json's "build", run by SuperSpider's tools/weave.py.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SUPERSPIDER="${SUPERSPIDER:-$HOME/stuff/superspider}"
[ $# -gt 0 ] || { echo "usage: $0 GAME.zip | DISC1.adf DISC2.adf" >&2; exit 2; }
(cd "$SUPERSPIDER" && dune build)
WEAVE_RECIPES="$HERE" exec python3 -I "$SUPERSPIDER/tools/weave.py" build "$@"
