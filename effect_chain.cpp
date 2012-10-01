#define GL_GLEXT_PROTOTYPES 1

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include "util.h"
#include "effect_chain.h"
#include "gamma_expansion_effect.h"
#include "lift_gamma_gain_effect.h"
#include "colorspace_conversion_effect.h"

EffectChain::EffectChain(unsigned width, unsigned height)
	: width(width), height(height), finalized(false) {}

void EffectChain::add_input(const ImageFormat &format)
{
	input_format = format;
	current_color_space = format.color_space;
	current_gamma_curve = format.gamma_curve;
}

void EffectChain::add_output(const ImageFormat &format)
{
	output_format = format;
}
	
Effect *instantiate_effect(EffectId effect)
{
	switch (effect) {
	case GAMMA_CONVERSION:
		return new GammaExpansionEffect();
	case RGB_PRIMARIES_CONVERSION:
		return new GammaExpansionEffect();
	case LIFT_GAMMA_GAIN:
		return new LiftGammaGainEffect();
	}
	assert(false);
}

Effect *EffectChain::add_effect(EffectId effect_id)
{
	Effect *effect = instantiate_effect(effect_id);

	if (effect->needs_linear_light() && current_gamma_curve != GAMMA_LINEAR) {
		GammaExpansionEffect *gamma_conversion = new GammaExpansionEffect();
		gamma_conversion->set_int("source_curve", current_gamma_curve);
		effects.push_back(gamma_conversion);
		current_gamma_curve = GAMMA_LINEAR;
	}

	if (effect->needs_srgb_primaries() && current_color_space != COLORSPACE_sRGB) {
		assert(current_gamma_curve == GAMMA_LINEAR);
		ColorSpaceConversionEffect *colorspace_conversion = new ColorSpaceConversionEffect();
		colorspace_conversion->set_int("source_space", current_color_space);
		colorspace_conversion->set_int("destination_space", COLORSPACE_sRGB);
		effects.push_back(colorspace_conversion);
		current_color_space = COLORSPACE_sRGB;
	}

	// not handled yet
	assert(!effect->needs_many_samples());
	assert(!effect->needs_mipmaps());

	effects.push_back(effect);
	return effect;
}

// GLSL pre-1.30 doesn't support token pasting. Replace PREFIX(x) with <effect_id>_x.
std::string replace_prefix(const std::string &text, const std::string &prefix)
{
	std::string output;
	size_t start = 0;

	while (start < text.size()) {
		size_t pos = text.find("PREFIX(", start);
		if (pos == std::string::npos) {
			output.append(text.substr(start, std::string::npos));
			break;
		}

		output.append(text.substr(start, pos - start));
		output.append(prefix);
		output.append("_");

		pos += strlen("PREFIX(");
	
		// Output stuff until we find the matching ), which we then eat.
		int depth = 1;
		size_t end_arg_pos = pos;
		while (end_arg_pos < text.size()) {
			if (text[end_arg_pos] == '(') {
				++depth;
			} else if (text[end_arg_pos] == ')') {
				--depth;
				if (depth == 0) {
					break;
				}
			}
			++end_arg_pos;
		}
		output.append(text.substr(pos, end_arg_pos - pos));
		++end_arg_pos;
		assert(depth == 0);
		start = end_arg_pos;
	}
	return output;
}

void EffectChain::finalize()
{
	std::string frag_shader = read_file("header.glsl");

	for (unsigned i = 0; i < effects.size(); ++i) {
		char effect_id[256];
		sprintf(effect_id, "eff%d", i);
	
		frag_shader += "\n";
		frag_shader += std::string("#define FUNCNAME ") + effect_id + "\n";
		frag_shader += replace_prefix(effects[i]->output_convenience_uniforms(), effect_id);
		frag_shader += replace_prefix(effects[i]->output_glsl(), effect_id);
		frag_shader += "#undef PREFIX\n";
		frag_shader += "#undef FUNCNAME\n";
		frag_shader += std::string("#define LAST_INPUT ") + effect_id + "\n";
		frag_shader += "\n";
	}
	printf("%s\n", frag_shader.c_str());
	
	glsl_program_num = glCreateProgram();
	GLhandleARB vs_obj = compile_shader(read_file("vs.glsl"), GL_VERTEX_SHADER);
	GLhandleARB fs_obj = compile_shader(frag_shader, GL_FRAGMENT_SHADER);
	glAttachObjectARB(glsl_program_num, vs_obj);
	check_error();
	glAttachObjectARB(glsl_program_num, fs_obj);
	check_error();
	glLinkProgram(glsl_program_num);
	check_error();
}