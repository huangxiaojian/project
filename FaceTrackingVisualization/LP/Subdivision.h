#ifndef SUBDIVISION_H
#define SUBDIVISION_H

//#include "LSVector.h"
#include "stdafx.h"
#include <vector>
#include <string.h>
#include <stdio.h>

#include "utilVector.h"

class LS_Edge;
class LS_Face;
class LS_Vertex;

using namespace std;

typedef pair<LS_Vertex*, LS_Vertex*> LS_VertexPair;

//typedef Point3D LSPoint;
//typedef Vector3D LSVector;

#define USE_MEM_POOL 1

#if USE_MEM_POOL
	#define MP_VERTS 	450000
	#define MP_EDGES	750000
	#define MP_FACES	500000
#endif

//MARK
template <class T>
bool SearchIndex(vector<T*> vt, int index)
{
	for(vector<T*>::iterator itr = vt.begin(); itr != vt.end(); itr++)
		if((*itr)->m_index == index)
			return true;
	return false;
}


class LS_Vertex
{
public:
	LS_Vertex():m_index(0), m_isBoundary(false){}
	

//private:
	//UNMARK
	int m_index;
	bool m_isBoundary;

	vector<LS_Edge*> edges;
	vector<LS_Face*> faces;


	//LSPoint pos;
	Point3D pos;
	//LSPoint newPos;
	Point3D newPos;

	//MARK
	//LSPoint texel;
	//LSVector normal;
	Vector3D normal;

	int creationLevel;

	LS_Edge** GetEdgePointer( LS_Edge *ptr );
	LS_Face** GetFacePointer( LS_Face *ptr );
	LS_Vertex* GetNewVertexInEdge( LS_Vertex *v );

	void UpdateNormal();

	bool SearchEdgesIndex(int index)
	{
		return SearchIndex<LS_Edge>(edges, index);
	}
	bool SearchFacesIndex(int index)
	{
		return SearchIndex<LS_Face>(faces, index);
	}

	//MARK
	LS_Edge* GetEdgeOfVertex(LS_Vertex* v);

	friend class LS_Surface;
	friend class LS_Face;
	friend class LS_Edge;
#if USE_MEM_POOL
	void* operator new(size_t bytes);
	void operator delete(void* p);
	bool mp_inuse;
#endif
};
typedef vector<LS_Vertex*> LS_VertexList;


class LS_Edge
{
public:
	LS_Edge() : oldVertex(0), m_index(0)								{}

//private:

	int m_index;

	LS_Vertex *vertices[2];
	LS_Vertex *oldVertex;

	LS_Vertex** GetVertexPointer( LS_Vertex *v )
	{
		if ( vertices[0] == v )
			return &vertices[0];
		else
			return &vertices[1];
	}

	//MARK
	bool SearchVerticesIndex(int index)
	{
		if((oldVertex) && (oldVertex->m_index == index ))
			return true;
		else if(vertices[0]->m_index == index || vertices[1]->m_index == index)
			return true;
		else
			return false;
	}

	friend class LS_Surface;
	friend class LS_Vertex;
	friend class LS_Face;
	friend bool operator!=( LS_Edge*& e, const LS_VertexPair &vp );
	friend bool operator==( LS_Edge*& e, const LS_VertexPair &vp );

	template <class T>
	friend bool ::SearchIndex(vector<T*> vt, int index);


#if USE_MEM_POOL
	void* operator new(size_t bytes);
	void operator delete(void* p);
	bool mp_inuse;
#endif
};
typedef vector<LS_Edge*> LS_EdgeList;

bool operator!=( LS_Edge*& e, const LS_VertexPair &vp );
bool operator==( LS_Edge*& e, const LS_VertexPair &vp );
//must be like this, or how to put the new vertex ?
//two point are the same, then edge is the same no matter direction


class LS_Face
{
public:
	LS_Face():m_index(0){}
	//MARK
//private:

	int m_index;

	//UNMARK
	LS_Vertex *vertices[3];
	LS_Edge *edges[3];

	//LSVector centroid;
	//LSVector normal;
	Vector3D centroid;
	Vector3D normal;

	void UpdateCentroid()
	{
		/*centroid = ( vertices[0]->pos.ToVector() + vertices[1]->pos.ToVector() 
					+ vertices[2]->pos.ToVector() ) / 3;*/
		centroid = ( vertices[0]->pos + vertices[1]->pos 
			+ vertices[2]->pos ) / 3;
	}

	void UpdateNormal()
	{
		normal = ( vertices[1]->pos - vertices[0]->pos )%(
						vertices[2]->pos - vertices[0]->pos );
		normal.normalizeSelf();
		/*normal = ( vertices[1]->pos.ToVector() - vertices[0]->pos.ToVector() ).Cross(
			vertices[2]->pos.ToVector() - vertices[0]->pos.ToVector() );
		normal.Normalize();*/
	}

	//MARK
	//Search
	bool SearchVerticesIndex(int index)
	{
		for(int i = 0; i < sizeof(vertices)/sizeof(LS_Vertex*); i++)
			if(vertices[i]->m_index == index)
				return true;
		return false;
	}
	bool SearchEdgesindex(int index)
	{
		for(int i = 0; i < sizeof(edges)/sizeof(LS_Edge*); i++)
			if(edges[i]->m_index == index)
				return true;
		return false;
	}

	friend class LS_Surface;
	friend class LS_Vertex;
	friend class LS_Edge;

	template <class T>
	friend bool ::SearchIndex(vector<T*> vt, int index);


#if USE_MEM_POOL
	void* operator new(size_t bytes);
	void operator delete(void* p);
	bool mp_inuse;
#endif
};
typedef vector<LS_Face*> LS_FaceList;

class LS_Surface
{
public:
	LS_Surface() : subdivisionLevel(0)				
	{
#if USE_MEM_POOL
		vertices.reserve( MP_VERTS );
		edges.reserve( MP_EDGES );
		faces.reserve( MP_FACES );
#endif
	}
	~LS_Surface();

	//void InitSelf();
	void Reset();
	//void ReadObjFromFile(const char* filename, float scale = 1.0);
	void ReadObj(const char* filename, float scale = 1.0);
	void WriteObj(const char* filename, bool flag = false);

	void AddFaceOfIndex(int i1,int i2, int i3);
	void UpdateNormals();

	void Subdivide();
	void Subdivide(int count)
	{
		while(count--)
			Subdivide();
	}

	//MARK
	//Search
	vector<int> SearchVerticeIndexInEdges(int index)
	{
		vector<int> vi;
		for(vector<LS_Edge*>::iterator eitr = edges.begin(); eitr != edges.end(); eitr++)
			if((*eitr)->SearchVerticesIndex(index))
				vi.push_back((*eitr)->m_index);
		return vi;
	}
	vector<int> SearchVerticeIntdexInFaces(int index)
	{
		vector<int> vi;
		for(vector<LS_Face*>::iterator fitr = faces.begin(); fitr != faces.end(); fitr++)
			if((*fitr)->SearchVerticesIndex(index))
				vi.push_back((*fitr)->m_index);
		return vi;
	}
	vector<int> SearchEdgeIndexInVertices(int index)
	{
		vector<int> vi;
		for(vector<LS_Vertex*>::iterator vitr = vertices.begin(); vitr != vertices.end(); vitr++)
			if((*vitr)->SearchEdgesIndex(index))
				vi.push_back((*vitr)->m_index);
		return vi;
	}
	vector<int> SearchEdgeIndexInFaces(int index)
	{
		vector<int> vi;
		for(vector<LS_Face*>::iterator fitr = faces.begin(); fitr != faces.end(); fitr++)
			if((*fitr)->SearchEdgesindex(index))
				vi.push_back((*fitr)->m_index);
		return vi;
	}
	vector<int> SearchFaceIndexInVertices(int index)
	{
		vector<int> vi;
		for(vector<LS_Vertex*>::iterator vitr = vertices.begin(); vitr != vertices.end(); vitr++)
			if((*vitr)->SearchFacesIndex(index))
				vi.push_back((*vitr)->m_index);
		return vi;
	}
	vector<int> SearchFaceIndexInEdges(int index)
	{
		vector<int> vi;
		for(vector<LS_Edge*>::iterator eitr = edges.begin(); eitr != edges.end(); eitr++)
			if((*eitr)->SearchVerticesIndex(index))
				vi.push_back((*eitr)->m_index);
		return vi;
	}

	//MARK
	LS_FaceList& GetFaces(){return faces;}
	LS_EdgeList& GetEdges(){return edges;}
	LS_VertexList& GetVertices(){return vertices;}

	void MarkBoundaryVertex()
	{
		LS_Vertex* v0, *v1;
		int faceNum;
		for(int i = 0; i < edges.size(); i++)
		{
			faceNum = 0;
			v0 = edges[i]->vertices[0];
			v1 = edges[i]->vertices[1];
			for(int j = 0; j < v0->faces.size(); j++)
			{
				for(int k = 0; k < 3; k++)
					if(v0->faces[j]->vertices[k]->m_index == v1->m_index)
						faceNum++;
			}
			if(faceNum == 1)
			{
				v0->m_isBoundary = true;
				v1->m_isBoundary = true;
			}
		}
	}

	void PrintBoundary()
	{
		printf("Boundary vertex index:\n");
		for(int i = 0; i < vertices.size(); i++)
			if(vertices[i]->m_isBoundary)
				printf("%d\n", vertices[i]->m_index);
		printf("***********************\n");
	}

//protected:
	LS_Edge*  _AddEdge( LS_Vertex *v1, LS_Vertex *v2 );

	void SubdivideEdge( LS_Edge *e );
	void SubdivideFace( LS_Face *f );
	void RepositionVertex( LS_Vertex *v );

	int skipSpace(char *str)
	{
		for(int i = 0; str[i]; i++)
			if(str[i] == ' ' || str[i] == '\t')continue;
			else return i;
		return -1;
	}

#define EYE

	bool isInFaceIndex(int index)//from 0
	{
		/*int face_index[] = {156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 
			167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 
			181, 182, 183, 190, 191, 198, 199, 200, 201, 202, 203, 204, 205};
		int len = sizeof(face_index) / sizeof(face_index[0]);
		for(int i = 0; i < len; i++)
			if(index == face_index[i])
				return true;*/
#ifdef EYE
		if((index == 190 || index == 191) || (index > 197))
#else
		if((index > 155 && index < 184) || (index == 190 || index == 191) || (index > 197))
#endif
			return true;
		return false;
	}
	int toNewIndex(int index)//from 0
	{
		//return index;
#ifdef EYE
		if(index > 116)
			return index-10;
		if(index > 43)
			return index - 9;
		if(index == 40)
			return index - 6;
		if(index > 4)
			return index - 1;
		return index;
		/*if(index < 108)
			return index;
		return index - 1;*/
#else
		if(index < 59)
			return index;
		if(index < 108)
			return index - 8;
		return index - 9;
#endif
	}
	bool isInVertexIndex(int index)//from 0
	{
		//return false;
		/*int vertex_index[] = {58, 59, 60, 61, 62, 63, 64, 65, 107, 111};
		int len = sizeof(vertex_index) / sizeof(vertex_index[0]);
		for(int i = 0; i < len; i++)
			if(index == vertex_index[i])
				return true;*/
#ifdef EYE
		if((index == 4) || (index > 34 && index < 44 && index != 40) || (index == 120) || (index == 116))
		//if(index == 107 || index == 111)
#else
		if((index > 57 && index < 66) || index == 107 || index == 111)
#endif
			return true;
		return false;
	}

	
//protected:
	LS_VertexList vertices;
	LS_EdgeList edges;
	LS_FaceList faces;

	int subdivisionLevel;

	friend class LS_Vertex;
	friend class LS_Edge;
	friend class LS_Face;

};

#endif // SUBDIVISION_H
