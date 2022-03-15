import sys
import glob
import os
import subprocess

selected_ids = {
             47984: "donna",
             # 82675: "capoccione",
             91347: "rinoceronte",
             93743: "facciaappesa",
             115532: "macchina",
             # 178340: "uccelletto",
             257881: "cloth",
             308214: "pumpkin",
             376253: "nike",
             529449: "gufi",
             762586: "scarpone",
             762595: "braccio",
             971425: "cervo",
             # 1356649, # tentacolo
             65942: "buchi"}

def main(dirname):
    json_names = glob.glob(f'{dirname}/jsons/*')
    append = 0    
    line_thickness = 0.004
    
    for json_name in json_names:

        mesh_id = os.path.basename(json_name).split('.')[0]
        # if mesh_id != 'nefertiti': continue
        # if int(mesh_id) not in selected_ids.keys(): continue    
        # if int(mesh_id) == 376253: line_thickness /= 2

        mesh_name = f'{dirname}/meshes/{mesh_id}.ply'
        scene = f'--scene {dirname}/scenes/{mesh_id}/scene.json'
        # scene = ''

        cmd = f'./bin/splinetime {mesh_name} --test {json_name} --timings {dirname}/timings.csv {scene}'
        if append: cmd += ' --append-timings'
        print('cmd:', cmd)
        retcode = subprocess.run(cmd, timeout=600, shell=True).returncode
        append = 1

if __name__ == '__main__':
    main(sys.argv[1])