"""Verify the byte-preserved lwIP source subset without modifying it."""
import hashlib
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parents[1] / "third_party" / "lwip"
expected = {}
for record in (root / "source-sha256.txt").read_text(encoding="utf-8").splitlines():
    digest, relative = record.split("  ", 1)
    expected[relative] = digest
vendor = root / "vendor"
actual = {
    path.relative_to(vendor).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
    for path in vendor.rglob("*") if path.is_file()
}
if actual != expected:
    changed = sorted(set(actual) ^ set(expected) | {
        name for name in actual.keys() & expected.keys() if actual[name] != expected[name]
    })
    sys.exit("lwIP source manifest mismatch: " + ", ".join(changed))
print(f"PASS: {len(actual)} lwIP vendor files match the recorded source bytes")
