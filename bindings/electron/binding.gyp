{
  "targets": [
    {
      "target_name": "shared_audio_node",
      "sources": [
        "src/node_audio_wrapper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../include"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "libraries": [
        "-L../../build",
        "-lSharedAudioCore"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ 
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "NODE_ADDON_API_DISABLE_DEPRECATED"
      ],
      "conditions": [
        ["OS=='win'", {
          "libraries": [
            "-L../../build/Release",
            "SharedAudioCore.lib"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalIncludeDirectories": [
                "../../include"
              ]
            }
          }
        }],
        ["OS=='mac'", {
          "libraries": [
            "-L../../build",
            "-lSharedAudioCore"
          ],
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.12"
          }
        }],
        ["OS=='linux'", {
          "libraries": [
            "-L../../build",
            "-lSharedAudioCore",
            "-Wl,-rpath,../../build"
          ],
          "cflags_cc": [
            "-std=c++17",
            "-fexceptions"
          ]
        }]
      ]
    }
  ]
}