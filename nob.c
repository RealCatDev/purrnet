#define NOB_IMPLEMENTATION
#include "./nob.h"

#define CC "gcc"
#define AR "ar"

#define CFLAGS "-Wall", "-Werror", "-Wpedantic", "-I./include/"

#ifdef _WIN32
#  define LIBS "-lws2_32"
#else
#  define LIBS 
#endif

#define LDFLAGS LIBS

const char *target = "libpurrnet.a";

int build() {
  nob_mkdir_if_not_exists("build");

  Nob_File_Paths children = {0};
  nob_read_entire_dir("./src/", &children);

  Nob_String_Builder sb = {0};
  nob_sb_append_cstr(&sb, "./build/");
  nob_sb_append_cstr(&sb, target);
  nob_sb_append_null(&sb);

  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, AR, "rcs", sb.items);

  for (size_t i = 2; i < children.count; ++i) {
    Nob_String_Builder path_sb = {0};
    nob_sb_append_cstr(&path_sb, "./src/");
    nob_sb_append_cstr(&path_sb, children.items[i]);
    nob_sb_append_null(&path_sb);

    Nob_String_Builder output_sb = {0};
    nob_sb_append_cstr(&output_sb, "./build/");
    nob_sb_append_cstr(&output_sb, children.items[i]);
    nob_sb_append_cstr(&output_sb, ".o");
    nob_sb_append_null(&output_sb);

    Nob_Cmd obj_cmd = {0};
    nob_cmd_append(&obj_cmd, CC, "-c", CFLAGS);
    nob_cmd_append(&obj_cmd, "-o", output_sb.items);
    nob_cmd_append(&obj_cmd, path_sb.items);
    if (!nob_cmd_run_sync(obj_cmd)) return 1;
  
    nob_cmd_append(&cmd, output_sb.items);

    nob_cmd_free(obj_cmd);
    nob_sb_free(path_sb);
  }
  
  if (!nob_cmd_run_sync(cmd)) return 1;

  return 0;
}

int example() {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, CC, CFLAGS, "-o", "build/server", "example/server.c", "-I./include/");
  nob_cmd_append(&cmd, "-L./build/", "-lpurrnet", LDFLAGS);

  if (!nob_cmd_run_sync(cmd)) return 1;

  nob_log(NOB_INFO, "server built successfully!");

  cmd.count = 0;
  nob_cmd_append(&cmd, CC, CFLAGS, "-o", "build/client", "example/client.c", "-I./include/");
  nob_cmd_append(&cmd, "-L./build/", "-lpurrnet", LDFLAGS);

  if (!nob_cmd_run_sync(cmd)) return 1;

  nob_log(NOB_INFO, "client built successfully!");
  
  return 0;
}

int example_udp() {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, CC, CFLAGS, "-o", "build/server-udp", "example/server-udp.c", "-I./include/");
  nob_cmd_append(&cmd, "-L./build/", "-lpurrnet", LDFLAGS);

  if (!nob_cmd_run_sync(cmd)) return 1;

  nob_log(NOB_INFO, "server built successfully!");

  cmd.count = 0;
  nob_cmd_append(&cmd, CC, CFLAGS, "-o", "build/client-udp", "example/client-udp.c", "-I./include/");
  nob_cmd_append(&cmd, "-L./build/", "-lpurrnet", LDFLAGS);

  if (!nob_cmd_run_sync(cmd)) return 1;

  nob_log(NOB_INFO, "client built successfully!");
  
  return 0;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  
  const char *const program = nob_shift_args(&argc, &argv);

  int result = 0;
  if ((result = build()) != 0) return result;
  
  if (argc > 0) {
    const char *const sub = nob_shift_args(&argc, &argv);
    if (strcmp(sub, "example") == 0) {
      if ((result = example()) != 0) return result;
      return example_udp();
    }
  }

  return 0;
}