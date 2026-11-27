"""check_matches.py [--against <git rev>] : compare this module's report (module.json "report") with the committed one
(default HEAD). Lists lost and new matches; exits 1 if any match was lost. Run after tools/build_all.py."""
import json, os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gimg import MODULE, ROOT

REPORT = MODULE["report"]


def matched(report):
    return {f["name"] for u in report["units"] for f in u.get("functions", []) if f.get("fuzzy_match_percent") == 100}


rev = sys.argv[sys.argv.index("--against") + 1] if "--against" in sys.argv else "HEAD"
shown = subprocess.run(["git", "show", "%s:%s" % (rev, REPORT)], cwd=ROOT, capture_output=True, text=True)
old = json.loads(shown.stdout) if shown.returncode == 0 else {"units": []}
new = json.load(open(os.path.join(ROOT, REPORT)))
before, after = matched(old), matched(new)
lost, gained = sorted(before - after), sorted(after - before)
print("matched %d -> %d (%+d)" % (len(before), len(after), len(after) - len(before)))
for name in gained:
    print("  new   " + name)
for name in lost:
    print("  LOST  " + name)
sys.exit(1 if lost else 0)
