//
// Created by felix on 01-01-22.
//

#include <cstdio>
#include <shaderc/shaderc.hpp>
#include <span>
#include <cstring>

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments. Expects 2, got %d\n", argc - 1);
        return 1;
    }

    auto sourcePath = argv[1];
    auto targetPath = argv[2];

    auto ext = get_filename_ext(sourcePath);

    shaderc_shader_kind shaderKind;

    if (strcmp(ext, "vert") == 0) {
        shaderKind = shaderc_vertex_shader;
    } else if (strcmp(ext, "frag") == 0) {
        shaderKind = shaderc_fragment_shader;
    } else {
        fprintf(stderr, "Unknown shader extension .%s\n", ext);
        return 1;
    }

    std::string source;
    {
        FILE *inFile = fopen(sourcePath, "r");
        fseek(inFile, 0, SEEK_END);
        size_t fileSize = ftell(inFile);
        fseek(inFile, 0, SEEK_SET);
        source.resize(fileSize);
        fread(source.data(), 1, fileSize, inFile);
        fclose(inFile);
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    auto preprocessed = compiler.PreprocessGlsl(source, shaderKind, sourcePath, options);

    auto compiled = compiler.CompileGlslToSpv({preprocessed.cbegin(), preprocessed.cend()},
                                              shaderKind, sourcePath, options);

    auto compiledWords = std::span(compiled.cbegin(), compiled.cend());

    {
        FILE* outFile = fopen(targetPath, "wb");
        fwrite(compiledWords.data(), 4, compiledWords.size(), outFile);
        fclose(outFile);
    }

    return 0;
}