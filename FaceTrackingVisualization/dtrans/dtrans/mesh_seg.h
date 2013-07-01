#ifndef __DT_MESH_SEGMENTATION_HEADER__
#define __DT_MESH_SEGMENTATION_HEADER__


#include "mesh_model.h"


/* Part of a segmented triangle mesh, it contains indexes of triangle units
   belonging to this component of segmentation.
*/
struct dtMeshSegComponent
{
    dt_index_type *m_i_segtriangle;   /* a list of triangle indexes belonging to 
                                       this component. */
    dt_size_type   m_n_segtriangle;   /* number of triangle units in this 
                                       component */
	/* Load specified component of some segmentation from file. This function
	   returns 0 on success, -1 indicates an fopen failure. */
	int dtLoadMeshSegComponent(const char *filename);

	/* Destroy the segmentation component object and free its memory */
	void dtDestroyMeshSegComponent();
};


/* Segmentation of a mesh model, triangle units in the mesh are partitioned 
   into several components represented by a list of __dt_MeshSegComponent 
   objects. 
   
   Attention: components are allowed to intersect with each other.
*/
typedef struct __dt_MeshSegmentation_struct
{
    dtMeshSegComponent *seglist;   /* a list of segmented components */
    dt_size_type    n_segcomponent;   /* number of components spawned by 
                                         this segmentation */
}dtMeshSegmentation;


#endif /* __DT_MESH_SEGMENTATION_HEADER__ */

