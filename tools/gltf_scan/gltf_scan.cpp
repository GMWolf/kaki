//
// Created by felix on 20/02/2022.
//

#include <string>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <cstdio>
#include <span>

#include <fstream>
#include <filesystem>

static cgltf_image* getImage(cgltf_texture* tex) {
    return tex ? tex->image : nullptr;
};


enum class Usage {
    albedo,
    normal,
    metalRough,
    ao,
};

static Usage findImageUsage(cgltf_image* image, cgltf_data* data) {

    for(auto& material : std::span(data->materials, data->materials_count)) {

        if (getImage(material.pbr_metallic_roughness.base_color_texture.texture) == image){
            return Usage::albedo;
        }

        if (getImage(material.pbr_metallic_roughness.metallic_roughness_texture.texture) == image) {
            return Usage::metalRough;
        }

        if (getImage(material.normal_texture.texture) == image) {
            return Usage::normal;
        }

        if (getImage(material.occlusion_texture.texture) == image) {
            return Usage::ao;
        }
    }

    return Usage::albedo;

}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Expected 2 arguments\n");
        return 1;
    }

    std::string input = argv[1];
    std::string output = argv[2];

    cgltf_options options = {};
    cgltf_data* data{};
    cgltf_result result = cgltf_parse_file(&options, input.c_str(), &data);

    if (result != cgltf_result_success) {
        fprintf(stderr, "Failed to load gltf file: %s\n", input.c_str());
        return 1;
    }


    std::ofstream ofs(output);

    ofs << "rule tex\n"
           "  command = pvr-tex-tool -i $in -o $out -m -f $format -ics $ics\n";

    ofs << "\n";

    for(auto& image : std::span(data->images, data->images_count)) {
        auto usage = findImageUsage(&image, data);

        std::filesystem::path path(image.uri);

        auto outpath = path.replace_extension(".ktx2");

        ofs << "build " << outpath.string() << ": tex " << image.uri << "\n";
        switch (usage) {
            case Usage::albedo:
                ofs << "  format = BASISU_UASTC,UBN,sRGB\n";
                ofs << "  ics = sRGB\n";
                break;
            case Usage::normal:
                ofs << "  format = BC5,UBN,lRGB\n";
                ofs << "  ics = lRGB\n";
                break;
            case Usage::metalRough:
                ofs << "  format = BC5,UBN,lRGB\n";
                ofs << "  ics = lRGB\n";
                break;
            case Usage::ao:
                ofs << "  format = BC5,UBN,lRGB\n";
                ofs << "  ics = lRGB\n";
                break;
        }

        ofs << "\n";

    }

}