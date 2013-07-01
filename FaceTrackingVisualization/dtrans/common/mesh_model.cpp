#include "stdafx.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <string>
#include "mesh_model.h"

#include <GL/glut.h>

dtMeshModel::dtMeshModel() :m_vertex(NULL), m_normvec(NULL), 
	m_triangle(NULL), m_n_vertex(0), m_n_normvec(0), m_n_triangle(0)
{

}

/*
   CreateMeshModel allocates memory space for model structure according to 
   the model size specified in model->n_vertex, model->n_normvec, model->
   n_triangle, and initializes pointers to vertex, norm vector and triangle 
   arrays.

   You MUST fill in n_vertex, n_normvec and n_triangle before calling 
   CreateMeshModel().
 */
void dtMeshModel::CreateMeshModel()
{
    size_t
        vertex_siz   = sizeof(dtVertex)   * (size_t)m_n_vertex,
        normvec_siz  = sizeof(dtVector)   * (size_t)m_n_normvec,
        triangle_siz = sizeof(dtTriangle) * (size_t)m_n_triangle;

    size_t total_mem_siz = vertex_siz + normvec_siz + triangle_siz;
    //char *mem = (char*)__dt_malloc(total_mem_siz);
	char *mem = (char*)_malloca(total_mem_siz);

    /* Initialize pointers to vector, normvec and triangle arrays

       MeshModel data storage layout:
       [mem]=>[---vertex---|---norm_vector---|------triangle------]
    */
    m_vertex   = (dtVertex*)  (mem);
    m_normvec  = (dtVector*)  (mem + vertex_siz);
    m_triangle = (dtTriangle*)(mem + vertex_siz + normvec_siz);
}

/* DestroyMeshModel frees all memory space allocated in CreateMeshModel */
void dtMeshModel::DestroyMeshModel()
{
	if(m_vertex)
    {
		//free(m_vertex);  /* we just need to free once and ONLY once */
		_freea(m_vertex);
		m_vertex = NULL;
	}
}


/* Read .obj model file into model object, if reading file failed, it would 
   simply print an error message to stderr and crash. */
void dtMeshModel::ReadObjFileCommitOrCrash(const char *filename)
{
    int ret;
    if ((ret = ReadObjFile(filename)) != 0)
    {
        fprintf(stderr, "file: %s - ", filename);
        if (ret == -1) {
            perror("Reading model file error");
        }
        else {
            fprintf(stderr, "Syntax error on line: %u\n", ret);
        }
        exit(-1);
    }
}


/* Sort out a list containing the index of normal vectors of each vertex in the
   model, this list might be helpful in closest point iteration. 

   You have to manually free the pointer returned by this function after using 
   it. */
dt_index_type *dtMeshModel::SortOutVertexNormalList()
{
    dtTriangle   *triangle;
    dt_index_type i_vertex, i_normvec;
    dt_index_type i_triangle, iv; /* looping variable, we need to inspect every
                                     triangle unit to sort out vertex normal 
                                     informations. */
    dt_index_type *inorm_list = (dt_index_type*)__dt_malloc(
        (size_t)m_n_vertex * sizeof(dt_index_type));

    /* An initial guess of vertex normals, bullet proof for isolated vertices */
    memset(inorm_list, 0, (size_t)m_n_vertex * sizeof(dt_index_type));

    for (i_triangle = 0; i_triangle < m_n_triangle; i_triangle++)
    {
        triangle = m_triangle + i_triangle; /* inspect this one */
        for (iv = 0; iv < 3; iv++)
        {
            /* get the iv-th local vertex on this triangle unit */
            i_vertex  = triangle->i_vertex[iv];
            i_normvec = triangle->i_norm[iv];
   
            inorm_list[i_vertex] = i_normvec;  /* log it */
        }
    }

    return inorm_list;
}


/* ReadObjFile parse specified .obj model description file and read vertex, 
   normal vector and triangular surface information into *model

   This function returns 0 on success, otherwise it would return an -1 to 
   indicate an fopen() error (file not exist, privillage or some reason).
*/
int dtMeshModel::ReadObjFile(const char *filename)
{
    char pref[3];   /* we need only 2 characters to identify the prefixes */

    dtVertex *vertex; dtVector *normvec; dtTriangle *triangle;
    dt_index_type i_lv;

    FILE *fd = _GetMeshModelScale(
        filename, &m_n_vertex, &m_n_normvec, &m_n_triangle);

	bool vnflag = (m_n_normvec != 0);
	if(!vnflag)
		m_n_normvec = m_n_vertex;
	
	m_filename = filename;

    if (fd)
    {
        /* allocate memory space for mesh model structure */
        CreateMeshModel();
        vertex   = m_vertex;
        normvec  = m_normvec;
        triangle = m_triangle;

        /* parse .obj file */
#ifdef _CRT_SECURE_NO_WARNINGS
        while (fscanf(fd, "%2s", pref) != EOF)
#else
		while (fscanf_s(fd, "%2s", pref) != EOF)
#endif
        {
            if (strcmp(pref, "v") == 0)        /* vertex */
            {
#ifdef _CRT_SECURE_NO_WARNINGS
                fscanf(fd, "%lf %lf %lf", 
                    &(vertex->x), &(vertex->y), &(vertex->z));
#else
				fscanf_s(fd, "%lf %lf %lf", 
					&(vertex->x), &(vertex->y), &(vertex->z));
#endif
                vertex++;
            }
            else if (strcmp(pref, "vn") == 0)  /* norm vector */
            {
#ifdef _CRT_SECURE_NO_WARNINGS
                fscanf(fd, "%lf %lf %lf",
                    &(normvec->x), &(normvec->y), &(normvec->z));
#else
				fscanf_s(fd, "%lf %lf %lf",
					&(normvec->x), &(normvec->y), &(normvec->z));
#endif
                normvec++;
            }
            else if (strcmp(pref, "f") == 0)   /* triangle */
            {
                /* vertex/normvec indexes in .obj file are one-based, we have 
                   to convert them to zero-based array indexes latter */

                /* fscanf(fd, "%d//%d %d//%d %d//%d", */
                /*     &(triangle->i_vertex[0]), &(triangle->i_norm[0]), */
                /*     &(triangle->i_vertex[1]), &(triangle->i_norm[1]), */
                /*     &(triangle->i_vertex[2]), &(triangle->i_norm[2])); */
                for (i_lv = 0; i_lv < 3; i_lv++)
                {
					if(vnflag)
					{
						fscanf(fd, "%d/", &(triangle->i_vertex[i_lv]));
						if ((pref[0] = (char)fgetc(fd)) != '/') {
							ungetc(pref[0], fd);  fscanf(fd, "%*d/");
						}
						fscanf(fd, "%d", &(triangle->i_norm[i_lv]));
					}
					else
					{
						fscanf(fd, "%d", &(triangle->i_vertex[i_lv]));
					}
                }

                /* one-based .obj file index => zero-based array index */
				triangle->i_vertex[0]--; 
				triangle->i_vertex[1]--; 
				triangle->i_vertex[2]--; 
				if(vnflag)
				{
					triangle->i_norm[0]--;
					triangle->i_norm[1]--;
					triangle->i_norm[2]--;
				}
				else
				{
					triangle->i_norm[0] = triangle->i_vertex[0];
					triangle->i_norm[1] = triangle->i_vertex[1];
					triangle->i_norm[2] = triangle->i_vertex[2];
				}
                triangle++;
            }
            else if (*pref == '#')             /* comment line */
                _SkipThisLine(fd);

            else  _SkipThisLine(fd);        /* ignore other informations */
        }

        fclose(fd);
		if(!vnflag)
			ComputeVertexNormals();
        return 0;
    }
    else {
        return -1; /* fopen() error propagated from __get_mesh_model_scale() */
    }
}


/* SaveObjFile saves specified mesh model to a file, it would return 0 on 
   success, otherwise it would return -1 to indicate fopen() was failed.
   You should check system variable errno for further investigation.
*/
int dtMeshModel::SaveObjFile(const char *filename)
{
    const  dtVertex   *vertex   = m_vertex;
    const  dtVector   *normvec  = m_normvec;
    const  dtTriangle *triangle = m_triangle;
    dt_index_type ind;

    FILE *fd = fopen(filename, "w");
    if (fd != NULL)
    {
        /* save vertex information */
        for (ind = 0; ind < m_n_vertex; ind++, vertex++) {
            fprintf(fd, "v   %12.9f   %12.9f   %12.9f\n", 
                vertex->x, vertex->y, vertex->z);
        }
    
        /* save normal vector information */
        for (ind = 0; ind < m_n_normvec; ind++, normvec++) {
            fprintf(fd, "vn   %12.9f   %12.9f   %12.9f\n", 
                normvec->x, normvec->y, normvec->z);
        }
        /* save triangular unit information 
           
           Notice that vertex/normvec indexes in .obj file are one-based, so
           they need a incrementation before being written to filestream. */
        for (ind = 0; ind < m_n_triangle; ind++, triangle++)
        {
            fprintf(fd, "f %d//%d %d//%d %d//%d\n",
                triangle->i_vertex[0] + 1, triangle->i_norm[0] + 1,
                triangle->i_vertex[1] + 1, triangle->i_norm[1] + 1,
                triangle->i_vertex[2] + 1, triangle->i_norm[2] + 1);
        }

        fclose(fd);
        return 0;
    }
    else {
        return -1;
    }
}

int dtMeshModel::ExportObjFile(const char* filename)
{
	std::string str = m_filename;
	str = std::string(str, 0, str.find_last_of('.'));
	str += filename;
	str += ".obj";
	return SaveObjFile(str.c_str());
}

/* Skip current line / jump to the start of the next line */
void dtMeshModel::_SkipThisLine(FILE *fd)
{
    /* **skip all characters until encountering an '\n'.**

       It might be **PAINFULLY SLOW**, but using fgets would mess up the code 
       because you have to make sure that the entire line has been eventually 
       read into the temporary buffer.
    */
    int ret;
    while ((ret = fgetc(fd)) != '\n' && ret != EOF);
}

/* Count how many vertices, normal vectors and triangle surfaces were logged in
   the .obj file. 

   This function returns the file pointer of the .obj file on success, or it
   would return NULL to indicate an fopen() error. the user should check errno 
   to investigate why it failed.
 */
FILE *dtMeshModel::_GetMeshModelScale(const char *filename, 
    dt_size_type *n_vertex, dt_size_type *n_normvec, 
	dt_size_type *n_triangle)
{
    char pref[3];  /* we need only 2 characters to identify the prefixes */

    FILE *fd = fopen(filename, "r");
    if (fd)
    {
        *n_vertex = *n_normvec = *n_triangle = 0;

        /* inspect the prefix of each line:
           #:comment   v:vertex   vn:norm vector   f:triangle */
        while (fscanf(fd, "%2s", pref) != EOF)
        {
            if      (strcmp(pref, "v")  == 0)  *n_vertex   += 1;
            else if (strcmp(pref, "vn") == 0)  *n_normvec  += 1;
            else if (strcmp(pref, "f")  == 0)  *n_triangle += 1;
            /* else if (*pref == '#');  <comment line>       */
            /* else:                    <unexpected prefix>  */

            _SkipThisLine(fd);
        }

        /* reset file pointer to the beginning, bcos ReadObjFile() will make a
           second pass of the file and read the data into model structure. */
        fseek(fd, SEEK_SET, 0);
    }
    else {
        return NULL;   /* could not open file */
    }

    return fd;
}

void dtMeshModel::ComputeVertexNormals()
{
	memset(m_normvec, 0, sizeof(dtVector)*m_n_normvec);
	for(int i = 0; i < m_n_triangle; i++)
	{
		int index[3];
		for(int j = 0; j < 3; j++)
			index[j] = m_triangle[i].i_vertex[j];
		/*cross(m_vertices[m_triangle[i].i_vertex[1]]-m_verticexs[m_triangle[i].i_vertex[0]],
				m_vertices[m_triangle[i].i_vertex[2]]-m_verticexs[m_triangle[i].i_vertex[0]])*/
		dtVector& v0 = m_vertex[m_triangle[i].i_vertex[0]];
		dtVector& v1 = m_vertex[m_triangle[i].i_vertex[1]];
		dtVector& v2 = m_vertex[m_triangle[i].i_vertex[2]];
		
		dt_real_type x = ((v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y));
		dt_real_type y = ((v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z));
		dt_real_type z  =((v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x));

		dt_real_type len = sqrt(x*x+y*y+z*z);

		x /= len;
		y /= len;
		z /= len;

		for(int j = 0; j < 3; j++)
		{
			m_normvec[index[j]].x += x;
			m_normvec[index[j]].y += y;
			m_normvec[index[j]].z += z;
		}
		//cross done
	}

	//normalize()
	for(int i = 0; i < m_n_normvec; i++)
	{
		dt_real_type x = m_normvec[i].x;
		dt_real_type y = m_normvec[i].y;
		dt_real_type z = m_normvec[i].z;
		dt_real_type len = sqrt(x*x+y*y+z*z);
		m_normvec[i].x /= len;
		m_normvec[i].y /= len;
		m_normvec[i].z /= len;
	}
}

int dtMeshModel::GetIndexOfVertex(const dtVertex* v)
{
	int index = -1;
	for(int i = 0; i < m_n_vertex; i++)
		if(m_vertex[i].isEqual(v))
			index = i;
	return index;
}

void dtMeshModel::DrawMeshModel(bool wired)
{
	dt_index_type i_triangle;

	const  dtVertex   *vertex   = m_vertex;
	const  dtVector   *normvec  = m_normvec;
	const  dtTriangle *triangle = m_triangle;

	if(wired)
	{
		for (i_triangle = 0; i_triangle < m_n_triangle; 
			i_triangle++, triangle++)
		{
			glBegin(GL_LINE_LOOP);
			glNormal3dv((GLdouble*)(normvec + triangle->i_norm[0]));
			glVertex3dv((GLdouble*)(vertex  + triangle->i_vertex[0]));
			glNormal3dv((GLdouble*)(normvec + triangle->i_norm[1]));
			glVertex3dv((GLdouble*)(vertex  + triangle->i_vertex[1]));
			glNormal3dv((GLdouble*)(normvec + triangle->i_norm[2]));
			glVertex3dv((GLdouble*)(vertex  + triangle->i_vertex[2]));
			glEnd();
		}
	}
	else
	{
		glBegin(GL_TRIANGLES);
		for (i_triangle = 0; i_triangle < m_n_triangle; 
			i_triangle++, triangle++)
		{
			glNormal3dv((GLdouble*)(normvec + triangle->i_norm[0]));
			glVertex3dv((GLdouble*)(vertex  + triangle->i_vertex[0]));
			glNormal3dv((GLdouble*)(normvec + triangle->i_norm[1]));
			glVertex3dv((GLdouble*)(vertex  + triangle->i_vertex[1]));
			glNormal3dv((GLdouble*)(normvec + triangle->i_norm[2]));
			glVertex3dv((GLdouble*)(vertex  + triangle->i_vertex[2]));
		}
		glEnd();
	}
	glFlush();
}

void dtMeshModel::CalculateModelPosCorrection(double *cx, double *cy, double *cz)
{
	const dtVertex *vertex = m_vertex;
	GLdouble 
		xmax = vertex->x, ymax = vertex->y, zmax = vertex->z,
		xmin = vertex->x, ymin = vertex->y, zmin = vertex->z;

	dt_index_type i = 1;
	for ( ; i < m_n_vertex; i++)
	{
		xmax = xmax < vertex[i].x? vertex[i].x: xmax;
		ymax = ymax < vertex[i].y? vertex[i].y: ymax;
		zmax = zmax < vertex[i].z? vertex[i].z: zmax;

		xmin = xmin > vertex[i].x? vertex[i].x: xmin;
		ymin = ymin > vertex[i].y? vertex[i].y: ymin;
		zmin = zmin > vertex[i].z? vertex[i].z: zmin;
	}

	*cx = -0.5 * (xmin + xmax);
	*cy = -0.5 * (ymin + ymax);
	*cz = -0.5 * (zmin + zmax);
}