#define GL_GLEXT_PROTOTYPES 1

#include "saturation_effect.h"
#include "util.h"

SaturationEffect::SaturationEffect()
	: saturation(1.0f)
{
	register_float("saturation", &saturation);
}

std::string SaturationEffect::output_glsl()
{
	return read_file("saturation_effect.glsl");
}