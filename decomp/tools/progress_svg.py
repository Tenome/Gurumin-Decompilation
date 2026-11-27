"""progress_svg.py <report.json> <out.svg> : ByteMatch-style progress meter for the README.
The title follows the version folder the meter is written to (progress/jp or progress/en)."""
import json, os, sys

TITLES = {
    "jp": "Gurumin (JP PC) matching decompilation",
    "en": "Gurumin (EN Steam) game_en_steam.exe matching decompilation",
}


def bar(y, label, percent, detail, colour):
    width = 400
    filled = max(0.0, min(1.0, percent / 100.0)) * width
    return (f'<text x="20" y="{y}" class="label">{label}</text>'
            f'<text x="580" y="{y}" class="value" text-anchor="end">{percent:.2f}%</text>'
            f'<rect x="20" y="{y + 8}" width="{width}" height="12" rx="6" class="track"/>'
            f'<rect x="20" y="{y + 8}" width="{filled:.1f}" height="12" rx="6" fill="{colour}"/>'
            f'<text x="430" y="{y + 19}" class="detail">{detail}</text>')


def main():
    report = json.load(open(sys.argv[1], encoding="utf-8"))
    version = os.path.basename(os.path.dirname(os.path.abspath(sys.argv[2])))
    title = TITLES.get(version, TITLES["jp"])
    m = report["measures"]
    functions = float(m["matched_functions_percent"])
    code = float(m["matched_code_percent"])
    fuzzy = float(m["fuzzy_match_percent"])
    rows = [
        bar(60, "Functions matched", functions, f'{m["matched_functions"]} / {m["total_functions"]}', "#4caf50"),
        bar(110, "Code matched (bytes)", code, f'{int(m["matched_code"]):,} / {int(m["total_code"]):,}', "#2196f3"),
        bar(160, "Fuzzy match (bytes)", fuzzy, "weighted by similarity", "#ff9800"),
    ]
    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="600" height="200" viewBox="0 0 600 200">
<style>
.bg {{ fill: #1e2127; }}
.title {{ font: 600 16px sans-serif; fill: #ffffff; }}
.label {{ font: 13px sans-serif; fill: #d0d4da; }}
.value {{ font: 600 13px sans-serif; fill: #ffffff; }}
.detail {{ font: 11px sans-serif; fill: #9aa1ab; }}
.track {{ fill: #333842; }}
</style>
<rect width="600" height="200" rx="10" class="bg"/>
<text x="20" y="30" class="title">{title}</text>
{''.join(rows)}
</svg>
"""
    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as f:
        f.write(svg)


if __name__ == "__main__":
    main()
