#ifndef SRC_NODE_NATIVE_MODULE_H_
#define SRC_NODE_NATIVE_MODULE_H_

#if defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#include <map>
#include <set>
#include <string>
#include "env.h"
#include "node_mutex.h"
#include "node_union_bytes.h"
#include "v8.h"

namespace node {
namespace native_module {

using NativeModuleRecordMap = std::map<std::string, UnionBytes>;
using NativeModuleCacheMap =
    std::unordered_map<std::string,
                       std::unique_ptr<v8::ScriptCompiler::CachedData>>;

// The native (C++) side of the NativeModule in JS land, which
// handles compilation and caching of builtin modules (NativeModule)
// and bootstrappers, whose source are bundled into the binary
// as static data.
// This class should not depend on a particular isolate, context, or
// environment. Rather it should take them as arguments when necessary.
// The instances of this class are per-process.
class NativeModuleLoader {
 public:
  NativeModuleLoader();
  // TODO(joyeecheung): maybe we should make this a singleton, instead of
  // putting it in per_process.
  NativeModuleLoader(const NativeModuleLoader&) = delete;
  NativeModuleLoader& operator=(const NativeModuleLoader&) = delete;

  static void Initialize(v8::Local<v8::Object> target,
                         v8::Local<v8::Value> unused,
                         v8::Local<v8::Context> context,
                         void* priv);
  v8::Local<v8::Object> GetSourceObject(v8::Local<v8::Context> context) const;
  // Returns config.gypi as a JSON string
  v8::Local<v8::String> GetConfigString(v8::Isolate* isolate) const;

  // Run a script with JS source bundled inside the binary as if it's wrapped
  // in a function called with a null receiver and arguments specified in C++.
  // The returned value is empty if an exception is encountered.
  // JS code run with this method can assume that their top-level
  // declarations won't affect the global scope.
  v8::MaybeLocal<v8::Value> CompileAndCall(
      v8::Local<v8::Context> context,
      const char* id,
      std::vector<v8::Local<v8::String>>* parameters,
      std::vector<v8::Local<v8::Value>>* arguments,
      Environment* optional_env);

  bool Exists(const char* id);

 private:
  static void GetCacheUsage(const v8::FunctionCallbackInfo<v8::Value>& args);
  // Passing ids of builtin module source code into JS land as
  // internalBinding('native_module').moduleIds
  static void ModuleIdsGetter(v8::Local<v8::Name> property,
                              const v8::PropertyCallbackInfo<v8::Value>& info);
  // Passing config.gypi into JS land as internalBinding('native_module').config
  static void ConfigStringGetter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);
  // Get code cache for a specific native module
  static void GetCodeCache(const v8::FunctionCallbackInfo<v8::Value>& args);
  v8::MaybeLocal<v8::Uint8Array> GetCodeCache(v8::Isolate* isolate,
                                              const char* id) const;
  // Compile a specific native module as a function
  static void CompileFunction(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Generated by tools/js2c.py as node_javascript.cc
  void LoadJavaScriptSource();  // Loads data into source_
  UnionBytes GetConfig();       // Return data for config.gypi

  // Generated by tools/generate_code_cache.js as node_code_cache.cc when
  // the build is configured with --code-cache-path=.... They are noops
  // in node_code_cache_stub.cc
  void LoadCodeCache();      // Loads data into code_cache_

  // Compile a script as a NativeModule that can be loaded via
  // NativeModule.p.require in JS land.
  static v8::MaybeLocal<v8::Function> CompileAsModule(Environment* env,
                                                      const char* id);

  // For bootstrappers optional_env may be a nullptr.
  // If an exception is encountered (e.g. source code contains
  // syntax error), the returned value is empty.
  v8::MaybeLocal<v8::Function> LookupAndCompile(
      v8::Local<v8::Context> context,
      const char* id,
      std::vector<v8::Local<v8::String>>* parameters,
      Environment* optional_env);

  NativeModuleRecordMap source_;
  NativeModuleCacheMap code_cache_;
  UnionBytes config_;

  // Used to synchronize access to the code cache map
  Mutex code_cache_mutex_;
};

}  // namespace native_module

namespace per_process {
extern native_module::NativeModuleLoader native_module_loader;
}  // namespace per_process

}  // namespace node

#endif  // defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#endif  // SRC_NODE_NATIVE_MODULE_H_
