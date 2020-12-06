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
                area.setdefault("portals", list()).append(child)

            elif token == "@export":
                area.setdefault("exports", list()).append([child])

            # elif token == "@var":
                # area.setdefault("vars", dict()).setdefault(tokens.popleft(), list()).append([child])

            else:
                pass

            
    for tokens, child in tokenize(collection.children):
        while tokens:
            token = tokens.popleft()

            if token == "@ignore":
                break

            elif token == "@export":
                area.setdefault("exports", list()).append(list(child.objects))

            # elif token == "@var":
                # area.setdefault("vars", dict()).setdefault(tokens.popleft(), list()).append(list(child.objects))

            else:
                pass

        else:
            parse_area(child, area)


def tokenize(children):
    for child in children:
        yield deque(child.name.rsplit('.', 1)[0].split()), child


def structure_data(data):
    data["meshes"] = dict()
    
    for area in data["areas"]:
        area["exports"] = structure_exports(area.get("exports", list()), data["meshes"])
        area["navmesh"] = structure_navmesh(area["navmesh"])
        area["portals"] = structure_portals(area["portals"])
        link_navmesh_to_portals(area["navmesh"], area["portals"])

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


def structure_portals(objs):
    portals = list()

    for obj in objs:
        portals.append({
            "name": obj.name,
            "transform": obj.matrix_world,
            "width": round(obj.matrix_world.to_scale().x)
        })

    return portals


def structure_exports(objs_list, meshes):
    exports = list()

    for objs in objs_list:
        for obj in objs:
            if obj.type != "MESH":
                continue

            exports.append({
                "name": obj.name,
                "transform": obj.matrix_world,
                "mesh": obj.data.name,
            })
            meshes[obj.data.name] = obj.data

    return exports


def structure_meshes(meshes):
    return meshes
    


def link_navmesh_to_portals(navmesh, portals):
    def in_bounds(p, b):
        return abs(p.x) <= b.x and abs(p.y) <= b.y and abs(p.z) <= b.z
    
    for portal_i, portal in enumerate(portals):
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

        with open(os.path.join(dirname, name + ".xpt"), "w", encoding="utf8", newline="\n") as f:
            fw = f.write

            fw(INFO)
            fw("# EXPORTS\n")
            fw("\n")

            fw("# [mesh name] [position] [rotation] [scale]\n")
            for export in area["exports"]:
                p, r, s = export["transform"].decompose()
                fw("{} ".format(export["mesh"]))
                fw("{},{},{} ".format(*p.to_tuple()))
                fw("{},{},{},{} ".format(r.x, r.y, r.z, r.w))
                fw("{},{},{}\n".format(*s.to_tuple()))

        with open(os.path.join(dirname, name + ".nav"), "w", encoding="utf8", newline="\n") as f:
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

        with open(os.path.join(dirname, name + ".ptl"), "w", encoding="utf8", newline="\n") as f:
            fw = f.write

            fw(INFO)
            fw("# PORTALS\n")
            fw("\n")
            
            fw("# [linked_triangle] [width] [position] [rotation]\n")
            for portal in area["portals"]:
                p, r, s = portal["transform"].decompose()
                fw("{} ".format(portal["triangle"]))
                fw("{} ".format(int(round(s.x))))
                fw("{},{},{} ".format(*p.to_tuple()))
                fw("{},{},{},{}\n".format(r.x, r.y, r.z, r.w))

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
