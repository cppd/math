/*
Copyright (C) 2017-2026 Topological Manifold

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

#ifndef VOLUME_OPACITY_GLSL
#define VOLUME_OPACITY_GLSL

#include "fragments.glsl"
#include "volume_in.glsl"

#if defined(OPACITY)

vec4 g_opacity_fragment_v_1;
bool g_opacity_empty;

void opacity_build()
{
        g_opacity_fragment_v_1 = imageLoad(opacity_1, ivec2(gl_FragCoord.xy), gl_SampleID);
        g_opacity_empty = opacity_empty(g_opacity_fragment_v_1);
}

bool opacity_empty()
{
        return g_opacity_empty;
}

float opacity_depth()
{
        return opacity_depth(g_opacity_fragment_v_1);
}

OpacityFragment opacity_fragment()
{
        OpacityFragment res;
        res.v_0 = imageLoad(opacity_0, ivec2(gl_FragCoord.xy), gl_SampleID).xy;
        res.v_1 = g_opacity_fragment_v_1;
#ifdef RAY_TRACING
        res.v_2 = imageLoad(opacity_2, ivec2(gl_FragCoord.xy), gl_SampleID);
        res.v_3 = imageLoad(opacity_3, ivec2(gl_FragCoord.xy), gl_SampleID).xy;
#endif
        return res;
}

#else

void opacity_build()
{
}

bool opacity_empty()
{
        return true;
}

float opacity_depth()
{
        return 0;
}

#endif

#endif
