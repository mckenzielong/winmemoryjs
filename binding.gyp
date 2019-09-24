{
  "targets": [
    {
      "target_name": "winmemoryjs",
      "sources": [ 
        "lib/winmemoryjs.cc",
        "lib/process.cc",
        "lib/module.cc",
        "lib/thread.cc",
        "lib/process_entry.cc",
        "lib/module_entry.cc",
        "lib/thread_entry.cc"
      ],
      'include_dirs': ["<!@(node -p \"require('node-addon-api').include\")", "lib/include"],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      'defines': [ 'NAPI_CPP_EXCEPTIONS' ],
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
    },    
  ]
}
