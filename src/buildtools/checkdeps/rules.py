# Copyright 2012 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base classes to represent dependency rules, used by checkdeps.py"""


import os
import re


# Rules instances need a back reference in order to evaluate
# new_usages_require_review, but cannot hold a direct one because we
# deepcopy them. Use this list to enable indirect references.
deps_builders = []


class Rule(object):
  """Specifies a single rule for an include, which can be one of
  ALLOW, DISALLOW and TEMP_ALLOW.
  """

  # These are the prefixes used to indicate each type of rule. These
  # are also used as values for self.allow to indicate which type of
  # rule this is.
  ALLOW = '+'
  DISALLOW = '-'
  TEMP_ALLOW = '!'

  def __init__(self, allow, directory, dependent_directory, source):
    self.allow = allow
    self._dir = directory
    self._dependent_dir = dependent_directory
    self._source = source

  def __str__(self):
    return '"%s%s" from %s.' % (self.allow, self._dir, self._source)

  def AsDependencyTuple(self):
    """Returns a tuple (allow, dependent dir, dependee dir) for this rule,
    which is fully self-sufficient to answer the question whether the dependent
    is allowed to depend on the dependee, without knowing the external
    context."""
    return self.allow, self._dependent_dir or '.', self._dir or '.'

  def ParentOrMatch(self, other):
    """Returns true if the input string is an exact match or is a parent
    of the current rule. For example, the input "foo" would match "foo/bar"."""
    return self._dir == other or self._dir.startswith(other + '/')

  def ChildOrMatch(self, other):
    """Returns true if the input string would be covered by this rule. For
    example, the input "foo/bar" would match the rule "foo"."""
    return self._dir == other or other.startswith(self._dir + '/')


class MessageRule(Rule):
  """A rule that has a simple message as the reason for failing,
  unrelated to directory or source.
  """

  def __init__(self, reason):
    super(MessageRule, self).__init__(Rule.DISALLOW, '', '', '')
    self._reason = reason

  def __str__(self):
    return self._reason


def ParseRuleString(rule_string, source):
  """Returns a tuple of a character indicating what type of rule this
  is, and a string holding the path the rule applies to.
  """
  if not rule_string:
    raise Exception('The rule string "%s" is empty\nin %s' %
                    (rule_string, source))

  if not rule_string[0] in [Rule.ALLOW, Rule.DISALLOW, Rule.TEMP_ALLOW]:
    raise Exception(
      'The rule string "%s" does not begin with a "+", "-" or "!".' %
      rule_string)

  # If a directory is specified in a DEPS file with a trailing slash, then it
  # will not match as a parent directory in Rule's [Parent|Child]OrMatch above.
  # Ban them.
  if rule_string[-1] == '/':
    raise Exception(
      'The rule string "%s" ends with a "/" which is not allowed.'
      ' Please remove the trailing "/".' % rule_string)

  return rule_string[0], rule_string[1:]


class Rules(object):
  """Sets of rules for files in a directory.

  By default, rules are added to the set of rules applicable to all
  dependee files in the directory.  Rules may also be added that apply
  only to dependee files whose filename (last component of their path)
  matches a given regular expression; hence there is one additional
  set of rules per unique regular expression.
  """

  def __init__(self, deps_builder_index):
    """Initializes the current rules with an empty rule list for all
    files.
    """
    # We keep the general rules out of the specific rules dictionary,
    # as we need to always process them last.
    self._general_rules = []

    # Keys are regular expression strings, values are arrays of rules
    # that apply to dependee files whose basename matches the regular
    # expression.  These are applied before the general rules, but
    # their internal order is arbitrary.
    self._specific_rules = {}

    # Index of the DepsBuilder associated with this instance.
    self._deps_builder_index = deps_builder_index

  def __str__(self):
    result = ['Rules = {\n    (apply to all files): [\n%s\n    ],' % '\n'.join(
        '      %s' % x for x in self._general_rules)]
    for regexp, rules in list(self._specific_rules.items()):
      result.append('    (limited to files matching %s): [\n%s\n    ]' % (
          regexp, '\n'.join('      %s' % x for x in rules)))
    result.append('  }')
    return '\n'.join(result)

  def _FindFirstAncestorThatRequiresReview(self, include_path):
    deps_builder = deps_builders[self._deps_builder_index]
    return deps_builder.FindFirstAncestorThatRequiresReview(include_path)

  def AsDependencyTuples(self, include_general_rules, include_specific_rules):
    """Returns a list of tuples (allow, dependent dir, dependee dir) for the
    specified rules (general/specific). Currently only general rules are
    supported."""
    def AddDependencyTuplesImpl(deps, rules, extra_dependent_suffix=""):
      for rule in rules:
        (allow, dependent, dependee) = rule.AsDependencyTuple()
        tup = (allow, dependent + extra_dependent_suffix, dependee)
        deps.add(tup)

    deps = set()
    if include_general_rules:
      AddDependencyTuplesImpl(deps, self._general_rules)
    if include_specific_rules:
      for regexp, rules in list(self._specific_rules.items()):
        AddDependencyTuplesImpl(deps, rules, "/" + regexp)
    return deps

  def AddRule(self, rule_string, dependent_dir, source, dependee_regexp=None):
    """Adds a rule for the given rule string.

    Args:
      rule_string: The include_rule string read from the DEPS file to apply.
      source: A string representing the location of that string (filename, etc.)
              so that we can give meaningful errors.
      dependent_dir: The directory to which this rule applies.
      dependee_regexp: The rule will only be applied to dependee files
                       whose filename (last component of their path)
                       matches the expression. None to match all
                       dependee files.
    """
    rule_type, rule_dir = ParseRuleString(rule_string, source)

    if not dependee_regexp:
      rules_to_update = self._general_rules
    else:
      if dependee_regexp in self._specific_rules:
        rules_to_update = self._specific_rules[dependee_regexp]
      else:
        rules_to_update = []

    rules_to_update.insert(0, Rule(rule_type, rule_dir, dependent_dir, source))

    if not dependee_regexp:
      self._general_rules = rules_to_update
    else:
      self._specific_rules[dependee_regexp] = rules_to_update

  def RuleApplyingTo(self, include_path, dependee_path):
    """Returns the rule that applies to |include_path| for a dependee
    file located at |dependee_path|.
    """
    review_parent = self._FindFirstAncestorThatRequiresReview(include_path)
    almost_match = None

    dependee_filename = os.path.basename(dependee_path)
    for regexp, specific_rules in list(self._specific_rules.items()):
      if re.match(regexp, dependee_filename):
        for rule in specific_rules:
          if rule.ChildOrMatch(include_path):
            if review_parent is None:
              return rule
            if rule.ParentOrMatch(review_parent):
              return rule
            almost_match = rule
    for rule in self._general_rules:
      if rule.ChildOrMatch(include_path):
        if review_parent is None:
          return rule
        if rule.ParentOrMatch(review_parent):
          return rule
        almost_match = rule
    if almost_match:
      return MessageRule(f'no rule applying for directory "{review_parent}", '
                         'which is marked as new_usages_require_review=True.')
    return MessageRule('no rule applying.')
