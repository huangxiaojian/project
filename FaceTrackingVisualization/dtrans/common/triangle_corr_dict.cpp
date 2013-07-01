#include "stdafx.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>

#include "triangle_corr_dict.h"

const dtMeshModel* TriangleCorrsDict::__lambda_model;
dtVector TriangleCorrsDict::__lambda_norm; 

/* Create an empty triangle correspondence dictionary, it only allocates space
   for the lookup table (argv[]) and intialize it as an empty one. */
void TriangleCorrsDict::CreateEmptyTriangleCorrsDict(const dtMeshModel *target_model)
{
    dt_index_type i_entry = 0;

    /* allocate for the lookup table */
    m_corrsv_size = target_model->m_n_triangle;
    m_corrsv = 
        (dtTriangleCorrsDictEntv*)__dt_malloc(
            (size_t)m_corrsv_size * 
            sizeof(dtTriangleCorrsDictEntv));

    /* make the lookup table empty */
    for ( ; i_entry < m_corrsv_size; i_entry++)
    {
        m_corrsv[i_entry].n_corrstriangle = 0;
        m_corrsv[i_entry].corrs  =  NULL;
    }

    /* make tclist empty */
    m_tclist.MakeEmpty();
}


/* Construct a __dt_TriangleCorrsDict object from a __dt_TriangleCorrsList, 
   tclist is then migrated into tcdict and cannot be destroyed explicitly. */
void TriangleCorrsDict::CreateTriangleCorrsDict(const dtMeshModel *target_model, TriangleCorrsList *tclist)
{
    dt_index_type i_corrs = 0;   /* index of correspondence entry in tclist */
    dt_index_type i_tgt_triangle, i_cur = -1;

    /* create the corrs dictionary and allocate for the lookup table */
    CreateEmptyTriangleCorrsDict(target_model);

    /* migrate tclist to tcdict */
    tclist->SortUniqueTriangleCorrsList();
    m_tclist = *tclist;

    /* perform a linear scan to build the lookup table of tcdict */
    for ( ; i_corrs < tclist->GetListLength(); i_corrs++)
    {
        i_tgt_triangle = tclist->GetTriangleCorrEntry()[i_corrs].i_tgt_triangle;

        /* if we encoutered an entry for the next target triangle unit */
        if (i_cur != i_tgt_triangle)
        {
            /* log the new corrs entry */
            i_cur = i_tgt_triangle;
            m_corrsv[i_cur].corrs = &(tclist->GetTriangleCorrEntry()[i_corrs]);
        }

        m_corrsv[i_cur].n_corrstriangle += 1;
    }
}


/* Destroy specified dictionary object and free its memory */
void TriangleCorrsDict::DestroyTriangleCorrsDict()
{
    m_tclist.DestroyTriangleCorrsList();
    free(m_corrsv);
}



/* Functions to access entries in the correspondence dictionary, the 
   implementations are quite straight forward. */

dtTriangleCorrsDictEntv* TriangleCorrsDict::GetTriangleCorrsEntryVector(dt_index_type i_tgt_triangle)
{
    __DT_ASSERT(
        i_tgt_triangle < m_corrsv_size,
            "Target triangle index out of bounds in "
            "__dt_GetTriangleCorrsEntryVector");

    return m_corrsv + i_tgt_triangle;
}

dtTriangleCorrsEntry* TriangleCorrsDict::GetTriangleCorrsEntry(dt_index_type i_tgt_triangle,
    dt_index_type i_corrs_entry)
{
    const dtTriangleCorrsDictEntv *v_entry = 
        GetTriangleCorrsEntryVector(i_tgt_triangle);

    __DT_ASSERT(
        i_corrs_entry < v_entry->n_corrstriangle,
        "Corrs entry index out of bounds in __dt_GetTriangleCorrsEntry");

    return v_entry->corrs + i_corrs_entry;
}

dt_size_type TriangleCorrsDict::GetTriangleCorrsNumber(dt_index_type i_tgt_triangle)
{
    const dtTriangleCorrsDictEntv *v_entry = 
        GetTriangleCorrsEntryVector(i_tgt_triangle);

    return v_entry->n_corrstriangle;
}


/* calculate the coordinate of the centroid of specified triangle */
void TriangleCorrsDict::CalculateTriangleCentroid(
    const dtMeshModel *model, dt_index_type i_triangle, dt_real_type *centroid)
{
    /* get the coordinates of vertices in the triangle */
    dtTriangle *triangle = model->m_triangle + i_triangle;
    dtVertex 
        *v0 = model->m_vertex + triangle->i_vertex[0],
        *v1 = model->m_vertex + triangle->i_vertex[1],
        *v2 = model->m_vertex + triangle->i_vertex[2];

    /* cartesian coordinates of centroid are the means of the coordinates of
       the three vertices */
    centroid[0] = (v0->x + v1->x + v2->x) / 3;
    centroid[1] = (v0->y + v1->y + v2->y) / 3;
    centroid[2] = (v0->z + v1->z + v2->z) / 3;
}

__3dTree TriangleCorrsDict::CreateCentroidTree(const dtMeshModel *model)
{
    __3dTree centroid_tree;
    dt_size_type tree_size = model->m_n_triangle;

    /* create the exset of the tree, id of each centroid are named with the
       index of corresponded triangle  */
    __3dtree_Exemplar *exset = (__3dtree_Exemplar*)__dt_malloc(
        (size_t)tree_size * sizeof(__3dtree_Exemplar));

    dt_index_type i_tri = 0;
    for ( ; i_tri < tree_size; i_tri++)
    {
        CalculateTriangleCentroid(model, i_tri, exset[i_tri].pt);
        exset[i_tri].id = i_tri;
    }

    __3dtree_Create3DTree(exset, exset + tree_size, &centroid_tree);

    free(exset);
    return centroid_tree;
}

/* calculate dot(v1,v2) */
dt_real_type TriangleCorrsDict::VectorDot(const dtVector *v1, const dtVector *v2)
{
	dt_real_type x1 = v1->x, x2 = v2->x;
	dt_real_type y1 = v1->y, y2 = v2->y;
	dt_real_type z1 = v1->z, z2 = v2->z;

	return  x1*x2 + y1*y2 + z1*z2;
}

/* the angle between the normals of the two triangles should be less than 90-
   deg, this compatibility test prevents two nearby triangles with disparate
   orientation from entering the correspondence. */
int TriangleCorrsDict::NormCondition(__3dtree_Node *node)
{
    dtVector norm1 = 
        __dt_CalculateTriangleUnitNorm(__lambda_model, node->id);

    return  (VectorDot(&__lambda_norm, &norm1) > 0);
}


/* Resolving triangle correspondence by comparing the centroids of the deformed
   source and target triangles. Two triangles are compatible if their centroids
   are within a certain threshold of each other and the angle between their 
   normals is less than 90-deg.
*/
void TriangleCorrsDict::ResolveTriangleCorres(
    const dtMeshModel *deformed_source, const dtMeshModel *target, 
    dt_real_type threshold, TriangleCorrsList *tclist)
{
    __3dTree centroid_tree = CreateCentroidTree(deformed_source);

    /* n_triangle buffer space is large enough, though it might waste a lot
       of memory space, at least it would never overflow. */
    __3dtree_Node **result = (__3dtree_Node**)__dt_malloc(
        (size_t)deformed_source->m_n_triangle * sizeof(__3dtree_Node*));

    dt_real_type *dist_sq = (dt_real_type*)__dt_malloc(
        (size_t)deformed_source->m_n_triangle * sizeof(dt_real_type));
        

    dt_real_type  x0[3];  /* centroid of triangle on target model */
    dt_size_type  n_result;
    dt_index_type i_tri = 0, i_entry;

    dtTriangleCorrsEntry entry;
    __lambda_model = deformed_source;

    for ( ; i_tri < target->m_n_triangle; i_tri++)
    {
        CalculateTriangleCentroid(target, i_tri, x0);
        __lambda_norm = __dt_CalculateTriangleUnitNorm(target, i_tri);

        n_result = __3dtree_RangeSearch_Cond(
            centroid_tree, x0, threshold, result, dist_sq, NormCondition);

        /* append all found entries to the triangle corrs list */
        for (i_entry = 0 ; i_entry < n_result; i_entry++)
        {
            entry.i_src_triangle = result[i_entry]->id;
            entry.i_tgt_triangle = i_tri;
            entry.dist_sq = dist_sq[i_entry];
            tclist->AppendTriangleCorrsEntry(&entry);
        }
    }

    __3dtree_Destroy3DTree(centroid_tree);
    free(result); free(dist_sq);
}

#define DT_LARGER(x,y)  ((x)>(y)?(x):(y))
#define DT_SMALLER(x,y) ((x)<(y)?(x):(y))

/* Determine the threshold of triangle correspondence searching with the surface
   area of the bounding box and number of triangle units in the model: 

       threshold = sqrt(4*surface_area / n_triangle)
*/
dt_real_type TriangleCorrsDict::SelectTriangleCorrsThreshold(const dtMeshModel *model)
{
    dt_index_type iv = 1;
    dt_real_type x, y, z, dx, dy, dz,
        x_max = model->m_vertex[0].x, x_min = model->m_vertex[0].x, 
        y_max = model->m_vertex[0].y, y_min = model->m_vertex[0].y, 
        z_max = model->m_vertex[0].z, z_min = model->m_vertex[0].z;

    /* find the bounding box of the model */
    for ( ; iv < model->m_n_vertex; iv++)
    {
        x = model->m_vertex[iv].x;
        y = model->m_vertex[iv].y;
        z = model->m_vertex[iv].z;

        x_max = DT_LARGER(x, x_max);  x_min = DT_SMALLER(x, x_min);
        y_max = DT_LARGER(y, y_max);  y_min = DT_SMALLER(y, y_min);
        z_max = DT_LARGER(z, z_max);  z_min = DT_SMALLER(z, z_min);
    }

    /* width, height and depth of model's bounding box */
    dx = x_max - x_min;
    dy = y_max - y_min;
    dz = z_max - z_min;

    return sqrt(4 * (dx*dy + dy*dz + dx*dz) / model->m_n_triangle);
}

/* Easy to use version of __dt_ResolveTriangleCorres, users don't need to pick
   a threshold by hand, the threshold is estimated by a higher level process.
*/
void TriangleCorrsDict::ResolveTriangleCorresE(const dtMeshModel *deformed_source, 
	const dtMeshModel *target, TriangleCorrsList *tclist)
{
	//find the boundingbox
    dt_real_type 
        threshold_src = SelectTriangleCorrsThreshold(deformed_source),
        threshold_tgt = SelectTriangleCorrsThreshold(target);

    dt_real_type threshold = DT_LARGER(threshold_src, threshold_tgt);

    ResolveTriangleCorres(deformed_source, target, threshold, tclist);
}