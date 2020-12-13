if __name__ == "__main__" and __package__ is None:
    import os
    import sys
    sys.path.append(os.path.dirname(__file__))

import bmesh
import bpy
from collections import deque
import mathutils
import mesh
import os
import pprint
import re


VERSION = (0, 1, 0)


# These must be kept in sync with `Link` types in `navigation.c`
EDGE = 0
NEIGHBOR = 1
PORTAL = 2


def export_area(cooked_dir):
    data = dict()
    search_children(bpy.context.scene.collection, look_for_areas, data)
    structure_data(data)
    export_data(cooked_dir, data)


def search_children(collection, interpret, data, tags=None):
    for child in collection.children:
        tags_ = parse(child.name, tags)
        interpret(child, tags_, data)


def search_objects(collection, interpret, data, tags=None):
    for child in collection.objects:
        tags_ = parse(child.name, tags)
        interpret(child, tags_, data)


def parse(name, tags=None):
    tokens = tokenize(name)

    if not tags:
        tags = dict()

    while tokens:
        token = tokens.popleft()

        if token == "@area":
            tags["area"] = tokens.popleft()

        elif token == "@hidden":
            tags["hidden"] = ()

        elif token == "@ignore":
            tags["ignore"] = ()

        elif token == "@navmesh":
            tags["navmesh"] = ()

        elif token == "@portal":
            tags["portal"] = ()

        elif token == "@scenery":
            tags["scenery"] = ()

    return tags


def look_for_areas(child, tags, data):
    if "ignore" in tags:
        return

    if "area" in tags:
        name = tags["area"]
        area = data.setdefault("areas", dict()).setdefault(name, {"name": name})
        search_children(child, interpret_area_children, area)
        search_objects(child, interpret_area_objects, area)


def interpret_area_children(child, tags, area):
    if "ignore" in tags:
        return

    if "scenery" in tags:
        tags["scenery"] = ()

    search_children(child, interpret_area_children, area, tags=tags.copy())
    search_objects(child, interpret_area_objects, area, tags=tags.copy())
        

def interpret_area_objects(child, tags, area):
    transform = child.matrix_world
    if "transform" in tags:
        transform = tags["transform"] @ transform

    if child.instance_collection:
        inherited_tags = tags.copy()
        inherited_tags["transform"] = transform
        search_children(child.instance_collection, interpret_area_children, area, tags=inherited_tags.copy())
        search_objects(child.instance_collection, interpret_area_objects, area, tags=inherited_tags.copy())
        return

    if "ignore" in tags:
        return

    if "navmesh" in tags:
        if "navmesh" in area:
            raise Exception("Multiple navmeshes supplied for `@area {}`".format(area["name"]))
        else:
            area["navmesh"] = child

    if "portal" in tags:
        area.setdefault("network", list()).append({
            "name": child.name,
            "transform": transform,
            "width": round(child.matrix_world.to_scale().x)
        })

    if "scenery" in tags:
        if "hidden" in tags:
            return

        if child.type == "MESH":
            filename = os.path.splitext(os.path.basename(bpy.context.blend_data.filepath))[0]
            if (child.library):
                filename = os.path.splitext(os.path.basename(child.library.filepath))[0]

            mesh_name = filename + "_" + child.data.name

            area.setdefault("scenery", list()).append({
                "name": child.name,
                "transform": transform,
                "mesh": mesh_name,
            })
            area.setdefault("meshes", dict())[mesh_name] = child.data

        elif child.type == "LIGHT":
            print("LIGHT", "TAGS", tags)
            if child.data.type == "POINT":
                area.setdefault("lights", list()).append({
                    "name": child.name,
                    "transform": transform,
                    "color": child.data.color,
                    "energy": child.data.energy,
                })


def tokenize(string):
    # print("REGEX", re.sub(r"(@\w*)(?:\.[0-9]{3})", r"\1", string))
    return deque(re.sub(r"(@\w*)(?:\.[0-9]{3})", r"\1", string).split())


def structure_data(data):
    data["meshes"] = dict()
    
    for area in data.get("areas", dict()).values():
        print("Structuring {} ...".format(area["name"]), end="")
        area["navmesh"] = structure_navmesh(area["navmesh"])
        link_navmesh_to_network(area["navmesh"], area["network"])
        data["meshes"].update(area["meshes"])
        print("Done!")


def structure_navmesh(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)

    if len(bm.faces) > 64:
        raise Exception("{} has more than 64 triangles".format(obj.name))
    
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bmesh.ops.transform(bm, matrix=obj.matrix_world, verts=bm.verts)

    vertices = [x.co.copy() for x in bm.verts]
    triangles = list()

    for face in bm.faces:
        triangle = list()
        
        for loop in face.loops:
            edge = dict({"vertex_i": loop.vert.index})

            link_faces = loop.edge.link_faces

            if len(link_faces) == 1:
                edge["to"] = EDGE
                edge["target"] = 0
            elif len(link_faces) == 2:
                edge["to"] = NEIGHBOR
                for i, l in enumerate(link_faces):
                    if l.index != face.index:
                        edge["target"] = l.index
            else:
                raise Exception("Non-manifold geometry")

            triangle.append(edge)

        triangles.append(triangle)

    bm.free()

    return {"vertices": vertices, "triangles": triangles}


def link_navmesh_to_network(navmesh, network):
    def in_bounds(p, b):
        return abs(p.x) <= b.x and abs(p.y) <= b.y and abs(p.z) <= b.z
    
    for portal_i, portal in enumerate(network):
        transform = portal["transform"]
        half_width = portal["width"] / 2.0
        bounds = transform.to_scale()
        inverted_transform = transform.inverted()

        for triangle_i, triangle in enumerate(navmesh["triangles"]):
            for edge_i, edge in enumerate(triangle):
                a_i = triangle[edge_i]["vertex_i"]
                b_i = triangle[(edge_i + 1) % 3]["vertex_i"]
                a = inverted_transform @ navmesh["vertices"][a_i]
                b = inverted_transform @ navmesh["vertices"][b_i]

                if in_bounds(a, bounds) and in_bounds(b, bounds):
                    if "triangle" in portal:
                        raise Exception("`{}` contains too many edges".format(portal["name"]))
                    
                    portal["triangle"] = triangle_i

                    navmesh["vertices"][a_i] = transform @ mathutils.Vector((-half_width, 0, 0))
                    navmesh["vertices"][b_i] = transform @ mathutils.Vector((half_width, 0, 0))

                    edge["to"] = PORTAL
                    edge["target"] = portal_i

        if "triangle" not in portal:
            raise Exception("`{}` contains no edges".format(portal["name"]))


def export_data(cooked_dir, data):
    INFO = "# io_kowl v{}.{}.{}\n".format(*VERSION)

    dirname = os.path.join(cooked_dir, "areas")
    os.makedirs(dirname, exist_ok=True)

    for area in data.get("areas", dict()).values():
        name = area["name"]

        print("Exporting {} ... ".format(name), end="")

        # TODO Save area info

        with open(os.path.join(dirname, name + ".light_grid"), "w", encoding="utf8", newline="\n") as f:
            fw = f.write

            fw(INFO)
            fw("# LIGHTS\n")
            fw("\n")

            fw("# [energy] [color] [position]\n")
            for light in area.get("lights", list()):
                position, _, _ = light["transform"].decompose()
                fw("{} ".format(light["energy"]))
                
                fw("{},{},{} ".format(*light["color"]))
                fw("{},{},{}\n".format(*position.to_tuple()))

        with open(os.path.join(dirname, name + ".navmesh"), "w", encoding="utf8", newline="\n") as f:
            fw = f.write

            fw(INFO)
            fw("# NAVMESH\n")
            fw("\n")

            fw("# [to] [target] [points]\n")
            navmesh = area["navmesh"]
            for triangle in navmesh["triangles"]:
                tos = [x["to"] for x in triangle]
                targets = [x["target"] for x in triangle]
                vertex_is = [x["vertex_i"] for x in triangle]
                vertices = [navmesh["vertices"][i] for i in vertex_is]
                fw("{},{},{} ".format(*tos))
                fw("{},{},{} ".format(*targets))
                fw("{} {} {}\n".format(*["{:.3f},{:.3f},{:.3f}".format(*x.to_tuple()) for x in vertices]))

        with open(os.path.join(dirname, name + ".network"), "w", encoding="utf8", newline="\n") as f:
            fw = f.write

            fw(INFO)
            fw("# NETWORK\n")
            fw("\n")
            
            fw("# [linked_triangle] [width] [position] [rotation]\n")
            for portal in area["network"]:
                p, r, s = portal["transform"].decompose()
                fw("{} ".format(portal["triangle"]))
                fw("{} ".format(int(round(s.x))))
                fw("{},{},{} ".format(*p.to_tuple()))
                fw("{},{},{},{}\n".format(r.x, r.y, r.z, r.w))

        with open(os.path.join(dirname, name + ".scenery"), "w", encoding="utf8", newline="\n") as f:
            fw = f.write

            fw(INFO)
            fw("# SCENERY\n")
            fw("\n")

            fw("# [mesh name] [position] [rotation] [scale]\n")
            for scenery in area["scenery"]:
                p, r, s = scenery["transform"].decompose()
                fw("{} ".format(scenery["mesh"]))
                fw("{},{},{} ".format(*p.to_tuple()))
                fw("{},{},{},{} ".format(r.x, r.y, r.z, r.w))
                fw("{},{},{}\n".format(*s.to_tuple()))

        print("Done!")

    dirname = os.path.join(cooked_dir, "meshes")
    os.makedirs(dirname, exist_ok=True)

    for name, mesh_ in data["meshes"].items():
        print("Exporting {} ... ".format(name), end="")
        mesh.export_mesh(mesh_, os.path.join(dirname, name + ".mesh"))
        print("Done!")


if __name__ == "__main__":
    import sys
    export_area(sys.argv[-1])
