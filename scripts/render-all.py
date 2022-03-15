import sys
import glob
import os
import subprocess

def render(dirname, spp):
    output = dirname + '/renders'
    try:
        os.mkdir(f'{output}')
    except:
        pass

    scene_names = glob.glob(f'{dirname}/scenes/*')
    num_scenes = len(scene_names)


    for i, scene_name in enumerate(scene_names):
        name = os.path.basename(scene_name).split('.')[0]
        msg = f'[{i}/{num_scenes}] {scene_name}'
        print(msg + ' ' * max(0, 78-len(msg)))

        cmd = f'./bin/yscene render {scene_name}/scene.json --output {output}/{name}.png --samples {spp}'
        options = ' --embreebvh --envhidden --envname /Users/nazzaro/Documents/hdr-images/doge2.hdr'
        options += ' --denoise'
        options += ' --exposure 2.0'
        options += ' --resolution 4096'
        cmd += options
        print(cmd)
        retcode = subprocess.run(cmd, shell=True).returncode


if __name__ == '__main__':
    # names = ["115532.ply","47984.ply","529449.ply","91347.ply","257881.ply","65942.ply","93743.ply","762595.ply","971425.ply","82675.ply","1356649.ply","308214.ply","376253.ply","762586.ply","178340.ply"]
    # for name in names:
    #     cmd = f'cp mosaic/meshes/{name} /Users/nazzaro/dev/bezier/mosaic-new/meshes/{name}'
    #     print(cmd)
    #     retcode = subprocess.run(cmd, timeout=600, shell=True).returncode

    input_dir = sys.argv[1]
    spp = 1 if len(sys.argv) <= 2 else int(sys.argv[2])
    render(input_dir, spp)
