#define SHADER_ANALYZER_IMPL
#include "../shader_analyzer.h"

int main(int argc, char* argv[])
{
	sa_setRGAPath("rga/");
	sa_setDXCPath("dxc/");

	char const* filePath = "";
	char const* entryPoint = "";
	sa_ShaderType shaderType = sa_ShaderType_Compute;
	sa_ShaderOutputType outputType = sa_ShaderOutputType_ISA;

	bool printHelp = false;

	for(int i = 0; i < argc; i++)
	{
		if(_stricmp(argv[i], "-h") == 0)
		{
			printHelp = true;
		}
		else if(_stricmp(argv[i], "-i") == 0)
		{
			i++;
			filePath = argv[i];
		}
		else if(_stricmp(argv[i], "-e") == 0)
		{
			i++;
			entryPoint = argv[i];
		}
		else if(_stricmp(argv[i], "-t") == 0)
		{
			i++;
			if(_stricmp(argv[i], "cs") == 0)
			{
				shaderType = sa_ShaderType_Compute;
			}
			else if(_stricmp(argv[i], "ps") == 0)
			{
				shaderType = sa_ShaderType_Pixel;
			}
			else if(_stricmp(argv[i], "vs") == 0)
			{
				shaderType = sa_ShaderType_Vertex;
			}
		}
		else if(_stricmp(argv[i], "-ra") == 0)
		{
			outputType |= sa_ShaderOutputType_RegisterAnalysis;
		}
	}

	if(printHelp)
	{
		printf("-h: print help\n");
		printf("-i: input file\n");
		printf("-e: entry point\n");
		printf("-t: shader type (vs, ps, cs)\n");
		printf("-ra: register analysis\n");
	}

	bool filePathValid = filePath != nullptr && filePath[0] != 0;
	bool entryPointValid = entryPoint != nullptr && entryPoint[0] != 0;
	if(!filePathValid || !entryPointValid)
	{
		printf("File path or entry point is invalid. See -h for parameters.");
		return 0;
	}

	sa_SpirVShaderDesc desc = {};
	desc.HLSLFilePath = filePath;
	desc.HLSLEntryPoint = entryPoint;
	desc.Type = shaderType;

    sa_ShaderOutput output = sa_spirVShaderOutput(desc, outputType);

    printf("%s", output.ISA);
	if(outputType & sa_ShaderOutputType_RegisterAnalysis)
	{
		printf("%s", output.RegisterAnalysis);
	}
    sa_freeShaderOutput(output);
}