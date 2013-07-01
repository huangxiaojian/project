#ifndef DTCONVERTER_H
#define DTCONVERTER_H

#include "mesh_model.h"
#include "Subdivision.h"


class DTConverter{
public:	
	static void ConvertMesh(const LS_Surface& src, dtMeshModel& tgt)
	{
		tgt.DestroyMeshModel();

		tgt.m_filename = NULL;
		//if(!(tgt.m_n_vertex == src.vertices.size() && tgt.m_n_normvec == src.vertices.size() && tgt.m_n_triangle == src.faces.size()))
		{
			tgt.m_n_vertex = src.vertices.size();
			tgt.m_n_normvec = src.vertices.size();
			tgt.m_n_triangle = src.faces.size();
			tgt.CreateMeshModel();
		}
		
		
		for(int i = 0; i < tgt.m_n_vertex; i++)
		{
			//printf("%d\n", i);
			tgt.m_vertex[i].x = src.vertices[i]->pos[0];
			tgt.m_vertex[i].y = src.vertices[i]->pos[1];
			tgt.m_vertex[i].z = src.vertices[i]->pos[2];
		}
		//printf("Vertex!!!\n");
		for(int i = 0; i < tgt.m_n_triangle; i++)
		{
			for(int j = 0; j < 3; j++)
				tgt.m_triangle[i].i_norm[j] = tgt.m_triangle[i].i_vertex[j] = src.faces[i]->vertices[j]->m_index-1;
		}
		//printf("Face!!!\n");
		tgt.ComputeVertexNormals();
		//printf("Normal!!!\n");
	}
};

#endif