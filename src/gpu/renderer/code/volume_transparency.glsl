/*
Copyright (C) 2017-2022 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein.
Introduction to Algorithms. Third Edition.
The MIT Press, 2009.
6. Heapsort
*/

#ifndef VOLUME_TRANSPARENCY_GLSL
#define VOLUME_TRANSPARENCY_GLSL

#include "mesh_fragment.glsl"
#include "volume_in.glsl"

#if !defined(TRANSPARENCY)

void fragments_build()
{
}

bool fragments_empty()
{
        return true;
}

void fragments_pop()
{
}

TransparencyFragment fragments_top()
{
        TransparencyFragment f;
        return f;
}

#else

TransparencyFragment g_fragments[TRANSPARENCY_MAX_NODES];
int g_fragments_count;

int fragments_min_heapify_impl(const int i)
{
        const int left = 2 * i + 1;
        const int right = left + 1;
        int m;
        m = (left < g_fragments_count && g_fragments[left].depth < g_fragments[i].depth) ? left : i;
        m = (right < g_fragments_count && g_fragments[right].depth < g_fragments[m].depth) ? right : m;
        if (m != i)
        {
                const TransparencyFragment t = g_fragments[i];
                g_fragments[i] = g_fragments[m];
                g_fragments[m] = t;
                return m;
        }
        return -1;
}

void fragments_min_heapify(int i)
{
        do
        {
                i = fragments_min_heapify_impl(i);
        } while (i >= 0);
}

void fragments_build_min_heap()
{
        for (int i = g_fragments_count / 2 - 1; i >= 0; --i)
        {
                fragments_min_heapify(i);
        }
}

void fragments_pop()
{
        if (g_fragments_count > 1)
        {
                --g_fragments_count;
                g_fragments[0] = g_fragments[g_fragments_count];
                fragments_min_heapify(0);
                return;
        }
        g_fragments_count = 0;
}

bool fragments_empty()
{
        return g_fragments_count <= 0;
}

TransparencyFragment fragments_top()
{
        return g_fragments[0];
}

void fragments_build()
{
        g_fragments_count = 0;

        uint node_index = imageLoad(transparency_heads, ivec2(gl_FragCoord.xy), gl_SampleID).r;

        if (node_index == TRANSPARENCY_NULL_INDEX)
        {
                return;
        }

        while (node_index != TRANSPARENCY_NULL_INDEX && g_fragments_count < TRANSPARENCY_MAX_NODES)
        {
                g_fragments[g_fragments_count] = transparency_nodes[node_index].fragment;
                node_index = transparency_nodes[node_index].next;
                ++g_fragments_count;
        }

        fragments_build_min_heap();
}

#endif

#endif
