#include "stdafx.h"
#include "transformer.h"
#include "umfpack.h"

#include <string>

/* Create a deformation transfer object, once created, this object can help 
   deforming the target mesh like the source mesh deformation quicky and 
   faithfully. 
*/
void DTTransformer::CreateDeformationTransformer(
    const char *source_ref_name, const char *target_ref_name,
    const char *tricorrs_name, dt_size_type n_maxcorrs, 
	const dtMeshModel* source_obj, const dtMeshModel* target_obj)
{
    TriangleCorrsList tclist;

    cholmod_sparse   *A;
    __dt_SparseMatrix A_tri;
    void *symbolic_obj;        /* for umfpack's symbolic analysis */

    /* Load data */
	if(source_obj)
		memcpy(&m_source_ref, source_obj, sizeof(dtMeshModel));
	else
		m_source_ref.ReadObjFileCommitOrCrash(source_ref_name);
	if(target_obj)
		memcpy(&m_target, target_obj, sizeof(dtMeshModel));
	else
		m_target.ReadObjFileCommitOrCrash(target_ref_name);
    
    if (tclist.LoadTriangleCorrsList(
            tricorrs_name) == -1) {
        perror("Loading triangle correspondence failed");
        exit(1);
    }

    /* Initialize triangle correspondence dictionary */
	tclist.StripTriangleCorrsList(n_maxcorrs);
	m_tcdict.CreateTriangleCorrsDict(&m_target, &tclist);

    /* Precalculate inverse of surface matrices of source reference model*/
    __dt_InitializeSurfaceInvVList(&m_source_ref, &m_sinvlist);

    /* Allocate for linear system */
	DTEquation::_AllocDeformationEquation(&m_target, &m_tcdict, &A_tri, &m_C);
    m_c = __dt_CHOLMOD_dense_zeros(A_tri->ncol, 1);      /* rhs vector: ncol*1 */
    m_x = __dt_CHOLMOD_dense_zeros(A_tri->ncol, 1); /* solution vector: ncol*1 */

    /* Building coefficient matrix: 
       A_tri(triplet) ==> A(sparse) ==> At ==> AtA */
    printf("building equation...\n");
    DTEquation::_BuildCoefficientMatrix(&m_target, &m_tcdict, A_tri);
    A = __dt_CHOLMOD_triplet_to_sparse(A_tri); __dt_CHOLMOD_free_triplet(&A_tri);
    m_At  = __dt_CHOLMOD_transpose(A);    __dt_CHOLMOD_free_sparse(&A);
    m_AtA = __dt_CHOLMOD_AxAt(m_At);

    printf("factorizing...\n");
    /* factorize AtA */
    umfpack_di_symbolic(
        (int)m_AtA->nrow, (int)m_AtA->ncol, 
        (const int*)m_AtA->p, (const int*)m_AtA->i, (const double*)m_AtA->x, 
        &symbolic_obj, NULL, NULL);

    umfpack_di_numeric(
        (const int*)m_AtA->p, (const int*)m_AtA->i, (const double*)m_AtA->x, 
        symbolic_obj, &(m_numeric_obj), NULL, NULL);

    umfpack_di_free_symbolic(&symbolic_obj);
}

void DTTransformer::CreateDeformationTransformer(dtMeshModel* source_ref, dtMeshModel* target_ref, const char* tricorrs_name, dt_size_type n_maxcorrs)
{
	CreateDeformationTransformer(NULL, NULL, tricorrs_name, n_maxcorrs, source_ref, target_ref);
}

/* Transform the target model like source_ref==>source_deformed, m_target
   is modified to deformed model.  */
void DTTransformer::Transform2TargetMeshModel(const dtMeshModel *source_deformed)
{
    DTEquation::_BuildRhsConstantVector(source_deformed, &(m_target), 
        &(m_sinvlist), &(m_tcdict), m_C);

    __dt_CHOLMOD_Axc(m_At, m_C, m_c);

    umfpack_di_solve(UMFPACK_A, 
        (const int*)(m_AtA->p), (const int*)(m_AtA->i), (const double*)(m_AtA->x), 
        (double*)(m_x->x), (const double*)(m_c->x), 
        m_numeric_obj, NULL, NULL);

    _ApplyDeformationToModel(&(m_target), m_x);
}

/* Update the coordinates of vertices in specified model with solution vector x */
void DTTransformer::_ApplyDeformationToModel(dtMeshModel *model, __dt_DenseVector x)
{
    dt_index_type i = 0, ind = 0;
    for ( ; i < model->m_n_vertex; i++)
    {
        model->m_vertex[i].x = __dt_CHOLMOD_REFVEC(x, ind); ind++;
        model->m_vertex[i].y = __dt_CHOLMOD_REFVEC(x, ind); ind++;
        model->m_vertex[i].z = __dt_CHOLMOD_REFVEC(x, ind); ind++;
    }
}


/* Release the memory allocated for the transformer object */
void DTTransformer::DestroyDeformationTransformer()
{
	m_source_ref.DestroyMeshModel();
	m_target.DestroyMeshModel();
    __dt_DestroySurfaceInvVList(&(m_sinvlist));
	m_tcdict.DestroyTriangleCorrsDict();

    umfpack_di_free_numeric(&(m_numeric_obj));
    __dt_CHOLMOD_free_dense(&(m_x));
    __dt_CHOLMOD_free_dense(&(m_c));
    __dt_CHOLMOD_free_dense(&(m_C));
    __dt_CHOLMOD_free_sparse(&(m_At));
    __dt_CHOLMOD_free_sparse(&(m_AtA));
}

void DTTransformer::GetTargetMeshModel(dtMeshModel* mesh)
{
	memcpy(mesh, &m_target, sizeof(dtMeshModel));
}