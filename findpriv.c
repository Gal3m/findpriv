#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/capability.h>

int count_files = 0, count_executables = 0, count_caps = 0, count_setuid = 0;
int search_setuid = 0, search_cap = 0;
char setuid_names[512][1024];
char cap_names[512][1024];

int is_regular_file(const char * path) {
  struct stat path_stat;
  stat(path, & path_stat);
  return S_ISREG(path_stat.st_mode);
}

void fscanner(char * dir) {
  char path[4096];
  struct dirent * dp;
  DIR * dfd;

  if ((dfd = opendir(dir)) == NULL) {
    return;
  }

  while ((dp = readdir(dfd)) != NULL) {
    if (dp -> d_type == DT_DIR) {
      path[0] = '\0';
      if (strcmp(dp -> d_name, ".") == 0 || strcmp(dp -> d_name, "..") == 0)
        continue;
      sprintf(path, "%s/%s", dir, dp -> d_name);
      fscanner(path);
    } else {
      char file_path[4096];
      snprintf(file_path, sizeof file_path, "%s/%s", dir, dp -> d_name);
      struct stat sb;
      if (is_regular_file(file_path)) {
        count_files++;
        stat(file_path, & sb);
        if (sb.st_mode & S_IXUSR) {
          count_executables++;
          if (search_setuid && (sb.st_mode & S_ISUID)) {
            strncpy(setuid_names[count_setuid], file_path, sizeof file_path);
            count_setuid++;
          }
          if (search_cap) {
            cap_t cap = cap_get_file(file_path);
            if (cap != NULL) {
              strncpy(cap_names[count_caps], file_path, sizeof file_path);
              count_caps++;
            }
          }
        }
      }
    }
  }
  closedir(dfd);
}
void print_output() {
  if (count_setuid > 0) {
    printf("setuid executable: \n");
    for (int i = 0; i < count_setuid; i++)
      printf("%s\n", setuid_names[i]);
  }
  if (count_caps > 0) {
    printf("capability-aware executables: \n");
    for (int i = 0; i < count_caps; i++) {
      printf("%s ", cap_names[i]);
      cap_t cap = cap_get_file(cap_names[i]);
      char * cap_str = cap_to_text(cap, NULL);
      printf("%s\n", cap_str);
      cap_free(cap);
      cap_free(cap_str);
    }
  }

}

int main(int argc, char ** argv) {
  char * path = "/";
  int opt;
  while ((opt = getopt(argc, argv, "scp:")) != -1) {
    switch (opt) {
    case 's':
      search_setuid = 1;
      break;
    case 'c':
      search_cap = 1;
      break;
    case 'p':
      path = optarg;
      break;
    default:
      fprintf(stderr, "Usage: %s [-s] [-c] [-p path] \n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  if (!search_setuid && !search_cap) {
    search_setuid = 1;
    search_cap = 1;
  }
  fscanner(path);
  printf("Scanned %d files, ", count_files);
  printf("found %d executables\n", count_executables);
  print_output();
  return 0;
}
