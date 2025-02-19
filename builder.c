#include "builder.h"

int main(int argc, char** argv)
{
    bld_rebuild_yourself(&argc, &argv);

    bld_set_verbose(false);

    bool isDebug = false;

    Bld_Cmd cmd = {0}, linkCmd = {0};

    Files files = {0};
    builder_get_files_in_directory_recursivelly("src", &files);

    bld_command_append(&linkCmd, "cc", "-o", "main");

    for(size_t i = 0; i < files.count; ++i)
    {
        bld_command_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb");
        bld_command_append(&cmd, "-DGLFW_INCLUDE_VULKAN");
        bld_command_append(&cmd, isDebug ? "-DVKDEBUG" : "-DVKRELEASE");
        bld_command_append(&cmd, "-Idependencies/GLFW/include");
        bld_command_append(&cmd, "-Isrc", "-Ivendor");

        char* path = (char*) malloc(strlen(files.items[i]) + strlen("objs/.o") + 1);
        sprintf(path, "objs/%s.o", builder_filename_no_extension(files.items[i]));

        bld_command_append(&linkCmd, path);

        bld_command_append(&cmd, "-c", files.items[i], "-o", path);
        if(!bld_command_build_and_reset(&cmd)) return 1;
    }

    bld_command_append(&cmd, "./build_shaders.sh");
    if(!bld_command_build_and_reset(&cmd)) return 1;
    
    bld_command_append(&linkCmd, "-Ldependencies/GLFW/lib");
    bld_command_append(&linkCmd, "-l:libglfw3.a", "-lvulkan");
    bld_command_append(&linkCmd, "-lm");
    if(!bld_command_build_and_reset(&linkCmd)) return 1;
    
    bld_command_append(&cmd, "./main");
    if(!bld_command_run_and_reset(&cmd)) return 1;

    return 0;
}
