{
  "targets": [
    {
      "target_name": "test_cannot_run_js",
      "sources": [
        "test_cannot_run_js.c"
      ],
      "defines": [ "JSVM_EXPERIMENTAL" ],
    },
    {
      "target_name": "test_pending_exception",
      "sources": [
        "test_cannot_run_js.c"
      ],
      "defines": [ "JSVM_VERSION=8" ],
    }
  ]
}
