#ifndef SHADER_ANALYZER_H
#define SHADER_ANALYZER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum
{
    sa_ShaderType_Vertex = 0,
    sa_ShaderType_Pixel,
    sa_ShaderType_Compute,
    sa_ShaderType_Count
};
typedef unsigned int sa_ShaderType;

typedef struct
{
    char const* Binary;
    size_t BinarySize;
    sa_ShaderType Type;
} sa_SpirVShaderDesc;

typedef struct
{
    int VGPRCount;
} sa_ShaderStats;

void sa_setRGAPath(char const* rgaPath);
sa_ShaderStats sa_spirVShaderStatistics(sa_SpirVShaderDesc desc);
char* sa_spirVShaderISA(sa_SpirVShaderDesc desc);
char* sa_spirVShaderRegisterAnalysis(sa_SpirVShaderDesc desc);
void sa_free(void* data);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // SHADER_ANALYZER_H

#ifdef SHADER_ANALYZER_IMPL

#ifndef SHADER_ANALYZER_IMPL_H
#define SHADER_ANALYZER_IMPL_H

#include <sstream>
#include <fstream>
#include <filesystem>
#include <Windows.h>
static HANDLE win32RunProcess(std::string const& commandLine)
{
    std::string tempStorage = commandLine;

    PROCESS_INFORMATION processInformation{};
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    // The target process will be created in session of the
    // provider host process. If the provider was hosted in
    // wmiprvse.exe process, the target process will be launched
    // in session 0 and UI is invisible to the logged on user,
    // but the process can be found through task manager.
    BOOL creationResult = CreateProcessA(
        NULL,                   // Command line + module name
        &tempStorage[0],        // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,  // creation flags
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory 
        &startupInfo,           // Pointer to STARTUPINFO structure
        &processInformation);   // Pointer to PROCESS_INFORMATION structure

    return creationResult == TRUE ? processInformation.hProcess : NULL;
}

static char const* RGAPath = "rga/rga.exe";
static char const* const TempSpirvFilePath = "temp/temp_shader.spv";
static char const* GPU = "gfx1030";

static char const* const ShaderNames[sa_ShaderType_Count] =
{
	"vert",
	"frag",
	"comp"
};

void sa_setRGAPath(char const* rgaPath)
{
    RGAPath = rgaPath;
}

static void runRGA(sa_SpirVShaderDesc desc, char const* analysisCommand)
{
	// Prep filesystem
    {
        std::error_code errorCode;
        std::filesystem::create_directory("temp", errorCode);
    }

    char const* shaderName = ShaderNames[desc.Type];

    // Write out our temp file
    {
        std::ofstream spirvFile{TempSpirvFilePath, std::ios::binary};
        spirvFile.write(desc.Binary, desc.BinarySize);
        spirvFile.close();
    }

    // Run RGA
    {
        std::stringstream processCommandLine;
        processCommandLine << RGAPath << " ";
        processCommandLine << "-s vk-offline ";
        processCommandLine << "-c " << GPU << " ";
		processCommandLine << analysisCommand << " ";
        processCommandLine << "--" << shaderName << " " << TempSpirvFilePath << " ";

        HANDLE process = win32RunProcess(processCommandLine.str());
        WaitForSingleObject(process, INFINITE);
    }
}

static void rgaEpilogue()
{
	std::remove(TempSpirvFilePath);

	std::error_code errorCode;
	std::filesystem::remove("temp", errorCode);
}

sa_ShaderStats sa_spirVShaderStatistics(sa_SpirVShaderDesc desc)
{
	runRGA(desc, "-a temp/temp_analysis.txt");

    // Read analysis file i.e CSV for dummies
    sa_ShaderStats stats = {};

    std::stringstream analysisFileName;
	analysisFileName << "temp/" << GPU << "_temp_analysis_" << ShaderNames[desc.Type] << ".txt";

    {
        std::ifstream analysisFile{analysisFileName.str()};

        // Just find out the column of our VGPR count
        std::string out;
        int column = 0;
        for(; std::getline(analysisFile, out, ',') && out != "USED_VGPRs"; column++)
        {
        }

        // Skip to next row
        std::string dummyRead;
        std::getline(analysisFile, dummyRead, '\n');

        for(int currentColumn = 0; currentColumn < column; currentColumn++)
        {
            // Just read columns until we're at the column we want
            std::getline(analysisFile, dummyRead, ',');
        }

        std::string vgprCount;
        std::getline(analysisFile, vgprCount, ',');

        stats.VGPRCount = atoi(vgprCount.c_str());
    }
   
    // Delete our analysis file
	std::remove(analysisFileName.str().c_str());
	rgaEpilogue();

    return stats;
}

char* sa_spirVShaderISA(sa_SpirVShaderDesc desc)
{
	runRGA(desc, "--isa temp/temp_isa.txt");

	std::stringstream analysisFileName;
	analysisFileName << "temp/" << GPU << "_temp_isa_" << ShaderNames[desc.Type] << ".txt";
	
    char* output;
    {
        std::ifstream analysisFile{analysisFileName.str(), std::ifstream::binary};

	    analysisFile.seekg(0, std::ios::end);
	    size_t fileSize = analysisFile.tellg();
	    analysisFile.seekg(0, std::ios::beg);
	
	    output = reinterpret_cast<char*>(malloc(fileSize + 1));
	    analysisFile.read(output, fileSize);
        output[fileSize] = 0;
    }

	std::remove(analysisFileName.str().c_str());
	rgaEpilogue();

	return output;
}

char* sa_spirVShaderRegisterAnalysis(sa_SpirVShaderDesc desc)
{
	runRGA(desc, "--livereg temp/temp_register_analysis.txt");

	std::stringstream analysisFileName;
	analysisFileName << "temp/" << GPU << "_temp_register_analysis_" << ShaderNames[desc.Type] << ".txt";
	
    char* output;
    {
        std::ifstream analysisFile{analysisFileName.str(), std::ifstream::binary};

	    analysisFile.seekg(0, std::ios::end);
	    size_t fileSize = analysisFile.tellg();
	    analysisFile.seekg(0, std::ios::beg);
	
	    output = reinterpret_cast<char*>(malloc(fileSize + 1));
	    analysisFile.read(output, fileSize);
        output[fileSize] = 0;
    }

	std::remove(analysisFileName.str().c_str());
	rgaEpilogue();

	return output;
}

void sa_free(void* data)
{
    free(data);
}

#endif // SHADER_ANALYZER_IMPL_H

#endif // SHADER_ANALYZER_IMPL
