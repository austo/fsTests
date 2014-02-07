#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

// gcc file_reader.c -I/usr/local/include -L/usr/local/lib -luv -o file_reader

#define REHYDRATE_HELPER(r, h) uv_fs_req_cleanup(r);                          \
  file_helper *h = (file_helper *) r->data

uv_loop_t *loop;

typedef struct {  
  uv_fs_t req;    
  void *buf;
  size_t buflen;
  int fd;
  char path[1];
} file_helper;

void on_open(uv_fs_t *req);
void on_read(uv_fs_t *req);
void on_stat(uv_fs_t *req);

file_helper *
file_helper_new(char *path) {
  size_t pathlen = strlen(path) + 1;
  file_helper *helper = malloc(sizeof(*helper) + pathlen);
  memcpy(helper->path, path, pathlen);
  helper->buf = NULL;
  helper->req.data = helper;
  return helper;
}

void
file_helper_dispose(file_helper *helper) {
  free(helper->buf);
  free(helper);
}

void
on_stat(uv_fs_t* req) {   
  REHYDRATE_HELPER(req, helper);

  if (req->result < 0) {
    fprintf(stderr, "Stat error.\n");
    file_helper_dispose(helper);
    return;
  }

  helper->buflen = req->statbuf.st_size;
  helper->buf = malloc(helper->buflen);
  uv_fs_read(loop, &helper->req, helper->fd,
    helper->buf, helper->buflen, -1, on_read);  
}

void
on_open(uv_fs_t *req) {
  REHYDRATE_HELPER(req, helper);
  
  if (req->result == -1) {
    fprintf(stderr, "error opening file.\n");
    file_helper_dispose(helper);
    return;    
  }

  helper->fd = req->result;
  uv_fs_fstat(loop, &helper->req, helper->fd, on_stat);
}

void
on_read(uv_fs_t *req) {
  REHYDRATE_HELPER(req, helper);

  if (req->result < 0) {
    fprintf(stderr, "Read error.\n");    
  }
  else if ((size_t)req->result == helper->buflen) {    
    uv_fs_t close_req;
    uv_fs_close(loop, &close_req, helper->req.result, NULL);
    printf("length: %lu\n", helper->buflen);
  }
  file_helper_dispose(helper);
}

int
main(int argc, char **argv) {
  loop = uv_default_loop();
  file_helper *helper = file_helper_new(argv[1]);
  uv_fs_open(loop, &helper->req, helper->path, O_RDONLY, 0, on_open);
  return uv_run(loop, UV_RUN_DEFAULT);
}
