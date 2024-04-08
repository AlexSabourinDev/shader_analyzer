
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
    sa_setRGAPath("../rga/rga.exe");
    sa_ShaderStats stats = sa_spirVShaderStatistics(
        sa_SpirVShaderDesc
        {
            buffer.data(),
            buffer.size(),
            sa_ShaderType_Pixel
        });

	char* isa = sa_spirVShaderISA(sa_SpirVShaderDesc
	                  {
						  buffer.data(),
						  buffer.size(),
						  sa_ShaderType_Pixel
					  });

    printf("%s", isa);
    sa_free(isa);

    char* registerAnalysis = sa_spirVShaderRegisterAnalysis(sa_SpirVShaderDesc
	                  {
						  buffer.data(),
						  buffer.size(),
						  sa_ShaderType_Pixel
					  });

    printf("%s", registerAnalysis);
    sa_free(registerAnalysis);
}