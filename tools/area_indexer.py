if __name__ == "__main__":
    import os
    import sys
    filename = sys.argv[1]
    dirname = os.path.dirname(filename)
    areas = set()
    for f in os.listdir(os.path.join(dirname, "areas")):
        f, ext = os.path.splitext(f)
        if ext in {".navmesh", ".network", ".scenery"}:
            areas.add(f)

    # TODO Convert scripts to Python3
    # with open(filename, mode="w", encoding="utf8", newline="\n") as f:
    with open(filename, "w") as f:
        for area in areas:
            f.write(os.path.join("assets", "areas", area))
            f.write("\n")
