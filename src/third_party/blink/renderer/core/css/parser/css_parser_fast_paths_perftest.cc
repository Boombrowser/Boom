// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A benchmark to verify the CSS parser fastpath performance.

#include "third_party/blink/renderer/core/css/parser/css_parser_fast_paths.h"

#include <array>

#include "base/command_line.h"
#include "base/timer/elapsed_timer.h"
#include "testing/perf/perf_result_reporter.h"
#include "testing/perf/perf_test.h"
#include "third_party/blink/renderer/core/css/container_query_data.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/core/execution_context/security_context.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace blink {

struct FastPathSampleCase {
  CSSPropertyID property_id;
  const char* str;
};

// A dump of the values received by the fast path parser during a run
// of MotionMark's “multiply” subtest.
TEST(StyleFastPathPerfTest, MotionMarkMultiply) {
  constexpr const auto kCases = std::to_array<FastPathSampleCase>({
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(355.6972252029457deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.19621357875513%,0.6172480388304258)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(356.9636048166956deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.828430055876808%,0.6081607245304279)"},
      {CSSPropertyID::kTransform, "rotate(252.3693497827005deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.45158897959514%,0.5987765644564652)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(540.9049114637837deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.06670132317567%,0.5891158970394829)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(406.56134916787795deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,19.674697032991634%,0.5791977657979289)"},
      {CSSPropertyID::kTransform, "rotate(252.904911463784deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.06670132317567%,0.5891158970394829)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(270.3693497827005deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.45158897959514%,0.5987765644564652)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(237.9636048166957deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.828430055876808%,0.6081607245304279)"},
      {CSSPropertyID::kTransform, "rotate(179.6972252029457deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.19621357875513%,0.6172480388304258)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(305.5803704376202deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.55384612319145%,0.6260168678064939)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(480.6238011153308deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.90015194411091%,0.634444291579451)"},
      {CSSPropertyID::kTransform, "rotate(294.83885314320423deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.23387511092405%,0.6425061648644728)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(269.23739229684116deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.553684132727064%,0.6501772137661134)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(304.83174537819275deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.858179581387525%,0.657431182410825)"},
      {CSSPropertyID::kTransform, "rotate(285.6346043964139deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.145905204006745%,0.6642410370400733)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(364.6589007264731deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.415362951323452%,0.6705792340503116)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(228.91764720899613deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.665032220468245%,0.6764180563175577)"},
      {CSSPropertyID::kTransform, "rotate(500.42374771769664deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.893393408031287%,0.6817300188154574)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(327.1897758656341deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.098955588368142%,0.6864883399670996)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(256.2277271929785deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.28028777926747%,0.6906674694807629)"},
      {CSSPropertyID::kTransform, "rotate(420.5487522091006deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.436052854443574%,0.6942436569656181)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(386.1628807559489deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.565042748844654%,0.6971955390329324)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(454.0787509079927deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.666213233186333%,0.699504716734475)"},
      {CSSPropertyID::kTransform, "rotate(495.30335754919633deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.738716272444357%,0.7011562910891819)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(352.8418364091543deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.781927891295826%,0.7021393230824292)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(407.69729836285524deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.795469593178915%,0.7024471866046514)"},
      {CSSPropertyID::kTransform, "rotate(409.8707260950823deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.779221733570854%,0.7020777885543699)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(493.3609409905022deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.73332780736298%,0.7010336393617229)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(255.16464281824193deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.658189310275393%,0.6993217684610142)"},
      {CSSPropertyID::kTransform, "rotate(494.27651913493446deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.55445158150468%,0.6969534912641272)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(184.68941610708058deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.422981725611848%,0.6939440453118664)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(208.3945583373843deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.264840254386144%,0.6903121220476939)"},
      {CSSPropertyID::kTransform, "rotate(286.3818027299885deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.081248422819083%,0.6860793260891951)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(232.63991060759264deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.873553336258887%,0.6812695956233554)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(268.15682306832065deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.643192796385552%,0.6759086159012265)"},
      {CSSPropertyID::kTransform, "rotate(173.91992659025215deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.39166157946943%,0.6700232535048991)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(430.9162986904862deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.120480464232465%,0.6636410330979723)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(170.1329265493725deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.831168912086728%,0.6567896717597491)"},
      {CSSPropertyID::kTransform, "rotate(336.5568945183305deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.525221903043505%,0.649496679596846)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(395.17553905832256deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.204091083653083%,0.6417890297269425)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(370.97657174884796deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.869170109359167%,0.6336928962717642)"},
      {CSSPropertyID::kTransform, "rotate(268.9481725176618deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.521783868268415%,0.625233455779407)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(419.0790562031352deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.163181151454555%,0.6164347454476659)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(327.3585160566441deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.7945302750627%,0.6073195704647072)"},
      {CSSPropertyID::kTransform, "rotate(266.7764479247682deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.41691714751915%,0.5979094525014035)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(424.323358724357deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.031345296992566%,0.5882246116595915)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(478.9903625299198deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,19.638737417470008%,0.5782839748058638)"},
      {CSSPropertyID::kTransform, "rotate(428.3442573146339deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.02958017707566%,0.588180098026145)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(516.8192493027509deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.413273278985123%,0.5978182863114687)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(232.424252382517deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,20.78888970869096%,0.6071795532350615)"},
      {CSSPropertyID::kTransform, "rotate(258.1687811895701deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.155422342235266%,0.6162436327886638)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(492.06295309550205deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.51178264716604%,0.6249889751469271)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(226.1174775523147deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.85680093735161%,0.6333927693635378)"},
      {CSSPropertyID::kTransform, "rotate(181.34362950264276deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.18922861985122%,0.6414310000069803)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(393.7532033016762deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.507742911341467%,0.6490785453060384)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(271.35844350728206deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.81095451847519%,0.6563093245656318)"},
      {CSSPropertyID::kTransform, "rotate(401.17194907097416deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.097418758729376%,0.6630965022383826)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(311.2065480049667deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.36565053208841%,0.6694127548816107)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(440.4751406072937deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.614143425901368%,0.6752306050850942)"},
      {CSSPropertyID::kTransform, "rotate(482.9905108721845deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.841393035052324%,0.6805228231636948)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(186.7651078200552deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.045924303485943%,0.6852628929206604)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(283.8108010928379deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.226322348291877%,0.6894255322253269)"},
      {CSSPropertyID::kTransform, "rotate(288.13861810939727deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.38126583570675%,0.6929872528674985)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(194.75847307437516deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.509561577726814%,0.6959269377638936)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(266.6789007843312deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.610178661593874%,0.6982264079464295)"},
      {CSSPropertyID::kTransform, "rotate(163.90681001738193deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.682280173938558%,0.6998709478380001)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(143.4472718851514deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.72525049630536%,0.7008497560591331)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(464.30335754919633deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.738716272444357%,0.7011562910891819)"},
      {CSSPropertyID::kTransform, "rotate(442.47603705992856deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.722559493824708%,0.7007884867376568)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(435.9641469503367deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.676921693858546%,0.6997488211690368)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(189.7644290776033deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.602198920909785%,0.6980442341708002)"},
      {CSSPropertyID::kTransform, "rotate(206.8716377271931deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.49902788515583%,0.6956858990235777)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(188.27870692132896deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.368264345054218%,0.6926888661370181)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(359.97696586729006deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.210955327254947%,0.6890716041486575)"},
      {CSSPropertyID::kTransform, "rotate(197.95638798523782deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,24.028307100059145%,0.6848554694989923)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(364.2058581292528deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.821650924117105%,0.6800641372553149)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(319.7134433373213deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.5924085018252%,0.6747230244189648)"},
      {CSSPropertyID::kTransform, "rotate(161.4666543810154deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.342058784184232%,0.6688587328290556)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(427.4526880863808deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,23.072107430944836%,0.6624985330232087)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(191.65864340660053deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.784059818002767%,0.655669904006537)"},
      {CSSPropertyID::kTransform, "rotate(250.07170715544788deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.479398097106493%,0.6484001376426283)"},
      {CSSPropertyID::kDisplay, "block"},
      {CSSPropertyID::kTransform, "rotate(205.67930788112204deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,22.15956247369168%,0.6407160109049501)"},
      {CSSPropertyID::kVisibility, "visible"},
      {CSSPropertyID::kTransform, "rotate(496.4692384171266deg)"},
      {CSSPropertyID::kColor,
       "hsla(53.59680000042915,100%,21.825936599357863%,0.6326435248456137)"},
      {CSSPropertyID::kTransform, "rotate(450.42974914112926deg)"},
  });

  const std::string parse_iterations_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          "style-fastpath-parse-iterations");
  int parse_iterations =
      parse_iterations_str.empty() ? 10000 : stoi(parse_iterations_str);
  auto* context = MakeGarbageCollected<CSSParserContext>(
      kHTMLStandardMode, SecureContextMode::kInsecureContext);

  base::ElapsedTimer timer;
  for (int i = 0; i < parse_iterations; ++i) {
    int num_fast = 0;
    for (const FastPathSampleCase& c : kCases) {
      const CSSValue* value =
          CSSParserFastPaths::MaybeParseValue(c.property_id, c.str, context);
      if (value) {
        ++num_fast;
      }
    }
    CHECK_EQ(195, num_fast);
  }
  base::TimeDelta elapsed = timer.Elapsed();

  auto reporter = perf_test::PerfResultReporter("BlinkStyleFastParser",
                                                "MotionMarkMultiply");
  reporter.RegisterImportantMetric("ParseTime", "ns");
  reporter.AddResult("ParseTime", elapsed.InMicrosecondsF() * 1e3 /
                                      (parse_iterations * kCases.size()));
}

}  // namespace blink
