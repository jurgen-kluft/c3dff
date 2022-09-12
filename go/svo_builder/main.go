package main

import (
	. "github.com/jurgen-kluft/c3dff/go/g3dff"
)

/*

Q: How do we reduce the memory consumption of SVO, for a large open world the footprint is
   very large.
   In a large open-world there are a couple of general strategies used to reduce memory for
   just the triangle meshes and LODs:
   - procedural/masked placement of trees/vegetation
   - procedural placement of crates/buildings/barrels/structures

# Out-Of-Core (using SSD storage) SVO Builder:

	- Needs a .tri file as input
	- Partitioning
	- Voxelizing
	- SVO Building

There is gl math for go:
- https://github.com/bloeys/gglm
- https://github.com/go-gl/mathgl
- https://github.com/ungerik/go3d

We can use go-routines to partition in parallel and generate/update voxel data.

# Partitioning

## Voxel Octree

In a way still sparse since regions or chunks that are never intersected will NOT exist on disk.

- World  =  64 x  64 x   8 regions (32_km x 32_km x 4_km km, divided into 32768 regions)
- Region =  32 x  32 x  32 chunks  (512_m x 512_m x 512_m, divided into 32768 chunks)
- Chunk  = 256 x 256 x 256 voxels  (16_m x 16_m x 16_m, 16 million voxels)

Based on the world-space bounding box of the model data and the intersection with the world we can
determine the intersecting 'octree cells'. Together with the memory constraints we can determine how
many 'partitions' we need to construct so that they can be consumed by the voxelization.

So the voxelization can be quite heavy for storage space (SSD/HDD).

# SVO building

It seems that modifying a 'Chunk' should be done on the SVO, otherwise the size of disk will be huge.

So the process that acts on a Chunk could do the following:
- Load SVO and build 'Chunk Voxel Octree'
- Load .Tri partition file that needs to be merged into this Chunk
- Update 'Chunk Voxel Octree'
- From 'Chunk Voxel Octree' build SVO and save to disk

*/
type voxelChunk struct {
	width  int
	height int
	length int
	bounds Box
	voxels []byte
}

type voxelRegion struct {
	width  int
	height int
	length int
	bounds Box
	chunks []*voxelChunk
}

type voxelWorld struct {
	width   int
	height  int
	length  int
	bounds  Box
	regions []*voxelRegion
}

func partitioning(world voxelWorld, model_position Vector) {

}
