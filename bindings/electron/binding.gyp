{
  "targets": [
    {
      "target_name": "shared_audio_node",
      "sources": [
        "src/node_audio_wrapper.cpp",
        "src/node_cue_manager.cpp",
        "src/node_crossfade_engine.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../include",
        "<!(node -p \"process.env.JUCE_DIR || ''\")/modules"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ 
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
      ],
      "conditions": [
        ["OS==\"win\"", {
          "libraries": [
            "<(module_root_dir)/../../build/Release/SharedAudioCore.lib"
          ],
          "copies": [{
            "destination": "<(module_root_dir)/build/Release/",
            "files": [
              "<(module_root_dir)/../../build/Release/SharedAudioCore.dll"
            ]
          }],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": [ "/EHsc" ]
            }
          }
        }],
        ["OS==\"mac\"", {
          "libraries": [
            "<(module_root_dir)/../../build/libSharedAudioCore.dylib"
          ],
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.11"
          }
        }],
        ["OS==\"linux\"", {
          "libraries": [
            "<(module_root_dir)/../../build/libSharedAudioCore.so"
          ],
          "cflags": [ "-std=c++17" ],
          "cflags_cc": [ "-std=c++17" ]
        }]
      ]
    }
  ]
}