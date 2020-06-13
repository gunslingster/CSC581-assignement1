/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup obj
 */

#include <fstream>
#include <iostream>

#include "DNA_object_types.h"

#include "wavefront_obj.hh"
#include "wavefront_obj_file_handler.hh"

namespace io {
namespace obj {

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2
/**
 * Calculate a face normal's axis component by averaging over its vertex normals.
 */
MALWAYS_INLINE short face_normal_axis_component(const Polygon &poly_to_write,
                                                short axis,
                                                MVert *vertex_list)
{
  float sum = 0;
  for (uint i = 0; i < poly_to_write.total_vertices_per_poly; i++) {
    sum += vertex_list[poly_to_write.vertex_index[i] - 1].no[axis];
  }
  return short(sum / poly_to_write.total_vertices_per_poly);
}

static void write_geomtery_per_object(FILE *outfile, OBJ_object_to_export &object_to_export)
{
  /** Write object name, as seen in outliner. First two characters are ID code, so skipped. */
  fprintf(outfile, "o %s\n", object_to_export.object->id.name + 2);

  /** Write v x y z for all vertices. */
  for (uint i = 0; i < object_to_export.tot_vertices; i++) {
    MVert *vertex = &object_to_export.mvert[i];
    fprintf(outfile, "v %f %f %f\n", vertex->co[0], vertex->co[1], vertex->co[2]);
  }

  /**
   * Write texture coordinates, vt u v for all vertices in a object's texture space.
   */
  for (uint i = 0; i < object_to_export.tot_uv_vertices; i++) {
    std::array<float, 2> &uv_vertex = object_to_export.uv_coords[i];
    fprintf(outfile, "vt %f %f\n", uv_vertex[0], uv_vertex[1]);
  }

  /** Write vn nx ny nz for all face normals. */
  for (uint i = 0; i < object_to_export.tot_poly; i++) {
    MVert *vertex_list = object_to_export.mvert;
    const Polygon &polygon = object_to_export.polygon_list[i];
    fprintf(outfile,
            "vn %hd %hd %d\n",
            face_normal_axis_component(polygon, AXIS_X, vertex_list),
            face_normal_axis_component(polygon, AXIS_Y, vertex_list),
            face_normal_axis_component(polygon, AXIS_Z, vertex_list));
  }

  /**
   * Write f v1/vt1/vn1 .. total_vertices_per_poly , for all polygons.
   * i-th vn is always i + 1, guaranteed by face normal loop above.
   * Both loop over the same polygon list.
   */
  for (uint i = 0; i < object_to_export.tot_poly; i++) {
    const Polygon &polygon = object_to_export.polygon_list[i];
    fprintf(outfile, "f ");
    for (int j = 0; j < polygon.total_vertices_per_poly; j++) {
      /* This loop index is 0-based. Indices in OBJ start from 1. */
      fprintf(
          outfile, "%d/%d/%d ", polygon.vertex_index[j], polygon.uv_vertex_index[j] + 1, i + 1);
    }
    fprintf(outfile, "\n");
  }
}

/**
 * Low level writer to the OBJ file at filepath.
 */
void write_object_fprintf(const char *filepath,
                          std::vector<OBJ_object_to_export> &object_to_export)
{
  FILE *outfile = fopen(filepath, "w");
  if (outfile == NULL) {
    printf("Error in creating the file\n");
    return;
  }

  fprintf(outfile, "# Blender 2.90\n");
  for (uint i = 0; i < object_to_export.size(); i++) {
    write_geomtery_per_object(outfile, object_to_export[i]);
  }
  fclose(outfile);
}

}  // namespace obj
}  // namespace io