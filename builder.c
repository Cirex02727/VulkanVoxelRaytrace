#include "builder.h"

int main(int argc, char** argv)
{
    bld_rebuild_yourself("cc", &argc, &argv);

    bld_set_verbose(true);

    bool isDebug = true;

    Program mainProgram = {0};
    bld_add_source_path("src", &mainProgram);

    Bld_Cmd cmd = {0};
    bld_command_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb");
    if(!isDebug) bld_command_append(&cmd, "-O3");
    bld_command_append(&cmd, "-DGLFW_INCLUDE_VULKAN");
    bld_command_append(&cmd, isDebug ? "-DVKDEBUG" : "-DVKRELEASE");
    bld_command_append(&cmd, "-Idependencies/GLFW/include");
    bld_command_append(&cmd, "-Isrc", "-Ivendor");

    if(!bld_compile_program(&mainProgram, cmd)) return 1;

    Bld_Cmd buildShaders = {0};
    bld_command_append(&buildShaders, "./build_shaders.sh");
    if(!bld_command_exec(buildShaders, "[SHADERS]")) return 1;

    Bld_Cmd libraries = {0};
    bld_command_append(&libraries, "-Ldependencies/GLFW/lib");
    bld_command_append(&libraries, "-l:libglfw3.a", "-lvulkan");
    bld_command_append(&libraries, "-lm");
    if(!bld_link_program(&mainProgram, libraries, "main")) return 1;

    bld_end();
    return 0;
}
