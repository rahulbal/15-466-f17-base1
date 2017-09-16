#!/usr/bin/python2.7
from struct import *

def write_records(records, format, f):
    '''
    Write a sequence of tuples to a binary file of structures.
    '''
    record_struct = Struct(format)
    f.write(pack('@iii', 400, 300, len(records)))
    for r in records:
        f.write(record_struct.pack(*r))

x = 400.0
y = 300.0

sprites = [ (1, 16, 56, 0, 0, 32, 64), #player down
            (2, 80, 56, 0, 64, 96, 64), #player right
            (3, 80, 120, 64, 64, 96, 128), #player left
            (4, 16, 120, 0, 64, 32, 128), #player up
            (5, 212, 4, 208, 0, 216, 8), #wire vert
            (6, 204, 12, 200, 8, 208, 16), #wire hori
            (7, 220, 12, 216, 8, 224, 16), #wire up-right
            (8, 212, 12, 208, 8, 216, 16), #wire up-left
            (9, 220, 44, 216, 40, 224, 48), #wire down-right
            (10, 212, 44, 208, 40,216, 48),  #wire down-left
            (11, 36, 164, 32, 160, 40, 168), #floor
            (12, 28, 172, 24, 168, 32, 176), #wall
            (13, 136, 224, 96, 184, 184, 256), #sweeper
          ]

print sprites

with open('./dist/textures.blob', 'wb') as f:
    write_records(sprites, '@iffffff', f)
