
#define SHADER_ANALYZER_IMPL
#include "../shader_analyzer.h"

#include <vector>
int main()
{
    sa_setRGAPath("../rga/");

	sa_SpirVShaderDesc desc = {};
	desc.BinaryFilePath = "sample_brdf.spv";
	desc.Type = sa_ShaderType_Pixel;

    sa_ShaderOutput output = sa_spirVShaderOutput(desc, sa_ShaderOutputType_ISA | sa_ShaderOutputType_RegisterAnalysis | sa_ShaderOutputType_Stats);

    printf("%s", output.ISA);
    printf("%s", output.RegisterAnalysis);
    sa_freeShaderOutput(output);
}