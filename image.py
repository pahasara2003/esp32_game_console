
import os
import sys
import base64
import requests

API_KEY = os.environ["OPENROUTER_API_KEY"]

# Cheapest text-only SVG model
MODEL = "recraft/recraft-v4-vector"

prompt = " ".join(sys.argv[1:]) or (
    "A colorful retro gaming controller icon, "
    "modern flat vector design, clean shapes"
)

response = requests.post(
    "https://openrouter.ai/api/v1/images",
    headers={
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json",
    },
    json={
        "model": MODEL,
        "prompt": prompt,
        "output_format": "svg",
        "aspect_ratio": "1:1",
        "n": 1,
    },
    timeout=180,
)

response.raise_for_status()
result = response.json()

for i, item in enumerate(result["data"]):
    svg = base64.b64decode(item["b64_json"])

    if b"<svg" not in svg[:1000]:
        raise ValueError("Response is not SVG")

    filename = f"output_{i + 1}.svg"

    with open(filename, "wb") as f:
        f.write(svg)

    print(f"Saved {filename}")

print("Usage:", result.get("usage"))
