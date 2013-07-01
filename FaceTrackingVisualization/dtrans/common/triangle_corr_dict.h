#ifndef __DT_TRIANGLE_CORRESPONDENCE_DICTIONARY_HEADER__
#define __DT_TRIANGLE_CORRESPONDENCE_DICTIONARY_HEADER__

#include "triangle_corr.h"
#include "surface_matrix.h"
#include "3dtree.h"
#include "mesh_model.h"

/* __dt_TriangleCorrsList employs a linear data structure to store triangle
   units correspondence between mesh models, which make it hard to perform fast
   lookups for corresponded source triangles of a particular target triangle.
   We come up with a dictionary type which uses a data structure similar with
   crt argv[] to deal with this issue.

   A __dt_TriangleCorrsDict is constructed from a __dt_TriangleCorrsList object
   with linear (O(n)) time complexity, and provides a trival operation (O(1) 
   time) to look up for corresponded source triangles of specified target 
   triangles.

   The internal implementation (data layout) of this data type is quite similar
   with argv[] in ISO C crt, `corrsv' and `tclist' works in the same way with
   `argv' and the command line parameter string.
*/
typedef struct __dt_TriangleCorrsDict_entv_struct
{
    dtTriangleCorrsEntry *corrs;   /* an array of corresponded triangle 
                                         entries */
    dt_size_type   n_corrstriangle;   /* number of corresponded triangles, 
                                         length of corrs */
}dtTriangleCorrsDictEntv;


/* SAMPLE DATA LAYOUT:

   corrsv
   [0] --> [n_corrstriangle = 3, corrs -> [(3,0), (2,0), (6,0)]]
   [1] --> [n_corrstriangle = 2, corrs -> [(9,1), (7,1)]]
   [2] --> [n_corrstriangle = 0, corrs -> NULL]
   [3] --> [n_corrstriangle = 3, corrs -> [(5,3), (9,3), (1,3)]]
   ......

   we have:
       triangle No.0 on target mesh is corresponded with 3 triangle units on
       source mesh. They are triangle No.3, triangle No.2 and triangle No.6;

       triangle No.2 on target mesh has no corresponding triangles on source
       mesh, it is an isolated triangle unit.
*/
class TriangleCorrsDict
{
public:
	/* Create an empty triangle correspondence dictionary, it only allocates space
	   for the lookup table (argv[]) and intialize it as an empty one. */
	void CreateEmptyTriangleCorrsDict(const dtMeshModel *target_model);
    

	/* Construct a __dt_TriangleCorrsDict object from a __dt_TriangleCorrsList, 
	   tclist is then migrated into tcdict and cannot be destroyed explicitly. */
	void CreateTriangleCorrsDict(const dtMeshModel *target_model, TriangleCorrsList *tclist);

	/* Destroy specified dictionary object and free its memory */
	void DestroyTriangleCorrsDict();

	/* Functions to access entries in the correspondence dictionary, the 
	   implementations are quite straight forward. */

	dtTriangleCorrsDictEntv* GetTriangleCorrsEntryVector(dt_index_type i_tgt_triangle);

	dtTriangleCorrsEntry* GetTriangleCorrsEntry(dt_index_type i_tgt_triangle, dt_index_type i_corrs_entry);

	dt_size_type GetTriangleCorrsNumber(dt_index_type i_tgt_triangle);

	static void CalculateTriangleCentroid(
		const dtMeshModel *model, dt_index_type i_triangle, dt_real_type *centroid);

	static __3dTree CreateCentroidTree(const dtMeshModel *model);

	static dt_real_type VectorDot(const dtVector *v1, const dtVector *v2);
	
	static int NormCondition(__3dtree_Node *node);
	
	static void ResolveTriangleCorres(
		const dtMeshModel *deformed_source, const dtMeshModel *target, 
		dt_real_type threshold,	TriangleCorrsList *tclist);
	
	/* Determine the threshold of triangle correspondence searching */
	static dt_real_type SelectTriangleCorrsThreshold(const dtMeshModel *model);
	
	static void ResolveTriangleCorresE(const dtMeshModel *deformed_source, 
		const dtMeshModel *target, TriangleCorrsList *tclist);
	
	/* variables for currying */
	static const dtMeshModel *__lambda_model;
	static       dtVector     __lambda_norm;

private:
    dtTriangleCorrsDictEntv  *m_corrsv;  /* argv */
    TriangleCorrsList        m_tclist;  /* cmdline string */

    dt_size_type  m_corrsv_size;    /* equivalent with argc.
                                     Length of corrsv, should be equal to 
                                     the number of target triangle units */
};


#endif /* __DT_TRIANGLE_CORRESPONDENCE_DICTIONARY_HEADER__ */
