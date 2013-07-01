#ifndef __DT_TRIANGLE_CORRESPONDENCE_HEADER__
#define __DT_TRIANGLE_CORRESPONDENCE_HEADER__

#include "dt_type.h"
#include "mesh_model.h"

/* Once the source model has been properly deformed into the target model, we
   need to run the last procedure to resolve the triangle units correspondences
   between the source model and the target model, the centroids of deformed 
   source and target triangles are compared to see if this pair is compatible.

   The compatibility rule is quite simple: 
       1. the distance of triangle centroids is within a given threshold
       2. angle between their normals is less than 90-deg

   Two triangle units is compatible when both condition is satisfied and would 
   be appended to a triangle correspondence list --- the following structure.
*/

typedef struct __dt_TriangleCorrsEntry_struct
{
    /* correspondence pair: i_src_triangle <==> i_tgt_triangle */
    dt_index_type i_src_triangle;
    dt_index_type i_tgt_triangle;
    dt_real_type  dist_sq;    /* squared distance between the centroid of 
                                 triangle units */
}dtTriangleCorrsEntry;

/* List of triangle correspondences: a bunch of source-target index pairs
   src0  tgt0
   src1  tgt1
   ... */
class TriangleCorrsList
{
public:
	/* Create an triangle correspondence list containing no entries, but has some
	reserved space for coming elements */
	void CreateEmptyTriangleCorrsList();

	/* Create an triangle correspondence list with specified length, no additional
	   space reserved for appending new items */
	void CreateTriangleCorrsList(dt_size_type length);

	/* Free up the triangle correspondence list */
	void DestroyTriangleCorrsList();


	/* Append a triangle correspondence entry to the tail of the list. The space 
	   extends exponentially, this behavior is similar with std::vector */
	void AppendTriangleCorrsEntry(const dtTriangleCorrsEntry *entry);

	/* Sort the list to ascending order of i_tgt_triangle then strip out duplicated
	   entries */
	void SortUniqueTriangleCorrsList();


	/* Load triangle correspondences from file, 0 on success -1 on fail.
	   This function would create a new triangle correspondence list object so you
	   do not need to call __dt_CreateTriangleCorrsList() before calling this 
	   function, or you'll suffer from a memory leak.
	*/
	int LoadTriangleCorrsList(const char *filename);

	/* Save the triangle correspondences to a text file, the file format is quite
	   simple: the first line is the total number of entris, each following line 
	   contains 2 integers which represents triangle indexes on source mesh and 
	   target mesh and a decimal number indicating the distance between their 
	   centroids.

	   This function returns 0 on success, or it would return an -1 to indicate an
	   fopen() error, you can check system var errno for further investigation.
	*/
	int SaveTriangleCorrsList(const char *filename);


	/* Resolving triangle correspondence by comparing the centroids of the deformed
	   source and target triangles. Two triangles are compatible if their centroids
	   are within a certain threshold of each other and the angle between their 
	   normals is less than 90-deg.

	   This routine is implemented in corres_resolve/triangle_corr_resolve.c
	*/
	void ResolveTriangleCorres(const dtMeshModel *deformed_source, const dtMeshModel *target, dt_real_type threshold);

	/* Easy to use version of __dt_ResolveTriangleCorres, users don't need to pick
	   a threshold by hand, the threshold is estimated by a higher level process.
	*/
	//void ResolveTriangleCorresE(const dtMeshModel *deformed_source, const dtMeshModel *target);



	/* Our automatic optimal region selector in corres_resolve picked up a 
	   relatively large range to incorporate as much triangle pairs into 
	   correspondence as possible, but the resulting correspondence list was too 
	   large so we need to strip out some far triangle pairs to reduce the size
	   of our final deformation equations. Only nearest n_maxcorrs correspondence
	   entries were preserved.
	*/
	void StripTriangleCorrsList(dt_size_type n_maxcorrs);

	void MakeEmpty()
	{
		m_list_length = 0;
		m_list_capacity = 0;
		m_corr = NULL;
	}

	dt_size_type GetListLength(){return m_list_length;}
	dt_size_type GetListCapacity(){return m_list_capacity;}

	dtTriangleCorrsEntry* GetTriangleCorrEntry(){return m_corr;}

	static int TricorrsEntryCompare(const void *_e0, const void *_e1);

private:
    /* sequantial list with reserved space, expands like std::vector */
    dt_size_type m_list_length;
    dt_size_type m_list_capacity;

    /* list of triangle correspondences */
    dtTriangleCorrsEntry *m_corr;

};


#endif /* __DT_TRIANGLE_CORRESPONDENCE_HEADER__ */
