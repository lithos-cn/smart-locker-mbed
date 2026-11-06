"""Run native C++ logic/driver tests and a mock-interface firmware syntax check."""
from pathlib import Path
import argparse, os, shutil, subprocess

p=argparse.ArgumentParser()
p.add_argument('--compiler',help='Path to a GCC/Clang-compatible C++ compiler')
p.add_argument('--zig',help='Path to Zig; uses its c++ command')
args=p.parse_args()
root=Path(__file__).resolve().parents[1]
build=root/'build'/'host-tests'
build.mkdir(parents=True,exist_ok=True)
compiler=[args.zig,'c++'] if args.zig else [args.compiler or shutil.which('c++') or shutil.which('g++') or 'clang++']
env=os.environ.copy()
env['ZIG_GLOBAL_CACHE_DIR']=str(build/'zig-global-cache')
env['ZIG_LOCAL_CACHE_DIR']=str(build/'zig-local-cache')
common=compiler+['-std=c++14','-Wall','-Wextra','-Werror','-pedantic','-I'+str(root/'include')]

def run(command):
    subprocess.run(command,cwd=root,env=env,check=True)

for name, sources, extra in [
    ('test_control',['tests/test_control.cpp'],[]),
    ('test_drivers',['tests/test_drivers.cpp','src/adxl345.cpp','src/tmp102.cpp'],['-I'+str(root/'tests/fakes')])
]:
    executable=build/(name+('.exe' if os.name=='nt' else ''))
    run(common+extra+sources+['-o',str(executable)])
    run([str(executable)])
run(common+['-I'+str(root/'tests/fakes'),'-c','src/main.cpp','-o',str(build/'main.o')])
print('Main program mock-interface compile check passed. No ARM firmware or hardware test is implied.')
