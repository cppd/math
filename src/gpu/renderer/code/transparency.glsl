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

const uint TRANSPARENCY_MAX_NODES = 32;

struct TransparencyNode
{
        uint color_rg;
        uint color_ba;
        float depth;
        uint next;
};

TransparencyNode create_transparency_node(const vec3 color, const float alpha, const float depth, const uint next)
{
        TransparencyNode node;

        node.color_rg = packUnorm2x16(color.rg);
        node.color_ba = packUnorm2x16(vec2(color.b, alpha));
        node.depth = depth;
        node.next = next;

        return node;
}
