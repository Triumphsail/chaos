/*
 * Chaos 
 *
 * Copyright 2015 Operating Systems Laboratory EPFL
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SSSP_FOREST_
#define _SSSP_FOREST_

#include "../../utils/options_utils.h"
#include "../../core/x-lib.hpp"
#include <limits>

namespace algorithm {
  namespace sg_simple {
    template<typename F>
    class sssp_forest {
    public:
        static unsigned long checkpoint_size() {
          return 0;
        }

        static void take_checkpoint(unsigned char *buffer,
                                    per_processor_data **cpu_array,
                                    unsigned long num_processors) {
        }

        static void restore_checkpoint(unsigned char *buffer,
                                       per_processor_data **cpu_array,
                                       unsigned long num_processors) {
        }

        struct vertex {
            vertex_t component;
            vertex_t predecessor;
            weight_t distance;
            unsigned long active_phase;
        } __attribute__((__packed__));

        struct update {
            vertex_t target;
            vertex_t predecessor;
            weight_t distance;
            vertex_t component;
        } __attribute__((__packed__));

        static unsigned long vertex_state_bytes() {
          return sizeof(struct vertex);
        }

        static unsigned long split_size_bytes() {
          return sizeof(struct update);
        }

        static unsigned long split_key(unsigned char *buffer, unsigned long jump) {
          struct update *u = (struct update *) buffer;
          vertex_t key = u->target;
          key = key >> jump;
          return key;
        }

        static
        bool need_data_barrier() {
          return false;
        }

        class db_sync {
        public:
            void prep_db_data(per_processor_data **pcpu_array,
                              unsigned long me,
                              unsigned long processors) { }

            void finalize_db_data(per_processor_data **pcpu_array,
                                  unsigned long me,
                                  unsigned long processors) { }

            unsigned char *db_buffer() { return NULL; }

            unsigned long db_size() { return 0; }

            void db_generate() { }

            void db_merge() { }

            void db_absorb() { }
        };

        static
        db_sync *get_db_sync() { return NULL; }

        static bool need_scatter_merge(unsigned long bsp_phase) {
          return false;
        }

        static void vertex_apply(unsigned char *v,
                                 unsigned char *copy,
                                 unsigned long copy_machine,
                                 per_processor_data *per_cpu_data,
                                 unsigned long bsp_phase) {
          struct vertex *vtx = (struct vertex *) v;
          struct vertex *vtx_cpy = (struct vertex *) copy;
          if (vtx->component > vtx_cpy->component ||
              (vtx->component == vtx_cpy->component &&
               vtx->distance > vtx_cpy->distance)) {
            vtx->component = vtx_cpy->component;
            vtx->predecessor = vtx_cpy->predecessor;
            vtx->distance = vtx_cpy->distance;
            vtx->active_phase = bsp_phase;
          }
        }

        static bool init(unsigned char *vertex_state,
                         unsigned long vertex_index,
                         unsigned long bsp_phase,
                         per_processor_data *cpu_state) {
          struct vertex *vertices = (struct vertex *) vertex_state;
          vertices->component = vertex_index;
          vertices->predecessor = vertices->component;
          vertices->distance = 0;
          vertices->active_phase = 0;
          return true;
        }

        static bool need_init(unsigned long bsp_phase) {
          return (bsp_phase == 0);
        }

        static bool apply_one_update(unsigned char *vertex_state,
                                     unsigned char *update_stream,
                                     per_processor_data *per_cpu_data,
                                     bool local_tile,
                                     unsigned long bsp_phase) {
          struct update *u = (struct update *) update_stream;
          struct vertex *vertices = (struct vertex *) vertex_state;
          struct vertex *v = &vertices[x_lib::configuration::map_offset(u->target)];
          if ((u->component < v->component) ||
              (u->component == v->component &&
               u->distance < v->distance)) {
            v->predecessor = u->predecessor;
            v->distance = u->distance;
            v->active_phase = bsp_phase;
            v->component = u->component;
            return true;
          } else {
            return false;
          }
        }

        static bool generate_update(unsigned char *vertex_state,
                                    unsigned char *edge_format,
                                    unsigned char *update_stream,
                                    per_processor_data *per_cpu_data,
                                    bool local_tile,
                                    unsigned long bsp_phase) {
          vertex_t src, dst;
          weight_t edge_distance;
          F::read_edge(edge_format, src, dst, edge_distance);
          struct vertex *vertices = (struct vertex *) vertex_state;
          struct vertex *v = &vertices[x_lib::configuration::map_offset(src)];
          if (v->active_phase == bsp_phase) {
            struct update *u = (struct update *) update_stream;
            u->target = dst;
            u->predecessor = src;
            u->distance = v->distance + edge_distance;
            u->component = v->component;
            return true;
          } else {
            return false;
          }
        }

        // not used
        static void postprocessing() { }

        static void preprocessing() { }

        static per_processor_data *
        create_per_processor_data(unsigned long processor_id,
                                  unsigned long machines) {
          return NULL;
        }

        static unsigned long min_super_phases() {
          return 1;
        }

    };
  }
}
#endif
