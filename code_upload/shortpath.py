from ctypes import *

# load the shared object file
shortpath = CDLL('./new_path.dll')
short_path = shortpath.shortestpath
print(str(short_path()))






