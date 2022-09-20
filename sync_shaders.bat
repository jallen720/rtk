@echo off

del shaders\*.spv

for /r %%v in (shaders\*.vert,shaders\*.frag) do (
    %VULKAN_SDK%\Bin\glslangValidator.exe -V %%v -o %%v.spv && (
        echo compiled %%~nxv to %%~nxv.spv
    ) || (
        echo failed to compile %%~nxv
    )
)
