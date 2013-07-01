#ifndef __DT_TRANSFORMER_HEADER__  /* (not that Transformer) */
#define __DT_TRANSFORMER_HEADER__


#include "dt_equation.h"

class DTTransformer
{
public:

	/* Create a deformation transfer object, once created, this object can help 
	   deforming the target mesh like the source mesh deformation quicky and 
	   faithfully. There's a lot of initialization process so this procedure might
	   take significiant amount of time.
	*/
	void CreateDeformationTransformer(
		const char *source_ref_name, const char *target_ref_name,
		const char *tricorrs_name, dt_size_type n_maxcorrs,
		const dtMeshModel* source_obj = NULL, const dtMeshModel* target_obj = NULL);

	void CreateDeformationTransformer(
		dtMeshModel* source_ref, dtMeshModel* target_ref,
		const char* tricorrs_name, dt_size_type n_maxcorrs);

	/* Transform the target model like source_ref==>source_deformed, trans->target
	   is modified to deformed model.  */
	void Transform2TargetMeshModel(const dtMeshModel *source_deformed);

	/* Release the memory allocated for the transformer object */
	void DestroyDeformationTransformer();

	dtMeshModel& GetTargetMeshModel(){return m_target;}
	void GetTargetMeshModel(dtMeshModel* mesh);

	static void DTBegin(){__dt_CHOLMOD_start();}
	static void DTFinish(){__dt_CHOLMOD_finish();}

private:

	static void _ApplyDeformationToModel(dtMeshModel *model, __dt_DenseVector x);

    dtMeshModel m_source_ref;   /* source reference model */
    dtMeshModel m_target;       /* target reference/deformed model. 
                                 It represents the reference model when
                                 initializing the transformer object, 
                                 and turns into deformed target model in
                                 deformation transfer solving phase. */

    TriangleCorrsDict m_tcdict;  /* triangle units correspondence */

    /* the deformation equation: AtA * x = c, where c = At * C */
    cholmod_sparse *m_At, *m_AtA;
    cholmod_dense  *m_C, *m_c, *m_x;

    void *m_numeric_obj;     /* umfpack factorization result */

    __dt_SurfaceInvVList m_sinvlist;   /* inverse surface matrix list for 
                                        source reference model */
};

#endif /*__DT_TRANSFORMER_HEADER__*/
