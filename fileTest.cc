#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include <node_buffer.h>

#include <cstring>
#include <cstdio>
#include <assert.h>
#include <string>

using namespace v8;
using namespace std;

uv_loop_t *loop;

Persistent<Function> bufferCtor;

struct ReaderBaton {
  ReaderBaton(const char *p)
  : path(p) {
    this->buf = NULL;
    this->open_req.data = this;
    this->read_req.data = this;
    this->stat_req.data = this;
  }
  ~ReaderBaton() {
    delete[] this->buf;
  }

  string path;
  int fd;

  uv_fs_t open_req;
  uv_fs_t read_req;
  uv_fs_t stat_req;

  char *buf;
  size_t buflen;

  Persistent<Function> callback;
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
  uv_fs_read(uv_default_loop(), &baton->read_req, baton->fd,
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
  uv_fs_fstat(uv_default_loop(), &baton->stat_req, baton->fd, on_stat);
}

void
on_read(uv_fs_t *req) {
  uv_fs_req_cleanup(req);

  if (req->result < 0) {
    fprintf(stderr, "Read error.\n");
    return;
  }

  ReaderBaton *baton = static_cast<ReaderBaton *>(req->data);

  if (static_cast<size_t>(req->result) == baton->buflen) {
    uv_fs_t close_req;
    uv_fs_close(loop, &close_req, baton->open_req.result, NULL);
    
    HandleScope scope;

    // make buffer
    node::Buffer *slowBuffer = node::Buffer::New(baton->buflen);
    memcpy(node::Buffer::Data(slowBuffer), baton->buf, baton->buflen);

    Handle<Value> constructorArgs[3] = {
      slowBuffer->handle_,
      v8::Integer::New(baton->buflen),
      v8::Integer::New(0)
    };

    Local<Object> actualBuffer = bufferCtor->NewInstance(3, constructorArgs);

    Handle<Value> argv[2] = {
      Null(),
      actualBuffer
    };
    baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    baton->callback.Dispose();
    delete baton;
    scope.Close(Undefined());
  }
}

Handle<Value>
Read(const Arguments& args) {
  HandleScope scope;
  assert(args.Length() == 2);

  if (!args[1]->IsFunction()) {
    return v8::ThrowException(
      v8::Exception::TypeError(
        v8::String::New("Last arg should be callback function")));
  }

  Local<Value> strObj = args[0]->ToString();
  v8::String::Utf8Value path(strObj);

  ReaderBaton *baton = new ReaderBaton(*path);  
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));

  uv_fs_open(loop, &baton->open_req, baton->path.c_str(),
    O_RDONLY, 0, on_open);

  return scope.Close(Undefined());
}

void
Init(Handle<Object> exports) {  
  bufferCtor = Persistent<Function>::New(Local<Function>::Cast(
    Context::GetCurrent()->Global()->Get(String::New("Buffer"))));
  exports->Set(String::NewSymbol("read"),
    FunctionTemplate::New(Read)->GetFunction());
  loop = uv_default_loop();
}

NODE_MODULE(ft, Init)

