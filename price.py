# pip install requests
import requests

base = "https://openrouter.ai/api/v1/images/models"
rows = []
for m in requests.get(base, timeout=60).json()["data"]:
    try:
        d = requests.get(f"{base}/{m['id']}/endpoints", timeout=60).json()
    except Exception:
        continue
    for e in d.get("endpoints", []):
        sp = e.get("supported_parameters", {})
        svg = "svg" in sp.get("output_format", {}).get("values", [])
        n_max = sp.get("n", {}).get("max", 1)
        refs = sp.get("input_references", {}).get("max", 0)
        for p in e.get("pricing", []):
            if p["billable"] == "output_image" and p["unit"] == "image":   # per-image prices only
                rows.append((p["cost_usd"], m["id"], p.get("variant", ""), svg, n_max, refs))

for cost, mid, var, svg, n_max, refs in sorted(rows):
    print(f"${cost:<7} {mid:<50} {var:<8} {'SVG' if svg else '   '} n<={n_max} refs<={refs}")
