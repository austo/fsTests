#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include <node_buffer.h>

#include <cstdio>
#include <assert.h>

using namespace v8;
using namespace std;

uv_loop_t *loop;
static Persistent<String> oncomplete_sym;

struct ReaderBaton {
  ReaderBaton(const char *p)
  : path(p) {
    v8::HandleScope scope;
    object = Persistent<Object>::New(Object::New());
    this->buf = NULL;
    this->fsReq.data = this;    
  }

  ~ReaderBaton() {
    delete[] this->buf;
    assert(!object.IsEmpty());
    object.Dispose();
    object.Clear();
  }

  const char *path;
  int fd;

  uv_fs_t fsReq;  

  char *buf;
  size_t buflen;

  Persistent<Object> object;
};

void on_open(uv_fs_t *req);
void on_read(uv_fs_t *req);
void on_stat(uv_fs_t* req);

void
on_stat(uv_fs_t* req) {  
  uv_fs_req_cleanup(req);

  if (req->result < 0) {
    fprintf(stderr, "Stat error.\n");
    uv_fs_req_cleanup(req);
    return;
  }

  ReaderBaton *baton = static_cast<ReaderBaton *>(req->data);  
  baton->buflen = req->statbuf.st_size;
  baton->buf = new char[baton->buflen];
  uv_fs_read(uv_default_loop(), &baton->fsReq, baton->fd,
    baton->buf, baton->buflen, -1, on_read);  
}

void
on_open(uv_fs_t *req) {
  uv_fs_req_cleanup(req);

  if (req->result == -1) {
    fprintf(stderr, "error opening file.\n");
    return;    
  }

  ReaderBaton *baton = static_cast<ReaderBaton *>(req->data);
  baton->fd = req->result;
  uv_fs_fstat(uv_default_loop(), &baton->fsReq, baton->fd, on_stat);
}

void
on_read(uv_fs_t *req) {
  uv_fs_req_cleanup(req);

  HandleScope scope;
  ReaderBaton *baton = static_cast<ReaderBaton *>(req->data);
  Handle<Value> argv[2];

  if (req->result < 0) {
    argv[0] = String::New("Unable to read file");
    argv[1] = Null();
  }
  else if (static_cast<size_t>(req->result) == baton->buflen) {
    uv_fs_t close_req;
    uv_fs_close(loop, &close_req, baton->fd, NULL);    

    // make buffer
    node::Buffer *buf = node::Buffer::New(baton->buf, baton->buflen);
    
    argv[0] = Null();
    argv[1] = buf->handle_;
  }
  else {
    argv[0] = String::New("Unspecified error");
    argv[1] = Null();
  }

  node::MakeCallback(baton->object, oncomplete_sym, 2, argv);
  delete baton;  
}

Handle<Value>
Read(const Arguments& args) {
  HandleScope scope;

  assert(args.Length() == 2);
  assert(args[1]->IsFunction());

  Local<Value> strObj = args[0]->ToString();
  v8::String::Utf8Value path(strObj);

  ReaderBaton *baton = new ReaderBaton(*path);
  baton->object->Set(oncomplete_sym, args[1]);

  uv_fs_open(loop, &baton->fsReq, baton->path,
    O_RDONLY, 0, on_open);

  return Undefined();
}

void
Init(Handle<Object> exports) {  
  exports->Set(String::NewSymbol("read"),
    FunctionTemplate::New(Read)->GetFunction());
  oncomplete_sym = NODE_PSYMBOL("oncomplete");
  loop = uv_default_loop();
}

NODE_MODULE(ft, Init)

