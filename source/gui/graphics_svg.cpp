// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "graphics_svg.h"
#include "util/string_tools.h"
#include "util/file_system.h"
#include "util/node_xml.h"
#include "util/node_css.h"
#include <string.h>
#include <sstream>
#include <glm/ext/matrix_transform.hpp>
#include <regex>

typedef enum
{
	PAINT_NONE,
	PAINT_COLOR,
	PAINT_LINEAR_GRADIENT,
	PAINT_RADIAL_GRADIENT
}
paint_type_t;

typedef enum
{
	SPREAD_PAD,
	SPREAD_REFLECT,
	SPREAD_REPEAT
}
spread_t;

typedef enum
{
	JOIN_MITER_SQUARE,
	JOIN_ROUND,
	JOIN_BEVEL_BUTT
}
line_join_t;

typedef enum
{
	ORDER_FILL,
	ORDER_MARKERS,
	ORDER_STROKE
}
paint_order_t;

typedef enum
{
	UNITS_USER,
	UNITS_PX,
	UNITS_PT,
	UNITS_PC,
	UNITS_MM,
	UNITS_CM,
	UNITS_IN,
	UNITS_PERCENT,
	UNITS_EM,
	UNITS_EX
}
unit_t;

typedef enum
{
	ALIGN_NONE,
	ALIGN_MEET,
	ALIGN_SLICE
}
align_t;

typedef struct
{
	float value;
	unit_t units;
}
coordinate_t;

typedef struct
{
	coordinate_t x1;
	coordinate_t y1;
	coordinate_t x2;
	coordinate_t y2;
}
linear_gradient_t;

typedef struct
{
	coordinate_t cx;
	coordinate_t cy;
	coordinate_t r;
	coordinate_t fx;
	coordinate_t fy;
}
radial_gradient_t;

typedef std::pair<glm::vec2, glm::vec2> bounds_t;
typedef std::vector<glm::vec2> path_t;

class Paint
{
	public:
		paint_type_t type;
		glm::mat4 xform;
		spread_t spread;
		glm::vec2 f;
		std::map<float, glm::vec4> stops;

		Paint() :
			type(PAINT_NONE),
			xform(1.0f),
			spread(SPREAD_PAD),
			f(0.0f, 0.0f),
			stops()
		{
		}
};

class ElementSVG
{
	public:
		std::string id;                          // name of this element
		glm::mat4 xform;                         // transformation (translation, rotation, scaling)
		float opacity;                           // opacity of the shape.
		float fontSize;                          // current font size.
		bool visible;                            // visibility
		bool closed;                             // flag for closed paths
		std::vector<paint_order_t> paintOrder;   // paint order
		std::vector<path_t> paths;               // list of paths in this shape.

		Paint fill;                              // fill paint
		std::string fillGradientName;            // name of fill gradient, can be looked up in gradient collection.
		bool fillAlternate;                      // fill rule, nonzero (false) or evenodd (true).

		Paint stroke;                            // stroke paint
		std::string strokeGradientName;          // name of stroke gradient, can be looked up in gradient collection.
		float strokeWidth;                       // stroke width.
		float strokeDashOffset;                  // stroke dash offset.
		std::vector<float> strokeDashArray;      // stroke dash array.
		line_join_t strokeLineJoin;              // stroke join type.
		line_join_t strokeLineCap;               // stroke cap type.
		float miterLimit;                        // miter limit

		ElementSVG(void) :
			id(""),
			xform(1.0f),
			opacity(1.0f),
			fontSize(0.0f),
			visible(true),
			closed(false),
			paintOrder({ORDER_FILL, ORDER_STROKE, ORDER_MARKERS}),
			paths(),

			fill(),
			fillGradientName(""),
			fillAlternate(false),

			stroke(),
			strokeGradientName(""),
			strokeWidth(1.0f),
			strokeDashOffset(0.0f),
			strokeDashArray(),
			strokeLineJoin(JOIN_MITER_SQUARE),
			strokeLineCap(JOIN_BEVEL_BUTT),
			miterLimit(4.0f)
		{
		}
};

class GradientSVG : public Paint
{
	public:
		std::string id;
		std::string ref;
		linear_gradient_t linear;
		radial_gradient_t radial;
		bool globalUnits;

		GradientSVG(void) :
			Paint(),
			id(""),
			ref(""),
			linear(),
			radial(),
			globalUnits(false)
		{
		}
};

class ParserSVG
{
	public:
		std::vector<ElementSVG> shapes;
		std::map<std::string, GradientSVG> gradients;
		std::string currentGradient;

		glm::vec2 viewMin;
		glm::vec2 viewSize;
		float alignFracX;
		float alignFracY;
		align_t alignType;

		ParserSVG(void) :
			shapes(),
			gradients(),
			currentGradient(""),
			viewMin(0.0f, 0.0f),
			viewSize(0.0f, 0.0f),
			alignFracX(0.0f),
			alignFracY(0.0f),
			alignType(ALIGN_NONE)
		{
		}
};

/**
 * color names according to CSS named color definition https://drafts.csswg.org/css-color/#named-colors
 */
static const std::map<std::string, glm::uvec3> named_colors = {
	{"aliceblue",            glm::uvec3(240, 248, 255) },
	{"antiquewhite",         glm::uvec3(250, 235, 215) },
	{"aqua",                 glm::uvec3(  0, 255, 255) },
	{"aquamarine",           glm::uvec3(127, 255, 212) },
	{"azure",                glm::uvec3(240, 255, 255) },
	{"beige",                glm::uvec3(245, 245, 220) },
	{"bisque",               glm::uvec3(255, 228, 196) },
	{"black",                glm::uvec3(  0,   0,   0) },
	{"blanchedalmond",       glm::uvec3(255, 235, 205) },
	{"blue",                 glm::uvec3(  0,   0, 255) },
	{"blueviolet",           glm::uvec3(138,  43, 226) },
	{"brown",                glm::uvec3(165,  42,  42) },
	{"burlywood",            glm::uvec3(222, 184, 135) },
	{"cadetblue",            glm::uvec3( 95, 158, 160) },
	{"chartreuse",           glm::uvec3(127, 255,   0) },
	{"chocolate",            glm::uvec3(210, 105,  30) },
	{"coral",                glm::uvec3(255, 127,  80) },
	{"cornflowerblue",       glm::uvec3(100, 149, 237) },
	{"cornsilk",             glm::uvec3(255, 248, 220) },
	{"crimson",              glm::uvec3(220,  20,  60) },
	{"cyan",                 glm::uvec3(  0, 255, 255) },
	{"darkblue",             glm::uvec3(  0,   0, 139) },
	{"darkcyan",             glm::uvec3(  0, 139, 139) },
	{"darkgoldenrod",        glm::uvec3(184, 134,  11) },
	{"darkgray",             glm::uvec3(169, 169, 169) },
	{"darkgreen",            glm::uvec3(  0, 100,   0) },
	{"darkgrey",             glm::uvec3(169, 169, 169) },
	{"darkkhaki",            glm::uvec3(189, 183, 107) },
	{"darkmagenta",          glm::uvec3(139,   0, 139) },
	{"darkolivegreen",       glm::uvec3( 85, 107,  47) },
	{"darkorange",           glm::uvec3(255, 140,   0) },
	{"darkorchid",           glm::uvec3(153,  50, 204) },
	{"darkred",              glm::uvec3(139,   0,   0) },
	{"darksalmon",           glm::uvec3(233, 150, 122) },
	{"darkseagreen",         glm::uvec3(143, 188, 143) },
	{"darkslateblue",        glm::uvec3( 72,  61, 139) },
	{"darkslategray",        glm::uvec3( 47,  79,  79) },
	{"darkslategrey",        glm::uvec3( 47,  79,  79) },
	{"darkturquoise",        glm::uvec3(  0, 206, 209) },
	{"darkviolet",           glm::uvec3(148,   0, 211) },
	{"deeppink",             glm::uvec3(255,  20, 147) },
	{"deepskyblue",          glm::uvec3(  0, 191, 255) },
	{"dimgray",              glm::uvec3(105, 105, 105) },
	{"dimgrey",              glm::uvec3(105, 105, 105) },
	{"dodgerblue",           glm::uvec3( 30, 144, 255) },
	{"firebrick",            glm::uvec3(178,  34,  34) },
	{"floralwhite",          glm::uvec3(255, 250, 240) },
	{"forestgreen",          glm::uvec3( 34, 139,  34) },
	{"fuchsia",              glm::uvec3(255,   0, 255) },
	{"gainsboro",            glm::uvec3(220, 220, 220) },
	{"ghostwhite",           glm::uvec3(248, 248, 255) },
	{"gold",                 glm::uvec3(255, 215,   0) },
	{"goldenrod",            glm::uvec3(218, 165,  32) },
	{"gray",                 glm::uvec3(128, 128, 128) },
	{"green",                glm::uvec3(  0, 128,   0) },
	{"greenyellow",          glm::uvec3(173, 255,  47) },
	{"grey",                 glm::uvec3(128, 128, 128) },
	{"honeydew",             glm::uvec3(240, 255, 240) },
	{"hotpink",              glm::uvec3(255, 105, 180) },
	{"indianred",            glm::uvec3(205,  92,  92) },
	{"indigo",               glm::uvec3( 75,   0, 130) },
	{"ivory",                glm::uvec3(255, 255, 240) },
	{"khaki",                glm::uvec3(240, 230, 140) },
	{"lavender",             glm::uvec3(230, 230, 250) },
	{"lavenderblush",        glm::uvec3(255, 240, 245) },
	{"lawngreen",            glm::uvec3(124, 252,   0) },
	{"lemonchiffon",         glm::uvec3(255, 250, 205) },
	{"lightblue",            glm::uvec3(173, 216, 230) },
	{"lightcoral",           glm::uvec3(240, 128, 128) },
	{"lightcyan",            glm::uvec3(224, 255, 255) },
	{"lightgoldenrodyellow", glm::uvec3(250, 250, 210) },
	{"lightgray",            glm::uvec3(211, 211, 211) },
	{"lightgreen",           glm::uvec3(144, 238, 144) },
	{"lightgrey",            glm::uvec3(211, 211, 211) },
	{"lightpink",            glm::uvec3(255, 182, 193) },
	{"lightsalmon",          glm::uvec3(255, 160, 122) },
	{"lightseagreen",        glm::uvec3( 32, 178, 170) },
	{"lightskyblue",         glm::uvec3(135, 206, 250) },
	{"lightslategray",       glm::uvec3(119, 136, 153) },
	{"lightslategrey",       glm::uvec3(119, 136, 153) },
	{"lightsteelblue",       glm::uvec3(176, 196, 222) },
	{"lightyellow",          glm::uvec3(255, 255, 224) },
	{"lime",                 glm::uvec3(  0, 255,   0) },
	{"limegreen",            glm::uvec3( 50, 205,  50) },
	{"linen",                glm::uvec3(250, 240, 230) },
	{"magenta",              glm::uvec3(255,   0, 255) },
	{"maroon",               glm::uvec3(128,   0,   0) },
	{"mediumaquamarine",     glm::uvec3(102, 205, 170) },
	{"mediumblue",           glm::uvec3(  0,   0, 205) },
	{"mediumorchid",         glm::uvec3(186,  85, 211) },
	{"mediumpurple",         glm::uvec3(147, 112, 219) },
	{"mediumseagreen",       glm::uvec3( 60, 179, 113) },
	{"mediumslateblue",      glm::uvec3(123, 104, 238) },
	{"mediumspringgreen",    glm::uvec3(  0, 250, 154) },
	{"mediumturquoise",      glm::uvec3( 72, 209, 204) },
	{"mediumvioletred",      glm::uvec3(199,  21, 133) },
	{"midnightblue",         glm::uvec3( 25,  25, 112) },
	{"mintcream",            glm::uvec3(245, 255, 250) },
	{"mistyrose",            glm::uvec3(255, 228, 225) },
	{"moccasin",             glm::uvec3(255, 228, 181) },
	{"navajowhite",          glm::uvec3(255, 222, 173) },
	{"navy",                 glm::uvec3(  0,   0, 128) },
	{"oldlace",              glm::uvec3(253, 245, 230) },
	{"olive",                glm::uvec3(128, 128,   0) },
	{"olivedrab",            glm::uvec3(107, 142,  35) },
	{"orange",               glm::uvec3(255, 165,   0) },
	{"orangered",            glm::uvec3(255,  69,   0) },
	{"orchid",               glm::uvec3(218, 112, 214) },
	{"palegoldenrod",        glm::uvec3(238, 232, 170) },
	{"palegreen",            glm::uvec3(152, 251, 152) },
	{"paleturquoise",        glm::uvec3(175, 238, 238) },
	{"palevioletred",        glm::uvec3(219, 112, 147) },
	{"papayawhip",           glm::uvec3(255, 239, 213) },
	{"peachpuff",            glm::uvec3(255, 218, 185) },
	{"peru",                 glm::uvec3(205, 133,  63) },
	{"pink",                 glm::uvec3(255, 192, 203) },
	{"plum",                 glm::uvec3(221, 160, 221) },
	{"powderblue",           glm::uvec3(176, 224, 230) },
	{"purple",               glm::uvec3(128,   0, 128) },
	{"rebeccapurple",        glm::uvec3(102,  51, 153) },
	{"red",                  glm::uvec3(255,   0,   0) },
	{"rosybrown",            glm::uvec3(188, 143, 143) },
	{"royalblue",            glm::uvec3( 65, 105, 225) },
	{"saddlebrown",          glm::uvec3(139,  69,  19) },
	{"salmon",               glm::uvec3(250, 128, 114) },
	{"sandybrown",           glm::uvec3(244, 164,  96) },
	{"seagreen",             glm::uvec3( 46, 139,  87) },
	{"seashell",             glm::uvec3(255, 245, 238) },
	{"sienna",               glm::uvec3(160,  82,  45) },
	{"silver",               glm::uvec3(192, 192, 192) },
	{"skyblue",              glm::uvec3(135, 206, 235) },
	{"slateblue",            glm::uvec3(106,  90, 205) },
	{"slategray",            glm::uvec3(112, 128, 144) },
	{"slategrey",            glm::uvec3(112, 128, 144) },
	{"snow",                 glm::uvec3(255, 250, 250) },
	{"springgreen",          glm::uvec3(  0, 255, 127) },
	{"steelblue",            glm::uvec3( 70, 130, 180) },
	{"tan",                  glm::uvec3(210, 180, 140) },
	{"teal",                 glm::uvec3(  0, 128, 128) },
	{"thistle",              glm::uvec3(216, 191, 216) },
	{"tomato",               glm::uvec3(255,  99,  71) },
	{"turquoise",            glm::uvec3( 64, 224, 208) },
	{"violet",               glm::uvec3(238, 130, 238) },
	{"wheat",                glm::uvec3(245, 222, 179) },
	{"white",                glm::uvec3(255, 255, 255) },
	{"whitesmoke",           glm::uvec3(245, 245, 245) },
	{"yellow",               glm::uvec3(255, 255,   0) },
	{"yellowgreen",          glm::uvec3(154, 205,  50) },
};

/******************************************************************************* bounds functions */

static bounds_t pathBounds(const std::vector<glm::vec2>& path)
{
	bounds_t bounds;

	if (path.empty())
	{
		throw std::runtime_error("bounds of empty path");
	}

	bounds.first = path[0];
	bounds.second = path[0];

	for (std::vector<glm::vec2>::const_iterator iter = path.begin(); iter != path.end(); ++iter)
	{
		bounds.first.x = std::min(bounds.first.x, iter->x);
		bounds.first.y = std::min(bounds.first.y, iter->y);
		bounds.second.x = std::max(bounds.second.x, iter->x);
		bounds.second.y = std::max(bounds.second.y, iter->y);
	}

	return bounds;
}

static void mergeBounds(bounds_t& bounds, const bounds_t& curve, const bool init)
{
	if (init)
	{
		bounds = curve;
	}
	else
	{
		bounds.first.x = std::min(bounds.first.x, curve.first.x);
		bounds.first.y = std::min(bounds.first.y, curve.first.y);
		bounds.second.x = std::max(bounds.second.x, curve.second.x);
		bounds.second.y = std::max(bounds.second.y, curve.second.y);
	}
}

static bounds_t getLocalBounds(const std::vector<path_t>& paths)
{
	bounds_t bounds;

	for (std::vector<path_t>::const_iterator iter = paths.begin(); iter != paths.end(); ++iter)
	{
		const bounds_t pb = pathBounds(*iter);
		mergeBounds(bounds, pb, iter == paths.begin());
	}
	return bounds;
}

static bounds_t imageBounds(const std::vector<ElementSVG>& shapes)
{
	bounds_t bounds;

	for (std::vector<ElementSVG>::const_iterator shape = shapes.begin(); shape != shapes.end(); ++shape)
	{
		const bounds_t sb = getLocalBounds(shape->paths);
		mergeBounds(bounds, sb, shape == shapes.begin());
	}
	return bounds;
}

/******************************************************************************** shape functions */

static ElementSVG transform_shape(const std::vector<path_t>& paths, const ElementSVG& attr)
{
	ElementSVG shape = attr;

	// average scale
	const glm::mat4& t = attr.xform;
	float sx = sqrtf(t[0][0] * t[0][0] + t[1][0] * t[1][0]);
	float sy = sqrtf(t[0][1] * t[0][1] + t[1][1] * t[1][1]);
	const float scale = (sx + sy) * 0.5f;

	shape.strokeWidth *= scale;
	shape.strokeDashOffset *= scale;
	for (size_t i = 0; i < shape.strokeDashArray.size(); i++)
	{
		shape.strokeDashArray[i] *= scale;
	}

	shape.paths = paths;

	return shape;
}

static path_t createPath(const glm::mat4& xform, const std::vector<glm::vec2>& pts)
{
	path_t path(pts.size());

	// Transform path.
	for (size_t i = 0; i < pts.size(); i++)
	{
		path[i] = glm::vec2(xform * glm::vec4(pts[i].x, pts[i].y, 0.0f, 1.0f));
	}

	return path;
}

/************************************************************************ color parsing functions */

static glm::vec4 color_utof(const glm::uvec3& c)
{
	return glm::vec4(static_cast<float>(c.r) / 255.0f,
					 static_cast<float>(c.g) / 255.0f,
					 static_cast<float>(c.b) / 255.0f,
					 1.0f);
}

static glm::vec4 parse_color_hex(const std::string& str)
{
	glm::uvec3 color(0, 0, 0);          // default color

	if (sscanf(str.c_str(), "#%2x%2x%2x", &color.r, &color.g, &color.b) != 3)         // 2 digit hex, e.g #rrggbb
	{
		if (sscanf(str.c_str(), "#%1x%1x%1x", &color.r, &color.g, &color.b) == 3)     // 1 digit hex, e.g. #rgb
		{
			color *= 17;          // duplicate hex digit like ((c << 4) | c)
		}
	}
	return color_utof(color);
}

static glm::vec4 parse_color_rgb(const std::string& str)
{
	glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);          // default color

	if (std::sscanf(str.c_str(), "rgb(%f%%,%f%%,%f%%)", &color.r, &color.g, &color.b) == 3)
	{
		// percentual values 0‑100%
		// scale range from [0-100] to [0-1]
		color.r /= 100.0f;
		color.g /= 100.0f;
		color.b /= 100.0f;
	}
	else if (std::sscanf(str.c_str(), "rgb(%f,%f,%f)", &color.r, &color.g, &color.b) == 3)
	{
		// integer values 0‑255
		// scale range from [0-255] to [0-1]
		color.r /= 255.0f;
		color.g /= 255.0f;
		color.b /= 255.0f;
	}

	// clamp to [0‑1]
	color.r = glm::clamp(color.r, 0.0f, 1.0f);
	color.g = glm::clamp(color.g, 0.0f, 1.0f);
	color.b = glm::clamp(color.b, 0.0f, 1.0f);

	return color;
}

static glm::vec4 parse_color_rgba(const std::string& str)
{
	glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);          // default color

	if (std::sscanf(str.c_str(), "rgba(%f%%,%f%%,%f%%,%f)", &color.r, &color.g, &color.b, &color.a) == 4)
	{
		// percentual values 0‑100%
		// scale range from [0-100] to [0-1]
		// alpha channel must always be specified as value in [0-1].
		color.r /= 100.0f;
		color.g /= 100.0f;
		color.b /= 100.0f;
	}
	else if (std::sscanf(str.c_str(), "rgba(%f,%f,%f,%f)", &color.r, &color.g, &color.b, &color.a) == 4)
	{
		// integer values 0‑255
		// scale range from [0-255] to [0-1]
		// alpha channel must always be specified as value in [0-1].
		color.r /= 255.0f;
		color.g /= 255.0f;
		color.b /= 255.0f;
	}

	// clamp to [0‑1]
	color.r = glm::clamp(color.r, 0.0f, 1.0f);
	color.g = glm::clamp(color.g, 0.0f, 1.0f);
	color.b = glm::clamp(color.b, 0.0f, 1.0f);
	color.a = glm::clamp(color.a, 0.0f, 1.0f);

	return color;
}

/** parse color values.
 * colors may be specified as 3 or 6-digit hex value, decimal or percentual RGB values
 * with an optional alpha channel as floating point number in [0-1], or as a named color.
 * @param text color description.
 * @return RGBA color value.
 */
static glm::vec4 parse_color(const std::string& text)
{
	size_t index = text.find("#");

	if (index != std::string::npos)
	{
		return parse_color_hex(text.substr(index));
	}

	index = text.find("rgb(");

	if (index != std::string::npos)
	{
		return parse_color_rgb(text.substr(index));
	}

	index = text.find("rgba(");

	if (index != std::string::npos)
	{
		return parse_color_rgba(text.substr(index));
	}

	// look up named color.
	std::map<std::string, glm::uvec3>::const_iterator iter = named_colors.find(text);

	if (iter != named_colors.end())
	{
		return color_utof(iter->second);
	}
	return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

static float parseOpacity(const std::string& str)
{
	return glm::clamp(stof(str), 0.0f, 1.0f);
}

/*************************************************************************** coordinate functions */

static float convertToPixels(const float fontSize, const coordinate_t& c, const float orig, const float length, const bool scaleUser = false)
{
	const float dpi = 96;    // default for pixel data

	switch (c.units)
	{
		case UNITS_USER:
			if (scaleUser)
			{
				// treat value as fraction of origin/length.
				// needed to scale gradients in retrospect
				return convertToPixels(fontSize, {c.value * 100.0f, UNITS_PERCENT}, orig, length, false);
			}
			else
			{
				return c.value;
			}
		case UNITS_PX:         return c.value;
		case UNITS_PT:         return c.value / 72.0f * dpi;
		case UNITS_PC:         return c.value / 6.0f * dpi;
		case UNITS_MM:         return c.value / 25.4f * dpi;
		case UNITS_CM:         return c.value / 2.54f * dpi;
		case UNITS_IN:         return c.value * dpi;
		case UNITS_EM:         return c.value * fontSize;
		case UNITS_EX:         return c.value * fontSize * 0.5f; // rough estimate should be ok for most fonts.
		case UNITS_PERCENT:    return orig + c.value / 100.0f * length;
		default:
			break;
	}
	return c.value;
}

static coordinate_t parseCoordinateRaw(const std::string& str)
{
	coordinate_t coord = {0, UNITS_USER};

	// Regular expression for a floating‑point literal with optional exponent
	const std::regex numberPattern(R"(^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)");

	std::smatch match;

	if (!std::regex_search(str, match, numberPattern))
	{
		return coord;
	}

	coord.value = stof(match.str(0));
	const std::string unit = str.substr(match.str(0).size(), 2);

	if (unit == "px")
	{
		coord.units = UNITS_PX;
	}
	else if (unit == "pt")
	{
		coord.units = UNITS_PT;
	}
	else if (unit == "pc")
	{
		coord.units = UNITS_PC;
	}
	else if (unit == "mm")
	{
		coord.units = UNITS_MM;
	}
	else if (unit == "cm")
	{
		coord.units = UNITS_CM;
	}
	else if (unit == "in")
	{
		coord.units = UNITS_IN;
	}
	else if (unit.compare(0, 1, "%") == 0)
	{
		coord.units = UNITS_PERCENT;
	}
	else if (unit == "em")
	{
		coord.units = UNITS_EM;
	}
	else if (unit == "ex")
	{
		coord.units = UNITS_EX;
	}
	else
	{
		coord.units = UNITS_USER;
	}

	return coord;
}

static float parseCoordinate(const float fontSize, const std::string& str, const float orig, const float length)
{
	coordinate_t coord = parseCoordinateRaw(str);

	return convertToPixels(fontSize, coord, orig, length);
}

/*************************************************************** transformation parsing functions */

static std::vector<float> parseNumbers(const std::string& str)
{
	std::string inside = str;

	// replace optional ',' with spaces
	std::replace(inside.begin(), inside.end(), ',', ' ');

	std::istringstream stream(inside);
	std::vector<float> args;
	while (stream.good())
	{
		float value;
		stream >> value;
		args.push_back(value);
	}

	return args;
}

static glm::mat4 parseTransform(const std::string& str)
{
	size_t i = 0;
	glm::mat4 xform(1.0f);

	while (i < str.size())
	{
		std::regex regex(R"((\w+)\s*\(([^)]*)\))");
		std::smatch match;
		if (!std::regex_match(str, match, regex))
		{
			throw std::runtime_error("Invalid transform: " + str);
		}

		i += match.str(0).size();
		std::string name = match.str(1);
		std::vector<float> args = parseNumbers(match.str(2));

		glm::mat4 t(1.0f);
		if (name == "matrix")
		{
			if (args.size() != 6)
			{
				throw std::runtime_error("matrix needs 6 values");
			}
			// SVG -> mat4:
			// a c 0 e
			// b d 0 f
			// 0 0 1 0
			// 0 0 0 1
			t[0][0] = args[0]; // scaling in x
			t[0][1] = args[1]; // shear xy, along y-axis
			t[1][0] = args[2]; // shear yx, along x-axis
			t[1][1] = args[3]; // scaling in y
			t[3][0] = args[4]; // translation in x
			t[3][1] = args[5]; // translation in y
		}
		else if (name == "translate")
		{
			t[3][0] = (args.size() > 0) ? args[0] : 0.0f; // translation in x
			t[3][1] = (args.size() > 1) ? args[1] : 0.0f; // translation in y
		}
		else if (name == "scale")
		{
			t[0][0] = (args.size() > 0) ? args[0] : 0.0f; // scaling in x
			t[1][1] = (args.size() > 1) ? args[1] : 0.0f; // scaling in y
		}
		else if (name == "rotate")
		{
			const float angle = args[0] / 180.0f * static_cast<float>(M_PI);
			if (args.size() == 1)
			{
				// rotate around origin
				t = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
			}
			else if (args.size() == 3)
			{
				// translate to rotation center
				const glm::vec2 c(args[1], args[2]);
				t = glm::translate(glm::mat4(1.0f), glm::vec3(-c.x, -c.y, 0.0f));

				// apply rotation
				const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
				t = rot * t;

				// translate back to origin
				const glm::mat4 x = glm::translate(glm::mat4(1.0f), glm::vec3(c.x, c.y, 0.0f));
				t = x * t;
			}
			else
			{
				throw std::runtime_error("rotate needs 1 or 3 arguments");
			}

		}
		else if (name == "skewX")
		{
			const float angle = args[0] / 180.0f * static_cast<float>(M_PI);
			t[1][0] = tanf(angle);   // shear along x-axis
		}
		else if (name == "skewY")
		{
			const float angle = args[0] / 180.0f * static_cast<float>(M_PI);
			t[0][1] = tanf(angle);   // shear along y-axis
		}
		else
		{
			throw std::runtime_error("Unsupported transform: " + name);
		}

		xform = xform * t;
	}
	return xform;
}

/******************************************************************* other text parsing functions */

static std::string parseUrl(const std::string& text)
{
	// regular expression for "url(#gradientName)" or "url(gradientName)"
	std::regex regex(R"(url\(\s*#?(\w+)\s*\))");
	std::smatch match;

	if (std::regex_search(text, match, regex) && (match.size() > 1))
	{
		return match[1].str();
	}
	return "";
}

static line_join_t parseLineCap(const std::string& str)
{
	if (str == "butt")
	{
		return JOIN_BEVEL_BUTT;
	}
	else if (str == "round")
	{
		return JOIN_ROUND;
	}
	else if (str == "square")
	{
		return JOIN_MITER_SQUARE;
	}
	return JOIN_BEVEL_BUTT;
}

static line_join_t parseLineJoin(const std::string& str)
{
	if (str == "miter")
	{
		return JOIN_MITER_SQUARE;
	}
	else if (str == "round")
	{
		return JOIN_ROUND;
	}
	else if (str == "bevel")
	{
		return JOIN_BEVEL_BUTT;
	}
	return JOIN_MITER_SQUARE;
}

static std::vector<paint_order_t> parsePaintOrder(const std::string& str)
{
	if ((str == "normal") || (str == "fill stroke markers"))
	{
		return {ORDER_FILL, ORDER_STROKE, ORDER_MARKERS};
	}
	else if (str == "fill markers stroke")
	{
		return {ORDER_FILL, ORDER_MARKERS, ORDER_STROKE};
	}
	else if (str == "markers fill stroke")
	{
		return {ORDER_MARKERS, ORDER_FILL, ORDER_STROKE};
	}
	else if (str == "markers stroke fill")
	{
		return {ORDER_MARKERS, ORDER_STROKE, ORDER_FILL};
	}
	else if (str == "stroke fill markers")
	{
		return {ORDER_STROKE, ORDER_FILL, ORDER_MARKERS};
	}
	else if (str == "stroke markers fill")
	{
		return {ORDER_STROKE, ORDER_MARKERS, ORDER_FILL};
	}
	return {ORDER_FILL, ORDER_STROKE, ORDER_MARKERS};
}

/************************************************************************* stroke style functions */

static std::string::const_iterator getNextDashItem(std::string::const_iterator s, char* it)
{
	int n = 0;

	it[0] = '\0';
	// Skip white spaces and commas
	while (*s && (isspace(*s) || *s == ','))
	{
		s++;
	}
	// Advance until whitespace, comma or end.
	while (*s && (!isspace(*s) && *s != ','))
	{
		if (n < 63)
		{
			it[n++] = *s;
		}
		s++;
	}
	it[n++] = '\0';
	return s;
}

static void parseStrokeDashArray(const std::string& text, const float fontSize, const float viewLength, std::vector<float>& strokeDashArray)
{
	char item[64];
	float sum = 0.0f;
	std::string::const_iterator s = text.begin();

	// Handle "none"
	if (s[0] == 'n')
	{
		return;
	}

	// Parse dashes
	while (*s)
	{
		s = getNextDashItem(s, item);

		if (!*item)
		{
			break;
		}

		const float value = fabsf(parseCoordinate(fontSize, item, 0.0f, viewLength));
		strokeDashArray.push_back(value);
		sum += value;
	}

	if (sum <= 1e-6f)
	{
		strokeDashArray.clear();
	}
}

/************************************************************************ style parsing functions */

/** split string at semicolons. Parse each item as name/value pair.
 * @param str string to be parsed.
 * @return map of recovered name/value pairs.
 */
static std::map<std::string, std::string> parse_style(const std::string& str)
{
	std::map<std::string, std::string> result;

	std::stringstream stream(str);
	std::string decl;

	while (std::getline(stream, decl, ';'))
	{
		decl = trim(decl);

		if (decl.empty())
		{
			continue;
		}

		const size_t colonPos = decl.find(':');

		if (colonPos == std::string::npos)
		{
			continue;
		}

		std::string name  = decl.substr(0, colonPos);
		std::string value = decl.substr(colonPos + 1);
		name = trim(name);
		value = trim(value);

		if (!name.empty())
		{
			result[name] = value;
		}
	}
	return result;
}


static bool parseAttr(ElementSVG& attr, const std::string& name, const std::string& value, const float viewLength)
{
	const float fontSize = attr.fontSize;

	if (name == "style")
	{
		std::map<std::string, std::string> items = parse_style(value);
		for (std::map<std::string, std::string>::const_iterator iter = items.begin(); iter != items.end(); ++iter)
		{
			parseAttr(attr, iter->first, iter->second, viewLength);
		}
	}
	else if (name == "class")
	{
		// TODO: look up style attributes from CSS definition
	}
	else if (name == "display")
	{
		if (value == "none")
		{
			// Don't reset to true on display:inline, one display:none hides the whole subtree
			attr.visible = false;
		}
	}
	else if (name == "fill")
	{
		if (value == "none")
		{
			attr.fill.type = PAINT_NONE;
		}
		else if (value.compare(0, 4, "url(") == 0)
		{
			attr.fillGradientName = parseUrl(value);
		}
		else
		{
			attr.fill.type = PAINT_COLOR;
			attr.fill.stops[0.0f] = parse_color(value);
		}
	}
	else if (name == "opacity")
	{
		attr.opacity = parseOpacity(value);
	}
	else if (name == "fill-opacity")
	{
		attr.fill.stops[0.0f].a = parseOpacity(value);
	}
	else if (name == "stroke")
	{
		if (value == "none")
		{
			attr.stroke.type = PAINT_NONE;
		}
		else if (value.compare(0, 4, "url(") == 0)
		{
			attr.strokeGradientName = parseUrl(value);
		}
		else
		{
			attr.stroke.type = PAINT_COLOR;
			attr.stroke.stops[0.0f] = parse_color(value);
		}
	}
	else if (name == "stroke-width")
	{
		attr.strokeWidth = parseCoordinate(fontSize, value, 0.0f, viewLength);
	}
	else if (name == "stroke-dasharray")
	{
		parseStrokeDashArray(value, fontSize, viewLength, attr.strokeDashArray);
	}
	else if (name == "stroke-dashoffset")
	{
		attr.strokeDashOffset = parseCoordinate(fontSize, value, 0.0f, viewLength);
	}
	else if (name == "stroke-opacity")
	{
		attr.stroke.stops[0.0f].a = parseOpacity(value);
	}
	else if (name == "stroke-linecap")
	{
		attr.strokeLineCap = parseLineCap(value);
	}
	else if (name == "stroke-linejoin")
	{
		attr.strokeLineJoin = parseLineJoin(value);
	}
	else if (name == "stroke-miterlimit")
	{
		attr.miterLimit = std::max(stof(value), 0.0f);
	}
	else if (name == "fill-rule")
	{
		attr.fillAlternate = (value == "evenodd");     // alterate value: "nonzero" -> false
	}
	else if (name == "font-size")
	{
		attr.fontSize = parseCoordinate(fontSize, value, 0.0f, viewLength);
	}
	else if (name == "transform")
	{
		const glm::mat4 xform = parseTransform(value);
		attr.xform = attr.xform * xform;
	}
	else if (name == "paint-order")
	{
		attr.paintOrder = parsePaintOrder(value);
	}
	else if (name == "id")
	{
		attr.id = value;
	}
	else
	{
		return false;
	}
	return true;
}

/************************************************************************* path parsing functions */

/** add Bezier points to points.
 */
static void complete_bezier_points(std::vector<glm::vec2>& pts, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& d)
{
	const size_t subdivisions = 4;

	for (size_t s = 1; s <= subdivisions; s++)
	{
		const float t = static_cast<float>(s) / static_cast<float>(subdivisions);
		const float it = 1.0f - t;
		const glm::vec2 pos = it * it * it * a + 3.0f * it * it * t * b + 3.0f * it * t * t * c + t * t * t * d;
		pts.push_back(pos);
	}
}

/** get the number of arguments for all commands.
 * @param cmd path command letter.
 * @return number of arguments for given command.
 */
static size_t getArgsPerElement(const char cmd)
{
	switch (cmd)
	{
		case 'v':
		case 'V':
		case 'h':
		case 'H':
			return 1;
		case 'm':
		case 'M':
		case 'l':
		case 'L':
		case 't':
		case 'T':
			return 2;
		case 'q':
		case 'Q':
		case 's':
		case 'S':
			return 4;
		case 'c':
		case 'C':
			return 6;
		case 'a':
		case 'A':
			return 7;
		case 'z':
		case 'Z':
			return 0;
		default:
			break;
	}
	throw std::runtime_error("invalid path command");
}

/** glm 2D rotation matrix.
 * @param a rotation angle in radians.
 * @return 2D rotation matrix.
 */
static glm::mat2 rotation2D(const float a)
{
	const float s = std::sinf(a);
	const float c = std::cosf(a);

	return glm::mat2(c, -s, s, c);
}

/** sample an arc with points.
 * @param pts list of output points.
 * @param c center point.
 * @param r lengths of half axes.
 * @param phi rotation angle for the ellipse.
 * @param angle1 drawing start angle.
 * @param angle2 drawing end angle.
 */
static void add_arc_points(std::vector<glm::vec2>& pts,
                           const glm::vec2& c,
                           const glm::vec2& r,
						   const float phi,
                           const float angle1,
                           const float angle2)
{
	const float min_step = 10.0f * static_cast<float>(M_PI) / 180.0f;    // 10 degrees
	const size_t num_steps = static_cast<size_t>(ceilf(fabsf(angle2 - angle1) / min_step));
	const float step = (angle2 - angle1) / static_cast<float>(num_steps);
	const glm::mat2 rot = rotation2D(phi);

	for (size_t i = 1; i <= num_steps; i++)
	{
		const float a = -angle1 - static_cast<float>(i) * step;
		const glm::vec2 d(r.x * cosf(a), r.y * sinf(a));
		pts.push_back(rot * (c + d));
	}
}

/**  Compute the centre‑point parameters (c, a1, da) required by the SVG arc implementation note.
 * @param p1 start point of the arc.
 * @param p2 end point of the arc.
 * @param r radius of the arc in x and y direction.
 * @param phi opening angle of the arc.
 * @param fa large arc flag, specifies to draw the small (<180°)
 * or large (>180°) angle between the two points.
 * @param fs sweep flag determining the direction of drawing
 * to be clockwise (1) or counter-clockwise (0).
 * @param c center point of the arc, to be calculated as output parameter.
 * @param a1 angle of the start point with respect to the x axis.
 * @param da sweep angle of the arc.
 * @return true for a proper arc, false when the arc collapses to a straight line.
 */
static bool centreParams(const glm::vec2& p1,
                         const glm::vec2& p2,
                         const glm::vec2& r,
                         const float phi,
                         const bool fa,
                         const bool fs,
                         glm::vec2& c,
                         float& a1,
                         float& da)
{
	const glm::vec2 d = p1 - p2;
	const glm::mat2 R = rotation2D(phi);
	const glm::vec2 pt1 = R * d * 0.5f;                // (1) p′
	const float lambda = pt1.x * pt1.x / (r.x * r.x) + pt1.y * pt1.y / (r.y * r.y);
	const glm::vec2 sr = r * std::max(std::sqrt(lambda), 1.0f);      // scale radii

	// (2) centre offset in the rotated system
	const float sq = std::max(0.0f, sr.x * sr.x * sr.y * sr.y
								  - sr.x * sr.x * pt1.y * pt1.y
			                      - sr.y * sr.y * pt1.x * pt1.x);
	const float denom = sr.x * sr.x * pt1.y * pt1.y + sr.y * sr.y * pt1.x * pt1.x;

	const float sign = (fa == fs) ? -1.0f : 1.0f;
	const float coeff = (denom > 0.0f) ? (sign * std::sqrt(sq / denom)) : 0.0f;

	const glm::vec2 cPrime(coeff* sr.x * pt1.y / sr.y, -coeff * sr.y * pt1.x / sr.x);

	// (3) centre in original coordinates
	c = (p1 + p2) * 0.5f + R * cPrime;

	// (4) start‑angle and sweep‑angle
	const glm::vec2 u = (pt1 - cPrime) / sr;
	const glm::vec2 v = (-pt1 - cPrime) / sr;
	a1 = std::atan2(u.y, u.x);
	da = std::atan2(v.y, v.x) - a1;

	if (!fs && (da > 0))
	{
		da -= 2.0f * static_cast<float>(M_PI);
	}

	if (fs  && (da < 0))
	{
		da += 2.0f * static_cast<float>(M_PI);
	}
	return true;
}

static void pathArcTo(std::vector<glm::vec2>& pts,
                      const glm::vec2& p1,
                      const std::vector<float>& args,
                      const bool rel)
{
	// 1) unpack arguments
	const glm::vec2 r(std::fabs(args[0]), std::fabs(args[1]));
	const float phi = args[2] * static_cast<float>(M_PI) / 180.0f; // z‑axis rotation
	const bool fa = (args[3] > 0.5f);                              // large‑arc flag
	const bool fs = (args[4] > 0.5f);                              // sweep flag
	const glm::vec2 v1(args[5], args[6]);
	const glm::vec2 p2 = (rel ? (p1 + v1) : v1);                   // end point

	// 2) degenerate cases → straight line
	if ((glm::length(p1 - p2) < 1e-6f) || (r.x < 1e-6f) || (r.y < 1e-6f))
	{
		pts.push_back(p2);
		return;
	}

	// 3)  centre‑point parametrisation
	glm::vec2 center;
	float a1;
	float da;
	if (!centreParams(p1, p2, r, phi, fa, fs, center, a1, da))
	{
		// should not happen, handle as straight line
		pts.push_back(p2);
		return;
	}

	// 4) create points for the ellipse or circle
    add_arc_points(pts, center, r, phi, -a1, -a1 - da);
}

/** split path definitionn into commands and numerical values.
 * numerical values may contains a leading sign, integer part, decimal point, fractional part,
 * and exponent in scientific notation.
 * @param path text for the path definition.
 * @return list of split commands and numerical values.
 */
static std::vector<std::string> parsePathDescription(const std::string& path)
{
	std::vector<std::string> result;
	std::string token;
	std::regex pattern(R"([MmLlHhVvCcSsQqTtAaZz]|[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?)");

	std::string::const_iterator it = path.cbegin();

	while (it != path.cend())
	{
		std::smatch match;
		std::string::const_iterator searchStart(it);

		if (std::regex_search(searchStart, path.cend(), match, pattern))
		{
			result.push_back(match.str());
			it = searchStart + match.position() + match.length();
		}
		else
		{
			break;
		}
	}

	return result;
}

/** parse path description into a list of vertices.
 * @param text string of path description.
 * @param style current style attributes for the current path.
 * @return list of paths
 */
static std::vector<path_t> parse_path(const std::string& text, const ElementSVG& style, bool& closed)
{
	std::vector<std::string> path = parsePathDescription(text);

	char cmd = '\0';
	size_t req_args = 0;
	std::vector<float> args;
	std::vector<glm::vec2> pts;
	std::vector<path_t> plist;
	glm::vec2 start(0.0f, 0.0f);
	glm::vec2 last(0.0f, 0.0f);

	for (size_t i = 0; i < path.size(); i++)
	{
		if ((path[i].size() == 1) && isalpha(path[i][0]))
		{
			cmd = path[i][0];
			req_args = getArgsPerElement(cmd);
		}
		else
		{
			const float value = stof(path[i]);
			args.push_back(value);
		}

		if (cmd && (args.size() == req_args))
		{
			glm::vec2 current = last;
			switch (cmd)
			{
				case 'm':
				case 'M':   // move to
				{
					// save previous path
					if (pts.size() > 1)
					{
						plist.push_back(createPath(style.xform, pts));
						pts.clear();
					}

					const glm::vec2 a = glm::vec2(args[0], args[1]);
					current = ((cmd == 'm') ? (last + a) : a);
					start = current;
					pts.push_back(current);

					// M/m command may be continuted as L/l (lineto) with additional points
					cmd = (cmd == 'm') ? 'l' : 'L';
				}
				break;
				case 'l':
				case 'L':   // line to
				{
					const glm::vec2 a = glm::vec2(args[0], args[1]);
					current = ((cmd == 'l') ? (last + a) : a);
					pts.push_back(current);
				}
				break;
				case 'H':
				case 'h':   // horizontal line
					current = glm::vec2((cmd == 'h') ? (last.x + args[0]) : args[0], last.y);
					pts.push_back(current);
					break;
				case 'V':
				case 'v':   // vertical line
					current = glm::vec2(last.x, (cmd == 'v') ? (last.y + args[0]) : args[0]);
					pts.push_back(current);
					break;
				case 'C':
				case 'c':   // cubic Bézier curve
				{
					const glm::vec2 v1(args[0], args[1]);
					const glm::vec2 v2(args[2], args[3]);
					const glm::vec2 v3(args[4], args[5]);
					const glm::vec2 p1 = ((cmd == 'c') ? (last + v1) : v1);
					const glm::vec2 p2 = ((cmd == 'c') ? (last + v2) : v2);
					current = ((cmd == 'c') ? (last + v3) : v3);
					complete_bezier_points(pts, last, p1, p2, current);
				}
				break;
				case 'S':
				case 's':   // smooth cubic Bézier curve
				{
					const glm::vec2 v1(args[0], args[1]);
					const glm::vec2 v2(args[2], args[3]);
					const glm::vec2 p1 = 2.0f * last - pts[pts.size() - 1];
					const glm::vec2 p2 = ((cmd == 's') ? (last + v1) : v1);
					current = ((cmd == 's') ? (last + v2) : v2);
					complete_bezier_points(pts, last, p1, p2, current);
				}
				break;
				case 'Q':
				case 'q':   // quadratic Bézier curve
				{
					const glm::vec2 v1(args[0], args[1]);
					const glm::vec2 v2(args[2], args[3]);

					const glm::vec2 c0 = ((cmd == 'q') ? (last + v1) : v1);
					current = ((cmd == 'q') ? (last + v2) : v2);

					// convert to cubic Bézier
					const glm::vec2 c1 = last + 2.0f / 3.0f * (c0 - last);
					const glm::vec2 c2 = current + 2.0f / 3.0f * (c0 - current);

					complete_bezier_points(pts, last, c1, c2, current);
				}
				break;
				case 'T':
				case 't':   // smooth quadratic Bézier curve
				{
					const glm::vec2 v1(args[0], args[1]);

					current = ((cmd == 't') ? (last + v1) : v1);
					const glm::vec2 c0 = 2.0f * last - pts[pts.size() - 1];

					// Convert to cubix bezier
					const glm::vec2 c1 = last + 2.0f / 3.0f * (c0 - last);
					const glm::vec2 c2 = current + 2.0f / 3.0f * (c0 - current);

					complete_bezier_points(pts, last, c1, c2, current);
				}
				break;
				case 'A':
				case 'a':   // arc
					pathArcTo(pts, current, args, cmd == 'a');
					current = pts[pts.size() - 1];
					break;
				case 'Z':
				case 'z':   // close path
					pts.push_back(start);
					plist.push_back(createPath(style.xform, pts));
					closed = true;
					pts.clear();
					args.clear();
					current = start;
					break;
				default:
					throw std::runtime_error("invalid path command");
					// break;
			}
			args.erase(args.begin(), args.begin() + static_cast<std::vector<float>::difference_type>(req_args));

			// every command may be continued/repeated with additional arguments.
			// Do not clear the current state of command.
			last = current;
		}
	}

	// store remaining/not closed path
	if (pts.size() > 1)
	{
		plist.push_back(createPath(style.xform, pts));
	}

	return plist;
}

/********************************************************* other geometric form parsing functions */

static path_t parseRect(const ParserSVG& p, const ElementSVG& style, const std::map<std::string, std::string>& attr)
{
	glm::vec2 x1(0.0f, 0.0f);
	glm::vec2 size(0.0f, 0.0f);
	glm::vec2 r(-1.0f, -1.0f); // marks not set

	for (std::map<std::string, std::string>::const_iterator iter = attr.begin(); iter != attr.end(); ++iter)
	{
		if (iter->first == "x")
		{
			x1.x = parseCoordinate(style.fontSize, iter->second, p.viewMin.x, p.viewSize.x);
		}

		if (iter->first == "y")
		{
			x1.y = parseCoordinate(style.fontSize, iter->second, p.viewMin.y, p.viewSize.y);
		}

		if (iter->first == "width")
		{
			size.x = parseCoordinate(style.fontSize, iter->second, 0.0f, p.viewSize.x);
		}

		if (iter->first == "height")
		{
			size.y = parseCoordinate(style.fontSize, iter->second, 0.0f, p.viewSize.y);
		}

		if (iter->first == "rx")
		{
			r.x = fabsf(parseCoordinate(style.fontSize, iter->second, 0.0f, p.viewSize.x));
		}

		if (iter->first == "ry")
		{
			r.y = fabsf(parseCoordinate(style.fontSize, iter->second, 0.0f, p.viewSize.y));
		}
	}

	if ((r.x < 0.0f) && (r.y > 0.0f))
	{
		r.x = r.y;
	}

	if ((r.y < 0.0f) && (r.x > 0.0f))
	{
		r.y = r.x;
	}

	r.x = glm::clamp(r.x, 0.0f, size.x / 2.0f);
	r.y = glm::clamp(r.y, 0.0f, size.y / 2.0f);

	path_t pts;

	if ((fabsf(size.x) > 0.0f) && (fabsf(size.y) > 0.0f))
	{
		const float eps = 1e-5f;
		const glm::vec2 x2 = x1 + size;

		if ((r.x < eps) || (r.y < eps))
		{
			pts.push_back(glm::vec2(x1.x, x1.y));
			pts.push_back(glm::vec2(x2.x, x1.y));
			pts.push_back(glm::vec2(x2.x, x2.y));
			pts.push_back(glm::vec2(x1.x, x2.y));
		}
		else
		{
			// add start points for arcs
			if (size.y > 2 * r.y)
			{
				pts.push_back(glm::vec2(x1.x, x1.y + r.y));
			}
			add_arc_points(pts, glm::vec2(x1.x + r.x, x1.y + r.y), r, 0.0f, static_cast<float>(M_PI), 0.5f * static_cast<float>(M_PI));
			if (size.x > 2 * r.x)
			{
				pts.push_back(glm::vec2(x2.x - r.x, x1.y));
			}
			add_arc_points(pts, glm::vec2(x2.x - r.x, x1.y + r.y), r, 0.0f, 0.5f * static_cast<float>(M_PI), 0.0f);
			if (size.y > 2 * r.y)
			{
				pts.push_back(glm::vec2(x2.x, x2.y - r.y));
			}
			add_arc_points(pts, glm::vec2(x2.x - r.x, x2.y - r.y), r, 0.0f, 0.0f, -0.5f * static_cast<float>(M_PI));
			if (size.x > 2 * r.x)
			{
				pts.push_back(glm::vec2(x1.x + r.x, x2.y));
			}
			add_arc_points(pts, glm::vec2(x1.x + r.x, x2.y - r.y), r, 0.0f, -0.5f * static_cast<float>(M_PI), -static_cast<float>(M_PI));
		}
	}
	return pts;
}

static path_t parseCircle(const ParserSVG& p, const ElementSVG& style, const std::map<std::string, std::string>& attr)
{
	glm::vec2 c(0.0f, 0.0f);
	glm::vec2 r(0.0f, 0.0f);

	for (std::map<std::string, std::string>::const_iterator iter = attr.begin(); iter != attr.end(); ++iter)
	{
		if (iter->first == "cx")
		{
			c.x = parseCoordinate(style.fontSize, iter->second, p.viewMin.x, p.viewSize.x);
		}

		if (iter->first == "cy")
		{
			c.y = parseCoordinate(style.fontSize, iter->second, p.viewMin.y, p.viewSize.y);
		}

		if (iter->first == "r")
		{
			r.x = fabsf(parseCoordinate(style.fontSize, iter->second, 0.0f, glm::length(p.viewSize)));
			r.y = r.x;
		}
	}

	path_t pts;
	add_arc_points(pts, c, r, 0.0f, 0.0f, 2.0f * static_cast<float>(M_PI));
	return pts;
}

static path_t parseEllipse(const ParserSVG& p, const ElementSVG& style, const std::map<std::string, std::string>& attr)
{
	glm::vec2 c(0.0f, 0.0f);
	glm::vec2 r(0.0f, 0.0f);

	for (std::map<std::string, std::string>::const_iterator iter = attr.begin(); iter != attr.end(); ++iter)
	{
		if (iter->first == "cx")
		{
			c.x = parseCoordinate(style.fontSize, iter->second, p.viewMin.x, p.viewSize.x);
		}

		if (iter->first == "cy")
		{
			c.y = parseCoordinate(style.fontSize, iter->second, p.viewMin.y, p.viewSize.y);
		}

		if (iter->first == "rx")
		{
			r.x = fabsf(parseCoordinate(style.fontSize, iter->second, 0.0f, p.viewSize.x));
		}

		if (iter->first == "ry")
		{
			r.y = fabsf(parseCoordinate(style.fontSize, iter->second, 0.0f, p.viewSize.y));
		}
	}
	path_t pts;
	add_arc_points(pts, c, r, 0.0f, 0.0f, 2.0f * static_cast<float>(M_PI));
	return pts;
}

static path_t parseLine(const ParserSVG& p, const ElementSVG& style, const std::map<std::string, std::string>& attr)
{
	glm::vec2 x1(0.0f, 0.0f);
	glm::vec2 x2(0.0f, 0.0f);

	for (std::map<std::string, std::string>::const_iterator iter = attr.begin(); iter != attr.end(); ++iter)
	{
		if (iter->first == "x1")
		{
			x1.x = parseCoordinate(style.fontSize, iter->second, p.viewMin.x, p.viewSize.x);
		}

		if (iter->first == "y1")
		{
			x1.y = parseCoordinate(style.fontSize, iter->second, p.viewMin.y, p.viewSize.y);
		}

		if (iter->first == "x2")
		{
			x2.x = parseCoordinate(style.fontSize, iter->second, p.viewMin.x, p.viewSize.x);
		}

		if (iter->first == "y2")
		{
			x2.y = parseCoordinate(style.fontSize, iter->second, p.viewMin.y, p.viewSize.y);
		}
	}

	path_t pts;
	pts.push_back(x1);
	pts.push_back(x2);
	return pts;
}

static path_t parsePoly(const std::map<std::string, std::string>& attr)
{
	std::vector<glm::vec2> pts;

	std::map<std::string, std::string>::const_iterator iter = attr.find("points");
	if (iter != attr.end())
	{
		const std::vector<float> args = parseNumbers(iter->second);
		for (size_t i = 0; i + 1 < args.size(); i += 2)
		{
			const glm::vec2 cp(args[i], args[i+1]);
			pts.push_back(cp);
		}
	}

	return pts;
}

/************************************************************* general SVG text parsing functions */

static void parseCSSStyle(ParserSVG& p, const std::string& content)
{
	NodeCSS css;

	css.parse(content);

	const std::vector<NodeCSS>& nodes = css.children();
	for (std::vector<NodeCSS>::const_iterator n = nodes.begin(); n != nodes.end(); ++n)
	{
	}

	(void)p;
}

static void parseSVG(ParserSVG& p, const std::map<std::string, std::string>& attr)
{
	for (std::map<std::string, std::string>::const_iterator iter = attr.begin(); iter != attr.end(); ++iter)
	{
		/* ignore "width" and "height" attributes, because the graphics are scaled to unit size into [-1; 1]. */
		if (iter->first == "viewBox")
		{
			// viewbox sets the preferred clipping area of the graphics
			std::regex re(R"((-?\d*\.?\d+)\s*[, ]\s*(-?\d*\.?\d+)\s*[, ]\s*(-?\d*\.?\d+)\s*[, ]\s*(-?\d*\.?\d+))");
			std::smatch match;

			if (std::regex_search(iter->second, match, re) && (match.size() == 5))
			{
				p.viewMin.x = std::stof(match[1].str());
				p.viewMin.y = std::stof(match[2].str());
				p.viewSize.x = std::stof(match[3].str());
				p.viewSize.y = std::stof(match[4].str());
			}
			else
			{
				throw std::runtime_error("invalid SVG format");
			}
		}
		else if (iter->first == "preserveAspectRatio")
		{
			if (iter->second.find("none") != std::string::npos)
			{
				// No uniform scaling
				p.alignType = ALIGN_NONE;
			}
			else
			{
				// parse x alignment
				if (iter->second.find("xMin") != std::string::npos)
				{
					p.alignFracX = 0.0f;
				}
				else if (iter->second.find("xMid") != std::string::npos)
				{
					p.alignFracX = 0.5f;
				}
				else if (iter->second.find("xMax") != std::string::npos)
				{
					p.alignFracX = 1.0f;
				}

				// parse y alignment
				if (iter->second.find("yMin") != std::string::npos)
				{
					p.alignFracY = 0.0f;
				}
				else if (iter->second.find("yMid") != std::string::npos)
				{
					p.alignFracY = 0.5f;
				}
				else if (iter->second.find("yMax") != std::string::npos)
				{
					p.alignFracY = 1.0f;
				}

				// üarse meet/slice
				if (iter->second.find("slice") != std::string::npos)
				{
					p.alignType = ALIGN_SLICE;
				}
				else
				{
					p.alignType = ALIGN_MEET;
				}
			}
		}
	}
}

/******************************************************************************* gradient parsing */

static GradientSVG parseGradient(const std::map<std::string, std::string>& attr, const paint_type_t type)
{
	GradientSVG grad;
	grad.type = type;

	if (grad.type == PAINT_LINEAR_GRADIENT)
	{
		grad.linear.x1 = {0.0f, UNITS_PERCENT};
		grad.linear.y1 = {0.0f, UNITS_PERCENT};
		grad.linear.x2 = {100.0f, UNITS_PERCENT};
		grad.linear.y2 = {0.0f, UNITS_PERCENT};
	}
	else if (grad.type == PAINT_RADIAL_GRADIENT)
	{
		grad.radial.cx = {50.0f, UNITS_PERCENT};
		grad.radial.cy = {50.0f, UNITS_PERCENT};
		grad.radial.r  = {50.0f, UNITS_PERCENT};
	}

	for (std::map<std::string, std::string>::const_iterator iter = attr.begin(); iter != attr.end(); ++iter)
	{
		if (iter->first == "id")
		{
			grad.id = iter->second;
		}
		else if (iter->first == "gradientUnits")
		{
			if (iter->second == "objectBoundingBox")
			{
				grad.globalUnits = false;
			}
			else if (iter->second == "userSpaceOnUse")
			{
				grad.globalUnits = true;
			}
			else
			{
				throw std::runtime_error("invalid gradient reference");
			}
		}
		else if (iter->first == "gradientTransform")
		{
			grad.xform = parseTransform(iter->second);
		}
		else if (iter->first == "cx")
		{
			grad.radial.cx = parseCoordinateRaw(iter->second);
			if (grad.radial.cx.units == UNITS_USER)
			{

			}
		}
		else if (iter->first == "cy")
		{
			grad.radial.cy = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "r")
		{
			grad.radial.r = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "fx")
		{
			grad.radial.fx = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "fy")
		{
			grad.radial.fy = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "x1")
		{
			grad.linear.x1 = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "y1")
		{
			grad.linear.y1 = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "x2")
		{
			grad.linear.x2 = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "y2")
		{
			grad.linear.y2 = parseCoordinateRaw(iter->second);
		}
		else if (iter->first == "spreadMethod")
		{
			if (iter->second == "pad")
			{
				grad.spread = SPREAD_PAD;
			}
			else if (iter->second == "reflect")
			{
				grad.spread = SPREAD_REFLECT;
			}
			else if (iter->second == "repeat")
			{
				grad.spread = SPREAD_REPEAT;
			}
		}
		else if (iter->first == "xlink:href")
		{
			grad.ref = iter->second;
		}
	}

	return grad;
}

static void parseGradientStop(GradientSVG& grad, const ElementSVG& style, const std::map<std::string, std::string>& attr)
{
	float stopOffset = 0;
	glm::vec4 stopColor(0.0f, 0.0f, 0.0f, 1.0f);

	for (std::map<std::string, std::string>::const_iterator iter = attr.begin(); iter != attr.end(); ++iter)
	{
		const std::string& name = iter->first;
		const std::string& value = iter->second;

		if (name == "stop-color")
		{
			stopColor = parse_color(value);
		}
		else if (name == "stop-opacity")
		{
			stopColor.a = parseOpacity(value);
		}
		else if (name == "offset")
		{
			stopOffset = parseCoordinate(style.fontSize, value, 0.0f, 1.0f);
		}
	}

	grad.stops[stopOffset] = stopColor;
}

static glm::mat4 scaleGradient(const glm::mat4& xform, const glm::vec2& v, const glm::vec2& s)
{
	glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(v.x, v.y, 0.0f));
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(s.x, s.y, 1.0f));
	return glm::inverse(scale * trans * xform);
}

/** center viewbox in a square of [-1; 1].
 */
static void scaleToViewbox(ParserSVG& p, std::vector<ElementSVG>& shapes)
{
	const bounds_t bounds = imageBounds(shapes);

	if (fabsf(p.viewSize.x) <= 0.0f)
	{
		p.viewMin.x = bounds.first.x;
		p.viewSize.x = bounds.second.x - bounds.first.x;
	}

	if (fabsf(p.viewSize.y) <= 0.0f)
	{
		p.viewMin.y = bounds.first.y;
		p.viewSize.y = bounds.second.y - bounds.first.y;
	}

	const float target_size = 2.0f;
	float s = 1.0f;

	// zoom to viewport
	switch (p.alignType)
	{
		case ALIGN_MEET:
		case ALIGN_NONE:
			// fit whole image into viewbox
			s = target_size / std::max(p.viewSize.x, p.viewSize.y);
			break;
		case ALIGN_SLICE:
			s = target_size / std::min(p.viewSize.x, p.viewSize.y);
			break;
		default:
			throw std::runtime_error("invalid alignment");
	}
	glm::vec2 t = -p.viewMin - 0.5f * p.viewSize;
	t.x += p.alignFracX * (target_size - p.viewSize.x * s) / s;
	t.y += p.alignFracY * (target_size - p.viewSize.y * s) / s;

	// transform paths, gradients and stroke-dash-arrays
	for (std::vector<ElementSVG>::iterator shape = shapes.begin(); shape != shapes.end(); ++shape)
	{
		for (std::vector<path_t>::iterator path = shape->paths.begin(); path != shape->paths.end(); ++path)
		{
			for (size_t i = 0; i < path->size(); i++)
			{
				glm::vec2& pt = (*path)[i];
				pt = (pt + t) * s;
			}
		}

		if ((shape->fill.type == PAINT_LINEAR_GRADIENT) || (shape->fill.type == PAINT_RADIAL_GRADIENT))
		{
			shape->fill.xform = scaleGradient(shape->fill.xform, t, glm::vec2(s, s));
		}

		if ((shape->stroke.type == PAINT_LINEAR_GRADIENT) || (shape->stroke.type == PAINT_RADIAL_GRADIENT))
		{
			shape->stroke.xform = scaleGradient(shape->stroke.xform, t, glm::vec2(s, s));
		}

		shape->strokeWidth *= s;
		shape->strokeDashOffset *= s;
		for (size_t i = 0; i < shape->strokeDashArray.size(); i++)
		{
			shape->strokeDashArray[i] *= s;
		}
	}
}

static Paint assignGradient(const std::map<std::string, GradientSVG>& gradients,
                            const glm::vec2& viewMin,
                            const glm::vec2& viewSize,
                            const std::string& id,
                            const bounds_t& localBounds,
                            const glm::mat4& xform,
                            const float fontSize)
{
	Paint paint;

	std::map<std::string, GradientSVG>::const_iterator iter = gradients.find(id);

	if (iter == gradients.end())
	{
		return paint;
	}
	const GradientSVG& grad = iter->second;

	while (iter != gradients.end())
	{
		if (iter->second.stops.size() > 0)
		{
			paint.stops = iter->second.stops;
			break;
		}
		iter = gradients.find(iter->second.ref);
	}

	// The shape width and height.
	const glm::vec2 origin = (grad.globalUnits ? viewMin : localBounds.first);
	const glm::vec2 size = (grad.globalUnits ? viewSize : (localBounds.second - localBounds.first));

	if (grad.type == PAINT_LINEAR_GRADIENT)
	{
		glm::vec2 p1;
		p1.x = convertToPixels(fontSize, grad.linear.x1, origin.x, size.x, true);
		p1.y = convertToPixels(fontSize, grad.linear.y1, origin.y, size.y, true);
		glm::vec2 p2;
		p2.x = convertToPixels(fontSize, grad.linear.x2, origin.x, size.x, true);
		p2.y = convertToPixels(fontSize, grad.linear.y2, origin.y, size.y, true);

		// Calculate transform aligned to the line
		const glm::vec2 d = p2 - p1;
		const float s = glm::length(d);
		const float angle = atan2f(d.y, d.x);

		const glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(s, 1.0f, 1.0f));
		const glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(p1.x, p1.y, 0.0f));
		const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));

		paint.xform = translation * rotation * scale;
	}
	else if (grad.type == PAINT_RADIAL_GRADIENT)
	{
		glm::vec2 c;
		c.x = convertToPixels(fontSize, grad.radial.cx, origin.x, size.x, true);
		c.y = convertToPixels(fontSize, grad.radial.cy, origin.y, size.y, true);
		glm::vec2 f;
		f.x = convertToPixels(fontSize, grad.radial.fx, origin.x, size.x, true);
		f.y = convertToPixels(fontSize, grad.radial.fy, origin.y, size.y, true);
		const float sl = sqrtf(size.x * size.x + size.y * size.y) / sqrtf(2.0f);
		const float r  = convertToPixels(fontSize, grad.radial.r, 0, sl);

		// Calculate transform aligned to the circle
		const glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(r, r, 1.0f));
		const glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(c.x, c.y, 0.0f));
		paint.xform = translation * scale;
		paint.f = f / r;
	}

	paint.xform = xform * grad.xform * paint.xform;
	paint.spread = grad.spread;
	paint.type = grad.type;

	return paint;
}

static void fillGradients(std::vector<ElementSVG>& shapes,
						  const std::map<std::string, GradientSVG>& gradients,
						  const glm::vec2& viewMin,
						  const glm::vec2& viewSize,
						  const float fontSize)
{
	for (std::vector<ElementSVG>::iterator shape = shapes.begin(); shape != shapes.end(); ++shape)
	{
		if (shape->fillGradientName != "")
		{
			bounds_t localBounds = getLocalBounds(shape->paths);
			shape->fill = assignGradient(gradients,
			                             viewMin,
			                             viewSize,
			                             shape->fillGradientName,
			                             localBounds,
			                             shape->xform,
			                             fontSize);
		}

		if (shape->strokeGradientName != "")
		{
			bounds_t localBounds = getLocalBounds(shape->paths);
			shape->stroke = assignGradient(gradients,
			                               viewMin,
			                               viewSize,
			                               shape->strokeGradientName,
			                               localBounds,
			                               shape->xform,
			                               fontSize);
		}
	}
}

static void parse_node(const NodeXML& node, ParserSVG& p, const ElementSVG& style)
{
	ElementSVG attrib = style;

	for (std::map<std::string, std::string>::const_iterator iter = node.attributes().begin(); iter != node.attributes().end(); ++iter)
	{
		parseAttr(attrib, iter->first, iter->second, glm::length(p.viewSize));
	}

	if (node.tag() == "svg")
	{
		parseSVG(p, node.attributes());
	}
	else if (node.tag() == "style")
	{
		parseCSSStyle(p, node.content());
	}
	else if (node.tag() == "defs")
	{
		// just parse child nodes
	}
	else if (node.tag() == "g")
	{
		// just parse child nodes
	}
	else if (node.tag() == "linearGradient")
	{
		const GradientSVG grad = parseGradient(node.attributes(), PAINT_LINEAR_GRADIENT);
		p.gradients[grad.id] = grad;
		p.currentGradient = grad.id;
	}
	else if (node.tag() == "radialGradient")
	{
		const GradientSVG grad = parseGradient(node.attributes(), PAINT_RADIAL_GRADIENT);
		p.gradients[grad.id] = grad;
		p.currentGradient = grad.id;
	}
	else if (node.tag() == "stop")
	{
		// Add stop to the last gradient.
		std::map<std::string, GradientSVG>::iterator iter = p.gradients.find(p.currentGradient);

		if (iter != p.gradients.end())
		{
			parseGradientStop(iter->second, attrib, node.attributes());
		}
	}
	else if (node.tag() == "line")
	{
		std::vector<glm::vec2> pts = parseLine(p, attrib, node.attributes());

		std::vector<path_t> plist;
		path_t path = createPath(attrib.xform, pts);
		plist.push_back(path);
		attrib.closed = false;

		p.shapes.push_back(transform_shape(plist, style));
	}
	else if ((node.tag() == "polyline") || (node.tag() == "polygon"))
	{
		std::map<std::string, std::string>::const_iterator iter = node.attributes().find("points");

		if (iter != node.attributes().end())
		{
			attrib.closed = (node.tag() == "polygon");
			std::vector<glm::vec2> pts = parsePoly(node.attributes());

			std::vector<path_t> plist;
			path_t path = createPath(style.xform, pts);
			plist.push_back(path);

			p.shapes.push_back(transform_shape(plist, attrib));
		}
	}
	else if (node.tag() == "path")
	{
		/* parse attribute "d" last to include all style attributes in this tag. */
		std::map<std::string, std::string>::const_iterator iter = node.attributes().find("d");

		if (iter != node.attributes().end())
		{
			std::vector<path_t> plist = parse_path(iter->second, attrib, attrib.closed);
			p.shapes.push_back(transform_shape(plist, attrib));
		}
	}
	else if (node.tag() == "rect")
	{
		std::vector<glm::vec2> pts = parseRect(p, attrib, node.attributes());

		std::vector<path_t> plist;
		path_t path = createPath(attrib.xform, pts);
		plist.push_back(path);
		attrib.closed = true;

		p.shapes.push_back(transform_shape(plist, attrib));
	}
	else if (node.tag() == "circle")
	{
		std::vector<glm::vec2> pts = parseCircle(p, attrib, node.attributes());

		std::vector<path_t> plist;
		path_t path = createPath(attrib.xform, pts);
		plist.push_back(path);
		attrib.closed = true;

		p.shapes.push_back(transform_shape(plist, attrib));
	}
	else if (node.tag() == "ellipse")
	{
		std::vector<glm::vec2> pts = parseEllipse(p, attrib, node.attributes());

		std::vector<path_t> plist;
		path_t path = createPath(attrib.xform, pts);
		plist.push_back(path);
		attrib.closed = true;

		p.shapes.push_back(transform_shape(plist, attrib));
	}

	for (std::vector<NodeXML>::const_iterator n = node.children().begin(); n != node.children().end(); n++)
	{
		parse_node(*n, p, attrib);
	}
}

/** interpolates the color of gradients.
 * @param paint gradient description. It must contain at least one stop.
 * @param point coordinates for which their color is requested.
 * @return interpolated color.
 */
static glm::vec4 interpolate_gradient(const Paint& paint, const glm::vec2& point)
{
	if (paint.type == PAINT_COLOR)
	{
		return paint.stops.begin()->second;
	}
	else if (paint.type == PAINT_LINEAR_GRADIENT)
	{
		const glm::vec4 pc = paint.xform * glm::vec4(point.x, point.y, 0.0f, 0.0f);
		const float t = pc.x;

		// std::cout << "linear gradient at " << print_vec2(point) << "\t->\t" << t << std::endl;

		// Clamp to the outermost stops
		if (t <= paint.stops.begin()->first)
		{
			return paint.stops.begin()->second;
		}
		if (t >= paint.stops.rbegin()->first)
		{
			return paint.stops.rbegin()->second;
		}

		// Find the first stop whose key is > t.
		std::map<float, glm::vec4>::const_iterator upper = paint.stops.upper_bound(t);        // > t
		std::map<float, glm::vec4>::const_iterator lower = std::prev(upper);                  // ≤ t

		// Linear interpolation factor in [0,1].
		const float factor = (t - lower->first) / (upper->first - lower->first);

		// Linear interpolation per component.
		return glm::mix(lower->second, upper->second, factor);
	}
	else if (paint.type == PAINT_RADIAL_GRADIENT)
	{
		return paint.stops.begin()->second;
	}
	throw std::runtime_error("no gradient for interpolation");
}

/** determines the signed area of the triangle given by its three corner points.
 * @param a corner point of the triangle.
 * @param b corner point of the triangle.
 * @param c corner point of the triangle.
 * @return signed area of the described triangle. The result is positive,
 * if the triangle is considered convex and the points are given in counter-clockwise order.
 */
static float triangle_area(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
	return 0.5f * ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
}

/** determine, if the first point lies inside the triangle given by the other points.
 * @param p test point.
 * @param a corner point of the triangle.
 * @param b corner point of the triangle.
 * @param c corner point of the triangle.
 * @return flag for the test point inside the triangle.
 */
static bool is_point_in_triangle(const glm::vec2& p,
                                 const glm::vec2& a,
                                 const glm::vec2& b,
                                 const glm::vec2& c)
{
	// calculate areas of partial triangles
	const float areaABC = triangle_area(a, b, c);

	if (fabsf(areaABC) <= 0.0f)
	{
		return false;
	}
	const float areaPAB = triangle_area(a, b, p) / areaABC;
	const float areaPBC = triangle_area(b, c, p) / areaABC;
	const float areaPCA = triangle_area(c, a, p) / areaABC;

	// check if the sum of partial areas is the full area.
	// return (areaABC >= areaPAB + areaPBC + areaPCA);
	return areaPAB > 0.0f && areaPBC > 0.0f && areaPCA > 0.0f;
}

/** reduce angle into an interval of (-π, π].
 * @param a angle
 * @return angle unwrapped into (-π, π].
 */
static float wrapAngle(float a)
{
	const float twoPi = 2.0f * static_cast<float>(M_PI);

	while (a <= -static_cast<float>(M_PI))
	{
		a += twoPi;
	}
	while (a >   static_cast<float>(M_PI))
	{
		a -= twoPi;
	}
	return a;
}

/** Winding number of a closed path, measured relative to its first segment.
 * @param poly closed path.
 * @return signed winding number (…, -2, -1, 0, 1, 2, …). Positive winding number
 * means a counter-clockwise winding.
 */
static int winding_number(const std::vector<glm::vec2>& poly)
{
	if (poly.size() < 2)
	{
		return 0;
	}

	// vektor of first segment
	glm::vec2 prev;
	size_t i_start;
	for (i_start = 0; (glm::length(prev) <= 0.0f) && (i_start < poly.size()); i_start++)
	{
		prev = poly[(i_start + 1) % poly.size()] - poly[i_start];
	}

	if (glm::length(prev) <= 0.0f)
	{
		return 0; // degenerate line
	}

	float totalAngle = 0.0f;

	for (size_t i = i_start; i < poly.size(); ++i)
	{
		glm::vec2 cur = poly[(i + 1) % poly.size()] - poly[i]; // current segment

		// skip consecutive identical points
		if (glm::length(cur) <= 0.0f)
		{
			continue;
		}

		// signed angle between prev and cur
		float cross = prev.x * cur.y - prev.y * cur.x;   // 2D cross product
		float dot   = prev.x * cur.x + prev.y * cur.y;   // scalar product
		float delta = std::atan2(cross, dot);            // signed angle
		delta = wrapAngle(delta);
		totalAngle += delta;

		prev = cur;
	}

	const float twoPi = 2.0f * static_cast<float>(M_PI);
	return static_cast<int>(std::round(totalAngle / twoPi));
}

/** triangulate a polygon.
 * @param points list of points on the contour or interior of the surface.
 * @return list of vertex indices suitable for drawing.
 * Every triplett of indices describe one triangle of points to draw.
 */
static std::vector<GLuint> triangulate_polygon(const std::vector<glm::vec2>& points)
{
	size_t n = points.size();

	if (n == 0)
	{
		// empty points, empty indices
		return {};
	}
	else if (n < 3)
	{
		// degenerate triangle, draw either as line or point.
		return {0, 0, static_cast<GLuint>(n - 1)};
	}

	const int winding = winding_number(points);
	std::vector<size_t> poly(n);
	for (size_t i = 0; i < n; i++)
	{
		poly[i] = i; // list of all point indices
	}

	std::vector<GLuint> indices;
	while (poly.size() > 3)
	{
		bool ear_found = false;

		// try to cut index i from the list of all vertices
		for (size_t i = 0; i < poly.size(); i++)
		{
			const size_t prev = (i + poly.size() - 1) % poly.size();
			const size_t next = (i + 1) % poly.size();

			const size_t ia = poly[prev];
			const size_t ib = poly[i];
			const size_t ic = poly[next];

			const glm::vec2& a = points[ia];
			const glm::vec2& b = points[ib];
			const glm::vec2& c = points[ic];

			// cut only convex ears.
			const bool convex = (triangle_area(a, b, c) * static_cast<float>(winding) > 0.0f);
			bool isEar = true;

			// check if any other point lies inside the current triangle.
			for (size_t j = 0; j < poly.size(); j++)
			{
				if ((j == prev) || (j == i) || (j == next))
				{
					continue;
				}
				const size_t ij = poly[j];

				if (is_point_in_triangle(points[ij], a, b, c))
				{
					isEar = false;   // point lies inside triangle
					break;
				}
			}

			if (convex && isEar)
			{
				indices.push_back(static_cast<GLuint>(ia));
				indices.push_back(static_cast<GLuint>(ib));
				indices.push_back(static_cast<GLuint>(ic));

				// remove ear point
				poly.erase(poly.begin() + static_cast<ptrdiff_t>(i));
				ear_found = true;
				break;    // restart loop because number of remaining points changed
			}
		}

		if (!ear_found)
		{
			// std::cerr << "Could not triangulate the polygon!" << std::endl;
			break;
		}
	}

	// add last remaining triangle
	if (poly.size() == 3)
	{
		indices.push_back(static_cast<GLuint>(poly[0]));
		indices.push_back(static_cast<GLuint>(poly[1]));
		indices.push_back(static_cast<GLuint>(poly[2]));
	}
	return indices;
}

/** build a half-circle cap at the joint of lines.
 * @param center center of the half-circle
 * @param dir1 direction of incomming line.
 * @param dir2 direction of outgoing line.
 * @param halfW half line width.
 * @param verts list of all vertices.
 * @param idx list of all vertex indices.
 */
static void join_segments_round(const glm::vec2& center,
                                const glm::vec2& dir1,
                                const glm::vec2& dir2,
                                const float halfW,
                                const glm::vec4& color,
                                const float z,
                                std::vector<Vertex>& verts,
                                std::vector<GLuint>& idx)
{
	const float cross = dir1.x * dir2.y - dir1.y * dir2.x;
	const float dot = glm::dot(dir1, dir2);
	const float totalAngle = atan2f(cross, dot);

	// add mid point of the half circle
	const glm::vec3 normal(0.0f, 0.0f, 1.0f);
	const glm::vec2 tex(0.0f, 0.0f);
	const Vertex mid(glm::vec3(center.x, -center.y, z), normal, color, tex);
	const size_t centerIdx = verts.size();

	verts.push_back(mid);

	// points along the half circle
	const float min_step = 10.0f * static_cast<float>(M_PI) / 180.0f;    // 10 degrees
	const size_t num_steps = static_cast<size_t>(ceilf(fabsf(totalAngle) / min_step));
	const float step = totalAngle / static_cast<float>(num_steps);
	const glm::vec2 norm = ((cross > 0.0f) ? glm::vec2(-dir1.y, dir1.x) : glm::vec2(dir1.y, -dir1.x));

	for (size_t i = 0; i <= num_steps; i++)
	{
		const float angle = static_cast<float>(i) * step;
		const glm::vec2 offset = dir1 * fabsf(sinf(angle)) - norm * cosf(angle);
		const glm::vec2 p = center + halfW * offset;
		const Vertex v(glm::vec3(p.x, -p.y, z), normal, color, tex);
		verts.push_back(v);
		const size_t vIdx = centerIdx + i + 1;

		if (i > 0)
		{
			// triangle of the center, previous, and current vertices.
			idx.push_back(static_cast<GLuint>(centerIdx));
			idx.push_back(static_cast<GLuint>(vIdx - 1));
			idx.push_back(static_cast<GLuint>(vIdx));
		}
	}
}

/** build a straight cap at the joint of lines.
 * @param center joint point
 * @param dir1 direction of incomming line.
 * @param dir2 direction of outgoing line.
 * @param halfW half line width.
 * @param color line color
 * @param z layer
 * @param verts list of all vertices.
 * @param idx list of all vertex indices.
 */
static void join_segments_bevel(const glm::vec2& center,
                                const glm::vec2& dir1,
                                const glm::vec2& dir2,
                                const float halfW,
                                const glm::vec4& color,
                                const float z,
                                std::vector<Vertex>& verts,
                                std::vector<GLuint>& idx)
{
	const float cross = dir1.x * dir2.y - dir1.y * dir2.x;
	const float dot = glm::dot(dir1, dir2);
	const float angle = atan2f(cross, dot);

	// add hinge point
	const glm::vec3 normal(0.0f, 0.0f, 1.0f);
	const glm::vec2 tex(0.0f, 0.0f);
	const size_t centerIdx = verts.size();

	const glm::vec2 norm = ((cross > 0.0f) ? glm::vec2(-dir1.y, dir1.x) : glm::vec2(dir1.y, -dir1.x));
	const glm::vec2 offset = dir1 * fabsf(sinf(angle)) - norm * cosf(angle);

	const glm::vec2 p1 = center - halfW * norm;
	const glm::vec2 p2 = center + halfW * offset;

	const Vertex mid(glm::vec3(center.x, -center.y, z), normal, color, tex);
	const Vertex v1(glm::vec3(p1.x, -p1.y, z), normal, color, tex);
	const Vertex v2(glm::vec3(p2.x, -p2.y, z), normal, color, tex);

	verts.push_back(mid);
	verts.push_back(v1);
	verts.push_back(v2);

	// triangle of the center, previous, and current vertices.
	idx.push_back(static_cast<GLuint>(centerIdx + 0));
	idx.push_back(static_cast<GLuint>(centerIdx + 1));
	idx.push_back(static_cast<GLuint>(centerIdx + 2));
}

/** extend cap at the joint of lines.
 * @param center joint point
 * @param dir1 direction of incomming line.
 * @param dir2 direction of outgoing line.
 * @param halfW half line width.
 * @param color line color
 * @param z layer
 * @param verts list of all vertices.
 * @param idx list of all vertex indices.
 */
static void join_segments_miter(const glm::vec2& center,
                                const glm::vec2& dir1,
                                const glm::vec2& dir2,
                                const float halfW,
                                const float limit,
                                const glm::vec4& color,
                                const float z,
                                std::vector<Vertex>& verts,
                                std::vector<GLuint>& idx)
{
	const float cross = dir1.x * dir2.y - dir1.y * dir2.x;
	const float dot = glm::dot(dir1, dir2);
	const float angle = atan2f(cross, dot);

	// add hinge point (identical to bevel triangle)
	const glm::vec3 normal(0.0f, 0.0f, 1.0f);
	const glm::vec2 tex(0.0f, 0.0f);
	const size_t centerIdx = verts.size();

	const glm::vec2 norm = ((cross > 0.0f) ? glm::vec2(-dir1.y, dir1.x) : glm::vec2(dir1.y, -dir1.x));
	const glm::vec2 offset = dir1 * fabsf(sinf(angle)) - norm * cosf(angle);

	const glm::vec2 p1 = center - halfW * norm;
	const glm::vec2 p2 = center + halfW * offset;

	const Vertex mid(glm::vec3(center.x, -center.y, z), normal, color, tex);
	const Vertex v1(glm::vec3(p1.x, -p1.y, z), normal, color, tex);
	const Vertex v2(glm::vec3(p2.x, -p2.y, z), normal, color, tex);

	verts.push_back(mid);
	verts.push_back(v1);
	verts.push_back(v2);

	// triangle of the center, previous, and current vertices.
	idx.push_back(static_cast<GLuint>(centerIdx + 0));
	idx.push_back(static_cast<GLuint>(centerIdx + 1));
	idx.push_back(static_cast<GLuint>(centerIdx + 2));

	// add top triangle
	const glm::vec2 miterDir = glm::normalize(dir1 - dir2);
	const float miterLen = halfW / sinf(0.5f * (static_cast<float>(M_PI) - angle));

	// position of tip
	const glm::vec2 pt = center + miterDir * std::min(miterLen, limit * halfW);
	const Vertex top(glm::vec3(pt.x, -pt.y, z), normal, color, tex);
	verts.push_back(top);

	// triangle of the center, previous, and current vertices.
	idx.push_back(static_cast<GLuint>(centerIdx + 1));
	idx.push_back(static_cast<GLuint>(centerIdx + 2));
	idx.push_back(static_cast<GLuint>(centerIdx + 3));
}

static void join_segments(const ElementSVG& shape,
                          const glm::vec2& center,
                          const glm::vec2& dir1,
                          const glm::vec2& dir2,
                          const line_join_t join,
                          const float z,
                          std::vector<Vertex>& vertices,
                          std::vector<GLuint>& indices)
{
	if (shape.stroke.stops.size() == 0)
	{
		return;
	}
	const glm::vec4& color = shape.stroke.stops.begin()->second;
	switch (join)
	{
		case JOIN_BEVEL_BUTT:
			join_segments_bevel(center, dir1, dir2, shape.strokeWidth * 0.5f, color, z, vertices, indices);
			break;
		case JOIN_MITER_SQUARE:
			join_segments_miter(center, dir1, dir2, shape.strokeWidth * 0.5f, shape.miterLimit, color, z, vertices, indices);
			break;
		case JOIN_ROUND:
			join_segments_round(center, dir1, dir2, shape.strokeWidth * 0.5f, color, z, vertices, indices);
			break;
		default:
			break;
	}
}

/** converts stroke line into OpenGL vertices and indices to be rendered as triangles.
 * @param svg       container object.
 * @param points    points of a path.
 * @param shape     SVG element.
 * @param z         current z layer.
 */
static void add_shape_contour(GraphicsSVG& svg, const ElementSVG& shape, const std::vector<glm::vec2>& points, const size_t z)
{
	const float zoffset = 1e-4f;
	const float pz = static_cast<float>(z) * zoffset;

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	const float halfW = shape.strokeWidth * 0.5f;
	const size_t n = points.size();
	glm::vec2 prevdir(0.0f, 0.0f);

	if (shape.stroke.stops.size() == 0)
	{
		return;
	}

	// convert segements into rectangles
	for (size_t i = 0; i + 1 < n; i++)
	{
		const glm::vec2& curr = points[i];
		const glm::vec2& next = points[i + 1];
		const glm::vec2 diff = next - curr;

		// ignore identical consecutive points
		if (glm::length(diff) <= 0.0f)
		{
			continue;
		}

		const glm::vec2 dir = glm::normalize(diff);

		if ((i == 0) && !shape.closed)
		{
			// add start cap
			join_segments(shape, curr, -dir, dir, shape.strokeLineCap, pz, vertices, indices);
		}
		else if (glm::length(prevdir) > 0.0f)
		{
			join_segments(shape, curr, prevdir, dir, shape.strokeLineJoin, pz, vertices, indices);
		}

		// add two triangles for the segment rectangle
		const glm::vec2 norm = glm::vec2(-dir.y, dir.x);
		const glm::vec2 p1 = curr + halfW * norm;
		const glm::vec2 p2 = curr - halfW * norm;
		const glm::vec2 p3 = next + halfW * norm;
		const glm::vec2 p4 = next - halfW * norm;

		const glm::vec4 color1 = interpolate_gradient(shape.stroke, p1);
		const glm::vec4 color2 = interpolate_gradient(shape.stroke, p2);
		const glm::vec4 color3 = interpolate_gradient(shape.stroke, p3);
		const glm::vec4 color4 = interpolate_gradient(shape.stroke, p4);
		const glm::vec3 normal(0.0f, 0.0f, 1.0f);
		const glm::vec2 tex(0.0f, 0.0f);

		const Vertex v1(glm::vec3(p1.x, -p1.y, pz), normal, color1, tex);
		const Vertex v2(glm::vec3(p2.x, -p2.y, pz), normal, color2, tex);
		const Vertex v3(glm::vec3(p3.x, -p3.y, pz), normal, color3, tex);
		const Vertex v4(glm::vec3(p4.x, -p4.y, pz), normal, color4, tex);

		const GLuint i1 = static_cast<GLuint>(vertices.size());
		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);

		// first triangle
		indices.push_back(i1 + 0);
		indices.push_back(i1 + 1);
		indices.push_back(i1 + 2);

		// second triangle
		indices.push_back(i1 + 1);
		indices.push_back(i1 + 3);
		indices.push_back(i1 + 2);

		if (glm::length(dir) > 0.0f)
		{
			prevdir = dir;
		}

		if ((i + 2 == n) && !shape.closed)
		{
			// add end cap
			join_segments(shape, next, dir, -dir, shape.strokeLineCap, pz, vertices, indices);
		}
	}

	if (shape.closed)
	{
		const GLuint nv = static_cast<GLuint>(vertices.size());

		// first triangle
		indices.push_back(nv - 2);
		indices.push_back(nv - 1);
		indices.push_back(0);

		// second triangle
		indices.push_back(nv - 1);
		indices.push_back(1);
		indices.push_back(0);

		const glm::vec2 dir = glm::normalize(points[1] - points[0]);
		join_segments(shape, points[0], prevdir, dir, shape.strokeLineJoin, pz, vertices, indices);
	}

	svg.add(vertices, indices, GL_TRIANGLES);
}

static void add_shape_surface(GraphicsSVG& svg, const ElementSVG& shape, const std::vector<glm::vec2>& points, const size_t z)
{
	const float zoffset = 1e-4f;

	if (shape.fill.stops.size() == 0)
	{
		return;
	}

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices = triangulate_polygon(points);

	for (std::vector<glm::vec2>::const_iterator point = points.begin(); point != points.end(); ++point)
	{
		const glm::vec4 color = interpolate_gradient(shape.fill, *point);
		const glm::vec3 pos(point->x, -point->y, static_cast<float>(z) * zoffset);
		Vertex v(pos, glm::vec3(0.0f, 0.0f, 1.0f), color, glm::vec2(0.0f, 0.0f));
		vertices.push_back(v);
	}

	svg.add(vertices, indices, GL_TRIANGLES);
}

GraphicsSVG::GraphicsSVG(const std::string& file_name) :
	ShapeSet()
{
	// fix locale settings for numerical parsing
	setlocale(LC_ALL | ~LC_NUMERIC, "");

	// read file content
	FileSystem fs;
	const std::string content = fs.read_file(file_name);

	NodeXML xml;
	xml.parse(content);

	// parse file content
	ParserSVG p;
	ElementSVG attributes;

	parse_node(xml, p, attributes);

	// Create gradients after all definitions have been parsed
	fillGradients(p.shapes, p.gradients, p.viewMin, p.viewSize, attributes.fontSize);

	// Scale to viewBox
	scaleToViewbox(p, p.shapes);

	size_t z = 0;
	for (std::vector<ElementSVG>::const_iterator shape = p.shapes.begin(); shape != p.shapes.end(); ++shape)
	{
		for (std::vector<path_t>::const_iterator path = shape->paths.begin(); path != shape->paths.end(); ++path)
		{
			for (size_t i = 0; i < shape->paintOrder.size(); i++)
			{
				switch (shape->paintOrder[i])
				{
					case ORDER_STROKE:

						if (shape->stroke.type != PAINT_NONE)
						{
							add_shape_contour(*this, *shape, *path, z);
						}
						break;
					case ORDER_FILL:

						if (shape->fill.type != PAINT_NONE)
						{
							add_shape_surface(*this, *shape, *path, z);
						}
						break;
					default:
						break;
				}
				z++;
			}
		}
	}
}

GraphicsSVG::~GraphicsSVG(void)
{
}
