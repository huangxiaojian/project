#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "transformer.h"
#include "mesh_seg.h"
#include "triangle_corr_dict.h"

#include "Subdivision.h"

#include "DTConverter.h"

#include <time.h>
#include <string.h>

#define N_MAXCORRS 3

#define SUBDIVISION_NUM 3

void showhelp()
{
	printf("-------------------------\n");
	printf("Argument1: dtrans $obj_src $obj_tgt $out.tricorres $obj_src_index.obj\n");
	printf("-------------------------\n");
}

//int main(int argc, char *argv[])
int myMain(int argc, char *argv[])
{
	LS_Surface source_obj, source_deformed_obj;
    DTTransformer trans;
    dtMeshModel source_ref_obj, source_deformed;
#ifdef _DEBUG
	const char 
		*source_ref = "0_out_z.obj",  /* source reference model */
		*target_ref = "modify_ref.obj",  /* target reference model */
		*tricorrs = "hxjout.tricorrs";  /* vertex constraints specified with Corres!
											 */
	//char str[] = "horse-0.obj";
	//char src_deformed[][FILENAME_MAX] = {"in_0.obj"};
#else
    const char 
        *source_ref = argv[1],  /* filename of source reference model */
        *target_ref = argv[2],  /* filename of target reference model */
        *tricorrs   = argv[3];  /* filename of triangle correspondence */

	char **src_deformed = &argv[4]; /* deformed source mesh filenames */
#endif
    


    /* number of deformed source model files specified in command line */
#ifdef _DEBUG
	dt_size_type n_deformed_source = 1;
#else
    //dt_size_type n_deformed_source = argc - 4;
	dt_size_type n_deformed_source = 1;
#endif
    dt_index_type i_source = 0;

    char deformed_mesh_name[FILENAME_MAX];  /* deformed target mesh filename */
#ifdef _DEBUG
	if(1)
#else
    if (argc > 3)
#endif
    {
        DTTransformer::DTBegin();

        /* Create a transformer object for deforming the target mesh using 
           source mesh deformations */
        printf("reading data...\n");
#ifdef _DEBUG
		source_obj.ReadObj("0_out_z.obj");
		printf("read 0_out_z.obj done\n");
#else
		source_obj.ReadObj(argv[1]);
		//printf("read %s done\n", argv[1]);
#endif
		for(int i = 0; i < SUBDIVISION_NUM; i++)
			source_obj.Subdivide();
		//printf("subdivision done\n");
		DTConverter::ConvertMesh(source_obj, source_ref_obj);
		//printf("Convert mesh done\n");
        trans.CreateDeformationTransformer(source_ref, target_ref, tricorrs, N_MAXCORRS, &source_ref_obj);
		//printf("create done\n");

        /* Transfer the deformation of each deformed source mesh to the target
           mesh, so that the target mesh would deform like the source mesh  */
        //for ( ; i_source < n_deformed_source; i_source++)
        //{
            /* read deformed source model */
            printf("loading source deformed meshes...\n");
#ifdef _DEBUG
			source_deformed_obj.ReadObj("90_out_z.obj");
			printf("read 90_out_z.obj done\n");
#else
			source_deformed_obj.ReadObj(argv[4]);
#endif
			for(int i = 0; i < SUBDIVISION_NUM; i++)
				source_deformed_obj.Subdivide();
			DTConverter::ConvertMesh(source_deformed_obj, source_deformed);
			//source_deformed.ReadObjFileCommitOrCrash(src_deformed[i_source]);

            /* deform the target model like source_ref=>source_deformed */
            printf("deforming...\n");

			clock_t start = clock();
            trans.Transform2TargetMeshModel(&source_deformed);
			printf("Time = %lf\n", (double)(clock()-start)/CLOCKS_PER_SEC);

            /* save deformed target mesh to file: out_##.obj */
            printf("deformation complete, save deformed mesh to file\n");
            /*snprintf(
                deformed_mesh_name, sizeof(deformed_mesh_name), 
                "out_%d.obj", i_source);*/
			int result = _snprintf_s(
				deformed_mesh_name, sizeof(deformed_mesh_name), 
				"out_%d.obj", i_source);
			if(result == sizeof(deformed_mesh_name) || result < 0)
				deformed_mesh_name[sizeof(deformed_mesh_name)-1] = 0;
			trans.GetTargetMeshModel().SaveObjFile(deformed_mesh_name);

            /* complete */
			source_deformed.DestroyMeshModel();
        //}

		trans.DestroyDeformationTransformer();
        DTTransformer::DTFinish();

    }
    else {
		if(argc == 2 && strcmp(argv[1], "-h"))
		{
			::showhelp();
			return 0;
		}
        printf(
            "usage: %s source_ref target_ref tricorres"
            " <one or more deformed source model>\n", argv[0]);
    }

    return 0;
}
