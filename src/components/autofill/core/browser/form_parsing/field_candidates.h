// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_PARSING_FIELD_CANDIDATES_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_PARSING_FIELD_CANDIDATES_H_

#include <vector>

#include "base/containers/flat_map.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/form_parsing/autofill_parsing_utils.h"
#include "components/autofill/core/common/dense_set.h"
#include "components/autofill/core/common/is_required.h"
#include "components/autofill/core/common/unique_ids.h"

namespace autofill {

// Represents a possible type for a given field.
struct FieldCandidate {
  // The associated type for this candidate.
  FieldType type = internal::IsRequired();

  // Based on which attribute the `type` was derived.
  MatchAttribute match_attribute = internal::IsRequired();

  // A non-negative number indicating how sure the type is for this specific
  // candidate. The higher the more confidence.
  float score = internal::IsRequired();
};

// Each field can be of different types. This class collects all these possible
// types and determines which type is the most likely.
class FieldCandidates {
 public:
  FieldCandidates();
  FieldCandidates(FieldCandidates&& other);
  FieldCandidates& operator=(FieldCandidates&& other);
  ~FieldCandidates();

  // Includes a possible |type| for a given field.
  //
  // Callers are responsible for the scores they add. FieldCandidates is
  // agnostic to the source of these scores and will select the best candidate
  // based solely on their numeric values. BestHeuristicType() uses |score| to
  // determine the most likely type for this given field. Please see
  // field_candidates.cc for details on how this type is actually chosen.
  void AddFieldCandidate(FieldType type,
                         MatchAttribute match_attribute,
                         float score);

  // Determines the best type based on the current possible types.
  FieldType BestHeuristicType() const;

  // The MatchAttributes responsible for determining `BestHeuristicType()`.
  DenseSet<MatchAttribute> BestHeuristicTypeReason() const;

 private:
  // Internal storage for all the possible types for a given field.
  std::vector<FieldCandidate> field_candidates_;
};

// A map from the field's global ID to its possible candidates.
using FieldCandidatesMap = base::flat_map<FieldGlobalId, FieldCandidates>;

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_PARSING_FIELD_CANDIDATES_H_
