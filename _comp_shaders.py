import os

supported_extensions = [
    "vert",
    "frag",
    "geom",
]

def IsShaderSrc(file):
    # Ensure file is a file.
    if not os.path.isfile(f"shaders/{file}"):
        return False

    # Check if file uses supported extension.
    for supported_extension in supported_extensions:
        if f".{supported_extension}" in file:
            return True

    return False

if __name__ == "__main__":
    for file in os.listdir("shaders"):
        if IsShaderSrc(file):
            os.system(f"%VULKAN_SDK%/Bin/glslangValidator.exe -V shaders/{file} -o shaders/bin/{file}.spv")
