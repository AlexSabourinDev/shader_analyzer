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
		char const* BinaryFilePath;
		char const* HLSLFilePath;
		char const* HLSLEntryPoint;
		sa_ShaderType Type;
	} sa_SpirVShaderDesc;

	typedef struct
	{
		int VGPRCount;
	} sa_ShaderStats;

	enum
	{
		sa_ShaderOutputType_Stats = 0x01,
		sa_ShaderOutputType_ISA = 0x02,
		sa_ShaderOutputType_RegisterAnalysis = 0x04,
	};
	typedef unsigned int sa_ShaderOutputType;

	typedef struct // Written to depending on sa_ShaderOutput
	{
		sa_ShaderStats Stats;
		char* ISA;
		char* RegisterAnalysis;
	} sa_ShaderOutput;

	void sa_setRGAPath(char const* rgaPath);
	void sa_setDXCPath(char const* dxcPath);
	sa_ShaderOutput sa_spirVShaderOutput(sa_SpirVShaderDesc desc, sa_ShaderOutputType outputType);
	void sa_freeShaderOutput(sa_ShaderOutput output);

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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
static void win32RunProcess(std::string const& commandLine, HANDLE outputStream = NULL)
{
	std::string tempStorage = commandLine;

	PROCESS_INFORMATION processInformation{};
	STARTUPINFOA startupInfo{};
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.hStdOutput = outputStream;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;

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
		TRUE,                  // Set handle inheritance to FALSE
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,  // creation flags
		NULL,                   // Use parent's environment block
		NULL,                   // Use parent's starting directory 
		&startupInfo,           // Pointer to STARTUPINFO structure
		&processInformation);   // Pointer to PROCESS_INFORMATION structure

	if (creationResult == TRUE)
	{
		WaitForSingleObject(processInformation.hProcess, INFINITE);
		CloseHandle(processInformation.hProcess);
	}
}

// DXC Support

static char const* RGAPath = "rga/";
static char const* DXCPath = "dxc/";
static char const* const TempSpirvFilePath = "temp/temp_shader.spv";
static char const* const GPU = "gfx1030";
static char const* const AMDLLPCGPU = "10.3.0";

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

void sa_setDXCPath(char const* dxcPath)
{
	DXCPath = dxcPath;
}

sa_ShaderOutput sa_spirVShaderOutput(sa_SpirVShaderDesc desc, sa_ShaderOutputType outputType)
{
	// Prep filesystem
	{
		std::error_code errorCode;
		std::filesystem::create_directory("temp", errorCode);
	}

	char const* shaderName = ShaderNames[desc.Type];

	// Write out our temp file
	char const* spirvFilePath = "";

	// If both a binary path and data are specified, prefer the file paths.
	if (desc.HLSLFilePath != nullptr)
	{
		static char const* const dxcShaderNames[sa_ShaderType_Count] =
		{
			"vs_6_6",
			"ps_6_6",
			"cs_6_6"
		};

		// Compile with DXC
		std::stringstream processCommandLine;
		processCommandLine << DXCPath << "dxc.exe ";
		processCommandLine << "-spirv ";
		processCommandLine << "-T " << dxcShaderNames[desc.Type] << " ";
		processCommandLine << "-E " << desc.HLSLEntryPoint << " ";
		processCommandLine << "-fspv-target-env=vulkan1.3 ";
		processCommandLine << "-WX ";
		processCommandLine << "-O3 ";
		processCommandLine << "-Zi ";
		processCommandLine << "-enable-16bit-types ";
		processCommandLine << "-HV 2021 ";
		processCommandLine << "-Zpr ";
		processCommandLine << "-Fo " << TempSpirvFilePath << " ";
		processCommandLine << desc.HLSLFilePath;

		win32RunProcess(processCommandLine.str());

		spirvFilePath = TempSpirvFilePath;
	}
	else if (desc.BinaryFilePath != nullptr)
	{
		spirvFilePath = desc.BinaryFilePath;
	}
	else if(desc.Binary != nullptr)
	{
		std::ofstream spirvFile { TempSpirvFilePath, std::ios::binary };
		spirvFile.write(desc.Binary, desc.BinarySize);
		spirvFile.close();

		spirvFilePath = TempSpirvFilePath;
	}

	// Run RGA
	if ((outputType & (sa_ShaderOutputType_Stats | sa_ShaderOutputType_RegisterAnalysis)) != 0)
	{
		std::stringstream processCommandLine;
		processCommandLine << RGAPath << "rga.exe ";
		processCommandLine << "-s vk-offline ";
		processCommandLine << "-c " << GPU << " ";

		if ((outputType & sa_ShaderOutputType_Stats) != 0)
		{
			processCommandLine << "-a temp/temp_analysis.txt ";
		}

		if ((outputType & sa_ShaderOutputType_RegisterAnalysis) != 0)
		{
			processCommandLine << "--livereg temp/temp_register_analysis.txt ";
		}

		processCommandLine << "--" << shaderName << " " << spirvFilePath << " ";

		win32RunProcess(processCommandLine.str());
	}

	sa_ShaderOutput output = {};

	if ((outputType & sa_ShaderOutputType_Stats) != 0)
	{
		// Read analysis file i.e CSV for dummies
		sa_ShaderStats stats = {};

		std::stringstream statsFileName;
		statsFileName << "temp/" << GPU << "_temp_analysis_" << shaderName << ".txt";

		{
			std::ifstream statsFile { statsFileName.str() };

			// Just find out the column of our VGPR count
			std::string out;
			int column = 0;
			for (; std::getline(statsFile, out, ',') && out != "USED_VGPRs"; column++)
			{
			}

			// Skip to next row
			std::string dummyRead;
			std::getline(statsFile, dummyRead, '\n');

			for (int currentColumn = 0; currentColumn < column; currentColumn++)
			{
				// Just read columns until we're at the column we want
				std::getline(statsFile, dummyRead, ',');
			}

			std::string vgprCount;
			std::getline(statsFile, vgprCount, ',');

			output.Stats.VGPRCount = atoi(vgprCount.c_str());
		}
   
		// Delete our analysis file
		std::remove(statsFileName.str().c_str());
	}

	if ((outputType & sa_ShaderOutputType_ISA) != 0)
	{
		char const* const tempCompilationBinaryPath = "temp/temp.bin";
		std::stringstream amdllpcCommandLine;
		amdllpcCommandLine << RGAPath << "utils/amdllpc.exe ";
		amdllpcCommandLine << "-v ";
		amdllpcCommandLine << "--include-llvm-ir ";
		amdllpcCommandLine << "--auto-layout-desc ";
		amdllpcCommandLine << "-o=\"" << tempCompilationBinaryPath << "\" ";
		amdllpcCommandLine << "--gfxip=" << AMDLLPCGPU << " ";
		amdllpcCommandLine << "-trim-debug-info=false ";
		amdllpcCommandLine << spirvFilePath << " ";
		win32RunProcess(amdllpcCommandLine.str());

		SECURITY_ATTRIBUTES securityAttributes = {};
		securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
		securityAttributes.lpSecurityDescriptor = NULL;
		securityAttributes.bInheritHandle = TRUE;

		char const* const tempLLVMObjDumpOutputPath = "temp/temp_dissassembly.txt";
		HANDLE disassemblyOutput = CreateFileA(tempLLVMObjDumpOutputPath,
		                                       FILE_APPEND_DATA,
		                                       FILE_SHARE_WRITE | FILE_SHARE_READ,
		                                       &securityAttributes,
		                                       CREATE_ALWAYS,
		                                       FILE_ATTRIBUTE_NORMAL,
		                                       NULL);

		std::stringstream llvmObjDumpCommandLine;
		llvmObjDumpCommandLine << "\"" << RGAPath << "utils/lc/opencl/bin/llvm-objdump.exe" << "\" ";
		llvmObjDumpCommandLine << "--disassemble ";
		llvmObjDumpCommandLine << "--symbolize-operands ";
		llvmObjDumpCommandLine << "--line-numbers ";
		llvmObjDumpCommandLine << "--source ";
		llvmObjDumpCommandLine << "--triple=amdgcn--amdpal ";
		llvmObjDumpCommandLine << "--mcpu=" << GPU << " ";
		llvmObjDumpCommandLine << "\"" << tempCompilationBinaryPath << "\" ";
		win32RunProcess(llvmObjDumpCommandLine.str(), disassemblyOutput);
		CloseHandle(disassemblyOutput);

		{
			std::ifstream isaFile { tempLLVMObjDumpOutputPath, std::ifstream::binary };

			isaFile.seekg(0, std::ios::end);
			size_t fileSize = isaFile.tellg();
			isaFile.seekg(0, std::ios::beg);
	
			output.ISA = reinterpret_cast<char*>(malloc(fileSize + 1));
			isaFile.read(output.ISA, fileSize);
			output.ISA[fileSize] = 0;
		}

		std::remove(tempCompilationBinaryPath);
		std::remove(tempLLVMObjDumpOutputPath);
	}

	if ((outputType & sa_ShaderOutputType_RegisterAnalysis) != 0)
	{
		std::stringstream registerFileName;
		registerFileName << "temp/" << GPU << "_temp_register_analysis_" << ShaderNames[desc.Type] << ".txt";

		{
			std::ifstream analysisFile { registerFileName.str(), std::ifstream::binary };

			analysisFile.seekg(0, std::ios::end);
			size_t fileSize = analysisFile.tellg();
			analysisFile.seekg(0, std::ios::beg);
	
			output.RegisterAnalysis = reinterpret_cast<char*>(malloc(fileSize + 1));
			analysisFile.read(output.RegisterAnalysis, fileSize);
			output.RegisterAnalysis[fileSize] = 0;
		}

		std::remove(registerFileName.str().c_str());
	}

	// Final cleanup
	{
		std::remove(TempSpirvFilePath);
		std::error_code errorCode;
		std::filesystem::remove("temp", errorCode);
	}

	return output;
}

void sa_freeShaderOutput(sa_ShaderOutput output)
{
	free(output.ISA);
	free(output.RegisterAnalysis);
}

#endif // SHADER_ANALYZER_IMPL_H

#endif // SHADER_ANALYZER_IMPL
