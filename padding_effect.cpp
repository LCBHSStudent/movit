#include <GL/glew.h>
#include <assert.h>

#include "effect_util.h"
#include "padding_effect.h"
#include "util.h"

using namespace std;

namespace movit {

PaddingEffect::PaddingEffect()
	: border_color(0.0f, 0.0f, 0.0f, 0.0f),
	  output_width(1280),
	  output_height(720),
	  top(0),
	  left(0),
	  pad_from_bottom(0)
{
	register_vec4("border_color", (float *)&border_color);
	register_int("width", &output_width);
	register_int("height", &output_height);
	register_float("top", &top);
	register_float("left", &left);
	register_int("pad_from_bottom", &pad_from_bottom);
}

string PaddingEffect::output_fragment_shader()
{
	return read_file("padding_effect.frag");
}

void PaddingEffect::set_gl_state(GLuint glsl_program_num, const string &prefix, unsigned *sampler_num)
{
	Effect::set_gl_state(glsl_program_num, prefix, sampler_num);

	float offset[2];
	offset[0] = left / output_width;
	if (pad_from_bottom) {
		offset[1] = top / output_height;
	} else {
		offset[1] = (output_height - input_height - top) / output_height;
	}
	set_uniform_vec2(glsl_program_num, prefix, "offset", offset);

	float scale[2] = {
		float(output_width) / input_width,
		float(output_height) / input_height
	};
	set_uniform_vec2(glsl_program_num, prefix, "scale", scale);

	// Due to roundoff errors, the test against 0.5 is seldom exact,
	// even though we test for less than and not less-than-or-equal.
	// We'd rather keep an extra border pixel in those very rare cases
	// (where the image is shifted pretty much exactly a half-pixel)
	// than losing a pixel in the common cases of integer shift.
	// Thus the 1e-3 fudge factors.
	float texcoord_min[2] = {
		float((0.5f - 1e-3) / input_width),
		float((0.5f - 1e-3) / input_height)
	};
	set_uniform_vec2(glsl_program_num, prefix, "texcoord_min", texcoord_min);

	float texcoord_max[2] = {
		float(1.0f - (0.5f - 1e-3) / input_width),
		float(1.0f - (0.5f - 1e-3) / input_height)
	};
	set_uniform_vec2(glsl_program_num, prefix, "texcoord_max", texcoord_max);
}
	
// We don't change the pixels of the image itself, so the only thing that 
// can make us less flexible is if the border color can be interpreted
// differently in different modes.

// 0.0 and 1.0 are interpreted the same, no matter the gamma ramp.
// Alpha is not affected by gamma per se, but the combination of
// premultiplied alpha and non-linear gamma curve does not make sense,
// so if could possibly be converting blank alpha to non-blank
// (ie., premultiplied), we need our output to be in linear light.
bool PaddingEffect::needs_linear_light() const
{
	if ((border_color.r == 0.0 || border_color.r == 1.0) &&
	    (border_color.g == 0.0 || border_color.g == 1.0) &&
	    (border_color.b == 0.0 || border_color.b == 1.0) &&
	    border_color.a == 1.0) {
		return false;
	}
	return true;
}

// The white point is the same (D65) in all the color spaces we currently support,
// so any gray would be okay, but we don't really have a guarantee for that.
// Stay safe and say that only pure black and pure white is okay.
// Alpha is not affected by color space.
bool PaddingEffect::needs_srgb_primaries() const
{
	if (border_color.r == 0.0 && border_color.g == 0.0 && border_color.b == 0.0) {
		return false;
	}
	if (border_color.r == 1.0 && border_color.g == 1.0 && border_color.b == 1.0) {
		return false;
	}
	return true;
}

Effect::AlphaHandling PaddingEffect::alpha_handling() const
{
	// If the border color is black, it doesn't matter if we're pre- or postmultiplied.
	// Note that for non-solid black (i.e. alpha < 1.0), we're equally fine with
	// pre- and postmultiplied, but later effects might change this status
	// (consider e.g. blur), so setting DONT_CARE_ALPHA_TYPE is inappropriate,
	// as it propagate blank alpha through this effect.
	if (border_color.r == 0.0 && border_color.g == 0.0 && border_color.b == 0.0 && border_color.a == 1.0) {
		return DONT_CARE_ALPHA_TYPE;
	}

	// If the border color is solid, we preserve blank alpha, as we never output any
	// new non-solid pixels.
	if (border_color.a == 1.0) {
		return INPUT_PREMULTIPLIED_ALPHA_KEEP_BLANK;
	}

	// Otherwise, we're going to output our border color in premultiplied alpha,
	// so the other pixels better be premultiplied as well.
	return INPUT_AND_OUTPUT_PREMULTIPLIED_ALPHA;
}
	
void PaddingEffect::get_output_size(unsigned *width, unsigned *height, unsigned *virtual_width, unsigned *virtual_height) const
{
	*virtual_width = *width = output_width;
	*virtual_height = *height = output_height;
}
	
void PaddingEffect::inform_input_size(unsigned input_num, unsigned width, unsigned height)
{
	assert(input_num == 0);
	input_width = width;
	input_height = height;
}

}  // namespace movit
