
#define SHADER_ANALYZER_IMPL
#include "../shader_analyzer.h"

#include <vector>
int main()
{
    std::vector<char> buffer;
    {
        std::ifstream spirvFile{"sample_brdf.spv", std::ios::binary};
        buffer= std::vector<char>{std::istreambuf_iterator<char>(spirvFile), {}};
    }
    sa_setRGAPath("../rga/");
    sa_ShaderOutput output = sa_spirVShaderOutput(
        sa_SpirVShaderDesc
        {
            buffer.data(),
            buffer.size(),
            sa_ShaderType_Pixel
        }, sa_ShaderOutputType_ISA);

    printf("%s", output.ISA);
    printf("%s", output.RegisterAnalysis);
    sa_freeShaderOutput(output);
}