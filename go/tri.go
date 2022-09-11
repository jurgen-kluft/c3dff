package g3dff

import ()

type triFormat int

const (
	_ triFormat = iota
)

/*
.tri format, a binary format suitable for 'direct loading / streaming'

Header:
    [4]  = '.tri'          | identifier
    [4]  = 0x00010000      | version
    [4]  = 0x00000000      | active array's: vertices(0x1), normals(0x2), colors(0x4), triangles(0x00010000)
    [24] = float[6]        | { float min[3], float max[3] }
    [4]  = 3254150         | number of vertices
    [4]  = 8254150         | number of triangles
    [4]  = 254150          | number of normals (0 if not present)
    [4]  = 54150           | number of colors  (0 if not present)

Data: array's in little endian format
    vertices:   [] { float v[3]; }
    triangles:	[] { u32 v[3]; }             // depending on the features that are active
                [] { u32 v[3],c[3]; }
                [] { u32 v[3],n; }
                [] { u32 v[3],c[3],n; }
    colors: 	[] { float rgb[3]; }
    normals: 	[] { float n[3]; }
*/
