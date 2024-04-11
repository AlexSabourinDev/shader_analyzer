%~dp0\rga\utils\amdllpc.exe -v --include-llvm-ir --auto-layout-desc -o="amdllpc_temp.bin" --gfxip=10.3.0 -trim-debug-info=false %1 >NUL
%~dp0\rga\utils\lc\opencl\bin\llvm-objdump.exe --disassemble --symbolize-operands --line-numbers --source --triple=amdgcn--amdpal --mcpu=gfx1030 "amdllpc_temp.bin"
del "amdllpc_temp.bin"