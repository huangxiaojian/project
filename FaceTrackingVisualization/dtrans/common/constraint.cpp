#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "constraint.h"


/* Allocate for constraint_list with the size information specified in
   m_list_length */
void VertexConstraintList::CreateConstraintList()
{
    /* allocate memory space for vertex constraint entries */
    m_vertexInfo = (dtVertexInfo*)__dt_malloc(
            (size_t)m_list_length * sizeof(dtVertexInfo));
}


/* Load vertex constraints from file, these constraints are often specifed by
   users with the graphical interactive tool "Corres!" in our package. The 
   entries are sorted to arrange source vertex indexes in ascending order, 
   which would accelerate the construction of vertex info (type-index) list.

   This function returns an 0 on success, or it would return an -1 to indicate
   the occurrence of an fopen() error.
*/
int VertexConstraintList::LoadConstraints(const char *filename)
{
#ifdef _CRT_SECURE_NO_WARNINGS
    FILE *fd = fopen(filename, "r");
#else
	FILE *fd;
	fopen_s(&fd, filename, "r");
#endif
    dt_index_type i_entry = 0;  /* looping variable: entry index */

    if (fd != NULL)
    {
        /* get constraint list size and allocate for it */
        fscanf(fd, "%d", &(m_list_length));
        CreateConstraintList();

        /* read vertex constraint entries */
        for ( ; i_entry < m_list_length; i_entry++)
        {
            fscanf(fd, "%d,%d", 
                &(m_vertexInfo[i_entry].vconsEntry.i_src_vertex),
                &(m_vertexInfo[i_entry].vconsEntry.i_tgt_vertex));
        }

        fclose(fd);

        /* sort all entries in ascending order, which would accelerate building
           the vertex type-index list */
        SortConstraintEntries(this);
        return 0;
    }
    else {
        return -1;    /* fopen() blew up */
    }
}

int VertexConstraintList::LoadConstraintsWithPosition(const char * filename)
{
#ifdef _CRT_SECURE_NO_WARNINGS
    FILE *fd = fopen(filename, "r");
#else
	FILE *fd;
	fopen_s(&fd, filename, "r");
#endif
    dt_index_type i_entry = 0;  /* looping variable: entry index */

    if (fd != NULL)
    {
        /* get constraint list size and allocate for it */
        fscanf(fd, "%d", &(m_list_length));
        CreateConstraintList();

        /* read vertex constraint entries */
        for ( ; i_entry < m_list_length; i_entry++)
        {
			dtVertexInfo* vf = m_vertexInfo + i_entry;
            fscanf(fd, "%d,%d;%lf%lf%lf,%lf%lf%lf", 
                &(vf->vconsEntry.i_src_vertex),
                &(vf->vconsEntry.i_tgt_vertex),
				&(vf->vl.x), &(vf->vl.y), &(vf->vl.z),
				&(vf->vr.x), &(vf->vr.y), &(vf->vr.z));
        }

        fclose(fd);

        /* sort all entries in ascending order, which would accelerate building
           the vertex type-index list */
        SortConstraintEntries(this);
        return 0;
    }
    else {
        return -1;    /* fopen() blew up */
    }
}

/* Release all resources allocated for the constraint list object */
void VertexConstraintList::ReleaseConstraints() 
{
	if(m_vertexInfo)
    {
		free(m_vertexInfo);
		m_vertexInfo = NULL;
	}
}

/* Save constraint list to file

   This function returns 0 on success, or it would return an -1 to indicate an
   fopen() error, go checking system variable errno for furture investivation
*/
int VertexConstraintList::SaveConstraints(const char *filename)
{
    FILE *fd = fopen(filename, "w");
    dt_index_type i_entry = 0;   /* looping variable: entry index */

    if (fd != NULL)
    {
        /* first line: number of constrant entries  */
        fprintf(fd, "%d\n", m_list_length);

        /* each following line contains a pair of unsigned integer, 
           representing constrained vertex indexes in source and target mesh.
        */
        for ( ; i_entry < m_list_length; i_entry++)
        {
            fprintf(fd, "%d, %d\n", 
                m_vertexInfo[i_entry].vconsEntry.i_src_vertex,
                m_vertexInfo[i_entry].vconsEntry.i_tgt_vertex);
        }

        fclose(fd);
        return 0;
    }
    else {
        return -1;   /* fopen() error brought this routine down */
    }
}

int VertexConstraintList::SaveConstraintsWithPosition(const char *filename, const dtMeshModel* modelL, const dtMeshModel* modelR)
{
	FILE *fp = fopen(filename, "w");
	dt_index_type i_entry = 0;
	dtVertex *vl, *vr;
	if(fp != NULL)
	{
		fprintf(fp, "%d\n", m_list_length);
		for( ; i_entry < m_list_length; i_entry++)
		{
			vl = modelL->m_vertex + m_vertexInfo[i_entry].vconsEntry.i_src_vertex;
			vr = modelR->m_vertex + m_vertexInfo[i_entry].vconsEntry.i_tgt_vertex;
			fprintf(fp, "%d, %d; %lf %lf %lf, %lf %lf %lf\n", 
				m_vertexInfo[i_entry].vconsEntry.i_src_vertex, m_vertexInfo[i_entry].vconsEntry.i_tgt_vertex, 
				vl->x, vl->y, vl->z, vr->x, vr->y, vr->z);
		}
		fclose(fp);
		return 0;
	}
	else
	{
		return -1;
	}
}

/* Find corresponding target vertex with specified vertex constraint. */
dtVertex* VertexConstraintList::GetMappedVertex(const dtMeshModel *target_model, dt_index_type i_cons)
{
    dt_index_type i_tgtvertex = m_vertexInfo[i_cons].vconsEntry.i_tgt_vertex;

    __DT_ASSERT(i_cons < m_list_length, 
        "Constraint entry index out of bounds in __dt_GetMappedVertex");
        
    return target_model->m_vertex + i_tgtvertex;
}

/* Given a vertex correspondence constraint of one constrained vertex, find
   the coordinate of the corresponded vertex on the target mesh. */
dt_real_type VertexConstraintList::GetMappedVertexCoord(const dtMeshModel *target_model,dt_index_type i_cons, dt_index_type i_dimension)
{
    const dtVertex *v = GetMappedVertex(target_model, i_cons);

    /* Memory layout hack: structure fields x, y and z must be defined in
       certain order to make this trick work properly. */
    return *(&(v->x) + i_dimension);
}


/* a helper function for qsort to compare any two entries in constraint list,
   indexes of source vertices are key values to determine the partial order 
   relationship. */
int VertexConstraintList::CompareConstraintEntries(const void *_1, const void *_2)
{
    const dtVertexConstraintEntry 
        *entry1 = (dtVertexConstraintEntry*)_1,
        *entry2 = (dtVertexConstraintEntry*)_2;

    return entry1->i_src_vertex - entry2->i_src_vertex;
}

/* Vertex constraint entries are sorted after being read from file. */
void VertexConstraintList::SortConstraintEntries(VertexConstraintList *constraint_list)
/* Sort all constraint entries in ascending order */
{
	qsort(constraint_list->m_vertexInfo, (size_t)constraint_list->m_list_length,
		sizeof(dtVertexInfo), CompareConstraintEntries);
}

