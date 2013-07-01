#ifndef DT_MESH_MODEL_H
#define DT_MESH_MODEL_H


#include "dt_type.h"

/* 3D mesh model made up with triangular units, it contains coordinates of all
   vertices of the object as well as norm vector of each vertex. Triangular
   surface units can be derived from indexes arrays of those vertices and norms.
*/

//#define OBJNONORMAL

struct dtMeshModel
{
	dtMeshModel();
	/* CreateMeshModel allocates memory space for model structure according to 
	   the model size specified in model->n_vertex, model->n_normvec, model->
	   n_triangle, and initializes pointers to vertex, norm vector and triangle 
	   arrays.

	   You MUST fill in n_vertex, n_normvec and n_triangle before calling 
	   CreateMeshModel().
	 */
	void CreateMeshModel();

	/* DestroyMeshModel frees all memory space allocated in CreateMeshModel.
	 */
	void DestroyMeshModel();


	/* Computer Vertex Normals if no normal info in the .obj file
	 */
	void ComputeVertexNormals();

	/* ReadObjFile parse specified .obj model description file and read vertex, 
	   normal vector and triangular surface information into *model

	   This function returns 0 on success, otherwise it would return an -1 to 
	   indicate an fopen() error (file not exist, privillage or some reason).
	*/
	int ReadObjFile(const char *filename);

	/* Read .obj model file into model object, if reading file failed, it would 
	   simply print an error message to stderr and crash. */
	void ReadObjFileCommitOrCrash(const char *filename);

	/* SaveObjFile saves specified mesh model to a file, it would return 0 on 
	   success, otherwise it would return -1 to indicate fopen() was failed.
	   You should check system variable errno for further investigation.
	*/
	int SaveObjFile(const char *filename);
	int ExportObjFile(const char* filename);

	/* Sort out a list containing the index of normal vectors of each vertex in the
	   model, this list might be helpful in closest point iteration. 
   
	   You have to manually free the pointer returned by this function after using 
	   it. 
	*/
	dt_index_type *SortOutVertexNormalList();

	void DrawMeshModel(bool wired = false);

	int GetIndexOfVertex(const dtVertex* v);

	void CalculateModelPosCorrection(double *cx, double *cy, double *cz);

	/* Skip current line / jump to the start of the next line */
	static void _SkipThisLine(FILE *fd);

	static FILE *_GetMeshModelScale(
		const char *filename, dt_size_type *n_vertex, 
		dt_size_type *n_normvec, dt_size_type *n_triangle);


    /* Vertices, norm vectors and triangular surfaces in the model */
    dtVertex   *m_vertex;       /* array of all vertices in the model */
    dtVector   *m_normvec;      /* array of norm vectors */
    dtTriangle *m_triangle;     /* array of all triangular units */

    /* Number of vertices, norm vectors and triangles in the model */
    dt_size_type m_n_vertex;
    dt_size_type m_n_normvec;
    dt_size_type m_n_triangle;

	const char* m_filename;
};


#endif /* DT_MESH_MODEL_H */
