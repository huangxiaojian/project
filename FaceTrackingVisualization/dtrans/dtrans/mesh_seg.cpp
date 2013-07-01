#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include "mesh_seg.h"


/* Load specified component of some segmentation from file. This function
   returns 0 on success, -1 indicates an fopen failure. */
int dtMeshSegComponent::dtLoadMeshSegComponent(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    dt_index_type i_entry = 0;    /* loop var */

    if (fp != NULL)
    {
        /* read number of triangle units in this seg component from the 
           first line of file. */
        fscanf(fp, "%d", &(m_n_segtriangle));

        /* allocate for triangle index list */
        m_i_segtriangle = (dt_index_type*)__dt_malloc(
            (size_t)m_n_segtriangle * sizeof(dtMeshSegComponent));

        /* read indexes of triangle units in this seg component */
        for ( ; i_entry < m_n_segtriangle; i_entry++) {
            fscanf(fp, "%d", &(m_i_segtriangle[i_entry]));
        }

        return 0;    /* successfully done */
    }
    else {
        return -1;   /* fopen() failed */
    }
}

/* Destroy the segmentation component object and free its memory */
void dtMeshSegComponent::dtDestroyMeshSegComponent() 
{
	if(m_i_segtriangle)
    {
		free(m_i_segtriangle);
		m_i_segtriangle = NULL;
	}
}

