#!/usr/bin/env python3

import argparse
import json
from collections import Counter, defaultdict, deque
from pathlib import Path


def load_json(path):
    with Path(path).open("r", encoding="utf-8") as f:
        return json.load(f)


def walk_elements(elements):
    """Recursively yield clang-uml elements."""
    for element in elements:
        yield element
        for key in ("elements", "children"):
            children = element.get(key)
            if isinstance(children, list):
                yield from walk_elements(children)


def element_name(element):
    return (
        element.get("display_name")
        or element.get("name")
        or str(element.get("id", "<unnamed>"))
    )


def source_file(element):
    loc = element.get("source_location")
    if isinstance(loc, dict):
        return loc.get("file") or ""
    return ""


def layer_from_path(path):
    if not path:
        return None

    path = str(path).replace("\\", "/")

    if path == "src/main.cpp":
        return "root"

    if path.startswith("src/"):
        parts = path.split("/")
        if len(parts) >= 3:
            return parts[1]

    return None


def include_element_path(element):
    display = element.get("display_name", "")
    if isinstance(display, str) and display.startswith("src/"):
        return display

    name = element.get("name", "")
    if name == "main.cpp":
        return "src/main.cpp"

    return ""


def index_elements(data):
    elements = list(walk_elements(data.get("elements", [])))
    return elements, {
        str(e["id"]): e
        for e in elements
        if "id" in e
    }


def weak_components(ids, relationships):
    adjacency = defaultdict(set)

    for rel in relationships:
        src = str(rel.get("source", ""))
        dst = str(rel.get("destination", ""))

        if src in ids and dst in ids:
            adjacency[src].add(dst)
            adjacency[dst].add(src)

    unseen = set(ids)
    components = []

    while unseen:
        start = next(iter(unseen))
        queue = deque([start])
        unseen.remove(start)
        component = []

        while queue:
            node = queue.popleft()
            component.append(node)

            for neighbor in adjacency[node]:
                if neighbor in unseen:
                    unseen.remove(neighbor)
                    queue.append(neighbor)

        components.append(component)

    return sorted(components, key=len, reverse=True)


def section(lines, title):
    lines.append("")
    lines.append(title)
    lines.append("=" * len(title))


def subsection(lines, title):
    lines.append("")
    lines.append(title)
    lines.append("-" * len(title))


def analyze_type_model(data, lines):
    elements, by_id = index_elements(data)
    relationships = data.get("relationships", [])

    incoming = Counter()
    outgoing = Counter()
    rel_by_element = defaultdict(Counter)
    unresolved = []

    for rel in relationships:
        src = str(rel.get("source", ""))
        dst = str(rel.get("destination", ""))
        kind = rel.get("type", "<missing>")

        if src not in by_id or dst not in by_id:
            unresolved.append(rel)
            continue

        outgoing[src] += 1
        incoming[dst] += 1
        rel_by_element[src][f"out:{kind}"] += 1
        rel_by_element[dst][f"in:{kind}"] += 1

    layer_counts = Counter()
    for e in elements:
        layer = layer_from_path(source_file(e))
        if layer:
            layer_counts[layer] += 1

    rel_types = Counter(
        r.get("type", "<missing>")
        for r in relationships
    )

    components = weak_components(set(by_id), relationships)

    isolated = [
        e for e in elements
        if incoming[str(e.get("id"))] == 0
        and outgoing[str(e.get("id"))] == 0
    ]

    section(lines, "TYPE MODEL")

    lines.append(f"Elements: {len(elements)}")
    lines.append(f"Relationships: {len(relationships)}")
    lines.append(f"Unresolved relationship endpoints: {len(unresolved)}")
    lines.append(f"Weakly connected components: {len(components)}")
    lines.append(f"Isolated elements: {len(isolated)}")

    subsection(lines, "Element types")
    for kind, count in sorted(
        Counter(e.get("type", "<missing>") for e in elements).items()
    ):
        lines.append(f"{kind}: {count}")

    subsection(lines, "Relationship types")
    for kind, count in sorted(rel_types.items()):
        lines.append(f"{kind}: {count}")

    subsection(lines, "Elements by source layer")
    for layer, count in sorted(layer_counts.items()):
        lines.append(f"{layer}: {count}")

    subsection(lines, "Highest relationship degree")
    lines.append(
        "Degree = generated incoming + outgoing relationships in the "
        "clang-uml type model."
    )
    lines.append("")
    lines.append(f"{'total':>5} {'out':>5} {'in':>5}  element")

    ranked = sorted(
        elements,
        key=lambda e: (
            incoming[str(e.get("id"))] + outgoing[str(e.get("id"))],
            element_name(e),
        ),
        reverse=True,
    )

    for e in ranked[:20]:
        eid = str(e.get("id"))
        total = incoming[eid] + outgoing[eid]
        layer = layer_from_path(source_file(e)) or "?"
        lines.append(
            f"{total:5d} {outgoing[eid]:5d} {incoming[eid]:5d}  "
            f"{element_name(e)} [{layer}]"
        )

    subsection(lines, "Weakly connected components")
    for i, component in enumerate(components, 1):
        names = sorted(element_name(by_id[x]) for x in component)
        preview = ", ".join(names[:12])
        if len(names) > 12:
            preview += f", ... (+{len(names) - 12})"
        lines.append(f"{i:2d}. {len(component):3d} elements: {preview}")

    subsection(lines, "Isolated elements")
    if isolated:
        for e in sorted(isolated, key=element_name):
            lines.append(
                f"{element_name(e)} "
                f"[{layer_from_path(source_file(e)) or '?'}] "
                f"{source_file(e)}"
            )
    else:
        lines.append("(none)")

    subsection(lines, "Friendship relationships")
    friendships = [
        r for r in relationships
        if r.get("type") == "friendship"
    ]
    if friendships:
        for r in friendships:
            src = by_id.get(str(r.get("source")))
            dst = by_id.get(str(r.get("destination")))
            lines.append(
                f"{element_name(src) if src else r.get('source')} -> "
                f"{element_name(dst) if dst else r.get('destination')}"
            )
    else:
        lines.append("(none)")

    subsection(lines, "Unresolved relationships")
    if unresolved:
        for r in unresolved:
            lines.append(json.dumps(r, sort_keys=True))
    else:
        lines.append("(none)")


def analyze_package_model(data, lines):
    elements, by_id = index_elements(data)
    relationships = data.get("relationships", [])

    internal_ids = {}
    external_ids = {}

    for eid, e in by_id.items():
        path = str(e.get("path", "")).replace("\\", "/")
        name = e.get("name", "")

        if path == "src" and name:
            internal_ids[eid] = name
        elif (
            "_deps" in path
            or "_deps" in str(e.get("display_name", ""))
        ):
            # Prefer the dependency identity from CMake FetchContent's
            # conventional _deps/<name>-src path instead of leaf package
            # names such as "detail" or "backends".
            normalized = path.replace("\\", "/")
            parts = normalized.split("/")

            dependency = None
            if "_deps" in parts:
                index = parts.index("_deps")
                if index + 1 < len(parts):
                    dependency = parts[index + 1]
                    if dependency.endswith("-src"):
                        dependency = dependency[:-4]

            external_ids[eid] = dependency or element_name(e)

    internal_edges = Counter()
    external_edges = Counter()

    for rel in relationships:
        src = str(rel.get("source", ""))
        dst = str(rel.get("destination", ""))

        if src in internal_ids and dst in internal_ids:
            internal_edges[(internal_ids[src], internal_ids[dst])] += 1

        elif src in internal_ids and dst in external_ids:
            external_edges[
                (internal_ids[src], external_ids[dst])
            ] += 1

    section(lines, "PACKAGE MODEL")

    lines.append(f"Recursive elements: {len(elements)}")
    lines.append(f"Relationships: {len(relationships)}")
    lines.append(f"AI3 source packages: {len(internal_ids)}")
    lines.append(f"Detected external _deps packages: {len(external_ids)}")

    subsection(lines, "AI3 source packages")
    for name in sorted(internal_ids.values()):
        lines.append(name)

    subsection(lines, "Internal package dependencies")
    if internal_edges:
        for (src, dst), count in sorted(internal_edges.items()):
            lines.append(f"{src} -> {dst}: {count}")
    else:
        lines.append("(none)")

    subsection(lines, "External package dependencies")
    if external_edges:
        for (src, dst), count in sorted(external_edges.items()):
            lines.append(f"{src} -> {dst}: {count}")
    else:
        lines.append("(none)")


def analyze_include_model(data, lines):
    elements, by_id = index_elements(data)
    relationships = data.get("relationships", [])

    file_by_id = {}
    layer_by_id = {}

    for eid, e in by_id.items():
        if e.get("type") != "file":
            continue

        path = include_element_path(e)
        if not path:
            continue

        file_by_id[eid] = path
        layer_by_id[eid] = layer_from_path(path)

    layer_edges = Counter()
    file_edges = []
    unresolved = []

    incoming = Counter()
    outgoing = Counter()

    for rel in relationships:
        src = str(rel.get("source", ""))
        dst = str(rel.get("destination", ""))

        if src not in file_by_id or dst not in file_by_id:
            unresolved.append(rel)
            continue

        src_file = file_by_id[src]
        dst_file = file_by_id[dst]

        outgoing[src] += 1
        incoming[dst] += 1
        file_edges.append((src_file, dst_file))

        src_layer = layer_by_id[src]
        dst_layer = layer_by_id[dst]

        if src_layer and dst_layer and src_layer != dst_layer:
            layer_edges[(src_layer, dst_layer)] += 1

    section(lines, "INCLUDE MODEL")

    lines.append(f"Recursive elements: {len(elements)}")
    lines.append(f"Source files: {len(file_by_id)}")
    lines.append(f"Include relationships: {len(relationships)}")
    lines.append(
        f"File-to-file relationships resolved: {len(file_edges)}"
    )
    lines.append(
        f"Non-file/unresolved relationship endpoints: {len(unresolved)}"
    )

    subsection(lines, "Derived internal layer dependencies")
    lines.append(
        "Counts below are concrete cross-layer include edges."
    )
    lines.append("")

    if layer_edges:
        for (src, dst), count in sorted(layer_edges.items()):
            lines.append(f"{src:14s} -> {dst:14s} {count:4d}")
    else:
        lines.append("(none)")

    subsection(lines, "Layer dependency matrix")

    layers = sorted(
        {
            layer
            for layer in layer_by_id.values()
            if layer is not None
        }
    )

    if layers:
        width = max(8, max(len(x) for x in layers) + 1)

        lines.append(
            "".ljust(width)
            + "".join(x[:7].rjust(8) for x in layers)
        )

        for src in layers:
            row = [src.ljust(width)]
            for dst in layers:
                value = (
                    0 if src == dst
                    else layer_edges.get((src, dst), 0)
                )
                row.append(f"{value:8d}")
            lines.append("".join(row))
    else:
        lines.append("(none)")

    subsection(lines, "Highest include degree")
    lines.append(
        "Degree = incoming + outgoing file include relationships."
    )
    lines.append("")
    lines.append(f"{'total':>5} {'out':>5} {'in':>5}  file")

    ranked = sorted(
        file_by_id,
        key=lambda eid: (
            incoming[eid] + outgoing[eid],
            file_by_id[eid],
        ),
        reverse=True,
    )

    for eid in ranked[:25]:
        total = incoming[eid] + outgoing[eid]
        lines.append(
            f"{total:5d} {outgoing[eid]:5d} {incoming[eid]:5d}  "
            f"{file_by_id[eid]}"
        )

    subsection(lines, "Concrete cross-layer include edges")
    cross_edges = []

    for src, dst in file_edges:
        src_layer = layer_from_path(src)
        dst_layer = layer_from_path(dst)

        if (
            src_layer
            and dst_layer
            and src_layer != dst_layer
        ):
            cross_edges.append((src_layer, dst_layer, src, dst))

    if cross_edges:
        for src_layer, dst_layer, src, dst in sorted(cross_edges):
            lines.append(
                f"[{src_layer} -> {dst_layer}] {src} -> {dst}"
            )
    else:
        lines.append("(none)")


def main():
    parser = argparse.ArgumentParser(
        description="Analyze AI3 clang-uml architecture models."
    )
    parser.add_argument("type_json")
    parser.add_argument("package_json")
    parser.add_argument("include_json")
    parser.add_argument("output")
    args = parser.parse_args()

    type_data = load_json(args.type_json)
    package_data = load_json(args.package_json)
    include_data = load_json(args.include_json)

    lines = []

    lines.append("AI3 Architecture Analysis")
    lines.append("=========================")
    lines.append("")
    lines.append(
        "This report combines three clang-uml models. The type model "
        "describes generated C++ semantic relationships; it is not a "
        "complete implementation dependency graph. The package and "
        "include models provide complementary dependency evidence."
    )

    analyze_type_model(type_data, lines)
    analyze_package_model(package_data, lines)
    analyze_include_model(include_data, lines)

    lines.append("")
    lines.append("END OF REPORT")
    lines.append("=============")
    lines.append("")

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines), encoding="utf-8")

    print(f"Wrote {output}")


if __name__ == "__main__":
    main()
