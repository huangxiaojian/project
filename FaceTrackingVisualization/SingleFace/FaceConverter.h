#ifndef FACECONVERTER_H
#define FACECONVERTER_H

#include "Subdivision.h"

#include "FTHelper.h"

class FaceConverter{
public:	
	static void ConvertMesh(FTHelper& src, LS_Surface& tgt)
	{
		/* convert vertex */
		tgt.Reset();
		for(int i = 0; i < src.GetVertexNum(); i++)
		{
			if(tgt.isInVertexIndex(i))
				continue;
			LS_Vertex* v = new LS_Vertex();
			v->pos[0] = src.GetVertices()[i].x;
			v->pos[1] = src.GetVertices()[i].y;
			v->pos[2] = -src.GetVertices()[i].z;

			tgt.vertices.push_back(v);
			v->creationLevel = tgt.subdivisionLevel;
			v->m_index = tgt.vertices.size();
		}

		/* convert face */
		for(int i = 0; i < src.GetTriangleNum(); i++)
		{
			if(tgt.isInFaceIndex(i))
				continue;
			//src.m_pTriangles[i] index begin from 0
			tgt.AddFaceOfIndex(tgt.toNewIndex(src.GetTriangles()[i].i), tgt.toNewIndex(src.GetTriangles()[i].k), tgt.toNewIndex(src.GetTriangles()[i].j));
		}
	}
};

#endif