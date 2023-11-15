#ifndef SHADER_ANALYZER_H
#define SHADER_ANALYZER_H

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

void sa_setRGAPath(char const* rgaPath)
{
    RGAPath = rgaPath;
}

sa_ShaderStats sa_spirVShaderStatistics(sa_SpirVShaderDesc desc)
{
    // Prep filesystem
    {
        std::error_code errorCode;
        std::filesystem::create_directory("temp", errorCode);
    }

    char const* shaderNames[sa_ShaderType_Count] =
    {
        "vert",
        "frag",
        "comp"
    };
    char const* shaderName = shaderNames[desc.Type];

    char const* gpu = "gfx1030";
    char const* spirvFilePath = "temp/temp_shader.spv";
    // Write out our temp file
    {
        std::ofstream spirvFile{spirvFilePath, std::ios::binary};
        spirvFile.write(desc.Binary, desc.BinarySize);
        spirvFile.close();
    }

    // Run RGA
    {
        std::stringstream processCommandLine;
        processCommandLine << RGAPath << " ";
        processCommandLine << "-s vk-offline ";
        processCommandLine << "-c " << gpu << " ";
        processCommandLine << "-a temp/temp_analysis.txt ";
        processCommandLine << "--" << shaderName << " " << spirvFilePath << " ";

        HANDLE process = win32RunProcess(processCommandLine.str());
        WaitForSingleObject(process, INFINITE);
    }

    // Read analysis file i.e CSV for dummies
    sa_ShaderStats stats = {};

    std::stringstream analysisFileName;
    {
        analysisFileName << "temp/" << gpu << "_temp_analysis_" << shaderName << ".txt";

        std::ifstream analysisFile{analysisFileName.str()};

        // Just find our the column of our VGPR count
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
    

    // Delete our temp file
    {
        std::remove(analysisFileName.str().c_str());
        std::remove(spirvFilePath);

        std::error_code errorCode;
        std::filesystem::remove("temp", errorCode);
    }

    return stats;
}

#endif // SHADER_ANALYZER_IMPL_H

#endif // SHADER_ANALYZER_IMPL
