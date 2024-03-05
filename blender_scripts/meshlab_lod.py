import pymeshlab
import time
import os

in_folder = ""
out_folder = ""

def decimate_mesh(mesh, target_faces, preserve_border = False, target_border = 1600):
    if preserve_border:
        mesh.compute_selection_from_mesh_border()
        mesh.meshing_decimation_quadric_edge_collapse(targetfacenum=target_border, preserveboundary=True, selected=True)
    
    mesh.set_selection_all()
    mesh.meshing_decimation_quadric_edge_collapse(targetfacenum=target_faces, preserveboundary=True, selected=True)



def make_lods():

    mesh = pymeshlab.MeshSet()

    for filename in os.listdir(in_folder):
        filepath = os.path.join(in_folder, filename)

        # remove .stl from filename
        tile = filename[:-4]
        tilepath = os.path.join(out_folder, tile)

        if not os.path.exists(tilepath):
            os.mkdir(tilepath)

        mesh.load_new_mesh(filepath)

        start_time = time.time()

        # LOD 0
        decimate_mesh(mesh, 50000)
        mesh.save_current_mesh(os.path.join(tilepath, tile + "_LOD0.ply"))

        # LOD 1
        decimate_mesh(mesh, 16000, preserve_border=True, target_border=1600)
        mesh.save_current_mesh(os.path.join(tilepath, tile + "_LOD1.ply"))

        # LOD 2
        decimate_mesh(mesh, 5000, preserve_border=True, target_border=400)
        mesh.save_current_mesh(os.path.join(tilepath, tile + "_LOD2.ply"))

        elapsed_time = (time.time() - start_time) / 60
        print("Lodding done for " + tile + " in " + str(elapsed_time) + " minutes")

def reduce_only():
    count = 0

    for filename in os.listdir(in_folder):
        filepath = os.path.join(in_folder, filename)
        mesh = pymeshlab.MeshSet()

        # remove .stl from filename
        tile = filename[:-4]
        tilepath = os.path.join(out_folder, tile)

        mesh.load_new_mesh(filepath)

        start_time = time.time()

        decimate_mesh(mesh, 5000, preserve_border=True, target_border=400)
        mesh.save_current_mesh(tilepath + ".ply")

        elapsed_time = (time.time() - start_time) / 60
        count += 1
        print(f"{count}/152 - Lodding done for " + tile + " in " + str(elapsed_time) + " minutes")


if __name__ == "__main__":
    start_time = time.time()
    #make_lods()
    reduce_only()
    elapsed_time = (time.time() - start_time) / 60
    print("Total execution time: " + str(elapsed_time) + " minutes")
