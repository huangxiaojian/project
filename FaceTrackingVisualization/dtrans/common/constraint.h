#ifndef DT_CONSTRAINT_H
#define DT_CONSTRAINT_H


/* This header file defines basic data structures for representing vertex 
   location constraints in the triangle correspondence resolving algorithm. */

//DONE

#include "dt_type.h"
#include "mesh_model.h"

/* 
   Vertex constraints forces the source mesh deforming to the shape of target
   mesh. Some "critical" vertices are specified to be deformed to the same 
   position of the corresponded one on the target mesh, which forces the 
   overall look of the deformed mesh similar to the target mesh. 

   CONSTRAINT (.cons) FILE FORMAT:
   --------------------------------------
   number of constraints
   source_vertex0,  target_vertex0
   source_vertex1,  target_vertex1
   source_vertex2,  target_vertex2
   ...
   --------------------------------------
*/
typedef struct __dt_VertexConstraintEntry_struct
{
    /* i_*_vertex: indexes of constrained vertices
       src: source mesh,  tgt: target mesh */
    dt_index_type i_src_vertex, i_tgt_vertex;

    /* source.vertex[i_src_vertex] should be deformed to 
       target.vertex[i_tgt_vertex], so the coordinate of deformed 
       i_src_vertex-th vertex is regarded as constant which equals to 
       i_tgt_vertex-th vertex on the target mesh. */

}dtVertexConstraintEntry;

struct dtVertexInfo
{
	dtVertexConstraintEntry vconsEntry;
	dtVertex vl;
	dtVertex vr;
};

class VertexConstraintList
{
public:
	/* Allocate for constraint_list with the size information specified in
	   constraint_list->list_length */
	void CreateConstraintList();

	//void CreateVertexPosition();

	/* Load vertex constraints from file, these constraints are often specifed by
	   users with the graphical interactive tool "Corres!" in our package. The 
	   entries are sorted to arrange source vertex indexes in ascending order, 
	   which would accelerate the construction of vertex info (type-index) list.

	   This function returns an 0 on success, or it would return an -1 to indicate
	   the occurrence of an fopen() error.
	*/
	int LoadConstraints(const char *filename);

	/* load .cons_v file */
	int LoadConstraintsWithPosition(const char * filename);

	/* Release all resources allocated for the constraint list object */
	void ReleaseConstraints();

	/* Release vector position */
	//void ReleaseVertexPosition();

	/* Save constraint list to file

	   This function returns 0 on success, or it would return an -1 to indicate an
	   fopen() error, go checking system variable errno for furture investivation.
	*/
	int SaveConstraints(const char *filename);

	/*save more infomation than SaveConstraints(), include point position */
	int SaveConstraintsWithPosition(const char *filename, const dtMeshModel* modelL, const dtMeshModel* modelR);

	/* Find corresponding target vertex with specified vertex constriant. */
	dtVertex* GetMappedVertex(const dtMeshModel *target_model, dt_index_type i_cons);

	/* Given a vertex correspondence constraint of one constrained vertex, find
	   the coordinate of the corresponded vertex on the target mesh. */
	dt_real_type GetMappedVertexCoord(const dtMeshModel *target_model, dt_index_type i_cons, dt_index_type i_dimension);

	static void SortConstraintEntries(VertexConstraintList *constraint_list);
	static int CompareConstraintEntries(const void *_1, const void *_2);

	dt_size_type GetListLength(){return m_list_length;}
	dtVertexConstraintEntry* GetConstraint(int i){return (dtVertexConstraintEntry*)(m_vertexInfo+i);}
	dtVector* GetSrcVertexPosition(int i){return (dtVertex*)(&(m_vertexInfo[i].vl));}
	dtVector* GetTgtVertexPosition(int i){return (dtVertex*)(&(m_vertexInfo[i].vr));}
	void SetSrcVertexIndex(int index, dt_index_type value){m_vertexInfo[index].vconsEntry.i_src_vertex = value;}
	void SetTgtVertexIndex(int index, dt_index_type value){m_vertexInfo[index].vconsEntry.i_tgt_vertex = value;}

	void SetListLength(dt_size_type list_length){m_list_length = list_length;}

private:
    //dtVertexConstraintEntry *m_constraint;
	dtVertexInfo* m_vertexInfo;
    dt_size_type  m_list_length;  /* number of constraint entries, should be 
                                   equal to the number of marker points you 
                                   specified with corrstool. */
	//dtVector *m_vertexPosition;
};


#endif /* CONSTRAINT_H */
