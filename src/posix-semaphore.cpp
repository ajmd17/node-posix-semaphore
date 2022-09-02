#include <node.h>
#include <nan.h>
#include <errno.h>
#include <string.h>

#include <sys/semaphore.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#define NAN_RETURN(X) info.GetReturnValue().Set(X)

using namespace v8;


/**
 * Make it simpler the next time V8 breaks API's and such with a wrapper fn...
 */
template <typename T, typename VT>
inline auto get_v(VT v8_value) -> T {
	return Nan::To<T>(v8_value).FromJust();
}

/**
* Make it simpler the next time V8 breaks API's and such with a wrapper fn...
*/
template <typename T, typename VT>
inline auto get_v(VT v8_value, T default_value) -> T {
	return Nan::To<T>(v8_value).FromMaybe(default_value);
}

template <typename VT>
inline auto get_obj(VT v8_obj) -> Local<Object> {
	return Nan::To<Object>(v8_obj).ToLocalChecked();
}


const char* to_c_str(const Nan::Utf8String& value) {
	return *value? *value: "<string conversion failed>";
}


#if (NODE_MODULE_VERSION < NODE_0_12_MODULE_VERSION)
NAN_INLINE v8::Local<v8::Value> NanThrowErrno(
	int errorno,
	const char *syscall = NULL,
	const char *msg = "",
	const char *path = NULL
) {
	do {
		Nan::HandleScope();
		return v8::Local<v8::Value>::New(node::ErrnoException(errorno, syscall, msg, path));
	} while (0);
}
#else
NAN_INLINE void NanThrowErrno(int errorno,
	const char *syscall = NULL,
	const char *msg = "",
	const char *path = NULL
) {
	do {
		Nan::HandleScope();
		v8::Isolate::GetCurrent()->ThrowException(node::ErrnoException(errorno, syscall, msg, path));
	} while (0);
}
#endif


void cleanup_semaphores(char *data, void *hint)
{
    auto *semaphore = static_cast<sem_t *>(hint);
    sem_close(semaphore);
}

NAN_METHOD(_sem_open) {
	Nan::HandleScope();

	if(info.Length() != 4 || !info[0]->IsString())
        return Nan::ThrowError("_sem_open() expects 4 args: (name: string, oflag: number, mode: number, value: number)");
	if(info.Length() != 4 || !info[1]->IsNumber())
        return Nan::ThrowError("_sem_open() expects 4 args: (name: string, oflag: number, mode: number, value: number)");
	if(info.Length() != 4 || !info[2]->IsNumber())
        return Nan::ThrowError("_sem_open() expects 4 args: (name: string, oflag: number, mode: number, value: number)");
	if(info.Length() != 4 || !info[3]->IsNumber())
        return Nan::ThrowError("_sem_open() expects 4 args: (name: string, oflag: number, mode: number, value: number)");

	Nan::Utf8String semaphore_name(info[0]);

	errno = 0;
	sem_t *val = sem_open(to_c_str(semaphore_name), get_v<int>(info[1]), get_v<int>(info[2]), get_v<int>(info[3]));

	if (val == SEM_FAILED) {
		return Nan::ThrowError((std::string("Failed to create semaphore. errno was: ") + std::to_string(errno)).c_str());
	}

	char *ch = new char[sizeof(uintptr_t)];
	std::memcpy(ch, &val, sizeof(sem_t *));

	Nan::MaybeLocal<Object> buf = node::Buffer::New(
        v8::Isolate::GetCurrent(), ch, sizeof(uintptr_t), cleanup_semaphores, static_cast<void *>(val));
	if (buf.IsEmpty()) {
		return Nan::ThrowError(std::string("Couldn't allocate Node Buffer").c_str());
	} else {
		info.GetReturnValue().Set(buf.ToLocalChecked());
	}
}

NAN_METHOD(_sem_wait) {
	Nan::HandleScope();

	if(info.Length() != 1
		|| !info[0]->IsObject()
	) return Nan::ThrowError("_sem_wait() expects 1 arg: (sem: buffer)");

	union {
		sem_t *s;
		char ch[sizeof(uintptr_t)];
	} ptr_value;

	std::memcpy(&ptr_value.ch[0], node::Buffer::Data(info[0]), node::Buffer::Length(info[0]));

	sem_t *val = ptr_value.s;
	if (val == SEM_FAILED || val == NULL) {
		return Nan::ThrowError("invalid semaphore");
	}

	errno = 0;
	if (sem_wait(val) != 0) {
		return Nan::ThrowError((std::string("failed to wait on semaphore, error was ") + std::to_string(errno)).c_str());
	}

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(_sem_post) {
	Nan::HandleScope();

	if(info.Length() != 1
		|| !info[0]->IsObject()
	) return Nan::ThrowError("_sem_post() expects 1 arg: (sem: buffer)");

	union {
		sem_t *s;
		char ch[sizeof(uintptr_t)];
	} ptr_value;

	std::memcpy(&ptr_value.ch[0], node::Buffer::Data(info[0]), node::Buffer::Length(info[0]));

	sem_t *val = ptr_value.s;
	if (val == SEM_FAILED || val == NULL) {
		return Nan::ThrowError("invalid semaphore");
	}

	errno = 0;
	if (sem_post(val) != 0) {
		return Nan::ThrowError((std::string("failed to post semaphore, error was ") + std::to_string(errno)).c_str());
	}

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(_sem_close) {
	Nan::HandleScope();

	if(info.Length() != 1
		|| !info[0]->IsObject()
	) return Nan::ThrowError("_sem_close() expects 1 arg: (sem: buffer)");

	union {
		sem_t *s;
		char ch[sizeof(uintptr_t)];
	} ptr_value;

	std::memcpy(&ptr_value.ch[0], node::Buffer::Data(info[0]), node::Buffer::Length(info[0]));

	sem_t *val = ptr_value.s;
	if (val == SEM_FAILED || val == NULL) {
		return Nan::ThrowError("invalid semaphore");
	}

	errno = 0;
	if (sem_close(val) != 0) {
		return Nan::ThrowError((std::string("failed to close semaphore, error was ") + std::to_string(errno)).c_str());
	}

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(_sem_unlink) {
	Nan::HandleScope();

	if(info.Length() != 1
		|| !info[0]->IsString()
	) return Nan::ThrowError("_sem_unlink() expects 1 arg: (sem: string)");

	Nan::Utf8String s_name(info[0]);
	const char* sc_name = to_c_str(s_name);

	errno = 0;
	if (sem_unlink(sc_name) != 0) {
		return Nan::ThrowError((std::string("failed to unlink semaphore, error was ") + std::to_string(errno)).c_str());
	}

	info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Init) {
	NAN_EXPORT(target, _sem_open);
	NAN_EXPORT(target, _sem_wait);
	NAN_EXPORT(target, _sem_post);
	NAN_EXPORT(target, _sem_close);
	NAN_EXPORT(target, _sem_unlink);

	NODE_DEFINE_CONSTANT(target, O_RDONLY);
	NODE_DEFINE_CONSTANT(target, O_RDWR);
	NODE_DEFINE_CONSTANT(target, O_CREAT);
	NODE_DEFINE_CONSTANT(target, O_EXCL);
	NODE_DEFINE_CONSTANT(target, O_TRUNC);
}

NODE_MODULE(posix_semaphore, Init)
