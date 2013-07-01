#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include "triangle_corr.h"


/* Create an triangle correspondence list containing no entries, but has some
   reserved space for coming elements */
void TriangleCorrsList::CreateEmptyTriangleCorrsList()
{
    m_list_length   = 0;
    m_list_capacity = 8;   /* reserve space for coming entries */

    /* allocate for reserved space (capacity) */
    m_corr = (dtTriangleCorrsEntry*)__dt_malloc(
        (size_t)m_list_capacity * sizeof(dtTriangleCorrsEntry));
}

/* Create an triangle correspondence list with specified length, no additional
   space reserved for appending new items */
void TriangleCorrsList::CreateTriangleCorrsList(dt_size_type length)
{
    if (length == 0) {
        /* create an empty list with some reserved space */
        CreateEmptyTriangleCorrsList();
    }
    else
    {
        m_list_length   = length;
        m_list_capacity = length;  /* no additional space reserved */
        m_corr = (dtTriangleCorrsEntry*)
            __dt_malloc((size_t)length * sizeof(dtTriangleCorrsEntry));
    }
}

/* Free up the triangle correspondence list */
void TriangleCorrsList::DestroyTriangleCorrsList()
{
	if(m_corr)
		free(m_corr);
}

/* Append a triangle correspondence entry to the tail of the list */
void TriangleCorrsList::AppendTriangleCorrsEntry(const dtTriangleCorrsEntry *entry)
{
    /* do we need to expand space? */
    if ((m_list_length) == m_list_capacity)
    {
        m_list_capacity *= 2;  /* space grow exponentially */
        m_corr = (dtTriangleCorrsEntry*)
            realloc(m_corr, (size_t)m_list_capacity * 
                sizeof(dtTriangleCorrsEntry));
    }

    m_corr[m_list_length++] = *entry;
}

/* Routine for qsort to compare 2 triangle correspondence entries. We want to 
   sort i_tgt_triangle to ascending order while entries with small centroid 
   distances should go in front, which would benefit the correspondence entry
   filtering procedure (__dt_StripTriangleCorrsList). */
int TriangleCorrsList::TricorrsEntryCompare(const void *_e0, const void *_e1)
{
    const dtTriangleCorrsEntry *e0 = (dtTriangleCorrsEntry*)_e0;
    const dtTriangleCorrsEntry *e1 = (dtTriangleCorrsEntry*)_e1;

    /* the comparision rule is quite similar with lexicalgraphical ordering */
    if      (e0->i_tgt_triangle < e1->i_tgt_triangle)   return -1;
    else if (e0->i_tgt_triangle > e1->i_tgt_triangle)   return 1;
    else
    {
        if      (e0->dist_sq < e1->dist_sq)  return -1;
        else if (e0->dist_sq > e1->dist_sq)  return 1;
        else {
            return  (e0->i_src_triangle - e1->i_src_triangle);
        }
    }
}

/* Sort the list to ascending order of i_tgt_triangle then strip out duplicated
   entries */
void TriangleCorrsList::SortUniqueTriangleCorrsList()
{
    int i_load, i_store = 0;

    /* sort */
    qsort(m_corr, (size_t)m_list_length, 
        sizeof(dtTriangleCorrsEntry), TricorrsEntryCompare);

    /* uniq */
    for (i_load = 1; i_load < m_list_length; i_load++)
    {
        /* store the item if it is not equal to previously stored one */
        if (TricorrsEntryCompare(
                &m_corr[i_load], &m_corr[i_store]) != 0)
        {
            m_corr[++i_store] = m_corr[i_load];
        }
    }

    /* uniqued list length: where we stored the last item */
    m_list_length = i_store + 1;
}

/* Our automatic optimal region selector in corres_resolve picked up a 
   relatively large range to incorporate as much triangle pairs into 
   correspondence as possible, but the resulting correspondence list was too 
   large so we need to strip out some far triangle pairs to reduce the size
   of our final deformation equations. Only nearest n_maxcorrs correspondence
   entries were preserved.
*/
void TriangleCorrsList::StripTriangleCorrsList(dt_size_type n_maxcorrs)
{
    dt_size_type  n_same = 0;
    dt_index_type i_load = 1, i_store = 0;

    /* tclist is sorted and uniqued to make the coming step easier */
    SortUniqueTriangleCorrsList();

    /* strip out redundant entries with just a linear scan (O(n)). */
    for ( ; i_load < m_list_length; i_load++)
    {
        /* count duplicated target triangles  */
        if (m_corr[i_store].i_tgt_triangle != 
            m_corr[i_load].i_tgt_triangle)
        {
            n_same = 0;    /* another target triangle index, reset counter */
        }
        else {
            n_same += 1;   /* old target triangle index, count it */
        }

        /* if there's less than n_maxcorrs entries incoorporated, 
           add another more. */
        if (n_same < n_maxcorrs) {
            m_corr[++i_store] = m_corr[i_load];
        }
        /* else: we'll simply drop this one. */
    }

    m_list_length = i_store + 1;
}


/* Save the triangle correspondences to a text file, the file format is quite
   simple: the first line is the total number of entris, each following line 
   contains 2 integers which represents triangle indexes on source mesh and 
   target mesh. 

   This function returns 0 on success, or it would return an -1 to indicate an
   fopen() error, you can check system var errno for further investigation.
*/
int TriangleCorrsList::SaveTriangleCorrsList(const char *filename)
{
    int i_entry = 0;
    FILE *fd = fopen(filename, "w");
    if (fd != NULL)
    {
        fprintf(fd, "%d\n", m_list_length);
        for ( ; i_entry < m_list_length; i_entry++)
        {
            fprintf(fd, "%d, %d, %12.9f\n", 
                m_corr[i_entry].i_src_triangle,
                m_corr[i_entry].i_tgt_triangle,
                m_corr[i_entry].dist_sq);
        }

        fclose(fd);
        return 0;   /* success */
    }
    else {  /* ouch, fopen() malfunction detected. */
        return -1;
    }
}

/* Load triangle correspondences from file, 0 on success -1 on fail.
   This function would create a new triangle correspondence list object so you
   do not need to call __dt_CreateTriangleCorrsList() before calling this 
   function, or you'll suffer from a memory leak.
*/
int TriangleCorrsList::LoadTriangleCorrsList(const char *filename)
{
    int i_entry = 0, n_entry = 0;
    FILE *fd = fopen(filename, "r");

    if (fd != NULL)
    {
        fscanf(fd, "%d", &n_entry);
        CreateTriangleCorrsList(n_entry);

        for ( ; i_entry < n_entry; i_entry++)
        {
            fscanf(fd, "%d,%d,%lf",
                &(m_corr[i_entry].i_src_triangle),
                &(m_corr[i_entry].i_tgt_triangle),
                &(m_corr[i_entry].dist_sq));
        }

        fclose(fd);
        return 0;
    }
    else {   /* contact! */
        return -1;
    }
}

