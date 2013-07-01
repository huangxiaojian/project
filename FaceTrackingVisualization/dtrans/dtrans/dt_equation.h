#ifndef __DT_DEFORMATION_EQUATION_HEADER__
#define __DT_DEFORMATION_EQUATION_HEADER__


#include "triangle_corr_dict.h"
#include "mesh_model.h"
#include "surface_matrix.h"


class DTEquation{
public:
	/* Allocate for coefficient matrix and rhs vector with proper size */
	static void _AllocDeformationEquation(
		const dtMeshModel *target_mesh, const TriangleCorrsDict *tcdict,
		__dt_SparseMatrix *A_tri, __dt_DenseVector *C);


	/* Build the coefficient matrix of deformation equations from target reference 
	   mesh, the coefficient matrix can be built only once to deform for a lot of 
	   deformed source meshes. 
	*/
	static void _BuildCoefficientMatrix(
		const dtMeshModel *target_ref, TriangleCorrsDict *tcdict,
		__dt_SparseMatrix A);


	/* Build rhs vector of the deformation equation for source_ref=>source_deform.
	   You just need to build rhs vector for each deformation while keeping the 
	   coefficient matrix unchanged.
	*/
	static void _BuildRhsConstantVector(
		const dtMeshModel *source_deformed, const dtMeshModel *target_ref,
		const __dt_SurfaceInvVList *sinvlist_ref,
		TriangleCorrsDict *tcdict,
		__dt_DenseVector C);

	/* Allocate for coefficient matrix and rhs vector with proper size */
	static void _AllocDeformationEquation(
		const dtMeshModel *target_mesh, TriangleCorrsDict *tcdict,
		__dt_SparseMatrix *A_tri, __dt_DenseVector *C);

private:

	static void _CalculateEquationSize(
		const dtMeshModel *target_mesh, TriangleCorrsDict *tcdict,
		/* output param */ dt_size_type *n_row, dt_size_type *n_col);

	static void _CalculateElementaryMatrix(
		dtMatrix3x3 inV, __dt_ElementaryMatrix m);

	static dt_index_type _GetVariableIndex(
		const dtMeshModel *model, dt_index_type i_triangle,
		dt_index_type i_vlocal, dt_index_type i_dim);

	static dt_index_type _AppendElemMatrixToLinearSystem(
		const dtMeshModel *model, dt_index_type i_triangle,
		__dt_ElementaryMatrix m, __dt_SparseMatrix M, dt_index_type i_row);

	static dt_index_type _AppendRhsVectorToLinearSystem(
		dtMatrix3x3 T, __dt_DenseVector C, dt_index_type i_row);
};


#endif /*__DT_DEFORMATION_EQUATION_HEADER__*/
