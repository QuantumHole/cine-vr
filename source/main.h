// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAIN_H
#define MAIN_H

#include "gui/projection.h"
#include "opengl/shader_set.h"
#include "player/player.h"

void quit(void);
void player_backward(void);
void player_forward(void);
void player_pause(void);
void player_play(void);
void player_previous(void);
void player_next(void);
void player_open_file(const std::string& file_name);
void player_show_desktop(void);
float player_volume(void);
void player_set_volume(const float vol);
Player& player(void);
Projection& projection(void);
void update_projection(void);
ShaderSet& shader(void);

#endif
