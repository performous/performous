#!/bin/bash
# Record the replay test games with a real Performous build (see ../README.md).
# Usage: record.sh <performous binary built with autoplay.patch> [<output dir, default: ..>]
# Needs python3, ffmpeg and Xvfb. Runs isolated: own HOME and config, no sound (ALSA null device).
set -eu
BIN=$(realpath "$1")
HERE=$(cd "$(dirname "$0")" && pwd)
DEST=$(realpath "${2:-$HERE/..}")
ROOT=$(cd "$HERE/../../.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

python3 -I "$HERE/gen.py" "$WORK/out"
for song in "$WORK"/out/songs/*/; do  # 80 s of silence to play along
	ffmpeg -loglevel error -f lavfi -i anullsrc=r=44100:cl=stereo -t 80 -c:a libvorbis "$song/song.ogg"
done
# One player per game: rendering is software only, fewer players means more frames
python3 -I - "$WORK/out" "$WORK/split" <<'EOF'
import json, os, sys
os.makedirs(sys.argv[2])
for f in sorted(os.listdir(sys.argv[1])):
    if not f.endswith('.json'): continue
    spec = json.load(open(os.path.join(sys.argv[1], f)))
    for p in spec['players']:
        one = dict(spec, players=[p], config=dict(spec['config'], **p.get('config', {})))
        json.dump(one, open(os.path.join(sys.argv[2], spec['song'][-1] + '-' + p['name'] + '.json'), 'w'))
EOF

pgrep -f 'Xvfb :99' >/dev/null || { Xvfb :99 -screen 0 1280x720x24 >/dev/null 2>&1 & sleep 2; }
for spec in "$WORK"/split/*.json; do
	name=$(basename "$spec")
	RES="$WORK/run/${name%.json}"
	mkdir -p "$RES"/{home,cfg/performous,data,cache}
	python3 -I - "$spec" "$RES/cfg/performous/config.xml" "$WORK/out/songs" <<'EOF'
import json, sys
cfg = dict(json.load(open(sys.argv[1]))['config'])
cfg.update({'graphic/video': False, 'graphic/webcam': False, 'graphic/window_width': 640, 'graphic/window_height': 360})
types = {bool: 'bool', float: 'float', int: 'int'}
out = ['<?xml version="1.0" encoding="UTF-8"?>', '<performous>']
for k, v in cfg.items():
    val = ('true' if v else 'false') if isinstance(v, bool) else repr(v)
    out.append(f'  <entry name="{k}" type="{types[type(v)]}" value="{val}"/>')
out.append(f'  <entry name="paths/songs" type="string_list"><stringvalue>{sys.argv[3]}</stringvalue></entry>')
out.append('</performous>')
open(sys.argv[2], 'w').write('\n'.join(out) + '\n')
EOF
	echo 'pcm.!default { type null }' > "$RES/asound.conf"
	echo "Playing $name"
	(cd "$RES" && timeout 300 env -u WAYLAND_DISPLAY DISPLAY=:99 PERFORMOUS_ROOT="$ROOT" PERFORMOUS_AUTOPLAY="$spec" \
	  HOME="$RES/home" XDG_CONFIG_HOME="$RES/cfg" XDG_DATA_HOME="$RES/data" XDG_CACHE_HOME="$RES/cache" \
	  ALSA_CONFIG_PATH="$RES/asound.conf" PULSE_SERVER=none PIPEWIRE_REMOTE=none \
	  "$BIN" > "$RES/console.log" 2>&1) || { echo "Failed, see $RES/console.log"; trap - EXIT; exit 1; }
	cp "$RES"/data/performous/reports/performous-report-1.json "$DEST/$name"
done
