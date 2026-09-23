"""Extract the selected 7.1.2 UI resources from an AppleDouble resource fork.

Usage: python scripts/build_classic_ui_resources.py OLD_FAMILY_RSRC CURRENT_FAMILY_RSRC
The original files are read only. Outputs are deterministic repository assets.
"""

import hashlib
import json
import pathlib
import re
import struct
import sys


OVERRIDES = {
    "WIND": [128, 129, 130, 131, 132],
    "DLOG": [137, 138, 144, 156, 166, 300],
    "DITL": [130, 135, 137, 138, 140, 145, 164, 166, 167, 300],
    "PICT": [176, 187, 300, 303, 304],
    "wctb": [128],
    "dctb": [137, 138, 144, 145, 156, 166],
}


def fork_bytes(path):
    data = pathlib.Path(path).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if data[:4] == bytes.fromhex("00051607"):
        for index in range(struct.unpack_from(">H", data, 24)[0]):
            entry_type, start, length = struct.unpack_from(">III", data, 26 + index * 12)
            if entry_type == 2:
                return data[start:start + length], digest
        raise ValueError(f"{path}: AppleDouble resource fork is missing")
    return data, digest


def parse_fork(data):
    data_start, map_start, _, _ = struct.unpack_from(">IIII", data)
    type_start = map_start + struct.unpack_from(">H", data, map_start + 24)[0]
    name_start = map_start + struct.unpack_from(">H", data, map_start + 26)[0]
    result = {}
    for index in range(struct.unpack_from(">H", data, type_start)[0] + 1):
        entry = type_start + 2 + index * 8
        kind = data[entry:entry + 4].decode("mac_roman")
        count, refs = struct.unpack_from(">HH", data, entry + 4)
        for item in range(count + 1):
            ref = type_start + refs + item * 12
            resource_id, name_offset = struct.unpack_from(">hh", data, ref)
            flags = data[ref + 4]
            offset = data_start + int.from_bytes(data[ref + 5:ref + 8], "big")
            length = struct.unpack_from(">I", data, offset)[0]
            name = b"" if name_offset == -1 else data[name_start + name_offset + 1:
                name_start + name_offset + 1 + data[name_start + name_offset]]
            result[kind, resource_id] = (name, flags, data[offset + 4:offset + 4 + length])
    return result


def make_fork(resources):
    data_part = bytearray()
    names = bytearray()
    types = bytearray()
    refs = bytearray()
    kinds = sorted({kind for kind, _ in resources})
    for kind in kinds:
        entries = sorted((key, resources[key]) for key in resources if key[0] == kind)
        types += kind.encode("mac_roman") + struct.pack(">HH", len(entries) - 1, 2 + 8 * len(kinds) + len(refs))
        for (_, resource_id), (name, flags, payload) in entries:
            name_offset = -1
            if name:
                name_offset = len(names)
                names += bytes([len(name)]) + name
            offset = len(data_part)
            data_part += struct.pack(">I", len(payload)) + payload
            if len(data_part) & 1:
                data_part += b"\0"
            refs += struct.pack(">hhB", resource_id, name_offset, flags) + offset.to_bytes(3, "big") + b"\0" * 4
    type_list = struct.pack(">H", len(kinds) - 1) + types + refs
    map_part = bytearray(28) + type_list + names
    struct.pack_into(">HH", map_part, 24, 28, 28 + len(type_list))
    data_start = 256
    map_start = data_start + len(data_part)
    header = struct.pack(">IIII", data_start, map_start, len(data_part), len(map_part))
    map_part[:16] = header
    return header + bytes(240) + data_part + map_part


def main(old_path, current_path):
    old_bytes, old_hash = fork_bytes(old_path)
    current_bytes, current_hash = fork_bytes(current_path)
    old = parse_fork(old_bytes)
    current = parse_fork(current_bytes)
    selected = {}
    for kind, ids in OVERRIDES.items():
        for resource_id in ids:
            key = kind, resource_id
            if key not in old or key not in current:
                if key in old and kind == "dctb" and resource_id == 145:
                    selected[key] = old[key]
                    continue
                if kind in ("wctb", "dctb"):
                    continue
                raise ValueError(f"Required resource {kind}:{resource_id} missing from an input")
            selected[key] = old[key]

    output = pathlib.Path("resources/classic-ui.rsrc")
    output.write_bytes(make_fork(selected))
    callers = {}
    for path in pathlib.Path("src/realmz_orig").glob("*.c"):
        source = path.read_text(encoding="latin1")
        for kind, ids in OVERRIDES.items():
            for resource_id in ids:
                # Report direct constant callers only; computed IDs need manual review.
                functions = {"WIND": "GetNewWindow", "DLOG": "GetNewDialog",
                             "PICT": "GetPicture|pict", "DITL": "Get1Resource"}.get(kind)
                if functions and re.search(r"\b(?:" + functions + r")\s*\(\s*" + str(resource_id) + r"\b", source):
                    callers.setdefault(f"{kind}:{resource_id}", []).append(path.name)

    def dimensions(kind, payload):
        if kind not in ("WIND", "DLOG", "PICT"):
            return None
        offset = 2 if kind == "PICT" else 0
        return list(struct.unpack_from(">hhhh", payload, offset))

    report = {
        "sources": {"7.1.2": old_hash, "current": current_hash},
        "resource_ids": {
            label: {kind: sorted(resource_id for resource_kind, resource_id in resources if resource_kind == kind)
                    for kind in sorted({resource_kind for resource_kind, _ in resources})}
            for label, resources in (("7.1.2", old), ("current", current))},
        "changed_shared_ids": {
            kind: sorted(resource_id for resource_kind, resource_id in old.keys() & current.keys()
                         if resource_kind == kind and old[resource_kind, resource_id] != current[resource_kind, resource_id])
            for kind in sorted({resource_kind for resource_kind, _ in old.keys() & current.keys()})},
        "overrides": [
            {"type": kind, "id": resource_id,
             "7.1.2_rect": dimensions(kind, old[kind, resource_id][2]),
             "current_rect": dimensions(kind, current[kind, resource_id][2]) if (kind, resource_id) in current else None,
             "callers": callers.get(f"{kind}:{resource_id}", [])}
            for kind, resource_id in selected],
    }
    pathlib.Path("docs").mkdir(exist_ok=True)
    pathlib.Path("docs/classic-ui-resource-inventory.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Wrote {output} ({output.stat().st_size} bytes) and resource inventory")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2])
