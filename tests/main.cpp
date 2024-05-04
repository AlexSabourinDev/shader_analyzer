
#define SHADER_ANALYZER_IMPL
#include "../shader_analyzer.h"

#include <vector>
int main(int argc, char* argv[])
{
    sa_setRGAPath("../rga/");
	sa_setDXCPath("../dxc/");

	sa_SpirVShaderDesc desc = {};
	desc.HLSLFilePath = "experiment_file.hlsl";
	desc.HLSLEntryPoint = "CS";
	desc.Type = sa_ShaderType_Compute;

    sa_ShaderOutput output = sa_spirVShaderOutput(desc, sa_ShaderOutputType_ISA | sa_ShaderOutputType_RegisterAnalysis | sa_ShaderOutputType_Stats);

    printf("%s", output.ISA);
    printf("%s", output.RegisterAnalysis);
    sa_freeShaderOutput(output);
}