
/* This defines makes assert's compile away to nothing.. :> */
#define NDEBUG 1

#include "stdafx.h"
#include "Subdivision.h"
#include <algorithm>
#include <assert.h>
#include <stdio.h>
//#include "caltime.h"


#if USE_MEM_POOL
	static LS_Vertex mp_vertex[MP_VERTS];
	static LS_Edge mp_edge[MP_EDGES];
	static LS_Face mp_face[MP_FACES];

	static LS_Vertex* mp_curVertex = mp_vertex;
	static LS_Vertex* mp_maxVertex = &mp_vertex[MP_VERTS];
	static LS_Edge* mp_curEdge = mp_edge;
	static LS_Edge* mp_maxEdge = &mp_edge[MP_EDGES];
	static LS_Face* mp_curFace = mp_face;
	static LS_Face* mp_maxFace = &mp_face[MP_FACES];


void* LS_Vertex::operator new(size_t bytes)
{
	LS_Vertex *startVertex( mp_curVertex );

	while( mp_curVertex->mp_inuse == true )
	{
		mp_curVertex++;
		if ( mp_curVertex == mp_maxVertex )
			mp_curVertex = mp_vertex;
		if ( mp_curVertex == startVertex )
		{
			printf("Out of LS_Vertex memory pool!\n");
			exit(-1);
		}
	}
	mp_curVertex->mp_inuse = true;

	return mp_curVertex;
}

void* LS_Edge::operator new(size_t bytes)
{
	LS_Edge *startEdge( mp_curEdge );

	while( mp_curEdge->mp_inuse == true )
	{
		mp_curEdge++;
		if ( mp_curEdge == mp_maxEdge )
			mp_curEdge = mp_edge;
		if ( mp_curEdge == startEdge )
		{
			printf("Out of LS_Edge memory pool!\n");
			exit(-1);
		}
	}
	mp_curEdge->mp_inuse = true;

	return mp_curEdge;
}

void* LS_Face::operator new(size_t bytes)
{
	LS_Face *startFace( mp_curFace );

	while( mp_curFace->mp_inuse == true )
	{
		mp_curFace++;
		if ( mp_curFace == mp_maxFace )
			mp_curFace = mp_face;
		if ( mp_curFace == startFace )
		{
			printf("Out of LS_Face memory pool!\n");
			exit(-1);
		}
	}
	mp_curFace->mp_inuse = true;

	return mp_curFace;
}

void LS_Vertex::operator delete(void* p)
{
	((LS_Vertex*)p)->mp_inuse = false;
}
void LS_Edge::operator delete(void* p)
{
	((LS_Edge*)p)->mp_inuse = false;
}
void LS_Face::operator delete(void* p)
{
	((LS_Face*)p)->mp_inuse = false;
}
#endif

/*******************************************************************************************
 *
 * LS_Vertex
 *
 *******************************************************************************************/

bool operator!=( LS_Edge*& e, const LS_VertexPair &vp )
{
	return ( !( ( e->vertices[0] == vp.first || e->vertices[0] == vp.second )
		&& ( e->vertices[1] == vp.first || e->vertices[1] == vp.second ) ) );
}

bool operator==( LS_Edge*& e, const LS_VertexPair &vp )
{
	return ( ( e->vertices[0] == vp.first || e->vertices[0] == vp.second )
		&& ( e->vertices[1] == vp.first || e->vertices[1] == vp.second ) );
}

LS_Edge** LS_Vertex::GetEdgePointer( LS_Edge *ptr )
{
	for( int i = 0; i < edges.size(); i++ )
		if ( edges[i] == ptr )
			return &edges[i];
	printf("LS_Vertex::GetEdgePointer() goes wrong\n");
	assert(false);
}

LS_Face** LS_Vertex::GetFacePointer( LS_Face *ptr )
{
	for( int i = 0; i < faces.size(); i++ )
		if ( faces[i] == ptr )
			return &faces[i];
	printf("LS_Vertex::GetFacePointer() goes wrong\n");
	assert(false);
}

LS_Vertex* LS_Vertex::GetNewVertexInEdge( LS_Vertex *v )
{
	for( int i = 0; i < edges.size(); i++ )
	{	
		if ( edges[i]->oldVertex == v )
			return ((edges[i]->vertices[1] == this) ? edges[i]->vertices[0] : edges[i]->vertices[1]);
	}
	printf("LS_Vertex::GetNewVertexInEage() goes wrong\n");
	assert(false);
}

void LS_Vertex::UpdateNormal()
{
	//normal.Clear();
	normal.clear();
	for( int i = 0; i < faces.size(); i++ )
		normal += faces[i]->normal;
	normal.normalizeSelf();
	//normal.Normalize();
}


LS_Edge* LS_Vertex::GetEdgeOfVertex(LS_Vertex* v)
{
	for(vector<LS_Edge*>::iterator eitr = edges.begin(); eitr != edges.end(); eitr++)
		if((*eitr)->vertices[0] == v || (*eitr)->vertices[1] == v)
			return (*eitr);
	return NULL;
}

/*******************************************************************************************
 *
 * LS_Surface
 *
 *******************************************************************************************/

LS_Surface::~LS_Surface()
{
	Reset();
	vertices.clear();
	edges.clear();
	faces.clear();
}

void LS_Surface::Reset()
{
	// Free up allocated memory and clear the lists

	LS_VertexList::iterator vertex_itr = vertices.begin();
	while( vertex_itr != vertices.end() )
	{
		delete *vertex_itr;
		vertex_itr++;
	}
	vertices.clear();
	
	LS_EdgeList::iterator edge_itr = edges.begin();
	while( edge_itr != edges.end() )
	{
		delete *edge_itr;
		edge_itr++;
	}
	edges.clear();

	LS_FaceList::iterator face_itr = faces.begin();
	while( face_itr != faces.end() )
	{
		delete *face_itr;
		face_itr++;
	}
	faces.clear();

	subdivisionLevel = 0;
#if USE_MEM_POOL
	vertices.reserve( MP_VERTS );
	edges.reserve( MP_EDGES );
	faces.reserve( MP_FACES );
#endif
}

LS_Edge* LS_Surface::_AddEdge( LS_Vertex *v1, LS_Vertex *v2 )
{
	// Make sure these are distinct vertices
	//assert( (v1 != v2) && (v1 >= 0 && v1 < vertices.size()) && (v2 >= 0 && v2 < vertices.size()) );
	//assert( (v1 != v2) && (v1->index > 0 && v1->index <= vertices.size()) && (v2->index > 0 && v2->index <= vertices.size()) );

	// Make sure the end points are in the list
	//assert ( edges.end() == find( edges.begin(), edges.end(), LS_VertexPair( v1, v2 ) ) );

	LS_Edge *newEdge = new LS_Edge;
	assert( newEdge );
	newEdge->vertices[0] = v1;
	newEdge->vertices[1] = v2;

	// Increment the vertex edge counter
	v1->edges.push_back(newEdge);
	v2->edges.push_back(newEdge);

	edges.push_back( newEdge );

	newEdge->m_index = edges.size();

	return newEdge;
}

void LS_Surface::SubdivideEdge( LS_Edge *e )
{
	LS_Vertex *newVertex = new LS_Vertex;
	assert( newVertex );
	LS_Edge *newEdge = new LS_Edge;
	assert( newEdge );

	// Add these new objects to the lists
	edges.push_back( newEdge );
	vertices.push_back( newVertex );

	newEdge->m_index = edges.size();
	newVertex->m_index = vertices.size();

	// Set the creationLevel of the new vertex
	newVertex->creationLevel = subdivisionLevel;

	// Set the position of the new vertex (lerp) 
	newVertex->pos = e->vertices[0]->pos 
			+ ( e->vertices[1]->pos - e->vertices[0]->pos ) / 2;

	newVertex->m_isBoundary = (e->vertices[0]->m_isBoundary && e->vertices[1]->m_isBoundary);

	//keep the same direction

	// Set the texel of the new vertex (lerp)
	//newVertex->texel = e->vertices[0]->texel
			//+ ( e->vertices[1]->texel - e->vertices[0]->texel ) / 2;

	// Set the new edge to point to the correct vertices
	newEdge->vertices[0] = newVertex;
	newEdge->vertices[1] = e->vertices[1];
	newEdge->oldVertex = e->vertices[0];
	// Save the vertex that this edge used to connect to
	e->oldVertex = e->vertices[1];
	// Set the old edge to point to the next vertex
	e->vertices[1] = newVertex;

	// Set the new vertex's edge pointers
	newVertex->edges.push_back(e);
	newVertex->edges.push_back(newEdge);
	
	// Set the other vertex's edge pointer
	//make one point[0] point to origin edge, the other point[1] point to the new edge
	LS_Edge **e_ptr = NULL;
	e_ptr = newEdge->vertices[1]->GetEdgePointer( e );
	*e_ptr = newEdge;
}

void LS_Surface::SubdivideFace( LS_Face *f )
{
	// Store the original face pointer
	LS_Face *origFace = f;

	// Find the new vertices that were created inside this face
	LS_Vertex *v1 = NULL, *v2 = NULL, *v3 = NULL;
	v1 = f->vertices[0]->GetNewVertexInEdge( f->vertices[1] );
	//if ( v1 == f->vertices[0] )
	//{
	//	v1 = f->vertices[1]->GetNewVertexInEdge( f->vertices[0] );
	//	assert( v1 );
	//}
	v2 = f->vertices[1]->GetNewVertexInEdge( f->vertices[2] );
	//if ( v2 == f->vertices[1] )
	//{
	//	v2 = f->vertices[2]->GetNewVertexInEdge( f->vertices[1] );
	//	assert( v2 );
	//}
	v3 = f->vertices[2]->GetNewVertexInEdge( f->vertices[0] );
	//if ( v3 == f->vertices[2] )
	//{
	//	v3 = f->vertices[0]->GetNewVertexInEdge( f->vertices[2] );
	//	assert( v3 );
	//}

	// Store the original face corners
	LS_Vertex *c1, *c2, *c3;
	c1 = f->vertices[0];
	c2 = f->vertices[1];
	c3 = f->vertices[2];

	//assert( *c1->GetFacePointer( origFace ) == origFace );
	//assert( *c2->GetFacePointer( origFace ) == origFace );
	//assert( *c3->GetFacePointer( origFace ) == origFace );
	
	// Create the new face (c1, v1, v3)
	f->vertices[0] = c1;
	f->vertices[1] = v1;
	f->vertices[2] = v3;
	//f->UpdateCentroid();
	//UNMARK
	//_AddEdge( v1, v3);
	//MARK
	f->edges[0] = _AddEdge( v1, v3 );
	//f->edges[1] = c1->GetEdgeOfVertex(v1);
	//f->edges[2] = c1->GetEdgeOfVertex(v3);
	// Fix the face pointers
	v1->faces.push_back(f);
	v3->faces.push_back(f);
	
	// Create the new face (v1, c2, v2)
	f = new LS_Face;
	assert( f );
	faces.push_back( f );
	
	f->m_index = faces.size();

	f->vertices[0] = v1;
	f->vertices[1] = c2;
	f->vertices[2] = v2;
	//f->UpdateCentroid();
	//UNMARK
	//_AddEdge( v1, v2 );
	//MARK
	f->edges[0] = _AddEdge( v1, v2 );
	//f->edges[1] = c2->GetEdgeOfVertex(v1);
	//f->edges[2] = c2->GetEdgeOfVertex(v2);

	// Fix the face pointers
	v1->faces.push_back(f);
	v2->faces.push_back(f);

	*c2->GetFacePointer( origFace ) = f;

	// Create the new face (v2, c3, v3)
	f = new LS_Face;
	assert( f );
	faces.push_back( f );

	f->m_index = faces.size();

	f->vertices[0] = v2;
	f->vertices[1] = c3;
	f->vertices[2] = v3;
	//f->UpdateCentroid();
	//UNMARK
	//_AddEdge( v2, v3 );
	//MARK
	f->edges[0] = _AddEdge( v2, v3 );
	//f->edges[1] = c3->GetEdgeOfVertex(v2);
	//f->edges[2] = c3->GetEdgeOfVertex(v3);
	// Fix the face pointers
	v2->faces.push_back(f);
	v3->faces.push_back(f);
	
	*c3->GetFacePointer( origFace ) = f;

	// Create the new face (v1, v2, v3)
	f = new LS_Face;
	assert( f );
	faces.push_back( f );

	f->m_index = faces.size();

	f->vertices[0] = v1;
	f->vertices[1] = v2;
	f->vertices[2] = v3;
	//f->UpdateCentroid();
	// Fix the face pointers
	//MARK
	//f->edges[0] = v1->GetEdgeOfVertex(v2);
	//f->edges[1] = v2->GetEdgeOfVertex(v3);
	//f->edges[2] = v3->GetEdgeOfVertex(v1);
	v1->faces.push_back(f);
	v2->faces.push_back(f);
	v3->faces.push_back(f);
}

#define BOUNDARY

void LS_Surface::RepositionVertex( LS_Vertex *v )
{
	assert( v->faces.size() > 0 );
	//LSVector cf;
	Vector3D cf;

	double selfweight;
	double neighbourweight;
#ifdef BOUNDARY
	if(v->m_isBoundary)
	{
		selfweight = 0.6;
		neighbourweight = 0.2;
	}
	else
	{
		selfweight = 0.25;
		neighbourweight = 0.375;
	}
#else
	selfweight = 0.25;
	neighbourweight = 0.375;
#endif

	//cf = v->pos.ToVector() * ( v->faces.size() * selfweight );
	cf = v->pos * ( v->faces.size() * selfweight );
	for( int i = 0; i < v->faces.size(); i++ )
	{
		int curFaceVert = 0;
		if ( v->faces[i]->vertices[ curFaceVert ] == v )
			curFaceVert++;
		//cf += v->faces[i]->vertices[ curFaceVert ]->pos.ToVector() * neighbourweight;
		cf += v->faces[i]->vertices[ curFaceVert ]->pos * neighbourweight;

		curFaceVert++;
		if ( v->faces[i]->vertices[ curFaceVert ] == v )
			curFaceVert++;
		//cf += v->faces[i]->vertices[ curFaceVert ]->pos.ToVector() * neighbourweight;
		cf += v->faces[i]->vertices[ curFaceVert ]->pos * neighbourweight;
	}

	cf = cf / (double)v->faces.size();
	v->newPos = cf;
	//v->newPos = cf.ToPoint();
}

void LS_Surface::Subdivide()
{
	subdivisionLevel++;
	// Linear subdivision of edges
	LS_EdgeList::iterator edge_itr = edges.begin();
	int numEdges = edges.size();
	int numVertices = vertices.size();
	//TimeHandle t = TimeBegin();
	for( int i = 0; i < numEdges; i++, edge_itr++ )
	{
		// Only subdivide old edges (ones not attached to a new vertex)
		if ( (*edge_itr)->vertices[0]->creationLevel != subdivisionLevel
					&& (*edge_itr)->vertices[1]->creationLevel != subdivisionLevel )
			SubdivideEdge( *edge_itr );
	}
	//printf("Subdivide Edge Time = %lf\n", TimeSlice(t));
	// Create the new faces / Connect the new vertices
	LS_FaceList::iterator face_itr = faces.begin();
	int orinumfaces = faces.size();
	//t = TimeBegin();
	for( int i = 0; i < orinumfaces; i++, face_itr++ )
	{
		// Connect the new vertices into faces
		SubdivideFace( *face_itr );
	}
	//printf("Subdivide Face Time = %lf\n", TimeSlice(t));
	// Reposition new vertices
	LS_VertexList::iterator vertex_itr = vertices.begin();
	//t = TimeBegin();
	for( int i = 0; i < vertices.size(); i++, vertex_itr++ )
	{
		RepositionVertex( *vertex_itr );
	}
	//printf("Reposition Time = %lf\n", TimeSlice(t));
	vertex_itr = vertices.begin();
	//t = TimeBegin();
	for( int i = 0; i < vertices.size(); i++, vertex_itr++ )
		(*vertex_itr)->pos = (*vertex_itr)->newPos;
	//printf("Other Time = %lf\n", TimeSlice(t));
	//UpdateNormals();
}

void LS_Surface::UpdateNormals()
{
	LS_FaceList::iterator face_itr = faces.begin();
	while( face_itr != faces.end() )
	{
		(*face_itr)->UpdateNormal();
		face_itr++;
	}

	LS_VertexList::iterator vertex_itr = vertices.begin();
	while( vertex_itr != vertices.end() )
	{
		(*vertex_itr)->UpdateNormal();
		vertex_itr++;
	}
}


void LS_Surface::WriteObj(const char* filename, bool flag)
{
#ifdef _CRT_SECURE_NO_WARNINGS
	FILE* fp = fopen(filename, "r");
#else
	FILE* fp;
	fopen_s(&fp, filename, "w");
#endif
	fprintf(fp, "# loop\n");
	int i = 1;
	for(LS_VertexList::iterator vitr = vertices.begin(); vitr != vertices.end(); vitr++, i++)
	{
		//fprintf(fp, "v %f %f %f\n", (*vitr)->pos.x, (*vitr)->pos.y, flag ? -(*vitr)->pos.z : (*vitr)->pos.z);
		fprintf(fp, "v %f %f %f\n", (*vitr)->pos[0], (*vitr)->pos[1], flag ? -(*vitr)->pos[2] : (*vitr)->pos[2]);
	}
	for(LS_FaceList::iterator fitr = faces.begin(); fitr != faces.end(); fitr++)
	{
		if(flag)
			fprintf(fp, "f %d %d %d\n", (*fitr)->vertices[0]->m_index, (*fitr)->vertices[2]->m_index, (*fitr)->vertices[1]->m_index);
		else
			fprintf(fp, "f %d %d %d\n", (*fitr)->vertices[0]->m_index, (*fitr)->vertices[1]->m_index, (*fitr)->vertices[2]->m_index);
	}
	fclose(fp);
}

void LS_Surface::ReadObj(const char* filename, float scale)
{
	Reset();
#ifdef _CRT_SECURE_NO_WARNINGS
	FILE* fp = fopen(filename, "r");
	int v_index = 0, f_index = 0;
#else
	FILE* fp;
	fopen_s(&fp, filename, "r");
	int v_index = 0, f_index = 0;
#endif
	if(!fp)
	{
		printf("open %s failed\n", filename);
		exit(0);
	}
	char str[256];
	while(!feof(fp))
	{
		fgets(str, 255, fp);
		int i = skipSpace(str);
		if(i == -1 || str[i] == '\n' || str[i] == '#')
			continue;
		if(str[i] == 'v')
		{
			//LSPoint p;
			v_index++;
			if(isInVertexIndex(v_index-1))
				continue;
			LS_Vertex* v = new LS_Vertex();
#ifdef _CRT_SECURE_NO_WARNINGS
			sscanf(str+i+1, "%lf%lf%lf", &v->pos[0], &v->pos[1], &v->pos[2]);
#else
			sscanf_s(str+i+1, "%lf%lf%lf", &v->pos[0], &v->pos[1], &v->pos[2]);
#endif
			v->pos[0] *= scale;
			v->pos[1] *= scale;
			v->pos[2] *= scale;

			vertices.push_back(v);
			v->creationLevel = subdivisionLevel;
			v->m_index = vertices.size();
		}
		else if(str[i] = 'f')
		{
			f_index++;
			if(isInFaceIndex(f_index-1))
				continue;
			int v1, v2, v3;
#ifdef _CRT_SECURE_NO_WARNINGS
			sscanf(str+i+1, "%d%d%d", &v1, &v2, &v3);
#else
			sscanf_s(str+i+1, "%d%d%d", &v1, &v2, &v3);
#endif
			AddFaceOfIndex(toNewIndex(v1-1), toNewIndex(v2-1), toNewIndex(v3-1));
			//AddFaceOfIndex(v1-1, v2-1, v3-1);
		}
	}
	fclose(fp);
}

void LS_Surface::AddFaceOfIndex(int i1, int i2, int i3)
{
	LS_Vertex *v1 = vertices[i1], *v2 = vertices[i2], *v3 = vertices[i3];
	LS_VertexList::iterator v_itr;
	LS_Edge *e1, *e2, *e3;
	LS_EdgeList::iterator e_itr;

	// Find the edges in the list, or insert them
	//maybe no need
	if ( edges.end() == 
				( e_itr = find( edges.begin(), edges.end(), LS_VertexPair( v1, v2 ) ) ) )
		e1 = _AddEdge( v1, v2 );
	else
		e1 = *e_itr;
	if ( edges.end() == 
				( e_itr = find( edges.begin(), edges.end(), LS_VertexPair( v2, v3 ) ) ) )
		e2 = _AddEdge( v2, v3 );
	else
		e2 = *e_itr;
	if ( edges.end() == 
				( e_itr = find( edges.begin(), edges.end(), LS_VertexPair( v3, v1 ) ) ) )
		e3 = _AddEdge( v3, v1 );
	else
		e3 = *e_itr;

	LS_Face *newFace = new LS_Face;
	assert( newFace );
	newFace->vertices[0] = v1;
	newFace->vertices[1] = v2;
	newFace->vertices[2] = v3;
	newFace->edges[0] = e1;
	newFace->edges[1] = e2;
	newFace->edges[2] = e3;
	newFace->UpdateCentroid();
	faces.push_back( newFace );

	newFace->m_index = faces.size();

	// Calculate the face normal
	//LSVector normal;
	Vector3D normal;
	//normal = ( v2->pos - v1->pos ).Cross( v3->pos - v1->pos );
	normal = ( v2->pos - v1->pos )%( v3->pos - v1->pos );
	normal.normalizeSelf();

	//normal.Normalize();
	
	// Set the face normals
	v1->normal = normal;
	v2->normal = normal;
	v3->normal = normal;

	v1->faces.push_back(newFace);
	v2->faces.push_back(newFace);
	v3->faces.push_back(newFace);
}


