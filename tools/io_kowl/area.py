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


VERSION = (0, 1, 0)


# These must be kept in sync with `Link` types in `navigation.c`
EDGE = 0
NEIGHBOR = 1
PORTAL = 2


def export_area(cooked_dir):
    data = dict()
    parse_areas(bpy.context.scene.collection, data.setdefault("areas", list()))
    structure_data(data)
    export_data(cooked_dir, data)


def parse_areas(collection, areas):
    for tokens, child in tokenize(collection.children):
        while tokens:
            token = tokens.popleft()

            if token == "@ignore":
                break

            elif token == "@area":
                area = {"name": tokens.popleft()}
                parse_area(child, area)
                areas.append(area)

            else:
                pass

        else:
            parse_areas(child, areas)


def parse_area(collection, area):
    for tokens, child in tokenize(collection.objects):
        while tokens:
            token = tokens.popleft()

            if token == "@ignore":
                break

            elif token == "@navmesh":
                if "navmesh" in area:
                    raise Exception("Multiple navmeshes supplied for `{}`".format(area["name"]))
                area["navmesh"] = child

            elif token == "@portal":
                area.setdefault("network", list()).append(child)

            elif token == "@scenery":
                area.setdefault("scenery", list()).append(child)

            # elif token == "@var":
                # area.setdefault("vars", dict()).setdefault(tokens.popleft(), list()).append([child])

            else:
                pass

            
    for tokens, child in tokenize(collection.children):
        while tokens:
            token = tokens.popleft()

            if token == "@ignore":
                break

            elif token == "@scenery":
                area.setdefault("scenery", list()).extend(child.objects)

            # elif token == "@var":
                # area.setdefault("vars", dict()).setdefault(tokens.popleft(), list()).append(list(child.objects))

            else:
                pass

        else:
            parse_area(child, area)


def parse_export():
    pass


def tokenize(children):
    for child in children:
        yield deque(child.name.rsplit('.', 1)[0].split()), child


def structure_data(data):
    data["meshes"] = dict()
    
    for area in data["areas"]:
        area["navmesh"] = structure_navmesh(area["navmesh"])
        area["network"] = structure_network(area["network"])
        area["lights"] = list()
        area["scenery"] = structure_scenery(area.get("scenery", list()), data["meshes"], area["lights"])
        link_navmesh_to_network(area["navmesh"], area["network"])

    structure_meshes(data["meshes"])


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


def structure_network(objs):
    network = list()

    for obj in objs:
        network.append({
            "name": obj.name,
            "transform": obj.matrix_world,
            "width": round(obj.matrix_world.to_scale().x)
        })

    return network


def structure_scenery(objs, meshes, lights):
    scenery = list()

    def recurse(transform, children):
        for child in children:
            if child.type == "EMPTY":
                if child.instance_collection:
                    recurse(transform @ child.matrix_world, child.instance_collection.objects)

            elif child.type == "MESH":
                scenery.append({
                    "name": child.name,
                    "transform": transform @ child.matrix_world,
                    "mesh": child.data.name,
                })
                meshes[child.data.name] = child.data

            elif child.type == "LIGHT":
                if child.data.type == "POINT":
                    lights.append({
                        "transform": transform @ child.matrix_world,
                        "color": child.data.color,
                        "energy": child.data.energy,
                    })
                    
            recurse(transform, child.children)

    recurse(mathutils.Matrix.Identity(4), objs)

    return scenery


def structure_meshes(meshes):
    return meshes
    


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


def export_data(cooked_dir, data):
    INFO = "# io_kowl v{}.{}.{}\n".format(*VERSION)

    dirname = os.path.join(cooked_dir, "areas")
    os.makedirs(dirname, exist_ok=True)

    for area in data["areas"]:
        name = area["name"]

        print("Exporting {} ... ".format(name), end="")

        # TODO Save area info

        with open(os.path.join(dirname, name + ".lighting"), "w", encoding="utf8", newline="\n") as f:
            fw = f.write

            fw(INFO)
            fw("# LIGHTS\n")
            fw("\n")

            fw("# [energy] [color] [position]\n")
            for light in area["lights"]:
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

        print("DONE")

    dirname = os.path.join(cooked_dir, "meshes")
    os.makedirs(dirname, exist_ok=True)

    for name, mesh_ in data["meshes"].items():
        print("Exporting {} ... ".format(name), end="")
        mesh.export_mesh(mesh_, os.path.join(dirname, name + ".mesh"))
        print("DONE")


if __name__ == "__main__":
    import sys
    export_area(sys.argv[-1])
