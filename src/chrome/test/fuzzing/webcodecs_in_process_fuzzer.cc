// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/fuzzing/in_process_proto_fuzzer.h"
#include "chrome/test/fuzzing/webcodecs_fuzzer_grammar.h"
#include "chrome/test/fuzzing/webcodecs_fuzzer_grammar.pb.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/libfuzzer/proto/lpm_interface.h"
#include "testing/libfuzzer/research/domatolpm/domatolpm.h"

// This fuzzer uses DomatoLPM to generate JS based on an existing Domato
// rule.
class WebcodecsInProcessFuzzer
    : public InProcessBinaryProtoFuzzer<
          domatolpm::generated::webcodecs_fuzzer_grammar::fuzzcase> {
 public:
  using FuzzCase = domatolpm::generated::webcodecs_fuzzer_grammar::fuzzcase;
  WebcodecsInProcessFuzzer() = default;
  void SetUpOnMainThread() override;
  int Fuzz(const FuzzCase& fuzz_case) override;
};

REGISTER_BINARY_PROTO_IN_PROCESS_FUZZER(WebcodecsInProcessFuzzer)

void WebcodecsInProcessFuzzer::SetUpOnMainThread() {
  InProcessFuzzer::SetUpOnMainThread();
  // Some of the functionality tested only runs in a secure context. about:blank
  // is not considered a secure context, whereas chrome://chrome-urls/ (for
  // example) is. Navigate to this page and execute the JS in this context.
  CHECK(ui_test_utils::NavigateToURL(browser(), GURL("chrome://chrome-urls/")));
}

int WebcodecsInProcessFuzzer::Fuzz(const FuzzCase& fuzz_case) {
  domatolpm::Context ctx;
  CHECK(domatolpm::webcodecs_fuzzer_grammar::handle_fuzzer(&ctx, fuzz_case));
  std::string_view js_str(ctx.GetBuilder()->view());
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderFrameHost* rfh = contents->GetPrimaryMainFrame();
  auto res = content::ExecJs(rfh, js_str);
  return 0;
}
