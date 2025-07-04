---
Language:        Cpp
# BasedOnStyle:  Microsoft

AccessModifierOffset: -4

AlignAfterOpenBracket: BlockIndent
AlignArrayOfStructures: Right
AlignConsecutiveAssignments: AcrossComments
AlignConsecutiveBitFields: AcrossComments
AlignConsecutiveDeclarations: AcrossComments
AlignConsecutiveMacros: AcrossComments
AlignEscapedNewlines: Left
AlignOperands:   AlignAfterOperator
AlignTrailingComments: true

AllowAllArgumentsOnNextLine: true
AllowAllConstructorInitializersOnNextLine: true
AllowAllParametersOfDeclarationOnNextLine: true

AllowShortBlocksOnASingleLine: Empty
AllowShortCaseLabelsOnASingleLine: false
AllowShortEnumsOnASingleLine: true
AllowShortFunctionsOnASingleLine: InlineOnly
AllowShortIfStatementsOnASingleLine: Never
AllowShortLambdasOnASingleLine: Inline
AllowShortLoopsOnASingleLine: false

#AlwaysBreakAfterDefinitionReturnType: None  # deprecated
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: true
AlwaysBreakTemplateDeclarations: Yes

#AttributeMacros: [] # TBD

BinPackArguments: true
BinPackParameters: true

BitFieldColonSpacing: Both
BraceWrapping:
  AfterCaseLabel:  true
  AfterClass:      true
  AfterControlStatement: Always
  AfterEnum:       true
  AfterFunction:   true
  AfterNamespace:  true
  AfterObjCDeclaration: true
  AfterStruct:     true
  AfterUnion:      true
  AfterExternBlock: true
  BeforeCatch:     true
  BeforeElse:      true
  BeforeLambdaBody: true
  BeforeWhile:     true
  IndentBraces:    false
  SplitEmptyFunction: true
  SplitEmptyRecord: true
  SplitEmptyNamespace: true
BreakBeforeBinaryOperators: None # TBD
BreakBeforeBraces: Custom
BreakBeforeTernaryOperators: false
BreakConstructorInitializers: BeforeComma
BreakInheritanceList: BeforeComma
BreakStringLiterals: true

ColumnLimit:     150 # TBD
CommentPragmas:  '^ IWYU pragma:' # TBD`
CompactNamespaces: false
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true

DeriveLineEnding: true
DerivePointerAlignment: true

DisableFormat:   false
EmptyLineAfterAccessModifier: Leave
EmptyLineBeforeAccessModifier: Leave
FixNamespaceComments: true
ForEachMacros:
  - foreach
  - Q_FOREACH
  - BOOST_FOREACH
IfMacros: [] # A vector of macros that should be interpreted as conditionals instead of as function calls
IncludeBlocks:   Preserve
IncludeCategories:
  - Regex:           '^"(llvm|llvm-c|clang|clang-c)/'
    Priority:        2
    SortPriority:    0
  - Regex:           '^(<|"(gtest|gmock|isl|json)/)'
    Priority:        3
    SortPriority:    0
  - Regex:           '.*'
    Priority:        1
    SortPriority:    0
IncludeIsMainRegex: '(Test)?$'
IncludeIsMainSourceRegex: ''

IndentAccessModifiers: false
IndentCaseBlocks: false
IndentCaseLabels: false
IndentExternBlock: NoIndent
IndentGotoLabels: false
IndentPPDirectives: None
IndentRequires: true # rename to IndentRequiresClause in clang-format 15
IndentWidth:     4
IndentWrappedFunctionNames: false

KeepEmptyLinesAtTheStartOfBlocks: false
LambdaBodyIndentation: Signature
MacroBlockBegin: '' # TBD
MacroBlockEnd:   '' # TBD
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
NamespaceMacros: [] # TBD A vector of macros which are used to open namespace blocks.

# TBD leave default
#PenaltyBreakAssignment: 2
#PenaltyBreakBeforeFirstCallParameter: 19
#PenaltyBreakComment: 300
#PenaltyBreakFirstLessLess: 120
#PenaltyBreakString: 1000
#PenaltyBreakTemplateDeclaration: 10
#PenaltyExcessCharacter: 1000000
#PenaltyIndentedWhitespace: 0
PenaltyReturnTypeOnItsOwnLine: 100000000

PointerAlignment: Left
RawStringFormats: [] # TBD
ReferenceAlignment: Left
ReflowComments:  false
ShortNamespaceLines: 20

SortIncludes:    Never
SortUsingDeclarations: false

SpaceAfterCStyleCast: true
SpaceAfterLogicalNot: false
SpaceAfterTemplateKeyword: false

SpaceAroundPointerQualifiers: Default

SpaceBeforeAssignmentOperators: true
SpaceBeforeCaseColon: false
SpaceBeforeCpp11BracedList: true
SpaceBeforeCtorInitializerColon: true
SpaceBeforeInheritanceColon: true
SpaceBeforeParens: ControlStatementsExceptControlMacros
SpaceBeforeRangeBasedForLoopColon: true
SpaceBeforeSquareBrackets: false

SpaceInEmptyBlock: false
SpaceInEmptyParentheses: false

SpacesBeforeTrailingComments: 1

SpacesInAngles: Never # TBD
SpacesInCStyleCastParentheses: false
SpacesInConditionalStatement: false
SpacesInLineCommentPrefix:
  Minimum: 1
  Maximum: -1
SpacesInParentheses: false
SpacesInSquareBrackets: false

Standard:        c++20
StatementAttributeLikeMacros:
  - emit
StatementMacros:
  - Q_UNUSED
  - QT_REQUIRE_VERSION
TabWidth:        4
UseCRLF:         false
UseTab:          Never
WhitespaceSensitiveMacros:
  - STRINGIZE
  - PP_STRINGIZE
  - BOOST_PP_STRINGIZE
...
