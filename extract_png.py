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
            (2, 80, 56, 64, 0, 96, 64), #player right
            (3, 80, 120, 64, 64, 96, 128), #player left
            (4, 16, 120, 0, 64, 32, 128), #player up
            (5, 208, 0, 208, 0, 216, 8), #wire vert
            (6, 200, 8, 200, 8, 208, 16), #wire hori
            (7, 192, 8, 192, 8, 200, 16), #wire up-right
            (8, 224, 8, 224, 8, 232, 16), #wire up-left
            (9, 192, 40, 192, 40, 200, 48), #wire down-right
            (10, 224, 40, 224, 40,232, 48),  #wire down-left
            (11, 32, 160, 32, 160, 40, 168), #floor
            (12, 24, 168, 24, 168, 32, 176), #wall
            (13, 132, 220, 96, 184, 184, 256), #sweeper
            (14, 208, 8, 208, 8, 216, 16), #wire right_up
            (15, 216, 8, 216, 8, 224, 16), #wire left_up
            (16, 192, 32, 192, 32, 200, 40), #wire right_down
            (17, 224, 32, 224, 32, 232, 40), #wire left_down
            (18, 192, 56, 192, 56, 200, 64), #wall dark
	    (19, 224, 80, 192, 72, 232, 96), #step_ctr
            (20, 112, 272, 0, 256, 224, 288), #text box
            (21, 0, 288, 0, 288, 208, 296), #alphabets
            (22, 0, 248, 0, 248, 80, 256) #numbers
          ]

with open('./dist/textures.blob', 'wb') as f:
    write_records(sprites, '@iffffff', f)
