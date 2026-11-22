// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ACTION_H
#define ACTION_H

/** list of available input actions. */
typedef enum
{
	ACTION_NONE,                ///< no action
	ACTION_BACK,                ///< return to previous menu
	ACTION_CALIBRATE_MONITORS,  ///< determine positions of physical screens
	ACTION_DESKTOP,             ///< show desktop screen
	ACTION_DESKTOP_WINDOW,      ///< show a single desktop window
	ACTION_DIRECTORY_SELECT,    ///< select a directory in file manager view
	ACTION_FILE_SELECT,         ///< select a file in file manager view
	ACTION_FILE_DELETE,         ///< delete selected file
	ACTION_FILE_OPEN,           ///< open selected file
	ACTION_PLAY_BACKWARD,       ///< jump forward in current file
	ACTION_PLAY_FORWARD,        ///< jump backward in current file
	ACTION_PLAY_NEXT,           ///< play next file
	ACTION_PLAY_PAUSE,          ///< pause the currently playing file
	ACTION_PLAY_PLAY,           ///< start playing the current file
	ACTION_PLAY_POSITION,       ///< jump to the marked position in current file
	ACTION_PLAY_PREVIOUS,       ///< play previous file
	ACTION_POWER,               ///< quit application
	ACTION_PROJECT_CUBE,        ///< show current file in cube projection
	ACTION_PROJECT_CYLINDER,    ///< show current file in curved screen projection
	ACTION_PROJECT_FISHEYE,     ///< show current file in fisheye projection
	ACTION_PROJECT_FLAT,        ///< show current file in flat screen projection
	ACTION_PROJECT_SPHERE,      ///< show current file in spherical projection
	ACTION_SELECT_AUDIO,        ///< select the audio track/language
	ACTION_SELECT_SUBTITLE,     ///< select the subtitle track/language
	ACTION_SETTINGS,            ///< go into settings submenu
	ACTION_TILE_CUBE_MONO,      ///< set tiling of source file for monoscopic cube projection
	ACTION_TILE_CUBE_STEREO,    ///< set tiling of source file for stereoscopic cube projection
	ACTION_TILE_LEFT_RIGHT,     ///< set tiling of source file for stereoscopic left/right (side-by-side) projection
	ACTION_TILE_MONO,           ///< set tiling of source file for monoscopic projection
	ACTION_TILE_TOP_BOTTOM,     ///< set tiling of source file for stereoscopic top/bottom projection
	ACTION_FLAG_MONO,           ///< force monoscopic display even for stereoscopic source files
	ACTION_FLAG_STRETCH,        ///< stretch texture horizontally
	ACTION_FLAG_SWITCH_EYES,    ///< switch eyes in stereoscopic display
	ACTION_PARAM_ANGLE,         ///< adjust projection angle
	ACTION_PARAM_ZOOM,          ///< adjust zoom
	ACTION_VOLUME               ///< adjust audo volume
}
action_t;

#endif
