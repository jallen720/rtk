import os

if __name__ == "__main__":
    for file in os.listdir("shaders"):
        if os.path.isfile(f"shaders/{file}"):
            os.system(f"%VULKAN_SDK%/Bin/glslangValidator.exe -V shaders/{file} -o shaders/bin/{file}.spv")
