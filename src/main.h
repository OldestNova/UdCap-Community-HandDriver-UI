//
// Created by max_3 on 25-5-23.
//

#ifndef MAIN_H
#define MAIN_H

// gdksurface-win32.c _gdk_win32_surface_compute_size (Fixed in GTK 4.18.3)
// >>>>
// size_changed = width != impl->next_layout.configured_width ||
//                  height != impl->next_layout.configured_height;
// ====
// size_changed = surface->width != impl->next_layout.configured_width ||
//                  surface->height != impl->next_layout.configured_height;
// <<<<

#endif //MAIN_H
