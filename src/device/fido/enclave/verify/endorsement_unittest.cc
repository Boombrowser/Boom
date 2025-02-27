// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/351564777): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "device/fido/enclave/verify/endorsement.h"

#include "base/time/time.h"
#include "device/fido/enclave/verify/claim.h"
#include "device/fido/enclave/verify/test_utils.h"
#include "device/fido/enclave/verify/utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device::enclave {

TEST(
    EndorsementTest,
    VerifyEndorsementStatement_WithEndorsementHasBadStatementType_ReturnsFalse) {
  EXPECT_FALSE(VerifyEndorsementStatement(
      base::Time::FromTimeT(17),
      MakeEndorsementStatement("bad statement type",
                               /*predicate_type=*/kPredicateV2,
                               /*issued_on=*/base::Time::FromTimeT(10),
                               /*not_before=*/base::Time::FromTimeT(15),
                               /*not_after=*/base::Time::FromTimeT(20))));
}

TEST(EndorsementTest,
     VerifyEndorsementStatement_WithInvalidValidityDuration_ReturnsFalse) {
  EXPECT_FALSE(VerifyEndorsementStatement(base::Time::FromTimeT(10),
                                          MakeValidEndorsementStatement()));
}

TEST(
    EndorsementTest,
    VerifyEndorsementStatement_WithValidEndorsementAndValidityDuration_ReturnsTrue) {
  EXPECT_TRUE(VerifyEndorsementStatement(base::Time::FromTimeT(17),
                                         MakeValidEndorsementStatement()));
}

TEST(EndorsementTest,
     VerifyEndorserPublicKey_WithValidLogEntryAndKey_ReturnsTrue) {
  auto endorser = ConvertPemToRaw(GetContentsFromFile("endorser.pem"));
  ASSERT_TRUE(endorser.has_value());
  std::string log_entry_str = GetContentsFromFile("logentry.json");
  EXPECT_TRUE(
      VerifyEndorserPublicKey(base::as_byte_span(log_entry_str), *endorser));
}

TEST(EndorsementTest,
     VerifyEndorserPublicKey_WithInvalidLogEntry_ReturnsFalse) {
  auto endorser = ConvertPemToRaw(GetContentsFromFile("endorser.pem"));
  ASSERT_TRUE(endorser.has_value());
  std::string log_entry_str = GetContentsFromFile("logentry_backslash.json");
  EXPECT_FALSE(
      VerifyEndorserPublicKey(base::as_byte_span(log_entry_str), *endorser));
}

TEST(EndorsementTest, VerifyEndorserPublicKey_WithInvalidKey_ReturnsFalse) {
  auto endorser = ConvertPemToRaw(GetContentsFromFile("rekor_pub_key.pem"));
  ASSERT_TRUE(endorser.has_value());
  std::string log_entry_str = GetContentsFromFile("logentry.json");
  EXPECT_FALSE(
      VerifyEndorserPublicKey(base::as_byte_span(log_entry_str), *endorser));
}

TEST(EndorsementTest,
     VerifyBindaryEndorsement_WithValidEndorsement_ReturnsTrue) {
  std::string endorsement_str = GetContentsFromFile("endorsement.json");
  std::string signature_str = GetContentsFromFile("endorsement.json.sig");
  std::string log_entry_str = GetContentsFromFile("logentry.json");
  auto endorser_pub_key =
      ConvertPemToRaw(GetContentsFromFile("endorser_public_key.pem"));
  ASSERT_TRUE(endorser_pub_key.has_value());
  auto rekor_pub_key =
      ConvertPemToRaw(GetContentsFromFile("rekor_pub_key.pem"));
  ASSERT_TRUE(rekor_pub_key.has_value());
  EXPECT_TRUE(VerifyBinaryEndorsement(
      base::Time::Now(), base::as_byte_span(endorsement_str),
      base::as_byte_span(signature_str), base::as_byte_span(log_entry_str),
      *endorser_pub_key, *rekor_pub_key));
}

TEST(EndorsementTest,
     VerifyBindaryEndorsement_WithInvalidEndorserPublicKey_ReturnsFalse) {
  std::string endorsement_str = GetContentsFromFile("endorsement.json");
  std::string signature_str = GetContentsFromFile("endorsement.json.sig");
  std::string log_entry_str = GetContentsFromFile("logentry.json");
  auto rekor_pub_key =
      ConvertPemToRaw(GetContentsFromFile("rekor_pub_key.pem"));
  ASSERT_TRUE(rekor_pub_key.has_value());
  EXPECT_FALSE(VerifyBinaryEndorsement(
      base::Time::Now(), base::as_byte_span(endorsement_str),
      base::as_byte_span(signature_str), base::as_byte_span(log_entry_str),
      *rekor_pub_key, *rekor_pub_key));
}

TEST(EndorsementTest,
     VerifyBindaryEndorsement_WithInvalidRekorPublicKey_ReturnsFalse) {
  std::string endorsement_str = GetContentsFromFile("endorsement.json");
  std::string signature_str = GetContentsFromFile("endorsement.json.sig");
  std::string log_entry_str = GetContentsFromFile("logentry.json");
  auto endorser_pub_key =
      ConvertPemToRaw(GetContentsFromFile("endorser_public_key.pem"));
  ASSERT_TRUE(endorser_pub_key.has_value());
  EXPECT_FALSE(VerifyBinaryEndorsement(
      base::Time::Now(), base::as_byte_span(endorsement_str),
      base::as_byte_span(signature_str), base::as_byte_span(log_entry_str),
      *endorser_pub_key, *endorser_pub_key));
}

TEST(EndorsementTest,
     VerifyBindaryEndorsement_WithRekorKeyAndNoLogEntry_ReturnsFalse) {
  std::string endorsement_str = GetContentsFromFile("endorsement.json");
  std::string signature_str = GetContentsFromFile("endorsement.json.sig");
  auto endorser_pub_key =
      ConvertPemToRaw(GetContentsFromFile("endorser_public_key.pem"));
  ASSERT_TRUE(endorser_pub_key.has_value());
  auto rekor_pub_key =
      ConvertPemToRaw(GetContentsFromFile("rekor_pub_key.pem"));
  ASSERT_TRUE(rekor_pub_key.has_value());
  EXPECT_FALSE(VerifyBinaryEndorsement(base::Time::Now(),
                                       base::as_byte_span(endorsement_str),
                                       base::as_byte_span(signature_str), {},
                                       *endorser_pub_key, *rekor_pub_key));
}

}  // namespace device::enclave
