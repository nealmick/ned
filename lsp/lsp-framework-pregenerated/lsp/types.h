#pragma once

/*#############################################################
 * NOTE: This is a generated file and it shouldn't be modified!
 *#############################################################*/

#include <tuple>
#include <string>
#include <vector>
#include <variant>
#include <string_view>
#include <lsp/uri.h>
#include <lsp/strmap.h>
#include <lsp/fileuri.h>
#include <lsp/nullable.h>
#include <lsp/json/json.h>
#include <lsp/enumeration.h>
#include <lsp/serialization.h>

namespace lsp{

inline constexpr std::string_view VersionStr{"3.17.0"};

using Null      = std::nullptr_t;
using uint      = unsigned int;
using String    = std::string;
using LSPArray  = json::Array;
using LSPObject = json::Object;
using LSPAny    = json::Any;

template<typename T>
using Opt = std::optional<T>;

template<typename... Args>
using Tuple = std::tuple<Args...>;

template<typename... Args>
using OneOf = std::variant<Args...>;

template<typename T>
using NullOr = Nullable<T>;

template<typename... Args>
using NullOrOneOf = NullableVariant<Args...>;

template<typename T>
using Array = std::vector<T>;

template<typename K, typename T>
using Map = StrMap<K, T>;

/*
 * SemanticTokenTypes
 *
 * A set of predefined token types. This set is not fixed
 * an clients can specify additional token types via the
 * corresponding client capabilities.
 * 
 * @since 3.16.0
 */
enum class SemanticTokenTypes{
	Namespace,
	/*
	 * Represents a generic type. Acts as a fallback for types which can't be mapped to
	 * a specific type like class or enum.
	 */
	Type,
	Class,
	Enum,
	Interface,
	Struct,
	TypeParameter,
	Parameter,
	Variable,
	Property,
	EnumMember,
	Event,
	Function,
	Method,
	Macro,
	Keyword,
	Modifier,
	Comment,
	String,
	Number,
	Regexp,
	Operator,
	/*
	 * @since 3.17.0
	 */
	Decorator,
	MAX_VALUE
};
using SemanticTokenTypesEnum = Enumeration<SemanticTokenTypes, String>;
template<>
const SemanticTokenTypesEnum::ConstInitType SemanticTokenTypesEnum::s_values[];

/*
 * SemanticTokenModifiers
 *
 * A set of predefined token modifiers. This set is not fixed
 * an clients can specify additional token types via the
 * corresponding client capabilities.
 * 
 * @since 3.16.0
 */
enum class SemanticTokenModifiers{
	Declaration,
	Definition,
	Readonly,
	Static,
	Deprecated,
	Abstract,
	Async,
	Modification,
	Documentation,
	DefaultLibrary,
	MAX_VALUE
};
using SemanticTokenModifiersEnum = Enumeration<SemanticTokenModifiers, String>;
template<>
const SemanticTokenModifiersEnum::ConstInitType SemanticTokenModifiersEnum::s_values[];

/*
 * DocumentDiagnosticReportKind
 *
 * The document diagnostic report kinds.
 * 
 * @since 3.17.0
 */
enum class DocumentDiagnosticReportKind{
	/*
	 * A diagnostic report with a full
	 * set of problems.
	 */
	Full,
	/*
	 * A report indicating that the last
	 * returned report is still accurate.
	 */
	Unchanged,
	MAX_VALUE
};
using DocumentDiagnosticReportKindEnum = Enumeration<DocumentDiagnosticReportKind, String>;
template<>
const DocumentDiagnosticReportKindEnum::ConstInitType DocumentDiagnosticReportKindEnum::s_values[];

/*
 * ErrorCodes
 *
 * Predefined error codes.
 */
enum class ErrorCodes{
	ParseError,
	InvalidRequest,
	MethodNotFound,
	InvalidParams,
	InternalError,
	/*
	 * Error code indicating that a server received a notification or
	 * request before the server has received the `initialize` request.
	 */
	ServerNotInitialized,
	UnknownErrorCode,
	MAX_VALUE
};
using ErrorCodesEnum = Enumeration<ErrorCodes, int>;
template<>
const ErrorCodesEnum::ConstInitType ErrorCodesEnum::s_values[];

/*
 * LSPErrorCodes
 */
enum class LSPErrorCodes{
	/*
	 * A request failed but it was syntactically correct, e.g the
	 * method name was known and the parameters were valid. The error
	 * message should contain human readable information about why
	 * the request failed.
	 * 
	 * @since 3.17.0
	 */
	RequestFailed,
	/*
	 * The server cancelled the request. This error code should
	 * only be used for requests that explicitly support being
	 * server cancellable.
	 * 
	 * @since 3.17.0
	 */
	ServerCancelled,
	/*
	 * The server detected that the content of a document got
	 * modified outside normal conditions. A server should
	 * NOT send this error code if it detects a content change
	 * in it unprocessed messages. The result even computed
	 * on an older state might still be useful for the client.
	 * 
	 * If a client decides that a result is not of any use anymore
	 * the client should cancel the request.
	 */
	ContentModified,
	/*
	 * The client has canceled a request and a server has detected
	 * the cancel.
	 */
	RequestCancelled,
	MAX_VALUE
};
using LSPErrorCodesEnum = Enumeration<LSPErrorCodes, int>;
template<>
const LSPErrorCodesEnum::ConstInitType LSPErrorCodesEnum::s_values[];

/*
 * FoldingRangeKind
 *
 * A set of predefined range kinds.
 */
enum class FoldingRangeKind{
	/*
	 * Folding range for a comment
	 */
	Comment,
	/*
	 * Folding range for an import or include
	 */
	Imports,
	/*
	 * Folding range for a region (e.g. `#region`)
	 */
	Region,
	MAX_VALUE
};
using FoldingRangeKindEnum = Enumeration<FoldingRangeKind, String>;
template<>
const FoldingRangeKindEnum::ConstInitType FoldingRangeKindEnum::s_values[];

/*
 * SymbolKind
 *
 * A symbol kind.
 */
enum class SymbolKind{
	File,
	Module,
	Namespace,
	Package,
	Class,
	Method,
	Property,
	Field,
	Constructor,
	Enum,
	Interface,
	Function,
	Variable,
	Constant,
	String,
	Number,
	Boolean,
	Array,
	Object,
	Key,
	Null,
	EnumMember,
	Struct,
	Event,
	Operator,
	TypeParameter,
	MAX_VALUE
};
using SymbolKindEnum = Enumeration<SymbolKind, uint>;
template<>
const SymbolKindEnum::ConstInitType SymbolKindEnum::s_values[];

/*
 * SymbolTag
 *
 * Symbol tags are extra annotations that tweak the rendering of a symbol.
 * 
 * @since 3.16
 */
enum class SymbolTag{
	/*
	 * Render a symbol as obsolete, usually using a strike-out.
	 */
	Deprecated,
	MAX_VALUE
};
using SymbolTagEnum = Enumeration<SymbolTag, uint>;
template<>
const SymbolTagEnum::ConstInitType SymbolTagEnum::s_values[];

/*
 * UniquenessLevel
 *
 * Moniker uniqueness level to define scope of the moniker.
 * 
 * @since 3.16.0
 */
enum class UniquenessLevel{
	/*
	 * The moniker is only unique inside a document
	 */
	Document,
	/*
	 * The moniker is unique inside a project for which a dump got created
	 */
	Project,
	/*
	 * The moniker is unique inside the group to which a project belongs
	 */
	Group,
	/*
	 * The moniker is unique inside the moniker scheme.
	 */
	Scheme,
	/*
	 * The moniker is globally unique
	 */
	Global,
	MAX_VALUE
};
using UniquenessLevelEnum = Enumeration<UniquenessLevel, String>;
template<>
const UniquenessLevelEnum::ConstInitType UniquenessLevelEnum::s_values[];

/*
 * MonikerKind
 *
 * The moniker kind.
 * 
 * @since 3.16.0
 */
enum class MonikerKind{
	/*
	 * The moniker represent a symbol that is imported into a project
	 */
	Import,
	/*
	 * The moniker represents a symbol that is exported from a project
	 */
	Export,
	/*
	 * The moniker represents a symbol that is local to a project (e.g. a local
	 * variable of a function, a class not visible outside the project, ...)
	 */
	Local,
	MAX_VALUE
};
using MonikerKindEnum = Enumeration<MonikerKind, String>;
template<>
const MonikerKindEnum::ConstInitType MonikerKindEnum::s_values[];

/*
 * InlayHintKind
 *
 * Inlay hint kinds.
 * 
 * @since 3.17.0
 */
enum class InlayHintKind{
	/*
	 * An inlay hint that for a type annotation.
	 */
	Type,
	/*
	 * An inlay hint that is for a parameter.
	 */
	Parameter,
	MAX_VALUE
};
using InlayHintKindEnum = Enumeration<InlayHintKind, uint>;
template<>
const InlayHintKindEnum::ConstInitType InlayHintKindEnum::s_values[];

/*
 * MessageType
 *
 * The message type
 */
enum class MessageType{
	/*
	 * An error message.
	 */
	Error,
	/*
	 * A warning message.
	 */
	Warning,
	/*
	 * An information message.
	 */
	Info,
	/*
	 * A log message.
	 */
	Log,
	/*
	 * A debug message.
	 * 
	 * @since 3.18.0
	 */
	Debug,
	MAX_VALUE
};
using MessageTypeEnum = Enumeration<MessageType, uint>;
template<>
const MessageTypeEnum::ConstInitType MessageTypeEnum::s_values[];

/*
 * TextDocumentSyncKind
 *
 * Defines how the host (editor) should sync
 * document changes to the language server.
 */
enum class TextDocumentSyncKind{
	/*
	 * Documents should not be synced at all.
	 */
	None,
	/*
	 * Documents are synced by always sending the full content
	 * of the document.
	 */
	Full,
	/*
	 * Documents are synced by sending the full content on open.
	 * After that only incremental updates to the document are
	 * send.
	 */
	Incremental,
	MAX_VALUE
};
using TextDocumentSyncKindEnum = Enumeration<TextDocumentSyncKind, uint>;
template<>
const TextDocumentSyncKindEnum::ConstInitType TextDocumentSyncKindEnum::s_values[];

/*
 * TextDocumentSaveReason
 *
 * Represents reasons why a text document is saved.
 */
enum class TextDocumentSaveReason{
	/*
	 * Manually triggered, e.g. by the user pressing save, by starting debugging,
	 * or by an API call.
	 */
	Manual,
	/*
	 * Automatic after a delay.
	 */
	AfterDelay,
	/*
	 * When the editor lost focus.
	 */
	FocusOut,
	MAX_VALUE
};
using TextDocumentSaveReasonEnum = Enumeration<TextDocumentSaveReason, uint>;
template<>
const TextDocumentSaveReasonEnum::ConstInitType TextDocumentSaveReasonEnum::s_values[];

/*
 * CompletionItemKind
 *
 * The kind of a completion entry.
 */
enum class CompletionItemKind{
	Text,
	Method,
	Function,
	Constructor,
	Field,
	Variable,
	Class,
	Interface,
	Module,
	Property,
	Unit,
	Value,
	Enum,
	Keyword,
	Snippet,
	Color,
	File,
	Reference,
	Folder,
	EnumMember,
	Constant,
	Struct,
	Event,
	Operator,
	TypeParameter,
	MAX_VALUE
};
using CompletionItemKindEnum = Enumeration<CompletionItemKind, uint>;
template<>
const CompletionItemKindEnum::ConstInitType CompletionItemKindEnum::s_values[];

/*
 * CompletionItemTag
 *
 * Completion item tags are extra annotations that tweak the rendering of a completion
 * item.
 * 
 * @since 3.15.0
 */
enum class CompletionItemTag{
	/*
	 * Render a completion as obsolete, usually using a strike-out.
	 */
	Deprecated,
	MAX_VALUE
};
using CompletionItemTagEnum = Enumeration<CompletionItemTag, uint>;
template<>
const CompletionItemTagEnum::ConstInitType CompletionItemTagEnum::s_values[];

/*
 * InsertTextFormat
 *
 * Defines whether the insert text in a completion item should be interpreted as
 * plain text or a snippet.
 */
enum class InsertTextFormat{
	/*
	 * The primary text to be inserted is treated as a plain string.
	 */
	PlainText,
	/*
	 * The primary text to be inserted is treated as a snippet.
	 * 
	 * A snippet can define tab stops and placeholders with `$1`, `$2`
	 * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
	 * the end of the snippet. Placeholders with equal identifiers are linked,
	 * that is typing in one will update others too.
	 * 
	 * See also: https://microsoft.github.io/language-server-protocol/specifications/specification-current/#snippet_syntax
	 */
	Snippet,
	MAX_VALUE
};
using InsertTextFormatEnum = Enumeration<InsertTextFormat, uint>;
template<>
const InsertTextFormatEnum::ConstInitType InsertTextFormatEnum::s_values[];

/*
 * InsertTextMode
 *
 * How whitespace and indentation is handled during completion
 * item insertion.
 * 
 * @since 3.16.0
 */
enum class InsertTextMode{
	/*
	 * The insertion or replace strings is taken as it is. If the
	 * value is multi line the lines below the cursor will be
	 * inserted using the indentation defined in the string value.
	 * The client will not apply any kind of adjustments to the
	 * string.
	 */
	AsIs,
	/*
	 * The editor adjusts leading whitespace of new lines so that
	 * they match the indentation up to the cursor of the line for
	 * which the item is accepted.
	 * 
	 * Consider a line like this: <2tabs><cursor><3tabs>foo. Accepting a
	 * multi line completion item is indented using 2 tabs and all
	 * following lines inserted will be indented using 2 tabs as well.
	 */
	AdjustIndentation,
	MAX_VALUE
};
using InsertTextModeEnum = Enumeration<InsertTextMode, uint>;
template<>
const InsertTextModeEnum::ConstInitType InsertTextModeEnum::s_values[];

/*
 * DocumentHighlightKind
 *
 * A document highlight kind.
 */
enum class DocumentHighlightKind{
	/*
	 * A textual occurrence.
	 */
	Text,
	/*
	 * Read-access of a symbol, like reading a variable.
	 */
	Read,
	/*
	 * Write-access of a symbol, like writing to a variable.
	 */
	Write,
	MAX_VALUE
};
using DocumentHighlightKindEnum = Enumeration<DocumentHighlightKind, uint>;
template<>
const DocumentHighlightKindEnum::ConstInitType DocumentHighlightKindEnum::s_values[];

/*
 * CodeActionKind
 *
 * A set of predefined code action kinds
 */
enum class CodeActionKind{
	/*
	 * Empty kind.
	 */
	Empty,
	/*
	 * Base kind for quickfix actions: 'quickfix'
	 */
	QuickFix,
	/*
	 * Base kind for refactoring actions: 'refactor'
	 */
	Refactor,
	/*
	 * Base kind for refactoring extraction actions: 'refactor.extract'
	 * 
	 * Example extract actions:
	 * 
	 * - Extract method
	 * - Extract function
	 * - Extract variable
	 * - Extract interface from class
	 * - ...
	 */
	RefactorExtract,
	/*
	 * Base kind for refactoring inline actions: 'refactor.inline'
	 * 
	 * Example inline actions:
	 * 
	 * - Inline function
	 * - Inline variable
	 * - Inline constant
	 * - ...
	 */
	RefactorInline,
	/*
	 * Base kind for refactoring rewrite actions: 'refactor.rewrite'
	 * 
	 * Example rewrite actions:
	 * 
	 * - Convert JavaScript function to class
	 * - Add or remove parameter
	 * - Encapsulate field
	 * - Make method static
	 * - Move method to base class
	 * - ...
	 */
	RefactorRewrite,
	/*
	 * Base kind for source actions: `source`
	 * 
	 * Source code actions apply to the entire file.
	 */
	Source,
	/*
	 * Base kind for an organize imports source action: `source.organizeImports`
	 */
	SourceOrganizeImports,
	/*
	 * Base kind for auto-fix source actions: `source.fixAll`.
	 * 
	 * Fix all actions automatically fix errors that have a clear fix that do not require user input.
	 * They should not suppress errors or perform unsafe fixes such as generating new types or classes.
	 * 
	 * @since 3.15.0
	 */
	SourceFixAll,
	MAX_VALUE
};
using CodeActionKindEnum = Enumeration<CodeActionKind, String>;
template<>
const CodeActionKindEnum::ConstInitType CodeActionKindEnum::s_values[];

/*
 * TraceValues
 */
enum class TraceValues{
	/*
	 * Turn tracing off.
	 */
	Off,
	/*
	 * Trace messages only.
	 */
	Messages,
	/*
	 * Verbose message tracing.
	 */
	Verbose,
	MAX_VALUE
};
using TraceValuesEnum = Enumeration<TraceValues, String>;
template<>
const TraceValuesEnum::ConstInitType TraceValuesEnum::s_values[];

/*
 * MarkupKind
 *
 * Describes the content type that a client supports in various
 * result literals like `Hover`, `ParameterInfo` or `CompletionItem`.
 * 
 * Please note that `MarkupKinds` must not start with a `$`. This kinds
 * are reserved for internal usage.
 */
enum class MarkupKind{
	/*
	 * Plain text is supported as a content format
	 */
	PlainText,
	/*
	 * Markdown is supported as a content format
	 */
	Markdown,
	MAX_VALUE
};
using MarkupKindEnum = Enumeration<MarkupKind, String>;
template<>
const MarkupKindEnum::ConstInitType MarkupKindEnum::s_values[];

/*
 * InlineCompletionTriggerKind
 *
 * Describes how an {@link InlineCompletionItemProvider inline completion provider} was triggered.
 * 
 * @since 3.18.0
 * @proposed
 */
enum class InlineCompletionTriggerKind{
	/*
	 * Completion was triggered explicitly by a user gesture.
	 */
	Invoked,
	/*
	 * Completion was triggered automatically while editing.
	 */
	Automatic,
	MAX_VALUE
};
using InlineCompletionTriggerKindEnum = Enumeration<InlineCompletionTriggerKind, uint>;
template<>
const InlineCompletionTriggerKindEnum::ConstInitType InlineCompletionTriggerKindEnum::s_values[];

/*
 * PositionEncodingKind
 *
 * A set of predefined position encoding kinds.
 * 
 * @since 3.17.0
 */
enum class PositionEncodingKind{
	/*
	 * Character offsets count UTF-8 code units (e.g. bytes).
	 */
	UTF8,
	/*
	 * Character offsets count UTF-16 code units.
	 * 
	 * This is the default and must always be supported
	 * by servers
	 */
	UTF16,
	/*
	 * Character offsets count UTF-32 code units.
	 * 
	 * Implementation note: these are the same as Unicode codepoints,
	 * so this `PositionEncodingKind` may also be used for an
	 * encoding-agnostic representation of character offsets.
	 */
	UTF32,
	MAX_VALUE
};
using PositionEncodingKindEnum = Enumeration<PositionEncodingKind, String>;
template<>
const PositionEncodingKindEnum::ConstInitType PositionEncodingKindEnum::s_values[];

/*
 * FileChangeType
 *
 * The file event type
 */
enum class FileChangeType{
	/*
	 * The file got created.
	 */
	Created,
	/*
	 * The file got changed.
	 */
	Changed,
	/*
	 * The file got deleted.
	 */
	Deleted,
	MAX_VALUE
};
using FileChangeTypeEnum = Enumeration<FileChangeType, uint>;
template<>
const FileChangeTypeEnum::ConstInitType FileChangeTypeEnum::s_values[];

/*
 * WatchKind
 */
enum class WatchKind{
	/*
	 * Interested in create events.
	 */
	Create,
	/*
	 * Interested in change events
	 */
	Change,
	/*
	 * Interested in delete events
	 */
	Delete,
	MAX_VALUE
};
using WatchKindEnum = Enumeration<WatchKind, uint>;
template<>
const WatchKindEnum::ConstInitType WatchKindEnum::s_values[];

/*
 * DiagnosticSeverity
 *
 * The diagnostic's severity.
 */
enum class DiagnosticSeverity{
	/*
	 * Reports an error.
	 */
	Error,
	/*
	 * Reports a warning.
	 */
	Warning,
	/*
	 * Reports an information.
	 */
	Information,
	/*
	 * Reports a hint.
	 */
	Hint,
	MAX_VALUE
};
using DiagnosticSeverityEnum = Enumeration<DiagnosticSeverity, uint>;
template<>
const DiagnosticSeverityEnum::ConstInitType DiagnosticSeverityEnum::s_values[];

/*
 * DiagnosticTag
 *
 * The diagnostic tags.
 * 
 * @since 3.15.0
 */
enum class DiagnosticTag{
	/*
	 * Unused or unnecessary code.
	 * 
	 * Clients are allowed to render diagnostics with this tag faded out instead of having
	 * an error squiggle.
	 */
	Unnecessary,
	/*
	 * Deprecated or obsolete code.
	 * 
	 * Clients are allowed to rendered diagnostics with this tag strike through.
	 */
	Deprecated,
	MAX_VALUE
};
using DiagnosticTagEnum = Enumeration<DiagnosticTag, uint>;
template<>
const DiagnosticTagEnum::ConstInitType DiagnosticTagEnum::s_values[];

/*
 * CompletionTriggerKind
 *
 * How a completion was triggered
 */
enum class CompletionTriggerKind{
	/*
	 * Completion was triggered by typing an identifier (24x7 code
	 * complete), manual invocation (e.g Ctrl+Space) or via API.
	 */
	Invoked,
	/*
	 * Completion was triggered by a trigger character specified by
	 * the `triggerCharacters` properties of the `CompletionRegistrationOptions`.
	 */
	TriggerCharacter,
	/*
	 * Completion was re-triggered as current completion list is incomplete
	 */
	TriggerForIncompleteCompletions,
	MAX_VALUE
};
using CompletionTriggerKindEnum = Enumeration<CompletionTriggerKind, uint>;
template<>
const CompletionTriggerKindEnum::ConstInitType CompletionTriggerKindEnum::s_values[];

/*
 * SignatureHelpTriggerKind
 *
 * How a signature help was triggered.
 * 
 * @since 3.15.0
 */
enum class SignatureHelpTriggerKind{
	/*
	 * Signature help was invoked manually by the user or by a command.
	 */
	Invoked,
	/*
	 * Signature help was triggered by a trigger character.
	 */
	TriggerCharacter,
	/*
	 * Signature help was triggered by the cursor moving or by the document content changing.
	 */
	ContentChange,
	MAX_VALUE
};
using SignatureHelpTriggerKindEnum = Enumeration<SignatureHelpTriggerKind, uint>;
template<>
const SignatureHelpTriggerKindEnum::ConstInitType SignatureHelpTriggerKindEnum::s_values[];

/*
 * CodeActionTriggerKind
 *
 * The reason why code actions were requested.
 * 
 * @since 3.17.0
 */
enum class CodeActionTriggerKind{
	/*
	 * Code actions were explicitly requested by the user or by an extension.
	 */
	Invoked,
	/*
	 * Code actions were requested automatically.
	 * 
	 * This typically happens when current selection in a file changes, but can
	 * also be triggered when file content changes.
	 */
	Automatic,
	MAX_VALUE
};
using CodeActionTriggerKindEnum = Enumeration<CodeActionTriggerKind, uint>;
template<>
const CodeActionTriggerKindEnum::ConstInitType CodeActionTriggerKindEnum::s_values[];

/*
 * FileOperationPatternKind
 *
 * A pattern kind describing if a glob pattern matches a file a folder or
 * both.
 * 
 * @since 3.16.0
 */
enum class FileOperationPatternKind{
	/*
	 * The pattern matches a file only.
	 */
	File,
	/*
	 * The pattern matches a folder only.
	 */
	Folder,
	MAX_VALUE
};
using FileOperationPatternKindEnum = Enumeration<FileOperationPatternKind, String>;
template<>
const FileOperationPatternKindEnum::ConstInitType FileOperationPatternKindEnum::s_values[];

/*
 * NotebookCellKind
 *
 * A notebook cell kind.
 * 
 * @since 3.17.0
 */
enum class NotebookCellKind{
	/*
	 * A markup-cell is formatted source that is used for display.
	 */
	Markup,
	/*
	 * A code-cell is source code.
	 */
	Code,
	MAX_VALUE
};
using NotebookCellKindEnum = Enumeration<NotebookCellKind, uint>;
template<>
const NotebookCellKindEnum::ConstInitType NotebookCellKindEnum::s_values[];

/*
 * ResourceOperationKind
 */
enum class ResourceOperationKind{
	/*
	 * Supports creating new files and folders.
	 */
	Create,
	/*
	 * Supports renaming existing files and folders.
	 */
	Rename,
	/*
	 * Supports deleting existing files and folders.
	 */
	Delete,
	MAX_VALUE
};
using ResourceOperationKindEnum = Enumeration<ResourceOperationKind, String>;
template<>
const ResourceOperationKindEnum::ConstInitType ResourceOperationKindEnum::s_values[];

/*
 * FailureHandlingKind
 */
enum class FailureHandlingKind{
	/*
	 * Applying the workspace change is simply aborted if one of the changes provided
	 * fails. All operations executed before the failing operation stay executed.
	 */
	Abort,
	/*
	 * All operations are executed transactional. That means they either all
	 * succeed or no changes at all are applied to the workspace.
	 */
	Transactional,
	/*
	 * If the workspace edit contains only textual file changes they are executed transactional.
	 * If resource changes (create, rename or delete file) are part of the change the failure
	 * handling strategy is abort.
	 */
	TextOnlyTransactional,
	/*
	 * The client tries to undo the operations already executed. But there is no
	 * guarantee that this is succeeding.
	 */
	Undo,
	MAX_VALUE
};
using FailureHandlingKindEnum = Enumeration<FailureHandlingKind, String>;
template<>
const FailureHandlingKindEnum::ConstInitType FailureHandlingKindEnum::s_values[];

/*
 * PrepareSupportDefaultBehavior
 */
enum class PrepareSupportDefaultBehavior{
	/*
	 * The client's default behavior is to select the identifier
	 * according the to language's syntax rule.
	 */
	Identifier,
	MAX_VALUE
};
using PrepareSupportDefaultBehaviorEnum = Enumeration<PrepareSupportDefaultBehavior, uint>;
template<>
const PrepareSupportDefaultBehaviorEnum::ConstInitType PrepareSupportDefaultBehaviorEnum::s_values[];

/*
 * TokenFormat
 */
enum class TokenFormat{
	Relative,
	MAX_VALUE
};
using TokenFormatEnum = Enumeration<TokenFormat, String>;
template<>
const TokenFormatEnum::ConstInitType TokenFormatEnum::s_values[];

/*
 * TextDocumentIdentifier
 *
 * A literal to identify a text document in the client.
 */
struct TextDocumentIdentifier{
	/*
	 * The text document's uri.
	 */
	DocumentUri uri;
};

/*
 * Position
 *
 * Position in a text document expressed as zero-based line and character
 * offset. Prior to 3.17 the offsets were always based on a UTF-16 string
 * representation. So a string of the form `a𐐀b` the character offset of the
 * character `a` is 0, the character offset of `𐐀` is 1 and the character
 * offset of b is 3 since `𐐀` is represented using two code units in UTF-16.
 * Since 3.17 clients and servers can agree on a different string encoding
 * representation (e.g. UTF-8). The client announces it's supported encoding
 * via the client capability [`general.positionEncodings`](https://microsoft.github.io/language-server-protocol/specifications/specification-current/#clientCapabilities).
 * The value is an array of position encodings the client supports, with
 * decreasing preference (e.g. the encoding at index `0` is the most preferred
 * one). To stay backwards compatible the only mandatory encoding is UTF-16
 * represented via the string `utf-16`. The server can pick one of the
 * encodings offered by the client and signals that encoding back to the
 * client via the initialize result's property
 * [`capabilities.positionEncoding`](https://microsoft.github.io/language-server-protocol/specifications/specification-current/#serverCapabilities). If the string value
 * `utf-16` is missing from the client's capability `general.positionEncodings`
 * servers can safely assume that the client supports UTF-16. If the server
 * omits the position encoding in its initialize result the encoding defaults
 * to the string value `utf-16`. Implementation considerations: since the
 * conversion from one encoding into another requires the content of the
 * file / line the conversion is best done where the file is read which is
 * usually on the server side.
 * 
 * Positions are line end character agnostic. So you can not specify a position
 * that denotes `\r|\n` or `\n|` where `|` represents the character offset.
 * 
 * @since 3.17.0 - support for negotiated position encoding.
 */
struct Position{
	/*
	 * Line position in a document (zero-based).
	 * 
	 * If a line number is greater than the number of lines in a document, it defaults back to the number of lines in the document.
	 * If a line number is negative, it defaults to 0.
	 */
	uint line;
	/*
	 * Character offset on a line in a document (zero-based).
	 * 
	 * The meaning of this offset is determined by the negotiated
	 * `PositionEncodingKind`.
	 * 
	 * If the character value is greater than the line length it defaults back to the
	 * line length.
	 */
	uint character;
};

/*
 * TextDocumentPositionParams
 *
 * A parameter literal used in requests to pass a text document and a position inside that
 * document.
 */
struct TextDocumentPositionParams{
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The position inside the text document.
	 */
	Position position;
};

/*
 * ProgressToken
 */
using ProgressToken = OneOf<int, String>;

/*
 * WorkDoneProgressParams
 */
struct WorkDoneProgressParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
};

/*
 * PartialResultParams
 */
struct PartialResultParams{
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
};

/*
 * ImplementationParams
 */
struct ImplementationParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
};

/*
 * Range
 *
 * A range in a text document expressed as (zero-based) start and end positions.
 * 
 * If you want to specify a range that contains a line including the line ending
 * character(s) then use an end position denoting the start of the next line.
 * For example:
 * ```ts
 * {
 *     start: { line: 5, character: 23 }
 *     end : { line 6, character : 0 }
 * }
 * ```
 */
struct Range{
	/*
	 * The range's start position.
	 */
	Position start;
	/*
	 * The range's end position.
	 */
	Position end;
};

/*
 * Location
 *
 * Represents a location inside a resource, such as a line
 * inside a text file.
 */
struct Location{
	DocumentUri uri;
	Range range;
};

/*
 * TextDocumentFilter_Language
 */
struct TextDocumentFilter_Language{
	/*
	 * A language id, like `typescript`.
	 */
	String language;
	/*
	 * A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
	 */
	Opt<String> scheme = {};
	/*
	 * A glob pattern, like **​/_*.{ts,js}. See TextDocumentFilter for examples.
	 */
	Opt<String> pattern = {};
};

/*
 * TextDocumentFilter_Scheme
 */
struct TextDocumentFilter_Scheme{
	/*
	 * A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
	 */
	String scheme;
	/*
	 * A language id, like `typescript`.
	 */
	Opt<String> language = {};
	/*
	 * A glob pattern, like **​/_*.{ts,js}. See TextDocumentFilter for examples.
	 */
	Opt<String> pattern = {};
};

/*
 * TextDocumentFilter_Pattern
 */
struct TextDocumentFilter_Pattern{
	/*
	 * A glob pattern, like **​/_*.{ts,js}. See TextDocumentFilter for examples.
	 */
	String pattern;
	/*
	 * A language id, like `typescript`.
	 */
	Opt<String> language = {};
	/*
	 * A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
	 */
	Opt<String> scheme = {};
};

/*
 * TextDocumentFilter
 *
 * A document filter denotes a document by different properties like
 * the {@link TextDocument.languageId language}, the {@link Uri.scheme scheme} of
 * its resource, or a glob-pattern that is applied to the {@link TextDocument.fileName path}.
 * 
 * Glob patterns can have the following syntax:
 * - `*` to match zero or more characters in a path segment
 * - `?` to match on one character in a path segment
 * - `**` to match any number of path segments, including none
 * - `{}` to group sub patterns into an OR expression. (e.g. `**​/_*.{ts,js}` matches all TypeScript and JavaScript files)
 * - `[]` to declare a range of characters to match in a path segment (e.g., `example.[0-9]` to match on `example.0`, `example.1`, …)
 * - `[!...]` to negate a range of characters to match in a path segment (e.g., `example.[!0-9]` to match on `example.a`, `example.b`, but not `example.0`)
 * 
 * @sample A language filter that applies to typescript files on disk: `{ language: 'typescript', scheme: 'file' }`
 * @sample A language filter that applies to all package.json paths: `{ language: 'json', pattern: '**package.json' }`
 * 
 * @since 3.17.0
 */
using TextDocumentFilter = OneOf<TextDocumentFilter_Language, TextDocumentFilter_Scheme, TextDocumentFilter_Pattern>;

/*
 * NotebookDocumentFilter_NotebookType
 */
struct NotebookDocumentFilter_NotebookType{
	/*
	 * The type of the enclosing notebook.
	 */
	String notebookType;
	/*
	 * A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
	 */
	Opt<String> scheme = {};
	/*
	 * A glob pattern.
	 */
	Opt<String> pattern = {};
};

/*
 * NotebookDocumentFilter_Scheme
 */
struct NotebookDocumentFilter_Scheme{
	/*
	 * A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
	 */
	String scheme;
	/*
	 * The type of the enclosing notebook.
	 */
	Opt<String> notebookType = {};
	/*
	 * A glob pattern.
	 */
	Opt<String> pattern = {};
};

/*
 * NotebookDocumentFilter_Pattern
 */
struct NotebookDocumentFilter_Pattern{
	/*
	 * A glob pattern.
	 */
	String pattern;
	/*
	 * The type of the enclosing notebook.
	 */
	Opt<String> notebookType = {};
	/*
	 * A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
	 */
	Opt<String> scheme = {};
};

/*
 * NotebookDocumentFilter
 *
 * A notebook document filter denotes a notebook document by
 * different properties. The properties will be match
 * against the notebook's URI (same as with documents)
 * 
 * @since 3.17.0
 */
using NotebookDocumentFilter = OneOf<NotebookDocumentFilter_NotebookType, NotebookDocumentFilter_Scheme, NotebookDocumentFilter_Pattern>;

/*
 * NotebookCellTextDocumentFilter
 *
 * A notebook cell text document filter denotes a cell text
 * document by different properties.
 * 
 * @since 3.17.0
 */
struct NotebookCellTextDocumentFilter{
	/*
	 * A filter that matches against the notebook
	 * containing the notebook cell. If a string
	 * value is provided it matches against the
	 * notebook type. '*' matches every notebook.
	 */
	OneOf<String, NotebookDocumentFilter> notebook;
	/*
	 * A language id like `python`.
	 * 
	 * Will be matched against the language id of the
	 * notebook cell document. '*' matches every language.
	 */
	Opt<String> language = {};
};

/*
 * DocumentFilter
 *
 * A document filter describes a top level text document or
 * a notebook cell document.
 * 
 * @since 3.17.0 - proposed support for NotebookCellTextDocumentFilter.
 */
using DocumentFilter = OneOf<TextDocumentFilter, NotebookCellTextDocumentFilter>;

/*
 * DocumentSelector
 *
 * A document selector is the combination of one or many document filters.
 * 
 * @sample `let sel:DocumentSelector = [{ language: 'typescript' }, { language: 'json', pattern: '**∕tsconfig.json' }]`;
 * 
 * The use of a string as a document filter is deprecated @since 3.16.0.
 */
using DocumentSelector = Array<DocumentFilter>;

/*
 * TextDocumentRegistrationOptions
 *
 * General text document registration options.
 */
struct TextDocumentRegistrationOptions{
	/*
	 * A document selector to identify the scope of the registration. If set to null
	 * the document selector provided on the client side will be used.
	 */
	NullOr<DocumentSelector> documentSelector;
};

/*
 * WorkDoneProgressOptions
 */
struct WorkDoneProgressOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * ImplementationOptions
 */
struct ImplementationOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * StaticRegistrationOptions
 *
 * Static registration options to be returned in the initialize
 * request.
 */
struct StaticRegistrationOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * ImplementationRegistrationOptions
 */
struct ImplementationRegistrationOptions : TextDocumentRegistrationOptions, ImplementationOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * TypeDefinitionParams
 */
struct TypeDefinitionParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
};

/*
 * TypeDefinitionOptions
 */
struct TypeDefinitionOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * TypeDefinitionRegistrationOptions
 */
struct TypeDefinitionRegistrationOptions : TextDocumentRegistrationOptions, TypeDefinitionOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * WorkspaceFolder
 *
 * A workspace folder inside a client.
 */
struct WorkspaceFolder{
	/*
	 * The associated URI for this workspace folder.
	 */
	Uri uri;
	/*
	 * The name of the workspace folder. Used to refer to this
	 * workspace folder in the user interface.
	 */
	String name;
};

/*
 * WorkspaceFoldersChangeEvent
 *
 * The workspace folder change event.
 */
struct WorkspaceFoldersChangeEvent{
	/*
	 * The array of added workspace folders
	 */
	Array<WorkspaceFolder> added;
	/*
	 * The array of the removed workspace folders
	 */
	Array<WorkspaceFolder> removed;
};

/*
 * DidChangeWorkspaceFoldersParams
 *
 * The parameters of a `workspace/didChangeWorkspaceFolders` notification.
 */
struct DidChangeWorkspaceFoldersParams{
	/*
	 * The actual workspace folder change event.
	 */
	WorkspaceFoldersChangeEvent event;
};

/*
 * ConfigurationItem
 */
struct ConfigurationItem{
	/*
	 * The scope to get the configuration section for.
	 */
	Opt<Uri> scopeUri = {};
	/*
	 * The configuration section asked for.
	 */
	Opt<String> section = {};
};

/*
 * ConfigurationParams
 *
 * The parameters of a configuration request.
 */
struct ConfigurationParams{
	Array<ConfigurationItem> items;
};

/*
 * DocumentColorParams
 *
 * Parameters for a {@link DocumentColorRequest}.
 */
struct DocumentColorParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
};

/*
 * Color
 *
 * Represents a color in RGBA space.
 */
struct Color{
	/*
	 * The red component of this color in the range [0-1].
	 */
	double red;
	/*
	 * The green component of this color in the range [0-1].
	 */
	double green;
	/*
	 * The blue component of this color in the range [0-1].
	 */
	double blue;
	/*
	 * The alpha component of this color in the range [0-1].
	 */
	double alpha;
};

/*
 * ColorInformation
 *
 * Represents a color range from a document.
 */
struct ColorInformation{
	/*
	 * The range in the document where this color appears.
	 */
	Range range;
	/*
	 * The actual color value for this color range.
	 */
	Color color;
};

/*
 * DocumentColorOptions
 */
struct DocumentColorOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * DocumentColorRegistrationOptions
 */
struct DocumentColorRegistrationOptions : TextDocumentRegistrationOptions, DocumentColorOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * ColorPresentationParams
 *
 * Parameters for a {@link ColorPresentationRequest}.
 */
struct ColorPresentationParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The color to request presentations for.
	 */
	Color color;
	/*
	 * The range where the color would be inserted. Serves as a context.
	 */
	Range range;
};

/*
 * TextEdit
 *
 * A text edit applicable to a text document.
 */
struct TextEdit{
	/*
	 * The range of the text document to be manipulated. To insert
	 * text into a document create a range where start === end.
	 */
	Range range;
	/*
	 * The string to be inserted. For delete operations use an
	 * empty string.
	 */
	String newText;
};

/*
 * ColorPresentation
 */
struct ColorPresentation{
	/*
	 * The label of this color presentation. It will be shown on the color
	 * picker header. By default this is also the text that is inserted when selecting
	 * this color presentation.
	 */
	String label;
	/*
	 * An {@link TextEdit edit} which is applied to a document when selecting
	 * this presentation for the color.  When `falsy` the {@link ColorPresentation.label label}
	 * is used.
	 */
	Opt<TextEdit> textEdit = {};
	/*
	 * An optional array of additional {@link TextEdit text edits} that are applied when
	 * selecting this color presentation. Edits must not overlap with the main {@link ColorPresentation.textEdit edit} nor with themselves.
	 */
	Opt<Array<TextEdit>> additionalTextEdits = {};
};

/*
 * FoldingRangeParams
 *
 * Parameters for a {@link FoldingRangeRequest}.
 */
struct FoldingRangeParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
};

/*
 * FoldingRange
 *
 * Represents a folding range. To be valid, start and end line must be bigger than zero and smaller
 * than the number of lines in the document. Clients are free to ignore invalid ranges.
 */
struct FoldingRange{
	/*
	 * The zero-based start line of the range to fold. The folded area starts after the line's last character.
	 * To be valid, the end must be zero or larger and smaller than the number of lines in the document.
	 */
	uint startLine;
	/*
	 * The zero-based end line of the range to fold. The folded area ends with the line's last character.
	 * To be valid, the end must be zero or larger and smaller than the number of lines in the document.
	 */
	uint endLine;
	/*
	 * The zero-based character offset from where the folded range starts. If not defined, defaults to the length of the start line.
	 */
	Opt<uint> startCharacter = {};
	/*
	 * The zero-based character offset before the folded range ends. If not defined, defaults to the length of the end line.
	 */
	Opt<uint> endCharacter = {};
	/*
	 * Describes the kind of the folding range such as `comment' or 'region'. The kind
	 * is used to categorize folding ranges and used by commands like 'Fold all comments'.
	 * See {@link FoldingRangeKind} for an enumeration of standardized kinds.
	 */
	Opt<FoldingRangeKindEnum> kind = {};
	/*
	 * The text that the client should show when the specified range is
	 * collapsed. If not defined or not supported by the client, a default
	 * will be chosen by the client.
	 * 
	 * @since 3.17.0
	 */
	Opt<String> collapsedText = {};
};

/*
 * FoldingRangeOptions
 */
struct FoldingRangeOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * FoldingRangeRegistrationOptions
 */
struct FoldingRangeRegistrationOptions : TextDocumentRegistrationOptions, FoldingRangeOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * DeclarationParams
 */
struct DeclarationParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
};

/*
 * DeclarationOptions
 */
struct DeclarationOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * DeclarationRegistrationOptions
 */
struct DeclarationRegistrationOptions : DeclarationOptions, TextDocumentRegistrationOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * SelectionRangeParams
 *
 * A parameter literal used in selection range requests.
 */
struct SelectionRangeParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The positions inside the text document.
	 */
	Array<Position> positions;
};

/*
 * SelectionRange
 *
 * A selection range represents a part of a selection hierarchy. A selection range
 * may have a parent selection range that contains it.
 */
struct SelectionRange{
	/*
	 * The {@link Range range} of this selection range.
	 */
	Range range;
	/*
	 * The parent selection range containing this range. Therefore `parent.range` must contain `this.range`.
	 */
	std::unique_ptr<SelectionRange> parent = {};
};

/*
 * SelectionRangeOptions
 */
struct SelectionRangeOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * SelectionRangeRegistrationOptions
 */
struct SelectionRangeRegistrationOptions : SelectionRangeOptions, TextDocumentRegistrationOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * WorkDoneProgressCreateParams
 */
struct WorkDoneProgressCreateParams{
	/*
	 * The token to be used to report progress.
	 */
	ProgressToken token;
};

/*
 * WorkDoneProgressCancelParams
 */
struct WorkDoneProgressCancelParams{
	/*
	 * The token to be used to report progress.
	 */
	ProgressToken token;
};

/*
 * CallHierarchyPrepareParams
 *
 * The parameter of a `textDocument/prepareCallHierarchy` request.
 * 
 * @since 3.16.0
 */
struct CallHierarchyPrepareParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
};

/*
 * CallHierarchyItem
 *
 * Represents programming constructs like functions or constructors in the context
 * of call hierarchy.
 * 
 * @since 3.16.0
 */
struct CallHierarchyItem{
	/*
	 * The name of this item.
	 */
	String name;
	/*
	 * The kind of this item.
	 */
	SymbolKindEnum kind;
	/*
	 * The resource identifier of this item.
	 */
	DocumentUri uri;
	/*
	 * The range enclosing this symbol not including leading/trailing whitespace but everything else, e.g. comments and code.
	 */
	Range range;
	/*
	 * The range that should be selected and revealed when this symbol is being picked, e.g. the name of a function.
	 * Must be contained by the {@link CallHierarchyItem.range `range`}.
	 */
	Range selectionRange;
	/*
	 * Tags for this item.
	 */
	Opt<Array<SymbolTagEnum>> tags = {};
	/*
	 * More detail for this item, e.g. the signature of a function.
	 */
	Opt<String> detail = {};
	/*
	 * A data entry field that is preserved between a call hierarchy prepare and
	 * incoming calls or outgoing calls requests.
	 */
	Opt<LSPAny> data = {};
};

/*
 * CallHierarchyOptions
 *
 * Call hierarchy options used during static registration.
 * 
 * @since 3.16.0
 */
struct CallHierarchyOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * CallHierarchyRegistrationOptions
 *
 * Call hierarchy options used during static or dynamic registration.
 * 
 * @since 3.16.0
 */
struct CallHierarchyRegistrationOptions : TextDocumentRegistrationOptions, CallHierarchyOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * CallHierarchyIncomingCallsParams
 *
 * The parameter of a `callHierarchy/incomingCalls` request.
 * 
 * @since 3.16.0
 */
struct CallHierarchyIncomingCallsParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	CallHierarchyItem item;
};

/*
 * CallHierarchyIncomingCall
 *
 * Represents an incoming call, e.g. a caller of a method or constructor.
 * 
 * @since 3.16.0
 */
struct CallHierarchyIncomingCall{
	/*
	 * The item that makes the call.
	 */
	CallHierarchyItem from;
	/*
	 * The ranges at which the calls appear. This is relative to the caller
	 * denoted by {@link CallHierarchyIncomingCall.from `this.from`}.
	 */
	Array<Range> fromRanges;
};

/*
 * CallHierarchyOutgoingCallsParams
 *
 * The parameter of a `callHierarchy/outgoingCalls` request.
 * 
 * @since 3.16.0
 */
struct CallHierarchyOutgoingCallsParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	CallHierarchyItem item;
};

/*
 * CallHierarchyOutgoingCall
 *
 * Represents an outgoing call, e.g. calling a getter from a method or a method from a constructor etc.
 * 
 * @since 3.16.0
 */
struct CallHierarchyOutgoingCall{
	/*
	 * The item that is called.
	 */
	CallHierarchyItem to;
	/*
	 * The range at which this item is called. This is the range relative to the caller, e.g the item
	 * passed to {@link CallHierarchyItemProvider.provideCallHierarchyOutgoingCalls `provideCallHierarchyOutgoingCalls`}
	 * and not {@link CallHierarchyOutgoingCall.to `this.to`}.
	 */
	Array<Range> fromRanges;
};

/*
 * SemanticTokensParams
 *
 * @since 3.16.0
 */
struct SemanticTokensParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
};

/*
 * SemanticTokens
 *
 * @since 3.16.0
 */
struct SemanticTokens{
	/*
	 * The actual tokens.
	 */
	Array<uint> data;
	/*
	 * An optional result id. If provided and clients support delta updating
	 * the client will include the result id in the next semantic token request.
	 * A server can then instead of computing all semantic tokens again simply
	 * send a delta.
	 */
	Opt<String> resultId = {};
};

/*
 * SemanticTokensPartialResult
 *
 * @since 3.16.0
 */
struct SemanticTokensPartialResult{
	Array<uint> data;
};

/*
 * SemanticTokensLegend
 *
 * @since 3.16.0
 */
struct SemanticTokensLegend{
	/*
	 * The token types a server uses.
	 */
	Array<String> tokenTypes;
	/*
	 * The token modifiers a server uses.
	 */
	Array<String> tokenModifiers;
};

/*
 * SemanticTokensOptionsRange
 */
struct SemanticTokensOptionsRange{
};

/*
 * SemanticTokensOptionsFull
 */
struct SemanticTokensOptionsFull{
	/*
	 * The server supports deltas for full documents.
	 */
	Opt<bool> delta = {};
};

/*
 * SemanticTokensOptions
 *
 * @since 3.16.0
 */
struct SemanticTokensOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * The legend used by the server
	 */
	SemanticTokensLegend legend;
	/*
	 * Server supports providing semantic tokens for a specific range
	 * of a document.
	 */
	Opt<OneOf<bool, SemanticTokensOptionsRange>> range = {};
	/*
	 * Server supports providing semantic tokens for a full document.
	 */
	Opt<OneOf<bool, SemanticTokensOptionsFull>> full = {};
};

/*
 * SemanticTokensRegistrationOptions
 *
 * @since 3.16.0
 */
struct SemanticTokensRegistrationOptions : TextDocumentRegistrationOptions, SemanticTokensOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * SemanticTokensDeltaParams
 *
 * @since 3.16.0
 */
struct SemanticTokensDeltaParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The result id of a previous response. The result Id can either point to a full response
	 * or a delta response depending on what was received last.
	 */
	String previousResultId;
};

/*
 * SemanticTokensEdit
 *
 * @since 3.16.0
 */
struct SemanticTokensEdit{
	/*
	 * The start offset of the edit.
	 */
	uint start;
	/*
	 * The count of elements to remove.
	 */
	uint deleteCount;
	/*
	 * The elements to insert.
	 */
	Opt<Array<uint>> data = {};
};

/*
 * SemanticTokensDelta
 *
 * @since 3.16.0
 */
struct SemanticTokensDelta{
	/*
	 * The semantic token edits to transform a previous result into a new result.
	 */
	Array<SemanticTokensEdit> edits;
	Opt<String> resultId = {};
};

/*
 * SemanticTokensDeltaPartialResult
 *
 * @since 3.16.0
 */
struct SemanticTokensDeltaPartialResult{
	Array<SemanticTokensEdit> edits;
};

/*
 * SemanticTokensRangeParams
 *
 * @since 3.16.0
 */
struct SemanticTokensRangeParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The range the semantic tokens are requested for.
	 */
	Range range;
};

/*
 * ShowDocumentParams
 *
 * Params to show a resource in the UI.
 * 
 * @since 3.16.0
 */
struct ShowDocumentParams{
	/*
	 * The uri to show.
	 */
	Uri uri;
	/*
	 * Indicates to show the resource in an external program.
	 * To show, for example, `https://code.visualstudio.com/`
	 * in the default WEB browser set `external` to `true`.
	 */
	Opt<bool> external = {};
	/*
	 * An optional property to indicate whether the editor
	 * showing the document should take focus or not.
	 * Clients might ignore this property if an external
	 * program is started.
	 */
	Opt<bool> takeFocus = {};
	/*
	 * An optional selection range if the document is a text
	 * document. Clients might ignore the property if an
	 * external program is started or the file is not a text
	 * file.
	 */
	Opt<Range> selection = {};
};

/*
 * ShowDocumentResult
 *
 * The result of a showDocument request.
 * 
 * @since 3.16.0
 */
struct ShowDocumentResult{
	/*
	 * A boolean indicating if the show was successful.
	 */
	bool success;
};

/*
 * LinkedEditingRangeParams
 */
struct LinkedEditingRangeParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
};

/*
 * LinkedEditingRanges
 *
 * The result of a linked editing range request.
 * 
 * @since 3.16.0
 */
struct LinkedEditingRanges{
	/*
	 * A list of ranges that can be edited together. The ranges must have
	 * identical length and contain identical text content. The ranges cannot overlap.
	 */
	Array<Range> ranges;
	/*
	 * An optional word pattern (regular expression) that describes valid contents for
	 * the given ranges. If no pattern is provided, the client configuration's word
	 * pattern will be used.
	 */
	Opt<String> wordPattern = {};
};

/*
 * LinkedEditingRangeOptions
 */
struct LinkedEditingRangeOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * LinkedEditingRangeRegistrationOptions
 */
struct LinkedEditingRangeRegistrationOptions : TextDocumentRegistrationOptions, LinkedEditingRangeOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * FileCreate
 *
 * Represents information on a file/folder create.
 * 
 * @since 3.16.0
 */
struct FileCreate{
	/*
	 * A file:// URI for the location of the file/folder being created.
	 */
	String uri;
};

/*
 * CreateFilesParams
 *
 * The parameters sent in notifications/requests for user-initiated creation of
 * files.
 * 
 * @since 3.16.0
 */
struct CreateFilesParams{
	/*
	 * An array of all files/folders created in this operation.
	 */
	Array<FileCreate> files;
};

/*
 * OptionalVersionedTextDocumentIdentifier
 *
 * A text document identifier to optionally denote a specific version of a text document.
 */
struct OptionalVersionedTextDocumentIdentifier : TextDocumentIdentifier{
	/*
	 * The version number of this document. If a versioned text document identifier
	 * is sent from the server to the client and the file is not open in the editor
	 * (the server has not received an open notification before) the server can send
	 * `null` to indicate that the version is unknown and the content on disk is the
	 * truth (as specified with document content ownership).
	 */
	NullOr<int> version;
};

/*
 * ChangeAnnotationIdentifier
 *
 * An identifier to refer to a change annotation stored with a workspace edit.
 */
using ChangeAnnotationIdentifier = String;

/*
 * AnnotatedTextEdit
 *
 * A special text edit with an additional change annotation.
 * 
 * @since 3.16.0.
 */
struct AnnotatedTextEdit : TextEdit{
	/*
	 * The actual identifier of the change annotation
	 */
	ChangeAnnotationIdentifier annotationId;
};

/*
 * TextDocumentEdit
 *
 * Describes textual changes on a text document. A TextDocumentEdit describes all changes
 * on a document version Si and after they are applied move the document to version Si+1.
 * So the creator of a TextDocumentEdit doesn't need to sort the array of edits or do any
 * kind of ordering. However the edits must be non overlapping.
 */
struct TextDocumentEdit{
	/*
	 * The text document to change.
	 */
	OptionalVersionedTextDocumentIdentifier textDocument;
	/*
	 * The edits to be applied.
	 * 
	 * @since 3.16.0 - support for AnnotatedTextEdit. This is guarded using a
	 * client capability.
	 */
	Array<OneOf<TextEdit, AnnotatedTextEdit>> edits;
};

/*
 * ResourceOperation
 *
 * A generic resource operation.
 */
struct ResourceOperation{
	/*
	 * The resource operation kind.
	 */
	String kind;
	/*
	 * An optional annotation identifier describing the operation.
	 * 
	 * @since 3.16.0
	 */
	Opt<ChangeAnnotationIdentifier> annotationId = {};
};

/*
 * CreateFileOptions
 *
 * Options to create a file.
 */
struct CreateFileOptions{
	/*
	 * Overwrite existing file. Overwrite wins over `ignoreIfExists`
	 */
	Opt<bool> overwrite = {};
	/*
	 * Ignore if exists.
	 */
	Opt<bool> ignoreIfExists = {};
};

/*
 * CreateFile
 *
 * Create file operation.
 */
struct CreateFile : ResourceOperation{
	/*
	 * The resource to create.
	 */
	DocumentUri uri;
	/*
	 * Additional options
	 */
	Opt<CreateFileOptions> options = {};

	CreateFile()
	{
		kind = "create";
	}
};

/*
 * RenameFileOptions
 *
 * Rename file options
 */
struct RenameFileOptions{
	/*
	 * Overwrite target if existing. Overwrite wins over `ignoreIfExists`
	 */
	Opt<bool> overwrite = {};
	/*
	 * Ignores if target exists.
	 */
	Opt<bool> ignoreIfExists = {};
};

/*
 * RenameFile
 *
 * Rename file operation
 */
struct RenameFile : ResourceOperation{
	/*
	 * The old (existing) location.
	 */
	DocumentUri oldUri;
	/*
	 * The new location.
	 */
	DocumentUri newUri;
	/*
	 * Rename options.
	 */
	Opt<RenameFileOptions> options = {};

	RenameFile()
	{
		kind = "rename";
	}
};

/*
 * DeleteFileOptions
 *
 * Delete file options
 */
struct DeleteFileOptions{
	/*
	 * Delete the content recursively if a folder is denoted.
	 */
	Opt<bool> recursive = {};
	/*
	 * Ignore the operation if the file doesn't exist.
	 */
	Opt<bool> ignoreIfNotExists = {};
};

/*
 * DeleteFile
 *
 * Delete file operation
 */
struct DeleteFile : ResourceOperation{
	/*
	 * The file to delete.
	 */
	DocumentUri uri;
	/*
	 * Delete options.
	 */
	Opt<DeleteFileOptions> options = {};

	DeleteFile()
	{
		kind = "delete";
	}
};

/*
 * ChangeAnnotation
 *
 * Additional information that describes document changes.
 * 
 * @since 3.16.0
 */
struct ChangeAnnotation{
	/*
	 * A human-readable string describing the actual change. The string
	 * is rendered prominent in the user interface.
	 */
	String label;
	/*
	 * A flag which indicates that user confirmation is needed
	 * before applying the change.
	 */
	Opt<bool> needsConfirmation = {};
	/*
	 * A human-readable string which is rendered less prominent in
	 * the user interface.
	 */
	Opt<String> description = {};
};

/*
 * WorkspaceEdit
 *
 * A workspace edit represents changes to many resources managed in the workspace. The edit
 * should either provide `changes` or `documentChanges`. If documentChanges are present
 * they are preferred over `changes` if the client can handle versioned document edits.
 * 
 * Since version 3.13.0 a workspace edit can contain resource operations as well. If resource
 * operations are present clients need to execute the operations in the order in which they
 * are provided. So a workspace edit for example can consist of the following two changes:
 * (1) a create file a.txt and (2) a text document edit which insert text into file a.txt.
 * 
 * An invalid sequence (e.g. (1) delete file a.txt and (2) insert text into file a.txt) will
 * cause failure of the operation. How the client recovers from the failure is described by
 * the client capability: `workspace.workspaceEdit.failureHandling`
 */
struct WorkspaceEdit{
	/*
	 * Holds changes to existing resources.
	 */
	Opt<Map<DocumentUri, Array<TextEdit>>> changes = {};
	/*
	 * Depending on the client capability `workspace.workspaceEdit.resourceOperations` document changes
	 * are either an array of `TextDocumentEdit`s to express changes to n different text documents
	 * where each text document edit addresses a specific version of a text document. Or it can contain
	 * above `TextDocumentEdit`s mixed with create, rename and delete file / folder operations.
	 * 
	 * Whether a client supports versioned document edits is expressed via
	 * `workspace.workspaceEdit.documentChanges` client capability.
	 * 
	 * If a client neither supports `documentChanges` nor `workspace.workspaceEdit.resourceOperations` then
	 * only plain `TextEdit`s using the `changes` property are supported.
	 */
	Opt<Array<OneOf<TextDocumentEdit, CreateFile, RenameFile, DeleteFile>>> documentChanges = {};
	/*
	 * A map of change annotations that can be referenced in `AnnotatedTextEdit`s or create, rename and
	 * delete file / folder operations.
	 * 
	 * Whether clients honor this property depends on the client capability `workspace.changeAnnotationSupport`.
	 * 
	 * @since 3.16.0
	 */
	Opt<Map<ChangeAnnotationIdentifier, ChangeAnnotation>> changeAnnotations = {};
};

/*
 * FileOperationPatternOptions
 *
 * Matching options for the file operation pattern.
 * 
 * @since 3.16.0
 */
struct FileOperationPatternOptions{
	/*
	 * The pattern should be matched ignoring casing.
	 */
	Opt<bool> ignoreCase = {};
};

/*
 * FileOperationPattern
 *
 * A pattern to describe in which file operation requests or notifications
 * the server is interested in receiving.
 * 
 * @since 3.16.0
 */
struct FileOperationPattern{
	/*
	 * The glob pattern to match. Glob patterns can have the following syntax:
	 * - `*` to match zero or more characters in a path segment
	 * - `?` to match on one character in a path segment
	 * - `**` to match any number of path segments, including none
	 * - `{}` to group sub patterns into an OR expression. (e.g. `**​/_*.{ts,js}` matches all TypeScript and JavaScript files)
	 * - `[]` to declare a range of characters to match in a path segment (e.g., `example.[0-9]` to match on `example.0`, `example.1`, …)
	 * - `[!...]` to negate a range of characters to match in a path segment (e.g., `example.[!0-9]` to match on `example.a`, `example.b`, but not `example.0`)
	 */
	String glob;
	/*
	 * Whether to match files or folders with this pattern.
	 * 
	 * Matches both if undefined.
	 */
	Opt<FileOperationPatternKindEnum> matches = {};
	/*
	 * Additional options used during matching.
	 */
	Opt<FileOperationPatternOptions> options = {};
};

/*
 * FileOperationFilter
 *
 * A filter to describe in which file operation requests or notifications
 * the server is interested in receiving.
 * 
 * @since 3.16.0
 */
struct FileOperationFilter{
	/*
	 * The actual file operation pattern.
	 */
	FileOperationPattern pattern;
	/*
	 * A Uri scheme like `file` or `untitled`.
	 */
	Opt<String> scheme = {};
};

/*
 * FileOperationRegistrationOptions
 *
 * The options to register for file operations.
 * 
 * @since 3.16.0
 */
struct FileOperationRegistrationOptions{
	/*
	 * The actual filters.
	 */
	Array<FileOperationFilter> filters;
};

/*
 * FileRename
 *
 * Represents information on a file/folder rename.
 * 
 * @since 3.16.0
 */
struct FileRename{
	/*
	 * A file:// URI for the original location of the file/folder being renamed.
	 */
	String oldUri;
	/*
	 * A file:// URI for the new location of the file/folder being renamed.
	 */
	String newUri;
};

/*
 * RenameFilesParams
 *
 * The parameters sent in notifications/requests for user-initiated renames of
 * files.
 * 
 * @since 3.16.0
 */
struct RenameFilesParams{
	/*
	 * An array of all files/folders renamed in this operation. When a folder is renamed, only
	 * the folder will be included, and not its children.
	 */
	Array<FileRename> files;
};

/*
 * FileDelete
 *
 * Represents information on a file/folder delete.
 * 
 * @since 3.16.0
 */
struct FileDelete{
	/*
	 * A file:// URI for the location of the file/folder being deleted.
	 */
	String uri;
};

/*
 * DeleteFilesParams
 *
 * The parameters sent in notifications/requests for user-initiated deletes of
 * files.
 * 
 * @since 3.16.0
 */
struct DeleteFilesParams{
	/*
	 * An array of all files/folders deleted in this operation.
	 */
	Array<FileDelete> files;
};

/*
 * MonikerParams
 */
struct MonikerParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
};

/*
 * Moniker
 *
 * Moniker definition to match LSIF 0.5 moniker definition.
 * 
 * @since 3.16.0
 */
struct Moniker{
	/*
	 * The scheme of the moniker. For example tsc or .Net
	 */
	String scheme;
	/*
	 * The identifier of the moniker. The value is opaque in LSIF however
	 * schema owners are allowed to define the structure if they want.
	 */
	String identifier;
	/*
	 * The scope in which the moniker is unique
	 */
	UniquenessLevelEnum unique;
	/*
	 * The moniker kind if known.
	 */
	Opt<MonikerKindEnum> kind = {};
};

/*
 * MonikerOptions
 */
struct MonikerOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * MonikerRegistrationOptions
 */
struct MonikerRegistrationOptions : TextDocumentRegistrationOptions, MonikerOptions{
};

/*
 * TypeHierarchyPrepareParams
 *
 * The parameter of a `textDocument/prepareTypeHierarchy` request.
 * 
 * @since 3.17.0
 */
struct TypeHierarchyPrepareParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
};

/*
 * TypeHierarchyItem
 *
 * @since 3.17.0
 */
struct TypeHierarchyItem{
	/*
	 * The name of this item.
	 */
	String name;
	/*
	 * The kind of this item.
	 */
	SymbolKindEnum kind;
	/*
	 * The resource identifier of this item.
	 */
	DocumentUri uri;
	/*
	 * The range enclosing this symbol not including leading/trailing whitespace
	 * but everything else, e.g. comments and code.
	 */
	Range range;
	/*
	 * The range that should be selected and revealed when this symbol is being
	 * picked, e.g. the name of a function. Must be contained by the
	 * {@link TypeHierarchyItem.range `range`}.
	 */
	Range selectionRange;
	/*
	 * Tags for this item.
	 */
	Opt<Array<SymbolTagEnum>> tags = {};
	/*
	 * More detail for this item, e.g. the signature of a function.
	 */
	Opt<String> detail = {};
	/*
	 * A data entry field that is preserved between a type hierarchy prepare and
	 * supertypes or subtypes requests. It could also be used to identify the
	 * type hierarchy in the server, helping improve the performance on
	 * resolving supertypes and subtypes.
	 */
	Opt<LSPAny> data = {};
};

/*
 * TypeHierarchyOptions
 *
 * Type hierarchy options used during static registration.
 * 
 * @since 3.17.0
 */
struct TypeHierarchyOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * TypeHierarchyRegistrationOptions
 *
 * Type hierarchy options used during static or dynamic registration.
 * 
 * @since 3.17.0
 */
struct TypeHierarchyRegistrationOptions : TextDocumentRegistrationOptions, TypeHierarchyOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * TypeHierarchySupertypesParams
 *
 * The parameter of a `typeHierarchy/supertypes` request.
 * 
 * @since 3.17.0
 */
struct TypeHierarchySupertypesParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	TypeHierarchyItem item;
};

/*
 * TypeHierarchySubtypesParams
 *
 * The parameter of a `typeHierarchy/subtypes` request.
 * 
 * @since 3.17.0
 */
struct TypeHierarchySubtypesParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	TypeHierarchyItem item;
};

/*
 * InlineValueContext
 *
 * @since 3.17.0
 */
struct InlineValueContext{
	/*
	 * The stack frame (as a DAP Id) where the execution has stopped.
	 */
	int frameId;
	/*
	 * The document range where execution has stopped.
	 * Typically the end position of the range denotes the line where the inline values are shown.
	 */
	Range stoppedLocation;
};

/*
 * InlineValueParams
 *
 * A parameter literal used in inline value requests.
 * 
 * @since 3.17.0
 */
struct InlineValueParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The document range for which inline values should be computed.
	 */
	Range range;
	/*
	 * Additional information about the context in which inline values were
	 * requested.
	 */
	InlineValueContext context;
};

/*
 * InlineValueOptions
 *
 * Inline value options used during static registration.
 * 
 * @since 3.17.0
 */
struct InlineValueOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * InlineValueRegistrationOptions
 *
 * Inline value options used during static or dynamic registration.
 * 
 * @since 3.17.0
 */
struct InlineValueRegistrationOptions : InlineValueOptions, TextDocumentRegistrationOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * InlayHintParams
 *
 * A parameter literal used in inlay hint requests.
 * 
 * @since 3.17.0
 */
struct InlayHintParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The document range for which inlay hints should be computed.
	 */
	Range range;
};

/*
 * MarkupContent
 *
 * A `MarkupContent` literal represents a string value which content is interpreted base on its
 * kind flag. Currently the protocol supports `plaintext` and `markdown` as markup kinds.
 * 
 * If the kind is `markdown` then the value can contain fenced code blocks like in GitHub issues.
 * See https://help.github.com/articles/creating-and-highlighting-code-blocks/#syntax-highlighting
 * 
 * Here is an example how such a string can be constructed using JavaScript / TypeScript:
 * ```ts
 * let markdown: MarkdownContent = {
 *  kind: MarkupKind.Markdown,
 *  value: [
 *    '# Header',
 *    'Some text',
 *    '```typescript',
 *    'someCode();',
 *    '```'
 *  ].join('\n')
 * };
 * ```
 * 
 * *Please Note* that clients might sanitize the return markdown. A client could decide to
 * remove HTML from the markdown to avoid script execution.
 */
struct MarkupContent{
	/*
	 * The type of the Markup
	 */
	MarkupKindEnum kind;
	/*
	 * The content itself
	 */
	String value;
};

/*
 * Command
 *
 * Represents a reference to a command. Provides a title which
 * will be used to represent a command in the UI and, optionally,
 * an array of arguments which will be passed to the command handler
 * function when invoked.
 */
struct Command{
	/*
	 * Title of the command, like `save`.
	 */
	String title;
	/*
	 * The identifier of the actual command handler.
	 */
	String command;
	/*
	 * Arguments that the command handler should be
	 * invoked with.
	 */
	Opt<LSPArray> arguments = {};
};

/*
 * InlayHintLabelPart
 *
 * An inlay hint label part allows for interactive and composite labels
 * of inlay hints.
 * 
 * @since 3.17.0
 */
struct InlayHintLabelPart{
	/*
	 * The value of this label part.
	 */
	String value;
	/*
	 * The tooltip text when you hover over this label part. Depending on
	 * the client capability `inlayHint.resolveSupport` clients might resolve
	 * this property late using the resolve request.
	 */
	Opt<OneOf<String, MarkupContent>> tooltip = {};
	/*
	 * An optional source code location that represents this
	 * label part.
	 * 
	 * The editor will use this location for the hover and for code navigation
	 * features: This part will become a clickable link that resolves to the
	 * definition of the symbol at the given location (not necessarily the
	 * location itself), it shows the hover that shows at the given location,
	 * and it shows a context menu with further code navigation commands.
	 * 
	 * Depending on the client capability `inlayHint.resolveSupport` clients
	 * might resolve this property late using the resolve request.
	 */
	Opt<Location> location = {};
	/*
	 * An optional command for this label part.
	 * 
	 * Depending on the client capability `inlayHint.resolveSupport` clients
	 * might resolve this property late using the resolve request.
	 */
	Opt<Command> command = {};
};

/*
 * InlayHint
 *
 * Inlay hint information.
 * 
 * @since 3.17.0
 */
struct InlayHint{
	/*
	 * The position of this hint.
	 * 
	 * If multiple hints have the same position, they will be shown in the order
	 * they appear in the response.
	 */
	Position position;
	/*
	 * The label of this hint. A human readable string or an array of
	 * InlayHintLabelPart label parts.
	 * 
	 * *Note* that neither the string nor the label part can be empty.
	 */
	OneOf<String, Array<InlayHintLabelPart>> label;
	/*
	 * The kind of this hint. Can be omitted in which case the client
	 * should fall back to a reasonable default.
	 */
	Opt<InlayHintKindEnum> kind = {};
	/*
	 * Optional text edits that are performed when accepting this inlay hint.
	 * 
	 * *Note* that edits are expected to change the document so that the inlay
	 * hint (or its nearest variant) is now part of the document and the inlay
	 * hint itself is now obsolete.
	 */
	Opt<Array<TextEdit>> textEdits = {};
	/*
	 * The tooltip text when you hover over this item.
	 */
	Opt<OneOf<String, MarkupContent>> tooltip = {};
	/*
	 * Render padding before the hint.
	 * 
	 * Note: Padding should use the editor's background color, not the
	 * background color of the hint itself. That means padding can be used
	 * to visually align/separate an inlay hint.
	 */
	Opt<bool> paddingLeft = {};
	/*
	 * Render padding after the hint.
	 * 
	 * Note: Padding should use the editor's background color, not the
	 * background color of the hint itself. That means padding can be used
	 * to visually align/separate an inlay hint.
	 */
	Opt<bool> paddingRight = {};
	/*
	 * A data entry field that is preserved on an inlay hint between
	 * a `textDocument/inlayHint` and a `inlayHint/resolve` request.
	 */
	Opt<LSPAny> data = {};
};

/*
 * InlayHintOptions
 *
 * Inlay hint options used during static registration.
 * 
 * @since 3.17.0
 */
struct InlayHintOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * The server provides support to resolve additional
	 * information for an inlay hint item.
	 */
	Opt<bool> resolveProvider = {};
};

/*
 * InlayHintRegistrationOptions
 *
 * Inlay hint options used during static or dynamic registration.
 * 
 * @since 3.17.0
 */
struct InlayHintRegistrationOptions : InlayHintOptions, TextDocumentRegistrationOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * DocumentDiagnosticParams
 *
 * Parameters of the document diagnostic request.
 * 
 * @since 3.17.0
 */
struct DocumentDiagnosticParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The additional identifier  provided during registration.
	 */
	Opt<String> identifier = {};
	/*
	 * The result id of a previous response if provided.
	 */
	Opt<String> previousResultId = {};
};

/*
 * CodeDescription
 *
 * Structure to capture a description for an error code.
 * 
 * @since 3.16.0
 */
struct CodeDescription{
	/*
	 * An URI to open with more information about the diagnostic error.
	 */
	Uri href;
};

/*
 * DiagnosticRelatedInformation
 *
 * Represents a related message and source code location for a diagnostic. This should be
 * used to point to code locations that cause or related to a diagnostics, e.g when duplicating
 * a symbol in a scope.
 */
struct DiagnosticRelatedInformation{
	/*
	 * The location of this related diagnostic information.
	 */
	Location location;
	/*
	 * The message of this related diagnostic information.
	 */
	String message;
};

/*
 * Diagnostic
 *
 * Represents a diagnostic, such as a compiler error or warning. Diagnostic objects
 * are only valid in the scope of a resource.
 */
struct Diagnostic{
	/*
	 * The range at which the message applies
	 */
	Range range;
	/*
	 * The diagnostic's message. It usually appears in the user interface
	 */
	String message;
	/*
	 * The diagnostic's severity. Can be omitted. If omitted it is up to the
	 * client to interpret diagnostics as error, warning, info or hint.
	 */
	Opt<DiagnosticSeverityEnum> severity = {};
	/*
	 * The diagnostic's code, which usually appear in the user interface.
	 */
	Opt<OneOf<int, String>> code = {};
	/*
	 * An optional property to describe the error code.
	 * Requires the code field (above) to be present/not null.
	 * 
	 * @since 3.16.0
	 */
	Opt<CodeDescription> codeDescription = {};
	/*
	 * A human-readable string describing the source of this
	 * diagnostic, e.g. 'typescript' or 'super lint'. It usually
	 * appears in the user interface.
	 */
	Opt<String> source = {};
	/*
	 * Additional metadata about the diagnostic.
	 * 
	 * @since 3.15.0
	 */
	Opt<Array<DiagnosticTagEnum>> tags = {};
	/*
	 * An array of related diagnostic information, e.g. when symbol-names within
	 * a scope collide all definitions can be marked via this property.
	 */
	Opt<Array<DiagnosticRelatedInformation>> relatedInformation = {};
	/*
	 * A data entry field that is preserved between a `textDocument/publishDiagnostics`
	 * notification and `textDocument/codeAction` request.
	 * 
	 * @since 3.16.0
	 */
	Opt<LSPAny> data = {};
};

/*
 * FullDocumentDiagnosticReport
 *
 * A diagnostic report with a full set of problems.
 * 
 * @since 3.17.0
 */
struct FullDocumentDiagnosticReport{
	/*
	 * A full document diagnostic report.
	 */
	String kind = "full";
	/*
	 * The actual items.
	 */
	Array<Diagnostic> items;
	/*
	 * An optional result id. If provided it will
	 * be sent on the next diagnostic request for the
	 * same document.
	 */
	Opt<String> resultId = {};
};

/*
 * UnchangedDocumentDiagnosticReport
 *
 * A diagnostic report indicating that the last returned
 * report is still accurate.
 * 
 * @since 3.17.0
 */
struct UnchangedDocumentDiagnosticReport{
	/*
	 * A document diagnostic report indicating
	 * no changes to the last result. A server can
	 * only return `unchanged` if result ids are
	 * provided.
	 */
	String kind = "unchanged";
	/*
	 * A result id which will be sent on the next
	 * diagnostic request for the same document.
	 */
	String resultId;
};

/*
 * DocumentDiagnosticReportPartialResult
 *
 * A partial result for a document diagnostic report.
 * 
 * @since 3.17.0
 */
struct DocumentDiagnosticReportPartialResult{
	Map<DocumentUri, OneOf<FullDocumentDiagnosticReport, UnchangedDocumentDiagnosticReport>> relatedDocuments;
};

/*
 * DiagnosticServerCancellationData
 *
 * Cancellation data returned from a diagnostic request.
 * 
 * @since 3.17.0
 */
struct DiagnosticServerCancellationData{
	bool retriggerRequest;
};

/*
 * DiagnosticOptions
 *
 * Diagnostic options.
 * 
 * @since 3.17.0
 */
struct DiagnosticOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * Whether the language has inter file dependencies meaning that
	 * editing code in one file can result in a different diagnostic
	 * set in another file. Inter file dependencies are common for
	 * most programming languages and typically uncommon for linters.
	 */
	bool interFileDependencies;
	/*
	 * The server provides support for workspace diagnostics as well.
	 */
	bool workspaceDiagnostics;
	/*
	 * An optional identifier under which the diagnostics are
	 * managed by the client.
	 */
	Opt<String> identifier = {};
};

/*
 * DiagnosticRegistrationOptions
 *
 * Diagnostic registration options.
 * 
 * @since 3.17.0
 */
struct DiagnosticRegistrationOptions : TextDocumentRegistrationOptions, DiagnosticOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * PreviousResultId
 *
 * A previous result id in a workspace pull request.
 * 
 * @since 3.17.0
 */
struct PreviousResultId{
	/*
	 * The URI for which the client knowns a
	 * result id.
	 */
	DocumentUri uri;
	/*
	 * The value of the previous result id.
	 */
	String value;
};

/*
 * WorkspaceDiagnosticParams
 *
 * Parameters of the workspace diagnostic request.
 * 
 * @since 3.17.0
 */
struct WorkspaceDiagnosticParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The currently known diagnostic reports with their
	 * previous result ids.
	 */
	Array<PreviousResultId> previousResultIds;
	/*
	 * The additional identifier provided during registration.
	 */
	Opt<String> identifier = {};
};

/*
 * WorkspaceFullDocumentDiagnosticReport
 *
 * A full document diagnostic report for a workspace diagnostic result.
 * 
 * @since 3.17.0
 */
struct WorkspaceFullDocumentDiagnosticReport : FullDocumentDiagnosticReport{
	/*
	 * The URI for which diagnostic information is reported.
	 */
	DocumentUri uri;
	/*
	 * The version number for which the diagnostics are reported.
	 * If the document is not marked as open `null` can be provided.
	 */
	NullOr<int> version;
};

/*
 * WorkspaceUnchangedDocumentDiagnosticReport
 *
 * An unchanged document diagnostic report for a workspace diagnostic result.
 * 
 * @since 3.17.0
 */
struct WorkspaceUnchangedDocumentDiagnosticReport : UnchangedDocumentDiagnosticReport{
	/*
	 * The URI for which diagnostic information is reported.
	 */
	DocumentUri uri;
	/*
	 * The version number for which the diagnostics are reported.
	 * If the document is not marked as open `null` can be provided.
	 */
	NullOr<int> version;
};

/*
 * WorkspaceDocumentDiagnosticReport
 *
 * A workspace diagnostic document report.
 * 
 * @since 3.17.0
 */
using WorkspaceDocumentDiagnosticReport = OneOf<WorkspaceFullDocumentDiagnosticReport, WorkspaceUnchangedDocumentDiagnosticReport>;

/*
 * WorkspaceDiagnosticReport
 *
 * A workspace diagnostic report.
 * 
 * @since 3.17.0
 */
struct WorkspaceDiagnosticReport{
	Array<WorkspaceDocumentDiagnosticReport> items;
};

/*
 * WorkspaceDiagnosticReportPartialResult
 *
 * A partial result for a workspace diagnostic report.
 * 
 * @since 3.17.0
 */
struct WorkspaceDiagnosticReportPartialResult{
	Array<WorkspaceDocumentDiagnosticReport> items;
};

/*
 * ExecutionSummary
 */
struct ExecutionSummary{
	/*
	 * A strict monotonically increasing value
	 * indicating the execution order of a cell
	 * inside a notebook.
	 */
	uint executionOrder;
	/*
	 * Whether the execution was successful or
	 * not if known by the client.
	 */
	Opt<bool> success = {};
};

/*
 * NotebookCell
 *
 * A notebook cell.
 * 
 * A cell's document URI must be unique across ALL notebook
 * cells and can therefore be used to uniquely identify a
 * notebook cell or the cell's text document.
 * 
 * @since 3.17.0
 */
struct NotebookCell{
	/*
	 * The cell's kind
	 */
	NotebookCellKindEnum kind;
	/*
	 * The URI of the cell's text document
	 * content.
	 */
	DocumentUri document;
	/*
	 * Additional metadata stored with the cell.
	 * 
	 * Note: should always be an object literal (e.g. LSPObject)
	 */
	Opt<LSPObject> metadata = {};
	/*
	 * Additional execution summary information
	 * if supported by the client.
	 */
	Opt<ExecutionSummary> executionSummary = {};
};

/*
 * NotebookDocument
 *
 * A notebook document.
 * 
 * @since 3.17.0
 */
struct NotebookDocument{
	/*
	 * The notebook document's uri.
	 */
	Uri uri;
	/*
	 * The type of the notebook.
	 */
	String notebookType;
	/*
	 * The version number of this document (it will increase after each
	 * change, including undo/redo).
	 */
	int version;
	/*
	 * The cells of a notebook.
	 */
	Array<NotebookCell> cells;
	/*
	 * Additional metadata stored with the notebook
	 * document.
	 * 
	 * Note: should always be an object literal (e.g. LSPObject)
	 */
	Opt<LSPObject> metadata = {};
};

/*
 * TextDocumentItem
 *
 * An item to transfer a text document from the client to the
 * server.
 */
struct TextDocumentItem{
	/*
	 * The text document's uri.
	 */
	DocumentUri uri;
	/*
	 * The text document's language identifier.
	 */
	String languageId;
	/*
	 * The version number of this document (it will increase after each
	 * change, including undo/redo).
	 */
	int version;
	/*
	 * The content of the opened text document.
	 */
	String text;
};

/*
 * DidOpenNotebookDocumentParams
 *
 * The params sent in an open notebook document notification.
 * 
 * @since 3.17.0
 */
struct DidOpenNotebookDocumentParams{
	/*
	 * The notebook document that got opened.
	 */
	NotebookDocument notebookDocument;
	/*
	 * The text documents that represent the content
	 * of a notebook cell.
	 */
	Array<TextDocumentItem> cellTextDocuments;
};

/*
 * VersionedNotebookDocumentIdentifier
 *
 * A versioned notebook document identifier.
 * 
 * @since 3.17.0
 */
struct VersionedNotebookDocumentIdentifier{
	/*
	 * The version number of this notebook document.
	 */
	int version;
	/*
	 * The notebook document's uri.
	 */
	Uri uri;
};

/*
 * NotebookCellArrayChange
 *
 * A change describing how to move a `NotebookCell`
 * array from state S to S'.
 * 
 * @since 3.17.0
 */
struct NotebookCellArrayChange{
	/*
	 * The start oftest of the cell that changed.
	 */
	uint start;
	/*
	 * The deleted cells
	 */
	uint deleteCount;
	/*
	 * The new cells, if any
	 */
	Opt<Array<NotebookCell>> cells = {};
};

/*
 * NotebookDocumentChangeEventCellsStructure
 */
struct NotebookDocumentChangeEventCellsStructure{
	/*
	 * The change to the cell array.
	 */
	NotebookCellArrayChange array;
	/*
	 * Additional opened cell text documents.
	 */
	Opt<Array<TextDocumentItem>> didOpen = {};
	/*
	 * Additional closed cell text documents.
	 */
	Opt<Array<TextDocumentIdentifier>> didClose = {};
};

/*
 * VersionedTextDocumentIdentifier
 *
 * A text document identifier to denote a specific version of a text document.
 */
struct VersionedTextDocumentIdentifier : TextDocumentIdentifier{
	/*
	 * The version number of this document.
	 */
	int version;
};

/*
 * TextDocumentContentChangeEvent_Range_Text
 */
struct TextDocumentContentChangeEvent_Range_Text{
	/*
	 * The range of the document that changed.
	 */
	Range range;
	/*
	 * The new text for the provided range.
	 */
	String text;
	/*
	 * The optional length of the range that got replaced.
	 * 
	 * @deprecated use range instead.
	 */
	Opt<uint> rangeLength = {};
};

/*
 * TextDocumentContentChangeEvent_Text
 */
struct TextDocumentContentChangeEvent_Text{
	/*
	 * The new text of the whole document.
	 */
	String text;
};

/*
 * TextDocumentContentChangeEvent
 *
 * An event describing a change to a text document. If only a text is provided
 * it is considered to be the full content of the document.
 */
using TextDocumentContentChangeEvent = OneOf<TextDocumentContentChangeEvent_Range_Text, TextDocumentContentChangeEvent_Text>;

/*
 * NotebookDocumentChangeEventCellsTextContent
 */
struct NotebookDocumentChangeEventCellsTextContent{
	VersionedTextDocumentIdentifier document;
	Array<TextDocumentContentChangeEvent> changes;
};

/*
 * NotebookDocumentChangeEventCells
 */
struct NotebookDocumentChangeEventCells{
	/*
	 * Changes to the cell structure to add or
	 * remove cells.
	 */
	Opt<NotebookDocumentChangeEventCellsStructure> structure = {};
	/*
	 * Changes to notebook cells properties like its
	 * kind, execution summary or metadata.
	 */
	Opt<Array<NotebookCell>> data = {};
	/*
	 * Changes to the text content of notebook cells.
	 */
	Opt<Array<NotebookDocumentChangeEventCellsTextContent>> textContent = {};
};

/*
 * NotebookDocumentChangeEvent
 *
 * A change event for a notebook document.
 * 
 * @since 3.17.0
 */
struct NotebookDocumentChangeEvent{
	/*
	 * The changed meta data if any.
	 * 
	 * Note: should always be an object literal (e.g. LSPObject)
	 */
	Opt<LSPObject> metadata = {};
	/*
	 * Changes to cells
	 */
	Opt<NotebookDocumentChangeEventCells> cells = {};
};

/*
 * DidChangeNotebookDocumentParams
 *
 * The params sent in a change notebook document notification.
 * 
 * @since 3.17.0
 */
struct DidChangeNotebookDocumentParams{
	/*
	 * The notebook document that did change. The version number points
	 * to the version after all provided changes have been applied. If
	 * only the text document content of a cell changes the notebook version
	 * doesn't necessarily have to change.
	 */
	VersionedNotebookDocumentIdentifier notebookDocument;
	/*
	 * The actual changes to the notebook document.
	 * 
	 * The changes describe single state changes to the notebook document.
	 * So if there are two changes c1 (at array index 0) and c2 (at array
	 * index 1) for a notebook in state S then c1 moves the notebook from
	 * S to S' and c2 from S' to S''. So c1 is computed on the state S and
	 * c2 is computed on the state S'.
	 * 
	 * To mirror the content of a notebook using change events use the following approach:
	 * - start with the same initial content
	 * - apply the 'notebookDocument/didChange' notifications in the order you receive them.
	 * - apply the `NotebookChangeEvent`s in a single notification in the order
	 *   you receive them.
	 */
	NotebookDocumentChangeEvent change;
};

/*
 * NotebookDocumentIdentifier
 *
 * A literal to identify a notebook document in the client.
 * 
 * @since 3.17.0
 */
struct NotebookDocumentIdentifier{
	/*
	 * The notebook document's uri.
	 */
	Uri uri;
};

/*
 * DidSaveNotebookDocumentParams
 *
 * The params sent in a save notebook document notification.
 * 
 * @since 3.17.0
 */
struct DidSaveNotebookDocumentParams{
	/*
	 * The notebook document that got saved.
	 */
	NotebookDocumentIdentifier notebookDocument;
};

/*
 * DidCloseNotebookDocumentParams
 *
 * The params sent in a close notebook document notification.
 * 
 * @since 3.17.0
 */
struct DidCloseNotebookDocumentParams{
	/*
	 * The notebook document that got closed.
	 */
	NotebookDocumentIdentifier notebookDocument;
	/*
	 * The text documents that represent the content
	 * of a notebook cell that got closed.
	 */
	Array<TextDocumentIdentifier> cellTextDocuments;
};

/*
 * SelectedCompletionInfo
 *
 * Describes the currently selected completion item.
 * 
 * @since 3.18.0
 * @proposed
 */
struct SelectedCompletionInfo{
	/*
	 * The range that will be replaced if this completion item is accepted.
	 */
	Range range;
	/*
	 * The text the range will be replaced with if this completion is accepted.
	 */
	String text;
};

/*
 * InlineCompletionContext
 *
 * Provides information about the context in which an inline completion was requested.
 * 
 * @since 3.18.0
 * @proposed
 */
struct InlineCompletionContext{
	/*
	 * Describes how the inline completion was triggered.
	 */
	InlineCompletionTriggerKindEnum triggerKind;
	/*
	 * Provides information about the currently selected item in the autocomplete widget if it is visible.
	 */
	Opt<SelectedCompletionInfo> selectedCompletionInfo = {};
};

/*
 * InlineCompletionParams
 *
 * A parameter literal used in inline completion requests.
 * 
 * @since 3.18.0
 * @proposed
 */
struct InlineCompletionParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * Additional information about the context in which inline completions were
	 * requested.
	 */
	InlineCompletionContext context;
};

/*
 * StringValue
 *
 * A string value used as a snippet is a template which allows to insert text
 * and to control the editor cursor when insertion happens.
 * 
 * A snippet can define tab stops and placeholders with `$1`, `$2`
 * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
 * the end of the snippet. Variables are defined with `$name` and
 * `${name:default value}`.
 * 
 * @since 3.18.0
 * @proposed
 */
struct StringValue{
	/*
	 * The kind of string value.
	 */
	String kind = "snippet";
	/*
	 * The snippet string.
	 */
	String value;
};

/*
 * InlineCompletionItem
 *
 * An inline completion item represents a text snippet that is proposed inline to complete text that is being typed.
 * 
 * @since 3.18.0
 * @proposed
 */
struct InlineCompletionItem{
	/*
	 * The text to replace the range with. Must be set.
	 */
	OneOf<String, StringValue> insertText;
	/*
	 * A text that is used to decide if this inline completion should be shown. When `falsy` the {@link InlineCompletionItem.insertText} is used.
	 */
	Opt<String> filterText = {};
	/*
	 * The range to replace. Must begin and end on the same line.
	 */
	Opt<Range> range = {};
	/*
	 * An optional {@link Command} that is executed *after* inserting this completion.
	 */
	Opt<Command> command = {};
};

/*
 * InlineCompletionList
 *
 * Represents a collection of {@link InlineCompletionItem inline completion items} to be presented in the editor.
 * 
 * @since 3.18.0
 * @proposed
 */
struct InlineCompletionList{
	/*
	 * The inline completion items
	 */
	Array<InlineCompletionItem> items;
};

/*
 * InlineCompletionOptions
 *
 * Inline completion options used during static registration.
 * 
 * @since 3.18.0
 * @proposed
 */
struct InlineCompletionOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * InlineCompletionRegistrationOptions
 *
 * Inline completion options used during static or dynamic registration.
 * 
 * @since 3.18.0
 * @proposed
 */
struct InlineCompletionRegistrationOptions : InlineCompletionOptions, TextDocumentRegistrationOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * Registration
 *
 * General parameters to register for a notification or to register a provider.
 */
struct Registration{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again.
	 */
	String id;
	/*
	 * The method / capability to register for.
	 */
	String method;
	/*
	 * Options necessary for the registration.
	 */
	Opt<LSPAny> registerOptions = {};
};

/*
 * RegistrationParams
 */
struct RegistrationParams{
	Array<Registration> registrations;
};

/*
 * Unregistration
 *
 * General parameters to unregister a request or notification.
 */
struct Unregistration{
	/*
	 * The id used to unregister the request or notification. Usually an id
	 * provided during the register request.
	 */
	String id;
	/*
	 * The method to unregister for.
	 */
	String method;
};

/*
 * UnregistrationParams
 */
struct UnregistrationParams{
	Array<Unregistration> unregisterations;
};

/*
 * WorkspaceEditClientCapabilitiesChangeAnnotationSupport
 */
struct WorkspaceEditClientCapabilitiesChangeAnnotationSupport{
	/*
	 * Whether the client groups edits with equal labels into tree nodes,
	 * for instance all edits labelled with "Changes in Strings" would
	 * be a tree node.
	 */
	Opt<bool> groupsOnLabel = {};
};

/*
 * WorkspaceEditClientCapabilities
 */
struct WorkspaceEditClientCapabilities{
	/*
	 * The client supports versioned document changes in `WorkspaceEdit`s
	 */
	Opt<bool> documentChanges = {};
	/*
	 * The resource operations the client supports. Clients should at least
	 * support 'create', 'rename' and 'delete' files and folders.
	 * 
	 * @since 3.13.0
	 */
	Opt<Array<ResourceOperationKindEnum>> resourceOperations = {};
	/*
	 * The failure handling strategy of a client if applying the workspace edit
	 * fails.
	 * 
	 * @since 3.13.0
	 */
	Opt<FailureHandlingKindEnum> failureHandling = {};
	/*
	 * Whether the client normalizes line endings to the client specific
	 * setting.
	 * If set to `true` the client will normalize line ending characters
	 * in a workspace edit to the client-specified new line
	 * character.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> normalizesLineEndings = {};
	/*
	 * Whether the client in general supports change annotations on text edits,
	 * create file, rename file and delete file changes.
	 * 
	 * @since 3.16.0
	 */
	Opt<WorkspaceEditClientCapabilitiesChangeAnnotationSupport> changeAnnotationSupport = {};
};

/*
 * DidChangeConfigurationClientCapabilities
 */
struct DidChangeConfigurationClientCapabilities{
	/*
	 * Did change configuration notification supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * DidChangeWatchedFilesClientCapabilities
 */
struct DidChangeWatchedFilesClientCapabilities{
	/*
	 * Did change watched files notification supports dynamic registration. Please note
	 * that the current protocol doesn't support static configuration for file changes
	 * from the server side.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Whether the client has support for {@link  RelativePattern relative pattern}
	 * or not.
	 * 
	 * @since 3.17.0
	 */
	Opt<bool> relativePatternSupport = {};
};

/*
 * WorkspaceSymbolClientCapabilitiesSymbolKind
 */
struct WorkspaceSymbolClientCapabilitiesSymbolKind{
	/*
	 * The symbol kind values the client supports. When this
	 * property exists the client also guarantees that it will
	 * handle values outside its set gracefully and falls back
	 * to a default value when unknown.
	 * 
	 * If this property is not present the client only supports
	 * the symbol kinds from `File` to `Array` as defined in
	 * the initial version of the protocol.
	 */
	Opt<Array<SymbolKindEnum>> valueSet = {};
};

/*
 * WorkspaceSymbolClientCapabilitiesTagSupport
 */
struct WorkspaceSymbolClientCapabilitiesTagSupport{
	/*
	 * The tags supported by the client.
	 */
	Array<SymbolTagEnum> valueSet;
};

/*
 * WorkspaceSymbolClientCapabilitiesResolveSupport
 */
struct WorkspaceSymbolClientCapabilitiesResolveSupport{
	/*
	 * The properties that a client can resolve lazily. Usually
	 * `location.range`
	 */
	Array<String> properties;
};

/*
 * WorkspaceSymbolClientCapabilities
 *
 * Client capabilities for a {@link WorkspaceSymbolRequest}.
 */
struct WorkspaceSymbolClientCapabilities{
	/*
	 * Symbol request supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Specific capabilities for the `SymbolKind` in the `workspace/symbol` request.
	 */
	Opt<WorkspaceSymbolClientCapabilitiesSymbolKind> symbolKind = {};
	/*
	 * The client supports tags on `SymbolInformation`.
	 * Clients supporting tags have to handle unknown tags gracefully.
	 * 
	 * @since 3.16.0
	 */
	Opt<WorkspaceSymbolClientCapabilitiesTagSupport> tagSupport = {};
	/*
	 * The client support partial workspace symbols. The client will send the
	 * request `workspaceSymbol/resolve` to the server to resolve additional
	 * properties.
	 * 
	 * @since 3.17.0
	 */
	Opt<WorkspaceSymbolClientCapabilitiesResolveSupport> resolveSupport = {};
};

/*
 * ExecuteCommandClientCapabilities
 *
 * The client capabilities of a {@link ExecuteCommandRequest}.
 */
struct ExecuteCommandClientCapabilities{
	/*
	 * Execute command supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * SemanticTokensWorkspaceClientCapabilities
 *
 * @since 3.16.0
 */
struct SemanticTokensWorkspaceClientCapabilities{
	/*
	 * Whether the client implementation supports a refresh request sent from
	 * the server to the client.
	 * 
	 * Note that this event is global and will force the client to refresh all
	 * semantic tokens currently shown. It should be used with absolute care
	 * and is useful for situation where a server for example detects a project
	 * wide change that requires such a calculation.
	 */
	Opt<bool> refreshSupport = {};
};

/*
 * CodeLensWorkspaceClientCapabilities
 *
 * @since 3.16.0
 */
struct CodeLensWorkspaceClientCapabilities{
	/*
	 * Whether the client implementation supports a refresh request sent from the
	 * server to the client.
	 * 
	 * Note that this event is global and will force the client to refresh all
	 * code lenses currently shown. It should be used with absolute care and is
	 * useful for situation where a server for example detect a project wide
	 * change that requires such a calculation.
	 */
	Opt<bool> refreshSupport = {};
};

/*
 * FileOperationClientCapabilities
 *
 * Capabilities relating to events from file operations by the user in the client.
 * 
 * These events do not come from the file system, they come from user operations
 * like renaming a file in the UI.
 * 
 * @since 3.16.0
 */
struct FileOperationClientCapabilities{
	/*
	 * Whether the client supports dynamic registration for file requests/notifications.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client has support for sending didCreateFiles notifications.
	 */
	Opt<bool> didCreate = {};
	/*
	 * The client has support for sending willCreateFiles requests.
	 */
	Opt<bool> willCreate = {};
	/*
	 * The client has support for sending didRenameFiles notifications.
	 */
	Opt<bool> didRename = {};
	/*
	 * The client has support for sending willRenameFiles requests.
	 */
	Opt<bool> willRename = {};
	/*
	 * The client has support for sending didDeleteFiles notifications.
	 */
	Opt<bool> didDelete = {};
	/*
	 * The client has support for sending willDeleteFiles requests.
	 */
	Opt<bool> willDelete = {};
};

/*
 * InlineValueWorkspaceClientCapabilities
 *
 * Client workspace capabilities specific to inline values.
 * 
 * @since 3.17.0
 */
struct InlineValueWorkspaceClientCapabilities{
	/*
	 * Whether the client implementation supports a refresh request sent from the
	 * server to the client.
	 * 
	 * Note that this event is global and will force the client to refresh all
	 * inline values currently shown. It should be used with absolute care and is
	 * useful for situation where a server for example detects a project wide
	 * change that requires such a calculation.
	 */
	Opt<bool> refreshSupport = {};
};

/*
 * InlayHintWorkspaceClientCapabilities
 *
 * Client workspace capabilities specific to inlay hints.
 * 
 * @since 3.17.0
 */
struct InlayHintWorkspaceClientCapabilities{
	/*
	 * Whether the client implementation supports a refresh request sent from
	 * the server to the client.
	 * 
	 * Note that this event is global and will force the client to refresh all
	 * inlay hints currently shown. It should be used with absolute care and
	 * is useful for situation where a server for example detects a project wide
	 * change that requires such a calculation.
	 */
	Opt<bool> refreshSupport = {};
};

/*
 * DiagnosticWorkspaceClientCapabilities
 *
 * Workspace client capabilities specific to diagnostic pull requests.
 * 
 * @since 3.17.0
 */
struct DiagnosticWorkspaceClientCapabilities{
	/*
	 * Whether the client implementation supports a refresh request sent from
	 * the server to the client.
	 * 
	 * Note that this event is global and will force the client to refresh all
	 * pulled diagnostics currently shown. It should be used with absolute care and
	 * is useful for situation where a server for example detects a project wide
	 * change that requires such a calculation.
	 */
	Opt<bool> refreshSupport = {};
};

/*
 * FoldingRangeWorkspaceClientCapabilities
 *
 * Client workspace capabilities specific to folding ranges
 * 
 * @since 3.18.0
 * @proposed
 */
struct FoldingRangeWorkspaceClientCapabilities{
	/*
	 * Whether the client implementation supports a refresh request sent from the
	 * server to the client.
	 * 
	 * Note that this event is global and will force the client to refresh all
	 * folding ranges currently shown. It should be used with absolute care and is
	 * useful for situation where a server for example detects a project wide
	 * change that requires such a calculation.
	 * 
	 * @since 3.18.0
	 * @proposed
	 */
	Opt<bool> refreshSupport = {};
};

/*
 * WorkspaceClientCapabilities
 *
 * Workspace specific client capabilities.
 */
struct WorkspaceClientCapabilities{
	/*
	 * The client supports applying batch edits
	 * to the workspace by supporting the request
	 * 'workspace/applyEdit'
	 */
	Opt<bool> applyEdit = {};
	/*
	 * Capabilities specific to `WorkspaceEdit`s.
	 */
	Opt<WorkspaceEditClientCapabilities> workspaceEdit = {};
	/*
	 * Capabilities specific to the `workspace/didChangeConfiguration` notification.
	 */
	Opt<DidChangeConfigurationClientCapabilities> didChangeConfiguration = {};
	/*
	 * Capabilities specific to the `workspace/didChangeWatchedFiles` notification.
	 */
	Opt<DidChangeWatchedFilesClientCapabilities> didChangeWatchedFiles = {};
	/*
	 * Capabilities specific to the `workspace/symbol` request.
	 */
	Opt<WorkspaceSymbolClientCapabilities> symbol = {};
	/*
	 * Capabilities specific to the `workspace/executeCommand` request.
	 */
	Opt<ExecuteCommandClientCapabilities> executeCommand = {};
	/*
	 * The client has support for workspace folders.
	 * 
	 * @since 3.6.0
	 */
	Opt<bool> workspaceFolders = {};
	/*
	 * The client supports `workspace/configuration` requests.
	 * 
	 * @since 3.6.0
	 */
	Opt<bool> configuration = {};
	/*
	 * Capabilities specific to the semantic token requests scoped to the
	 * workspace.
	 * 
	 * @since 3.16.0.
	 */
	Opt<SemanticTokensWorkspaceClientCapabilities> semanticTokens = {};
	/*
	 * Capabilities specific to the code lens requests scoped to the
	 * workspace.
	 * 
	 * @since 3.16.0.
	 */
	Opt<CodeLensWorkspaceClientCapabilities> codeLens = {};
	/*
	 * The client has support for file notifications/requests for user operations on files.
	 * 
	 * Since 3.16.0
	 */
	Opt<FileOperationClientCapabilities> fileOperations = {};
	/*
	 * Capabilities specific to the inline values requests scoped to the
	 * workspace.
	 * 
	 * @since 3.17.0.
	 */
	Opt<InlineValueWorkspaceClientCapabilities> inlineValue = {};
	/*
	 * Capabilities specific to the inlay hint requests scoped to the
	 * workspace.
	 * 
	 * @since 3.17.0.
	 */
	Opt<InlayHintWorkspaceClientCapabilities> inlayHint = {};
	/*
	 * Capabilities specific to the diagnostic requests scoped to the
	 * workspace.
	 * 
	 * @since 3.17.0.
	 */
	Opt<DiagnosticWorkspaceClientCapabilities> diagnostics = {};
	/*
	 * Capabilities specific to the folding range requests scoped to the workspace.
	 * 
	 * @since 3.18.0
	 * @proposed
	 */
	Opt<FoldingRangeWorkspaceClientCapabilities> foldingRange = {};
};

/*
 * TextDocumentSyncClientCapabilities
 */
struct TextDocumentSyncClientCapabilities{
	/*
	 * Whether text document synchronization supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports sending will save notifications.
	 */
	Opt<bool> willSave = {};
	/*
	 * The client supports sending a will save request and
	 * waits for a response providing text edits which will
	 * be applied to the document before it is saved.
	 */
	Opt<bool> willSaveWaitUntil = {};
	/*
	 * The client supports did save notifications.
	 */
	Opt<bool> didSave = {};
};

/*
 * CompletionClientCapabilitiesCompletionItemTagSupport
 */
struct CompletionClientCapabilitiesCompletionItemTagSupport{
	/*
	 * The tags supported by the client.
	 */
	Array<CompletionItemTagEnum> valueSet;
};

/*
 * CompletionClientCapabilitiesCompletionItemResolveSupport
 */
struct CompletionClientCapabilitiesCompletionItemResolveSupport{
	/*
	 * The properties that a client can resolve lazily.
	 */
	Array<String> properties;
};

/*
 * CompletionClientCapabilitiesCompletionItemInsertTextModeSupport
 */
struct CompletionClientCapabilitiesCompletionItemInsertTextModeSupport{
	Array<InsertTextModeEnum> valueSet;
};

/*
 * CompletionClientCapabilitiesCompletionItem
 */
struct CompletionClientCapabilitiesCompletionItem{
	/*
	 * Client supports snippets as insert text.
	 * 
	 * A snippet can define tab stops and placeholders with `$1`, `$2`
	 * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
	 * the end of the snippet. Placeholders with equal identifiers are linked,
	 * that is typing in one will update others too.
	 */
	Opt<bool> snippetSupport = {};
	/*
	 * Client supports commit characters on a completion item.
	 */
	Opt<bool> commitCharactersSupport = {};
	/*
	 * Client supports the following content formats for the documentation
	 * property. The order describes the preferred format of the client.
	 */
	Opt<Array<MarkupKindEnum>> documentationFormat = {};
	/*
	 * Client supports the deprecated property on a completion item.
	 */
	Opt<bool> deprecatedSupport = {};
	/*
	 * Client supports the preselect property on a completion item.
	 */
	Opt<bool> preselectSupport = {};
	/*
	 * Client supports the tag property on a completion item. Clients supporting
	 * tags have to handle unknown tags gracefully. Clients especially need to
	 * preserve unknown tags when sending a completion item back to the server in
	 * a resolve call.
	 * 
	 * @since 3.15.0
	 */
	Opt<CompletionClientCapabilitiesCompletionItemTagSupport> tagSupport = {};
	/*
	 * Client support insert replace edit to control different behavior if a
	 * completion item is inserted in the text or should replace text.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> insertReplaceSupport = {};
	/*
	 * Indicates which properties a client can resolve lazily on a completion
	 * item. Before version 3.16.0 only the predefined properties `documentation`
	 * and `details` could be resolved lazily.
	 * 
	 * @since 3.16.0
	 */
	Opt<CompletionClientCapabilitiesCompletionItemResolveSupport> resolveSupport = {};
	/*
	 * The client supports the `insertTextMode` property on
	 * a completion item to override the whitespace handling mode
	 * as defined by the client (see `insertTextMode`).
	 * 
	 * @since 3.16.0
	 */
	Opt<CompletionClientCapabilitiesCompletionItemInsertTextModeSupport> insertTextModeSupport = {};
	/*
	 * The client has support for completion item label
	 * details (see also `CompletionItemLabelDetails`).
	 * 
	 * @since 3.17.0
	 */
	Opt<bool> labelDetailsSupport = {};
};

/*
 * CompletionClientCapabilitiesCompletionItemKind
 */
struct CompletionClientCapabilitiesCompletionItemKind{
	/*
	 * The completion item kind values the client supports. When this
	 * property exists the client also guarantees that it will
	 * handle values outside its set gracefully and falls back
	 * to a default value when unknown.
	 * 
	 * If this property is not present the client only supports
	 * the completion items kinds from `Text` to `Reference` as defined in
	 * the initial version of the protocol.
	 */
	Opt<Array<CompletionItemKindEnum>> valueSet = {};
};

/*
 * CompletionClientCapabilitiesCompletionList
 */
struct CompletionClientCapabilitiesCompletionList{
	/*
	 * The client supports the following itemDefaults on
	 * a completion list.
	 * 
	 * The value lists the supported property names of the
	 * `CompletionList.itemDefaults` object. If omitted
	 * no properties are supported.
	 * 
	 * @since 3.17.0
	 */
	Opt<Array<String>> itemDefaults = {};
};

/*
 * CompletionClientCapabilities
 *
 * Completion client capabilities
 */
struct CompletionClientCapabilities{
	/*
	 * Whether completion supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports the following `CompletionItem` specific
	 * capabilities.
	 */
	Opt<CompletionClientCapabilitiesCompletionItem> completionItem = {};
	Opt<CompletionClientCapabilitiesCompletionItemKind> completionItemKind = {};
	/*
	 * Defines how the client handles whitespace and indentation
	 * when accepting a completion item that uses multi line
	 * text in either `insertText` or `textEdit`.
	 * 
	 * @since 3.17.0
	 */
	Opt<InsertTextModeEnum> insertTextMode = {};
	/*
	 * The client supports to send additional context information for a
	 * `textDocument/completion` request.
	 */
	Opt<bool> contextSupport = {};
	/*
	 * The client supports the following `CompletionList` specific
	 * capabilities.
	 * 
	 * @since 3.17.0
	 */
	Opt<CompletionClientCapabilitiesCompletionList> completionList = {};
};

/*
 * HoverClientCapabilities
 */
struct HoverClientCapabilities{
	/*
	 * Whether hover supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Client supports the following content formats for the content
	 * property. The order describes the preferred format of the client.
	 */
	Opt<Array<MarkupKindEnum>> contentFormat = {};
};

/*
 * SignatureHelpClientCapabilitiesSignatureInformationParameterInformation
 */
struct SignatureHelpClientCapabilitiesSignatureInformationParameterInformation{
	/*
	 * The client supports processing label offsets instead of a
	 * simple label string.
	 * 
	 * @since 3.14.0
	 */
	Opt<bool> labelOffsetSupport = {};
};

/*
 * SignatureHelpClientCapabilitiesSignatureInformation
 */
struct SignatureHelpClientCapabilitiesSignatureInformation{
	/*
	 * Client supports the following content formats for the documentation
	 * property. The order describes the preferred format of the client.
	 */
	Opt<Array<MarkupKindEnum>> documentationFormat = {};
	/*
	 * Client capabilities specific to parameter information.
	 */
	Opt<SignatureHelpClientCapabilitiesSignatureInformationParameterInformation> parameterInformation = {};
	/*
	 * The client supports the `activeParameter` property on `SignatureInformation`
	 * literal.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> activeParameterSupport = {};
};

/*
 * SignatureHelpClientCapabilities
 *
 * Client Capabilities for a {@link SignatureHelpRequest}.
 */
struct SignatureHelpClientCapabilities{
	/*
	 * Whether signature help supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports the following `SignatureInformation`
	 * specific properties.
	 */
	Opt<SignatureHelpClientCapabilitiesSignatureInformation> signatureInformation = {};
	/*
	 * The client supports to send additional context information for a
	 * `textDocument/signatureHelp` request. A client that opts into
	 * contextSupport will also support the `retriggerCharacters` on
	 * `SignatureHelpOptions`.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> contextSupport = {};
};

/*
 * DeclarationClientCapabilities
 *
 * @since 3.14.0
 */
struct DeclarationClientCapabilities{
	/*
	 * Whether declaration supports dynamic registration. If this is set to `true`
	 * the client supports the new `DeclarationRegistrationOptions` return value
	 * for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports additional metadata in the form of declaration links.
	 */
	Opt<bool> linkSupport = {};
};

/*
 * DefinitionClientCapabilities
 *
 * Client Capabilities for a {@link DefinitionRequest}.
 */
struct DefinitionClientCapabilities{
	/*
	 * Whether definition supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports additional metadata in the form of definition links.
	 * 
	 * @since 3.14.0
	 */
	Opt<bool> linkSupport = {};
};

/*
 * TypeDefinitionClientCapabilities
 *
 * Since 3.6.0
 */
struct TypeDefinitionClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `TypeDefinitionRegistrationOptions` return value
	 * for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports additional metadata in the form of definition links.
	 * 
	 * Since 3.14.0
	 */
	Opt<bool> linkSupport = {};
};

/*
 * ImplementationClientCapabilities
 *
 * @since 3.6.0
 */
struct ImplementationClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `ImplementationRegistrationOptions` return value
	 * for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports additional metadata in the form of definition links.
	 * 
	 * @since 3.14.0
	 */
	Opt<bool> linkSupport = {};
};

/*
 * ReferenceClientCapabilities
 *
 * Client Capabilities for a {@link ReferencesRequest}.
 */
struct ReferenceClientCapabilities{
	/*
	 * Whether references supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * DocumentHighlightClientCapabilities
 *
 * Client Capabilities for a {@link DocumentHighlightRequest}.
 */
struct DocumentHighlightClientCapabilities{
	/*
	 * Whether document highlight supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * DocumentSymbolClientCapabilitiesSymbolKind
 */
struct DocumentSymbolClientCapabilitiesSymbolKind{
	/*
	 * The symbol kind values the client supports. When this
	 * property exists the client also guarantees that it will
	 * handle values outside its set gracefully and falls back
	 * to a default value when unknown.
	 * 
	 * If this property is not present the client only supports
	 * the symbol kinds from `File` to `Array` as defined in
	 * the initial version of the protocol.
	 */
	Opt<Array<SymbolKindEnum>> valueSet = {};
};

/*
 * DocumentSymbolClientCapabilitiesTagSupport
 */
struct DocumentSymbolClientCapabilitiesTagSupport{
	/*
	 * The tags supported by the client.
	 */
	Array<SymbolTagEnum> valueSet;
};

/*
 * DocumentSymbolClientCapabilities
 *
 * Client Capabilities for a {@link DocumentSymbolRequest}.
 */
struct DocumentSymbolClientCapabilities{
	/*
	 * Whether document symbol supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Specific capabilities for the `SymbolKind` in the
	 * `textDocument/documentSymbol` request.
	 */
	Opt<DocumentSymbolClientCapabilitiesSymbolKind> symbolKind = {};
	/*
	 * The client supports hierarchical document symbols.
	 */
	Opt<bool> hierarchicalDocumentSymbolSupport = {};
	/*
	 * The client supports tags on `SymbolInformation`. Tags are supported on
	 * `DocumentSymbol` if `hierarchicalDocumentSymbolSupport` is set to true.
	 * Clients supporting tags have to handle unknown tags gracefully.
	 * 
	 * @since 3.16.0
	 */
	Opt<DocumentSymbolClientCapabilitiesTagSupport> tagSupport = {};
	/*
	 * The client supports an additional label presented in the UI when
	 * registering a document symbol provider.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> labelSupport = {};
};

/*
 * CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind
 */
struct CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind{
	/*
	 * The code action kind values the client supports. When this
	 * property exists the client also guarantees that it will
	 * handle values outside its set gracefully and falls back
	 * to a default value when unknown.
	 */
	Array<CodeActionKindEnum> valueSet;
};

/*
 * CodeActionClientCapabilitiesCodeActionLiteralSupport
 */
struct CodeActionClientCapabilitiesCodeActionLiteralSupport{
	/*
	 * The code action kind is support with the following value
	 * set.
	 */
	CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind codeActionKind;
};

/*
 * CodeActionClientCapabilitiesResolveSupport
 */
struct CodeActionClientCapabilitiesResolveSupport{
	/*
	 * The properties that a client can resolve lazily.
	 */
	Array<String> properties;
};

/*
 * CodeActionClientCapabilities
 *
 * The Client Capabilities of a {@link CodeActionRequest}.
 */
struct CodeActionClientCapabilities{
	/*
	 * Whether code action supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client support code action literals of type `CodeAction` as a valid
	 * response of the `textDocument/codeAction` request. If the property is not
	 * set the request can only return `Command` literals.
	 * 
	 * @since 3.8.0
	 */
	Opt<CodeActionClientCapabilitiesCodeActionLiteralSupport> codeActionLiteralSupport = {};
	/*
	 * Whether code action supports the `isPreferred` property.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> isPreferredSupport = {};
	/*
	 * Whether code action supports the `disabled` property.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> disabledSupport = {};
	/*
	 * Whether code action supports the `data` property which is
	 * preserved between a `textDocument/codeAction` and a
	 * `codeAction/resolve` request.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> dataSupport = {};
	/*
	 * Whether the client supports resolving additional code action
	 * properties via a separate `codeAction/resolve` request.
	 * 
	 * @since 3.16.0
	 */
	Opt<CodeActionClientCapabilitiesResolveSupport> resolveSupport = {};
	/*
	 * Whether the client honors the change annotations in
	 * text edits and resource operations returned via the
	 * `CodeAction#edit` property by for example presenting
	 * the workspace edit in the user interface and asking
	 * for confirmation.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> honorsChangeAnnotations = {};
};

/*
 * CodeLensClientCapabilities
 *
 * The client capabilities  of a {@link CodeLensRequest}.
 */
struct CodeLensClientCapabilities{
	/*
	 * Whether code lens supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * DocumentLinkClientCapabilities
 *
 * The client capabilities of a {@link DocumentLinkRequest}.
 */
struct DocumentLinkClientCapabilities{
	/*
	 * Whether document link supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Whether the client supports the `tooltip` property on `DocumentLink`.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> tooltipSupport = {};
};

/*
 * DocumentColorClientCapabilities
 */
struct DocumentColorClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `DocumentColorRegistrationOptions` return value
	 * for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * DocumentFormattingClientCapabilities
 *
 * Client capabilities of a {@link DocumentFormattingRequest}.
 */
struct DocumentFormattingClientCapabilities{
	/*
	 * Whether formatting supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * DocumentRangeFormattingClientCapabilities
 *
 * Client capabilities of a {@link DocumentRangeFormattingRequest}.
 */
struct DocumentRangeFormattingClientCapabilities{
	/*
	 * Whether range formatting supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Whether the client supports formatting multiple ranges at once.
	 * 
	 * @since 3.18.0
	 * @proposed
	 */
	Opt<bool> rangesSupport = {};
};

/*
 * DocumentOnTypeFormattingClientCapabilities
 *
 * Client capabilities of a {@link DocumentOnTypeFormattingRequest}.
 */
struct DocumentOnTypeFormattingClientCapabilities{
	/*
	 * Whether on type formatting supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * RenameClientCapabilities
 */
struct RenameClientCapabilities{
	/*
	 * Whether rename supports dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Client supports testing for validity of rename operations
	 * before execution.
	 * 
	 * @since 3.12.0
	 */
	Opt<bool> prepareSupport = {};
	/*
	 * Client supports the default behavior result.
	 * 
	 * The value indicates the default behavior used by the
	 * client.
	 * 
	 * @since 3.16.0
	 */
	Opt<PrepareSupportDefaultBehaviorEnum> prepareSupportDefaultBehavior = {};
	/*
	 * Whether the client honors the change annotations in
	 * text edits and resource operations returned via the
	 * rename request's workspace edit by for example presenting
	 * the workspace edit in the user interface and asking
	 * for confirmation.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> honorsChangeAnnotations = {};
};

/*
 * FoldingRangeClientCapabilitiesFoldingRangeKind
 */
struct FoldingRangeClientCapabilitiesFoldingRangeKind{
	/*
	 * The folding range kind values the client supports. When this
	 * property exists the client also guarantees that it will
	 * handle values outside its set gracefully and falls back
	 * to a default value when unknown.
	 */
	Opt<Array<FoldingRangeKindEnum>> valueSet = {};
};

/*
 * FoldingRangeClientCapabilitiesFoldingRange
 */
struct FoldingRangeClientCapabilitiesFoldingRange{
	/*
	 * If set, the client signals that it supports setting collapsedText on
	 * folding ranges to display custom labels instead of the default text.
	 * 
	 * @since 3.17.0
	 */
	Opt<bool> collapsedText = {};
};

/*
 * FoldingRangeClientCapabilities
 */
struct FoldingRangeClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration for folding range
	 * providers. If this is set to `true` the client supports the new
	 * `FoldingRangeRegistrationOptions` return value for the corresponding
	 * server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The maximum number of folding ranges that the client prefers to receive
	 * per document. The value serves as a hint, servers are free to follow the
	 * limit.
	 */
	Opt<uint> rangeLimit = {};
	/*
	 * If set, the client signals that it only supports folding complete lines.
	 * If set, client will ignore specified `startCharacter` and `endCharacter`
	 * properties in a FoldingRange.
	 */
	Opt<bool> lineFoldingOnly = {};
	/*
	 * Specific options for the folding range kind.
	 * 
	 * @since 3.17.0
	 */
	Opt<FoldingRangeClientCapabilitiesFoldingRangeKind> foldingRangeKind = {};
	/*
	 * Specific options for the folding range.
	 * 
	 * @since 3.17.0
	 */
	Opt<FoldingRangeClientCapabilitiesFoldingRange> foldingRange = {};
};

/*
 * SelectionRangeClientCapabilities
 */
struct SelectionRangeClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration for selection range providers. If this is set to `true`
	 * the client supports the new `SelectionRangeRegistrationOptions` return value for the corresponding server
	 * capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * PublishDiagnosticsClientCapabilitiesTagSupport
 */
struct PublishDiagnosticsClientCapabilitiesTagSupport{
	/*
	 * The tags supported by the client.
	 */
	Array<DiagnosticTagEnum> valueSet;
};

/*
 * PublishDiagnosticsClientCapabilities
 *
 * The publish diagnostic client capabilities.
 */
struct PublishDiagnosticsClientCapabilities{
	/*
	 * Whether the clients accepts diagnostics with related information.
	 */
	Opt<bool> relatedInformation = {};
	/*
	 * Client supports the tag property to provide meta data about a diagnostic.
	 * Clients supporting tags have to handle unknown tags gracefully.
	 * 
	 * @since 3.15.0
	 */
	Opt<PublishDiagnosticsClientCapabilitiesTagSupport> tagSupport = {};
	/*
	 * Whether the client interprets the version property of the
	 * `textDocument/publishDiagnostics` notification's parameter.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> versionSupport = {};
	/*
	 * Client supports a codeDescription property
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> codeDescriptionSupport = {};
	/*
	 * Whether code action supports the `data` property which is
	 * preserved between a `textDocument/publishDiagnostics` and
	 * `textDocument/codeAction` request.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> dataSupport = {};
};

/*
 * CallHierarchyClientCapabilities
 *
 * @since 3.16.0
 */
struct CallHierarchyClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
	 * return value for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * SemanticTokensClientCapabilitiesRequestsRange
 */
struct SemanticTokensClientCapabilitiesRequestsRange{
};

/*
 * SemanticTokensClientCapabilitiesRequestsFull
 */
struct SemanticTokensClientCapabilitiesRequestsFull{
	/*
	 * The client will send the `textDocument/semanticTokens/full/delta` request if
	 * the server provides a corresponding handler.
	 */
	Opt<bool> delta = {};
};

/*
 * SemanticTokensClientCapabilitiesRequests
 */
struct SemanticTokensClientCapabilitiesRequests{
	/*
	 * The client will send the `textDocument/semanticTokens/range` request if
	 * the server provides a corresponding handler.
	 */
	Opt<OneOf<bool, SemanticTokensClientCapabilitiesRequestsRange>> range = {};
	/*
	 * The client will send the `textDocument/semanticTokens/full` request if
	 * the server provides a corresponding handler.
	 */
	Opt<OneOf<bool, SemanticTokensClientCapabilitiesRequestsFull>> full = {};
};

/*
 * SemanticTokensClientCapabilities
 *
 * @since 3.16.0
 */
struct SemanticTokensClientCapabilities{
	/*
	 * Which requests the client supports and might send to the server
	 * depending on the server's capability. Please note that clients might not
	 * show semantic tokens or degrade some of the user experience if a range
	 * or full request is advertised by the client but not provided by the
	 * server. If for example the client capability `requests.full` and
	 * `request.range` are both set to true but the server only provides a
	 * range provider the client might not render a minimap correctly or might
	 * even decide to not show any semantic tokens at all.
	 */
	SemanticTokensClientCapabilitiesRequests requests;
	/*
	 * The token types that the client supports.
	 */
	Array<String> tokenTypes;
	/*
	 * The token modifiers that the client supports.
	 */
	Array<String> tokenModifiers;
	/*
	 * The token formats the clients supports.
	 */
	Array<TokenFormatEnum> formats;
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
	 * return value for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Whether the client supports tokens that can overlap each other.
	 */
	Opt<bool> overlappingTokenSupport = {};
	/*
	 * Whether the client supports tokens that can span multiple lines.
	 */
	Opt<bool> multilineTokenSupport = {};
	/*
	 * Whether the client allows the server to actively cancel a
	 * semantic token request, e.g. supports returning
	 * LSPErrorCodes.ServerCancelled. If a server does the client
	 * needs to retrigger the request.
	 * 
	 * @since 3.17.0
	 */
	Opt<bool> serverCancelSupport = {};
	/*
	 * Whether the client uses semantic tokens to augment existing
	 * syntax tokens. If set to `true` client side created syntax
	 * tokens and semantic tokens are both used for colorization. If
	 * set to `false` the client only uses the returned semantic tokens
	 * for colorization.
	 * 
	 * If the value is `undefined` then the client behavior is not
	 * specified.
	 * 
	 * @since 3.17.0
	 */
	Opt<bool> augmentsSyntaxTokens = {};
};

/*
 * LinkedEditingRangeClientCapabilities
 *
 * Client capabilities for the linked editing range request.
 * 
 * @since 3.16.0
 */
struct LinkedEditingRangeClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
	 * return value for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * MonikerClientCapabilities
 *
 * Client capabilities specific to the moniker request.
 * 
 * @since 3.16.0
 */
struct MonikerClientCapabilities{
	/*
	 * Whether moniker supports dynamic registration. If this is set to `true`
	 * the client supports the new `MonikerRegistrationOptions` return value
	 * for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * TypeHierarchyClientCapabilities
 *
 * @since 3.17.0
 */
struct TypeHierarchyClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
	 * return value for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * InlineValueClientCapabilities
 *
 * Client capabilities specific to inline values.
 * 
 * @since 3.17.0
 */
struct InlineValueClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration for inline value providers.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * InlayHintClientCapabilitiesResolveSupport
 */
struct InlayHintClientCapabilitiesResolveSupport{
	/*
	 * The properties that a client can resolve lazily.
	 */
	Array<String> properties;
};

/*
 * InlayHintClientCapabilities
 *
 * Inlay hint client capabilities.
 * 
 * @since 3.17.0
 */
struct InlayHintClientCapabilities{
	/*
	 * Whether inlay hints support dynamic registration.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Indicates which properties a client can resolve lazily on an inlay
	 * hint.
	 */
	Opt<InlayHintClientCapabilitiesResolveSupport> resolveSupport = {};
};

/*
 * DiagnosticClientCapabilities
 *
 * Client capabilities specific to diagnostic pull requests.
 * 
 * @since 3.17.0
 */
struct DiagnosticClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is set to `true`
	 * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
	 * return value for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * Whether the clients supports related documents for document diagnostic pulls.
	 */
	Opt<bool> relatedDocumentSupport = {};
};

/*
 * InlineCompletionClientCapabilities
 *
 * Client capabilities specific to inline completions.
 * 
 * @since 3.18.0
 * @proposed
 */
struct InlineCompletionClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration for inline completion providers.
	 */
	Opt<bool> dynamicRegistration = {};
};

/*
 * TextDocumentClientCapabilities
 *
 * Text document specific client capabilities.
 */
struct TextDocumentClientCapabilities{
	/*
	 * Defines which synchronization capabilities the client supports.
	 */
	Opt<TextDocumentSyncClientCapabilities> synchronization = {};
	/*
	 * Capabilities specific to the `textDocument/completion` request.
	 */
	Opt<CompletionClientCapabilities> completion = {};
	/*
	 * Capabilities specific to the `textDocument/hover` request.
	 */
	Opt<HoverClientCapabilities> hover = {};
	/*
	 * Capabilities specific to the `textDocument/signatureHelp` request.
	 */
	Opt<SignatureHelpClientCapabilities> signatureHelp = {};
	/*
	 * Capabilities specific to the `textDocument/declaration` request.
	 * 
	 * @since 3.14.0
	 */
	Opt<DeclarationClientCapabilities> declaration = {};
	/*
	 * Capabilities specific to the `textDocument/definition` request.
	 */
	Opt<DefinitionClientCapabilities> definition = {};
	/*
	 * Capabilities specific to the `textDocument/typeDefinition` request.
	 * 
	 * @since 3.6.0
	 */
	Opt<TypeDefinitionClientCapabilities> typeDefinition = {};
	/*
	 * Capabilities specific to the `textDocument/implementation` request.
	 * 
	 * @since 3.6.0
	 */
	Opt<ImplementationClientCapabilities> implementation = {};
	/*
	 * Capabilities specific to the `textDocument/references` request.
	 */
	Opt<ReferenceClientCapabilities> references = {};
	/*
	 * Capabilities specific to the `textDocument/documentHighlight` request.
	 */
	Opt<DocumentHighlightClientCapabilities> documentHighlight = {};
	/*
	 * Capabilities specific to the `textDocument/documentSymbol` request.
	 */
	Opt<DocumentSymbolClientCapabilities> documentSymbol = {};
	/*
	 * Capabilities specific to the `textDocument/codeAction` request.
	 */
	Opt<CodeActionClientCapabilities> codeAction = {};
	/*
	 * Capabilities specific to the `textDocument/codeLens` request.
	 */
	Opt<CodeLensClientCapabilities> codeLens = {};
	/*
	 * Capabilities specific to the `textDocument/documentLink` request.
	 */
	Opt<DocumentLinkClientCapabilities> documentLink = {};
	/*
	 * Capabilities specific to the `textDocument/documentColor` and the
	 * `textDocument/colorPresentation` request.
	 * 
	 * @since 3.6.0
	 */
	Opt<DocumentColorClientCapabilities> colorProvider = {};
	/*
	 * Capabilities specific to the `textDocument/formatting` request.
	 */
	Opt<DocumentFormattingClientCapabilities> formatting = {};
	/*
	 * Capabilities specific to the `textDocument/rangeFormatting` request.
	 */
	Opt<DocumentRangeFormattingClientCapabilities> rangeFormatting = {};
	/*
	 * Capabilities specific to the `textDocument/onTypeFormatting` request.
	 */
	Opt<DocumentOnTypeFormattingClientCapabilities> onTypeFormatting = {};
	/*
	 * Capabilities specific to the `textDocument/rename` request.
	 */
	Opt<RenameClientCapabilities> rename = {};
	/*
	 * Capabilities specific to the `textDocument/foldingRange` request.
	 * 
	 * @since 3.10.0
	 */
	Opt<FoldingRangeClientCapabilities> foldingRange = {};
	/*
	 * Capabilities specific to the `textDocument/selectionRange` request.
	 * 
	 * @since 3.15.0
	 */
	Opt<SelectionRangeClientCapabilities> selectionRange = {};
	/*
	 * Capabilities specific to the `textDocument/publishDiagnostics` notification.
	 */
	Opt<PublishDiagnosticsClientCapabilities> publishDiagnostics = {};
	/*
	 * Capabilities specific to the various call hierarchy requests.
	 * 
	 * @since 3.16.0
	 */
	Opt<CallHierarchyClientCapabilities> callHierarchy = {};
	/*
	 * Capabilities specific to the various semantic token request.
	 * 
	 * @since 3.16.0
	 */
	Opt<SemanticTokensClientCapabilities> semanticTokens = {};
	/*
	 * Capabilities specific to the `textDocument/linkedEditingRange` request.
	 * 
	 * @since 3.16.0
	 */
	Opt<LinkedEditingRangeClientCapabilities> linkedEditingRange = {};
	/*
	 * Client capabilities specific to the `textDocument/moniker` request.
	 * 
	 * @since 3.16.0
	 */
	Opt<MonikerClientCapabilities> moniker = {};
	/*
	 * Capabilities specific to the various type hierarchy requests.
	 * 
	 * @since 3.17.0
	 */
	Opt<TypeHierarchyClientCapabilities> typeHierarchy = {};
	/*
	 * Capabilities specific to the `textDocument/inlineValue` request.
	 * 
	 * @since 3.17.0
	 */
	Opt<InlineValueClientCapabilities> inlineValue = {};
	/*
	 * Capabilities specific to the `textDocument/inlayHint` request.
	 * 
	 * @since 3.17.0
	 */
	Opt<InlayHintClientCapabilities> inlayHint = {};
	/*
	 * Capabilities specific to the diagnostic pull model.
	 * 
	 * @since 3.17.0
	 */
	Opt<DiagnosticClientCapabilities> diagnostic = {};
	/*
	 * Client capabilities specific to inline completions.
	 * 
	 * @since 3.18.0
	 * @proposed
	 */
	Opt<InlineCompletionClientCapabilities> inlineCompletion = {};
};

/*
 * NotebookDocumentSyncClientCapabilities
 *
 * Notebook specific client capabilities.
 * 
 * @since 3.17.0
 */
struct NotebookDocumentSyncClientCapabilities{
	/*
	 * Whether implementation supports dynamic registration. If this is
	 * set to `true` the client supports the new
	 * `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
	 * return value for the corresponding server capability as well.
	 */
	Opt<bool> dynamicRegistration = {};
	/*
	 * The client supports sending execution summary data per cell.
	 */
	Opt<bool> executionSummarySupport = {};
};

/*
 * NotebookDocumentClientCapabilities
 *
 * Capabilities specific to the notebook document support.
 * 
 * @since 3.17.0
 */
struct NotebookDocumentClientCapabilities{
	/*
	 * Capabilities specific to notebook document synchronization
	 * 
	 * @since 3.17.0
	 */
	NotebookDocumentSyncClientCapabilities synchronization;
};

/*
 * ShowMessageRequestClientCapabilitiesMessageActionItem
 */
struct ShowMessageRequestClientCapabilitiesMessageActionItem{
	/*
	 * Whether the client supports additional attributes which
	 * are preserved and send back to the server in the
	 * request's response.
	 */
	Opt<bool> additionalPropertiesSupport = {};
};

/*
 * ShowMessageRequestClientCapabilities
 *
 * Show message request client capabilities
 */
struct ShowMessageRequestClientCapabilities{
	/*
	 * Capabilities specific to the `MessageActionItem` type.
	 */
	Opt<ShowMessageRequestClientCapabilitiesMessageActionItem> messageActionItem = {};
};

/*
 * ShowDocumentClientCapabilities
 *
 * Client capabilities for the showDocument request.
 * 
 * @since 3.16.0
 */
struct ShowDocumentClientCapabilities{
	/*
	 * The client has support for the showDocument
	 * request.
	 */
	bool support;
};

/*
 * WindowClientCapabilities
 */
struct WindowClientCapabilities{
	/*
	 * It indicates whether the client supports server initiated
	 * progress using the `window/workDoneProgress/create` request.
	 * 
	 * The capability also controls Whether client supports handling
	 * of progress notifications. If set servers are allowed to report a
	 * `workDoneProgress` property in the request specific server
	 * capabilities.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> workDoneProgress = {};
	/*
	 * Capabilities specific to the showMessage request.
	 * 
	 * @since 3.16.0
	 */
	Opt<ShowMessageRequestClientCapabilities> showMessage = {};
	/*
	 * Capabilities specific to the showDocument request.
	 * 
	 * @since 3.16.0
	 */
	Opt<ShowDocumentClientCapabilities> showDocument = {};
};

/*
 * GeneralClientCapabilitiesStaleRequestSupport
 */
struct GeneralClientCapabilitiesStaleRequestSupport{
	/*
	 * The client will actively cancel the request.
	 */
	bool cancel;
	/*
	 * The list of requests for which the client
	 * will retry the request if it receives a
	 * response with error code `ContentModified`
	 */
	Array<String> retryOnContentModified;
};

/*
 * RegularExpressionsClientCapabilities
 *
 * Client capabilities specific to regular expressions.
 * 
 * @since 3.16.0
 */
struct RegularExpressionsClientCapabilities{
	/*
	 * The engine's name.
	 */
	String engine;
	/*
	 * The engine's version.
	 */
	Opt<String> version = {};
};

/*
 * MarkdownClientCapabilities
 *
 * Client capabilities specific to the used markdown parser.
 * 
 * @since 3.16.0
 */
struct MarkdownClientCapabilities{
	/*
	 * The name of the parser.
	 */
	String parser;
	/*
	 * The version of the parser.
	 */
	Opt<String> version = {};
	/*
	 * A list of HTML tags that the client allows / supports in
	 * Markdown.
	 * 
	 * @since 3.17.0
	 */
	Opt<Array<String>> allowedTags = {};
};

/*
 * GeneralClientCapabilities
 *
 * General client capabilities.
 * 
 * @since 3.16.0
 */
struct GeneralClientCapabilities{
	/*
	 * Client capability that signals how the client
	 * handles stale requests (e.g. a request
	 * for which the client will not process the response
	 * anymore since the information is outdated).
	 * 
	 * @since 3.17.0
	 */
	Opt<GeneralClientCapabilitiesStaleRequestSupport> staleRequestSupport = {};
	/*
	 * Client capabilities specific to regular expressions.
	 * 
	 * @since 3.16.0
	 */
	Opt<RegularExpressionsClientCapabilities> regularExpressions = {};
	/*
	 * Client capabilities specific to the client's markdown parser.
	 * 
	 * @since 3.16.0
	 */
	Opt<MarkdownClientCapabilities> markdown = {};
	/*
	 * The position encodings supported by the client. Client and server
	 * have to agree on the same position encoding to ensure that offsets
	 * (e.g. character position in a line) are interpreted the same on both
	 * sides.
	 * 
	 * To keep the protocol backwards compatible the following applies: if
	 * the value 'utf-16' is missing from the array of position encodings
	 * servers can assume that the client supports UTF-16. UTF-16 is
	 * therefore a mandatory encoding.
	 * 
	 * If omitted it defaults to ['utf-16'].
	 * 
	 * Implementation considerations: since the conversion from one encoding
	 * into another requires the content of the file / line the conversion
	 * is best done where the file is read which is usually on the server
	 * side.
	 * 
	 * @since 3.17.0
	 */
	Opt<Array<PositionEncodingKindEnum>> positionEncodings = {};
};

/*
 * ClientCapabilities
 *
 * Defines the capabilities provided by the client.
 */
struct ClientCapabilities{
	/*
	 * Workspace specific client capabilities.
	 */
	Opt<WorkspaceClientCapabilities> workspace = {};
	/*
	 * Text document specific client capabilities.
	 */
	Opt<TextDocumentClientCapabilities> textDocument = {};
	/*
	 * Capabilities specific to the notebook document support.
	 * 
	 * @since 3.17.0
	 */
	Opt<NotebookDocumentClientCapabilities> notebookDocument = {};
	/*
	 * Window specific client capabilities.
	 */
	Opt<WindowClientCapabilities> window = {};
	/*
	 * General client capabilities.
	 * 
	 * @since 3.16.0
	 */
	Opt<GeneralClientCapabilities> general = {};
	/*
	 * Experimental client capabilities.
	 */
	Opt<LSPAny> experimental = {};
};

/*
 * _InitializeParamsClientInfo
 */
struct _InitializeParamsClientInfo{
	/*
	 * The name of the client as defined by the client.
	 */
	String name;
	/*
	 * The client's version as defined by the client.
	 */
	Opt<String> version = {};
};

/*
 * _InitializeParams
 *
 * The initialize parameters
 */
struct _InitializeParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The process Id of the parent process that started
	 * the server.
	 * 
	 * Is `null` if the process has not been started by another process.
	 * If the parent process is not alive then the server should exit.
	 */
	NullOr<int> processId;
	/*
	 * The rootUri of the workspace. Is null if no
	 * folder is open. If both `rootPath` and `rootUri` are set
	 * `rootUri` wins.
	 * 
	 * @deprecated in favour of workspaceFolders.
	 */
	NullOr<DocumentUri> rootUri;
	/*
	 * The capabilities provided by the client (editor or tool)
	 */
	ClientCapabilities capabilities;
	/*
	 * Information about the client
	 * 
	 * @since 3.15.0
	 */
	Opt<_InitializeParamsClientInfo> clientInfo = {};
	/*
	 * The locale the client is currently showing the user interface
	 * in. This must not necessarily be the locale of the operating
	 * system.
	 * 
	 * Uses IETF language tags as the value's syntax
	 * (See https://en.wikipedia.org/wiki/IETF_language_tag)
	 * 
	 * @since 3.16.0
	 */
	Opt<String> locale = {};
	/*
	 * The rootPath of the workspace. Is null
	 * if no folder is open.
	 * 
	 * @deprecated in favour of rootUri.
	 */
	Opt<NullOr<String>> rootPath = {};
	/*
	 * User provided initialization options.
	 */
	Opt<LSPAny> initializationOptions = {};
	/*
	 * The initial trace setting. If omitted trace is disabled ('off').
	 */
	Opt<TraceValuesEnum> trace = {};
};

/*
 * WorkspaceFoldersInitializeParams
 */
struct WorkspaceFoldersInitializeParams{
	/*
	 * The workspace folders configured in the client when the server starts.
	 * 
	 * This property is only available if the client supports workspace folders.
	 * It can be `null` if the client supports workspace folders but none are
	 * configured.
	 * 
	 * @since 3.6.0
	 */
	Opt<NullOr<Array<WorkspaceFolder>>> workspaceFolders = {};
};

/*
 * InitializeParams
 */
struct InitializeParams : _InitializeParams, WorkspaceFoldersInitializeParams{
};

/*
 * SaveOptions
 *
 * Save options.
 */
struct SaveOptions{
	/*
	 * The client is supposed to include the content on save.
	 */
	Opt<bool> includeText = {};
};

/*
 * TextDocumentSyncOptions
 */
struct TextDocumentSyncOptions{
	/*
	 * Open and close notifications are sent to the server. If omitted open close notification should not
	 * be sent.
	 */
	Opt<bool> openClose = {};
	/*
	 * Change notifications are sent to the server. See TextDocumentSyncKind.None, TextDocumentSyncKind.Full
	 * and TextDocumentSyncKind.Incremental. If omitted it defaults to TextDocumentSyncKind.None.
	 */
	Opt<TextDocumentSyncKindEnum> change = {};
	/*
	 * If present will save notifications are sent to the server. If omitted the notification should not be
	 * sent.
	 */
	Opt<bool> willSave = {};
	/*
	 * If present will save wait until requests are sent to the server. If omitted the request should not be
	 * sent.
	 */
	Opt<bool> willSaveWaitUntil = {};
	/*
	 * If present save notifications are sent to the server. If omitted the notification should not be
	 * sent.
	 */
	Opt<OneOf<bool, SaveOptions>> save = {};
};

/*
 * NotebookDocumentSyncOptionsNotebookSelector_NotebookCells
 */
struct NotebookDocumentSyncOptionsNotebookSelector_NotebookCells{
	String language;
};

/*
 * NotebookDocumentSyncOptionsNotebookSelector_Notebook
 */
struct NotebookDocumentSyncOptionsNotebookSelector_Notebook{
	/*
	 * The notebook to be synced If a string
	 * value is provided it matches against the
	 * notebook type. '*' matches every notebook.
	 */
	OneOf<String, NotebookDocumentFilter> notebook;
	/*
	 * The cells of the matching notebook to be synced.
	 */
	Opt<Array<NotebookDocumentSyncOptionsNotebookSelector_NotebookCells>> cells = {};
};

/*
 * NotebookDocumentSyncOptionsNotebookSelector_CellsCells
 */
struct NotebookDocumentSyncOptionsNotebookSelector_CellsCells{
	String language;
};

/*
 * NotebookDocumentSyncOptionsNotebookSelector_Cells
 */
struct NotebookDocumentSyncOptionsNotebookSelector_Cells{
	/*
	 * The cells of the matching notebook to be synced.
	 */
	Array<NotebookDocumentSyncOptionsNotebookSelector_CellsCells> cells;
	/*
	 * The notebook to be synced If a string
	 * value is provided it matches against the
	 * notebook type. '*' matches every notebook.
	 */
	Opt<OneOf<String, NotebookDocumentFilter>> notebook = {};
};

/*
 * NotebookDocumentSyncOptions
 *
 * Options specific to a notebook plus its cells
 * to be synced to the server.
 * 
 * If a selector provides a notebook document
 * filter but no cell selector all cells of a
 * matching notebook document will be synced.
 * 
 * If a selector provides no notebook document
 * filter but only a cell selector all notebook
 * document that contain at least one matching
 * cell will be synced.
 * 
 * @since 3.17.0
 */
struct NotebookDocumentSyncOptions{
	/*
	 * The notebooks to be synced
	 */
	Array<OneOf<NotebookDocumentSyncOptionsNotebookSelector_Notebook, NotebookDocumentSyncOptionsNotebookSelector_Cells>> notebookSelector;
	/*
	 * Whether save notification should be forwarded to
	 * the server. Will only be honored if mode === `notebook`.
	 */
	Opt<bool> save = {};
};

/*
 * NotebookDocumentSyncRegistrationOptions
 *
 * Registration options specific to a notebook.
 * 
 * @since 3.17.0
 */
struct NotebookDocumentSyncRegistrationOptions : NotebookDocumentSyncOptions{
	/*
	 * The id used to register the request. The id can be used to deregister
	 * the request again. See also Registration#id.
	 */
	Opt<String> id = {};
};

/*
 * CompletionOptionsCompletionItem
 */
struct CompletionOptionsCompletionItem{
	/*
	 * The server has support for completion item label
	 * details (see also `CompletionItemLabelDetails`) when
	 * receiving a completion item in a resolve call.
	 * 
	 * @since 3.17.0
	 */
	Opt<bool> labelDetailsSupport = {};
};

/*
 * CompletionOptions
 *
 * Completion options.
 */
struct CompletionOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * Most tools trigger completion request automatically without explicitly requesting
	 * it using a keyboard shortcut (e.g. Ctrl+Space). Typically they do so when the user
	 * starts to type an identifier. For example if the user types `c` in a JavaScript file
	 * code complete will automatically pop up present `console` besides others as a
	 * completion item. Characters that make up identifiers don't need to be listed here.
	 * 
	 * If code complete should automatically be trigger on characters not being valid inside
	 * an identifier (for example `.` in JavaScript) list them in `triggerCharacters`.
	 */
	Opt<Array<String>> triggerCharacters = {};
	/*
	 * The list of all possible characters that commit a completion. This field can be used
	 * if clients don't support individual commit characters per completion item. See
	 * `ClientCapabilities.textDocument.completion.completionItem.commitCharactersSupport`
	 * 
	 * If a server provides both `allCommitCharacters` and commit characters on an individual
	 * completion item the ones on the completion item win.
	 * 
	 * @since 3.2.0
	 */
	Opt<Array<String>> allCommitCharacters = {};
	/*
	 * The server provides support to resolve additional
	 * information for a completion item.
	 */
	Opt<bool> resolveProvider = {};
	/*
	 * The server supports the following `CompletionItem` specific
	 * capabilities.
	 * 
	 * @since 3.17.0
	 */
	Opt<CompletionOptionsCompletionItem> completionItem = {};
};

/*
 * HoverOptions
 *
 * Hover options.
 */
struct HoverOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * SignatureHelpOptions
 *
 * Server Capabilities for a {@link SignatureHelpRequest}.
 */
struct SignatureHelpOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * List of characters that trigger signature help automatically.
	 */
	Opt<Array<String>> triggerCharacters = {};
	/*
	 * List of characters that re-trigger signature help.
	 * 
	 * These trigger characters are only active when signature help is already showing. All trigger characters
	 * are also counted as re-trigger characters.
	 * 
	 * @since 3.15.0
	 */
	Opt<Array<String>> retriggerCharacters = {};
};

/*
 * DefinitionOptions
 *
 * Server Capabilities for a {@link DefinitionRequest}.
 */
struct DefinitionOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * ReferenceOptions
 *
 * Reference options.
 */
struct ReferenceOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * DocumentHighlightOptions
 *
 * Provider options for a {@link DocumentHighlightRequest}.
 */
struct DocumentHighlightOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * DocumentSymbolOptions
 *
 * Provider options for a {@link DocumentSymbolRequest}.
 */
struct DocumentSymbolOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * A human-readable string that is shown when multiple outlines trees
	 * are shown for the same document.
	 * 
	 * @since 3.16.0
	 */
	Opt<String> label = {};
};

/*
 * CodeActionOptions
 *
 * Provider options for a {@link CodeActionRequest}.
 */
struct CodeActionOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * CodeActionKinds that this server may return.
	 * 
	 * The list of kinds may be generic, such as `CodeActionKind.Refactor`, or the server
	 * may list out every specific kind they provide.
	 */
	Opt<Array<CodeActionKindEnum>> codeActionKinds = {};
	/*
	 * The server provides support to resolve additional
	 * information for a code action.
	 * 
	 * @since 3.16.0
	 */
	Opt<bool> resolveProvider = {};
};

/*
 * CodeLensOptions
 *
 * Code Lens provider options of a {@link CodeLensRequest}.
 */
struct CodeLensOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * Code lens has a resolve provider as well.
	 */
	Opt<bool> resolveProvider = {};
};

/*
 * DocumentLinkOptions
 *
 * Provider options for a {@link DocumentLinkRequest}.
 */
struct DocumentLinkOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * Document links have a resolve provider as well.
	 */
	Opt<bool> resolveProvider = {};
};

/*
 * WorkspaceSymbolOptions
 *
 * Server capabilities for a {@link WorkspaceSymbolRequest}.
 */
struct WorkspaceSymbolOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * The server provides support to resolve additional
	 * information for a workspace symbol.
	 * 
	 * @since 3.17.0
	 */
	Opt<bool> resolveProvider = {};
};

/*
 * DocumentFormattingOptions
 *
 * Provider options for a {@link DocumentFormattingRequest}.
 */
struct DocumentFormattingOptions{
	Opt<bool> workDoneProgress = {};
};

/*
 * DocumentRangeFormattingOptions
 *
 * Provider options for a {@link DocumentRangeFormattingRequest}.
 */
struct DocumentRangeFormattingOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * Whether the server supports formatting multiple ranges at once.
	 * 
	 * @since 3.18.0
	 * @proposed
	 */
	Opt<bool> rangesSupport = {};
};

/*
 * DocumentOnTypeFormattingOptions
 *
 * Provider options for a {@link DocumentOnTypeFormattingRequest}.
 */
struct DocumentOnTypeFormattingOptions{
	/*
	 * A character on which formatting should be triggered, like `{`.
	 */
	String firstTriggerCharacter;
	/*
	 * More trigger characters.
	 */
	Opt<Array<String>> moreTriggerCharacter = {};
};

/*
 * RenameOptions
 *
 * Provider options for a {@link RenameRequest}.
 */
struct RenameOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * Renames should be checked and tested before being executed.
	 * 
	 * @since version 3.12.0
	 */
	Opt<bool> prepareProvider = {};
};

/*
 * ExecuteCommandOptions
 *
 * The server capabilities of a {@link ExecuteCommandRequest}.
 */
struct ExecuteCommandOptions{
	Opt<bool> workDoneProgress = {};
	/*
	 * The commands to be executed on the server
	 */
	Array<String> commands;
};

/*
 * WorkspaceFoldersServerCapabilities
 */
struct WorkspaceFoldersServerCapabilities{
	/*
	 * The server has support for workspace folders
	 */
	Opt<bool> supported = {};
	/*
	 * Whether the server wants to receive workspace folder
	 * change notifications.
	 * 
	 * If a string is provided the string is treated as an ID
	 * under which the notification is registered on the client
	 * side. The ID can be used to unregister for these events
	 * using the `client/unregisterCapability` request.
	 */
	Opt<OneOf<String, bool>> changeNotifications = {};
};

/*
 * FileOperationOptions
 *
 * Options for notifications/requests for user operations on files.
 * 
 * @since 3.16.0
 */
struct FileOperationOptions{
	/*
	 * The server is interested in receiving didCreateFiles notifications.
	 */
	Opt<FileOperationRegistrationOptions> didCreate = {};
	/*
	 * The server is interested in receiving willCreateFiles requests.
	 */
	Opt<FileOperationRegistrationOptions> willCreate = {};
	/*
	 * The server is interested in receiving didRenameFiles notifications.
	 */
	Opt<FileOperationRegistrationOptions> didRename = {};
	/*
	 * The server is interested in receiving willRenameFiles requests.
	 */
	Opt<FileOperationRegistrationOptions> willRename = {};
	/*
	 * The server is interested in receiving didDeleteFiles file notifications.
	 */
	Opt<FileOperationRegistrationOptions> didDelete = {};
	/*
	 * The server is interested in receiving willDeleteFiles file requests.
	 */
	Opt<FileOperationRegistrationOptions> willDelete = {};
};

/*
 * ServerCapabilitiesWorkspace
 */
struct ServerCapabilitiesWorkspace{
	/*
	 * The server supports workspace folder.
	 * 
	 * @since 3.6.0
	 */
	Opt<WorkspaceFoldersServerCapabilities> workspaceFolders = {};
	/*
	 * The server is interested in notifications/requests for operations on files.
	 * 
	 * @since 3.16.0
	 */
	Opt<FileOperationOptions> fileOperations = {};
};

/*
 * ServerCapabilities
 *
 * Defines the capabilities provided by a language
 * server.
 */
struct ServerCapabilities{
	/*
	 * The position encoding the server picked from the encodings offered
	 * by the client via the client capability `general.positionEncodings`.
	 * 
	 * If the client didn't provide any position encodings the only valid
	 * value that a server can return is 'utf-16'.
	 * 
	 * If omitted it defaults to 'utf-16'.
	 * 
	 * @since 3.17.0
	 */
	Opt<PositionEncodingKindEnum> positionEncoding = {};
	/*
	 * Defines how text documents are synced. Is either a detailed structure
	 * defining each notification or for backwards compatibility the
	 * TextDocumentSyncKind number.
	 */
	Opt<OneOf<TextDocumentSyncOptions, TextDocumentSyncKindEnum>> textDocumentSync = {};
	/*
	 * Defines how notebook documents are synced.
	 * 
	 * @since 3.17.0
	 */
	Opt<OneOf<NotebookDocumentSyncOptions, NotebookDocumentSyncRegistrationOptions>> notebookDocumentSync = {};
	/*
	 * The server provides completion support.
	 */
	Opt<CompletionOptions> completionProvider = {};
	/*
	 * The server provides hover support.
	 */
	Opt<OneOf<bool, HoverOptions>> hoverProvider = {};
	/*
	 * The server provides signature help support.
	 */
	Opt<SignatureHelpOptions> signatureHelpProvider = {};
	/*
	 * The server provides Goto Declaration support.
	 */
	Opt<OneOf<bool, DeclarationOptions, DeclarationRegistrationOptions>> declarationProvider = {};
	/*
	 * The server provides goto definition support.
	 */
	Opt<OneOf<bool, DefinitionOptions>> definitionProvider = {};
	/*
	 * The server provides Goto Type Definition support.
	 */
	Opt<OneOf<bool, TypeDefinitionOptions, TypeDefinitionRegistrationOptions>> typeDefinitionProvider = {};
	/*
	 * The server provides Goto Implementation support.
	 */
	Opt<OneOf<bool, ImplementationOptions, ImplementationRegistrationOptions>> implementationProvider = {};
	/*
	 * The server provides find references support.
	 */
	Opt<OneOf<bool, ReferenceOptions>> referencesProvider = {};
	/*
	 * The server provides document highlight support.
	 */
	Opt<OneOf<bool, DocumentHighlightOptions>> documentHighlightProvider = {};
	/*
	 * The server provides document symbol support.
	 */
	Opt<OneOf<bool, DocumentSymbolOptions>> documentSymbolProvider = {};
	/*
	 * The server provides code actions. CodeActionOptions may only be
	 * specified if the client states that it supports
	 * `codeActionLiteralSupport` in its initial `initialize` request.
	 */
	Opt<OneOf<bool, CodeActionOptions>> codeActionProvider = {};
	/*
	 * The server provides code lens.
	 */
	Opt<CodeLensOptions> codeLensProvider = {};
	/*
	 * The server provides document link support.
	 */
	Opt<DocumentLinkOptions> documentLinkProvider = {};
	/*
	 * The server provides color provider support.
	 */
	Opt<OneOf<bool, DocumentColorOptions, DocumentColorRegistrationOptions>> colorProvider = {};
	/*
	 * The server provides workspace symbol support.
	 */
	Opt<OneOf<bool, WorkspaceSymbolOptions>> workspaceSymbolProvider = {};
	/*
	 * The server provides document formatting.
	 */
	Opt<OneOf<bool, DocumentFormattingOptions>> documentFormattingProvider = {};
	/*
	 * The server provides document range formatting.
	 */
	Opt<OneOf<bool, DocumentRangeFormattingOptions>> documentRangeFormattingProvider = {};
	/*
	 * The server provides document formatting on typing.
	 */
	Opt<DocumentOnTypeFormattingOptions> documentOnTypeFormattingProvider = {};
	/*
	 * The server provides rename support. RenameOptions may only be
	 * specified if the client states that it supports
	 * `prepareSupport` in its initial `initialize` request.
	 */
	Opt<OneOf<bool, RenameOptions>> renameProvider = {};
	/*
	 * The server provides folding provider support.
	 */
	Opt<OneOf<bool, FoldingRangeOptions, FoldingRangeRegistrationOptions>> foldingRangeProvider = {};
	/*
	 * The server provides selection range support.
	 */
	Opt<OneOf<bool, SelectionRangeOptions, SelectionRangeRegistrationOptions>> selectionRangeProvider = {};
	/*
	 * The server provides execute command support.
	 */
	Opt<ExecuteCommandOptions> executeCommandProvider = {};
	/*
	 * The server provides call hierarchy support.
	 * 
	 * @since 3.16.0
	 */
	Opt<OneOf<bool, CallHierarchyOptions, CallHierarchyRegistrationOptions>> callHierarchyProvider = {};
	/*
	 * The server provides linked editing range support.
	 * 
	 * @since 3.16.0
	 */
	Opt<OneOf<bool, LinkedEditingRangeOptions, LinkedEditingRangeRegistrationOptions>> linkedEditingRangeProvider = {};
	/*
	 * The server provides semantic tokens support.
	 * 
	 * @since 3.16.0
	 */
	Opt<OneOf<SemanticTokensOptions, SemanticTokensRegistrationOptions>> semanticTokensProvider = {};
	/*
	 * The server provides moniker support.
	 * 
	 * @since 3.16.0
	 */
	Opt<OneOf<bool, MonikerOptions, MonikerRegistrationOptions>> monikerProvider = {};
	/*
	 * The server provides type hierarchy support.
	 * 
	 * @since 3.17.0
	 */
	Opt<OneOf<bool, TypeHierarchyOptions, TypeHierarchyRegistrationOptions>> typeHierarchyProvider = {};
	/*
	 * The server provides inline values.
	 * 
	 * @since 3.17.0
	 */
	Opt<OneOf<bool, InlineValueOptions, InlineValueRegistrationOptions>> inlineValueProvider = {};
	/*
	 * The server provides inlay hints.
	 * 
	 * @since 3.17.0
	 */
	Opt<OneOf<bool, InlayHintOptions, InlayHintRegistrationOptions>> inlayHintProvider = {};
	/*
	 * The server has support for pull model diagnostics.
	 * 
	 * @since 3.17.0
	 */
	Opt<OneOf<DiagnosticOptions, DiagnosticRegistrationOptions>> diagnosticProvider = {};
	/*
	 * Inline completion options used during static registration.
	 * 
	 * @since 3.18.0
	 * @proposed
	 */
	Opt<OneOf<bool, InlineCompletionOptions>> inlineCompletionProvider = {};
	/*
	 * Workspace specific server capabilities.
	 */
	Opt<ServerCapabilitiesWorkspace> workspace = {};
	/*
	 * Experimental server capabilities.
	 */
	Opt<LSPAny> experimental = {};
};

/*
 * InitializeResultServerInfo
 */
struct InitializeResultServerInfo{
	/*
	 * The name of the server as defined by the server.
	 */
	String name;
	/*
	 * The server's version as defined by the server.
	 */
	Opt<String> version = {};
};

/*
 * InitializeResult
 *
 * The result returned from an initialize request.
 */
struct InitializeResult{
	/*
	 * The capabilities the language server provides.
	 */
	ServerCapabilities capabilities;
	/*
	 * Information about the server.
	 * 
	 * @since 3.15.0
	 */
	Opt<InitializeResultServerInfo> serverInfo = {};
};

/*
 * InitializeError
 *
 * The data type of the ResponseError if the
 * initialize request fails.
 */
struct InitializeError{
	/*
	 * Indicates whether the client execute the following retry logic:
	 * (1) show the message provided by the ResponseError to the user
	 * (2) user selects retry or cancel
	 * (3) if user selected retry the initialize method is sent again.
	 */
	bool retry;
};

/*
 * InitializedParams
 */
struct InitializedParams{
};

/*
 * DidChangeConfigurationParams
 *
 * The parameters of a change configuration notification.
 */
struct DidChangeConfigurationParams{
	/*
	 * The actual changed settings
	 */
	LSPAny settings;
};

/*
 * DidChangeConfigurationRegistrationOptions
 */
struct DidChangeConfigurationRegistrationOptions{
	Opt<OneOf<String, Array<String>>> section = {};
};

/*
 * ShowMessageParams
 *
 * The parameters of a notification message.
 */
struct ShowMessageParams{
	/*
	 * The message type. See {@link MessageType}
	 */
	MessageTypeEnum type;
	/*
	 * The actual message.
	 */
	String message;
};

/*
 * MessageActionItem
 */
struct MessageActionItem{
	/*
	 * A short title like 'Retry', 'Open Log' etc.
	 */
	String title;
};

/*
 * ShowMessageRequestParams
 */
struct ShowMessageRequestParams{
	/*
	 * The message type. See {@link MessageType}
	 */
	MessageTypeEnum type;
	/*
	 * The actual message.
	 */
	String message;
	/*
	 * The message action items to present.
	 */
	Opt<Array<MessageActionItem>> actions = {};
};

/*
 * LogMessageParams
 *
 * The log message parameters.
 */
struct LogMessageParams{
	/*
	 * The message type. See {@link MessageType}
	 */
	MessageTypeEnum type;
	/*
	 * The actual message.
	 */
	String message;
};

/*
 * DidOpenTextDocumentParams
 *
 * The parameters sent in an open text document notification
 */
struct DidOpenTextDocumentParams{
	/*
	 * The document that was opened.
	 */
	TextDocumentItem textDocument;
};

/*
 * DidChangeTextDocumentParams
 *
 * The change text document notification's parameters.
 */
struct DidChangeTextDocumentParams{
	/*
	 * The document that did change. The version number points
	 * to the version after all provided content changes have
	 * been applied.
	 */
	VersionedTextDocumentIdentifier textDocument;
	/*
	 * The actual content changes. The content changes describe single state changes
	 * to the document. So if there are two content changes c1 (at array index 0) and
	 * c2 (at array index 1) for a document in state S then c1 moves the document from
	 * S to S' and c2 from S' to S''. So c1 is computed on the state S and c2 is computed
	 * on the state S'.
	 * 
	 * To mirror the content of a document using change events use the following approach:
	 * - start with the same initial content
	 * - apply the 'textDocument/didChange' notifications in the order you receive them.
	 * - apply the `TextDocumentContentChangeEvent`s in a single notification in the order
	 *   you receive them.
	 */
	Array<TextDocumentContentChangeEvent> contentChanges;
};

/*
 * TextDocumentChangeRegistrationOptions
 *
 * Describe options to be used when registered for text document change events.
 */
struct TextDocumentChangeRegistrationOptions : TextDocumentRegistrationOptions{
	/*
	 * How documents are synced to the server.
	 */
	TextDocumentSyncKindEnum syncKind;
};

/*
 * DidCloseTextDocumentParams
 *
 * The parameters sent in a close text document notification
 */
struct DidCloseTextDocumentParams{
	/*
	 * The document that was closed.
	 */
	TextDocumentIdentifier textDocument;
};

/*
 * DidSaveTextDocumentParams
 *
 * The parameters sent in a save text document notification
 */
struct DidSaveTextDocumentParams{
	/*
	 * The document that was saved.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * Optional the content when saved. Depends on the includeText value
	 * when the save notification was requested.
	 */
	Opt<String> text = {};
};

/*
 * TextDocumentSaveRegistrationOptions
 *
 * Save registration options.
 */
struct TextDocumentSaveRegistrationOptions : TextDocumentRegistrationOptions, SaveOptions{
};

/*
 * WillSaveTextDocumentParams
 *
 * The parameters sent in a will save text document notification.
 */
struct WillSaveTextDocumentParams{
	/*
	 * The document that will be saved.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The 'TextDocumentSaveReason'.
	 */
	TextDocumentSaveReasonEnum reason;
};

/*
 * FileEvent
 *
 * An event describing a file change.
 */
struct FileEvent{
	/*
	 * The file's uri.
	 */
	DocumentUri uri;
	/*
	 * The change type.
	 */
	FileChangeTypeEnum type;
};

/*
 * DidChangeWatchedFilesParams
 *
 * The watched files change notification's parameters.
 */
struct DidChangeWatchedFilesParams{
	/*
	 * The actual file events.
	 */
	Array<FileEvent> changes;
};

/*
 * Pattern
 *
 * The glob pattern to watch relative to the base path. Glob patterns can have the following syntax:
 * - `*` to match zero or more characters in a path segment
 * - `?` to match on one character in a path segment
 * - `**` to match any number of path segments, including none
 * - `{}` to group conditions (e.g. `**​/_*.{ts,js}` matches all TypeScript and JavaScript files)
 * - `[]` to declare a range of characters to match in a path segment (e.g., `example.[0-9]` to match on `example.0`, `example.1`, …)
 * - `[!...]` to negate a range of characters to match in a path segment (e.g., `example.[!0-9]` to match on `example.a`, `example.b`, but not `example.0`)
 * 
 * @since 3.17.0
 */
using Pattern = String;

/*
 * RelativePattern
 *
 * A relative pattern is a helper to construct glob patterns that are matched
 * relatively to a base URI. The common value for a `baseUri` is a workspace
 * folder root, but it can be another absolute URI as well.
 * 
 * @since 3.17.0
 */
struct RelativePattern{
	/*
	 * A workspace folder or a base URI to which this pattern will be matched
	 * against relatively.
	 */
	OneOf<WorkspaceFolder, Uri> baseUri;
	/*
	 * The actual glob pattern;
	 */
	Pattern pattern;
};

/*
 * GlobPattern
 *
 * The glob pattern. Either a string pattern or a relative pattern.
 * 
 * @since 3.17.0
 */
using GlobPattern = OneOf<Pattern, RelativePattern>;

/*
 * FileSystemWatcher
 */
struct FileSystemWatcher{
	/*
	 * The glob pattern to watch. See {@link GlobPattern glob pattern} for more detail.
	 * 
	 * @since 3.17.0 support for relative patterns.
	 */
	GlobPattern globPattern;
	/*
	 * The kind of events of interest. If omitted it defaults
	 * to WatchKind.Create | WatchKind.Change | WatchKind.Delete
	 * which is 7.
	 */
	Opt<WatchKindEnum> kind = {};
};

/*
 * DidChangeWatchedFilesRegistrationOptions
 *
 * Describe options to be used when registered for text document change events.
 */
struct DidChangeWatchedFilesRegistrationOptions{
	/*
	 * The watchers to register.
	 */
	Array<FileSystemWatcher> watchers;
};

/*
 * PublishDiagnosticsParams
 *
 * The publish diagnostic notification's parameters.
 */
struct PublishDiagnosticsParams{
	/*
	 * The URI for which diagnostic information is reported.
	 */
	DocumentUri uri;
	/*
	 * An array of diagnostic information items.
	 */
	Array<Diagnostic> diagnostics;
	/*
	 * Optional the version number of the document the diagnostics are published for.
	 * 
	 * @since 3.15.0
	 */
	Opt<int> version = {};
};

/*
 * CompletionContext
 *
 * Contains additional information about the context in which a completion request is triggered.
 */
struct CompletionContext{
	/*
	 * How the completion was triggered.
	 */
	CompletionTriggerKindEnum triggerKind;
	/*
	 * The trigger character (a single character) that has trigger code complete.
	 * Is undefined if `triggerKind !== CompletionTriggerKind.TriggerCharacter`
	 */
	Opt<String> triggerCharacter = {};
};

/*
 * CompletionParams
 *
 * Completion parameters
 */
struct CompletionParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The completion context. This is only available if the client specifies
	 * to send this using the client capability `textDocument.completion.contextSupport === true`
	 */
	Opt<CompletionContext> context = {};
};

/*
 * CompletionItemLabelDetails
 *
 * Additional details for a completion item label.
 * 
 * @since 3.17.0
 */
struct CompletionItemLabelDetails{
	/*
	 * An optional string which is rendered less prominently directly after {@link CompletionItem.label label},
	 * without any spacing. Should be used for function signatures and type annotations.
	 */
	Opt<String> detail = {};
	/*
	 * An optional string which is rendered less prominently after {@link CompletionItem.detail}. Should be used
	 * for fully qualified names and file paths.
	 */
	Opt<String> description = {};
};

/*
 * InsertReplaceEdit
 *
 * A special text edit to provide an insert and a replace operation.
 * 
 * @since 3.16.0
 */
struct InsertReplaceEdit{
	/*
	 * The string to be inserted.
	 */
	String newText;
	/*
	 * The range if the insert is requested
	 */
	Range insert;
	/*
	 * The range if the replace is requested.
	 */
	Range replace;
};

/*
 * CompletionItem
 *
 * A completion item represents a text snippet that is
 * proposed to complete text that is being typed.
 */
struct CompletionItem{
	/*
	 * The label of this completion item.
	 * 
	 * The label property is also by default the text that
	 * is inserted when selecting this completion.
	 * 
	 * If label details are provided the label itself should
	 * be an unqualified name of the completion item.
	 */
	String label;
	/*
	 * Additional details for the label
	 * 
	 * @since 3.17.0
	 */
	Opt<CompletionItemLabelDetails> labelDetails = {};
	/*
	 * The kind of this completion item. Based of the kind
	 * an icon is chosen by the editor.
	 */
	Opt<CompletionItemKindEnum> kind = {};
	/*
	 * Tags for this completion item.
	 * 
	 * @since 3.15.0
	 */
	Opt<Array<CompletionItemTagEnum>> tags = {};
	/*
	 * A human-readable string with additional information
	 * about this item, like type or symbol information.
	 */
	Opt<String> detail = {};
	/*
	 * A human-readable string that represents a doc-comment.
	 */
	Opt<OneOf<String, MarkupContent>> documentation = {};
	/*
	 * Indicates if this item is deprecated.
	 * @deprecated Use `tags` instead.
	 */
	Opt<bool> deprecated = {};
	/*
	 * Select this item when showing.
	 * 
	 * *Note* that only one completion item can be selected and that the
	 * tool / client decides which item that is. The rule is that the *first*
	 * item of those that match best is selected.
	 */
	Opt<bool> preselect = {};
	/*
	 * A string that should be used when comparing this item
	 * with other items. When `falsy` the {@link CompletionItem.label label}
	 * is used.
	 */
	Opt<String> sortText = {};
	/*
	 * A string that should be used when filtering a set of
	 * completion items. When `falsy` the {@link CompletionItem.label label}
	 * is used.
	 */
	Opt<String> filterText = {};
	/*
	 * A string that should be inserted into a document when selecting
	 * this completion. When `falsy` the {@link CompletionItem.label label}
	 * is used.
	 * 
	 * The `insertText` is subject to interpretation by the client side.
	 * Some tools might not take the string literally. For example
	 * VS Code when code complete is requested in this example
	 * `con<cursor position>` and a completion item with an `insertText` of
	 * `console` is provided it will only insert `sole`. Therefore it is
	 * recommended to use `textEdit` instead since it avoids additional client
	 * side interpretation.
	 */
	Opt<String> insertText = {};
	/*
	 * The format of the insert text. The format applies to both the
	 * `insertText` property and the `newText` property of a provided
	 * `textEdit`. If omitted defaults to `InsertTextFormat.PlainText`.
	 * 
	 * Please note that the insertTextFormat doesn't apply to
	 * `additionalTextEdits`.
	 */
	Opt<InsertTextFormatEnum> insertTextFormat = {};
	/*
	 * How whitespace and indentation is handled during completion
	 * item insertion. If not provided the clients default value depends on
	 * the `textDocument.completion.insertTextMode` client capability.
	 * 
	 * @since 3.16.0
	 */
	Opt<InsertTextModeEnum> insertTextMode = {};
	/*
	 * An {@link TextEdit edit} which is applied to a document when selecting
	 * this completion. When an edit is provided the value of
	 * {@link CompletionItem.insertText insertText} is ignored.
	 * 
	 * Most editors support two different operations when accepting a completion
	 * item. One is to insert a completion text and the other is to replace an
	 * existing text with a completion text. Since this can usually not be
	 * predetermined by a server it can report both ranges. Clients need to
	 * signal support for `InsertReplaceEdits` via the
	 * `textDocument.completion.insertReplaceSupport` client capability
	 * property.
	 * 
	 * *Note 1:* The text edit's range as well as both ranges from an insert
	 * replace edit must be a [single line] and they must contain the position
	 * at which completion has been requested.
	 * *Note 2:* If an `InsertReplaceEdit` is returned the edit's insert range
	 * must be a prefix of the edit's replace range, that means it must be
	 * contained and starting at the same position.
	 * 
	 * @since 3.16.0 additional type `InsertReplaceEdit`
	 */
	Opt<OneOf<TextEdit, InsertReplaceEdit>> textEdit = {};
	/*
	 * The edit text used if the completion item is part of a CompletionList and
	 * CompletionList defines an item default for the text edit range.
	 * 
	 * Clients will only honor this property if they opt into completion list
	 * item defaults using the capability `completionList.itemDefaults`.
	 * 
	 * If not provided and a list's default range is provided the label
	 * property is used as a text.
	 * 
	 * @since 3.17.0
	 */
	Opt<String> textEditText = {};
	/*
	 * An optional array of additional {@link TextEdit text edits} that are applied when
	 * selecting this completion. Edits must not overlap (including the same insert position)
	 * with the main {@link CompletionItem.textEdit edit} nor with themselves.
	 * 
	 * Additional text edits should be used to change text unrelated to the current cursor position
	 * (for example adding an import statement at the top of the file if the completion item will
	 * insert an unqualified type).
	 */
	Opt<Array<TextEdit>> additionalTextEdits = {};
	/*
	 * An optional set of characters that when pressed while this completion is active will accept it first and
	 * then type that character. *Note* that all commit characters should have `length=1` and that superfluous
	 * characters will be ignored.
	 */
	Opt<Array<String>> commitCharacters = {};
	/*
	 * An optional {@link Command command} that is executed *after* inserting this completion. *Note* that
	 * additional modifications to the current document should be described with the
	 * {@link CompletionItem.additionalTextEdits additionalTextEdits}-property.
	 */
	Opt<Command> command = {};
	/*
	 * A data entry field that is preserved on a completion item between a
	 * {@link CompletionRequest} and a {@link CompletionResolveRequest}.
	 */
	Opt<LSPAny> data = {};
};

/*
 * CompletionListItemDefaultsEditRange_Insert_Replace
 */
struct CompletionListItemDefaultsEditRange_Insert_Replace{
	Range insert;
	Range replace;
};

/*
 * CompletionListItemDefaults
 */
struct CompletionListItemDefaults{
	/*
	 * A default commit character set.
	 * 
	 * @since 3.17.0
	 */
	Opt<Array<String>> commitCharacters = {};
	/*
	 * A default edit range.
	 * 
	 * @since 3.17.0
	 */
	Opt<OneOf<Range, CompletionListItemDefaultsEditRange_Insert_Replace>> editRange = {};
	/*
	 * A default insert text format.
	 * 
	 * @since 3.17.0
	 */
	Opt<InsertTextFormatEnum> insertTextFormat = {};
	/*
	 * A default insert text mode.
	 * 
	 * @since 3.17.0
	 */
	Opt<InsertTextModeEnum> insertTextMode = {};
	/*
	 * A default data value.
	 * 
	 * @since 3.17.0
	 */
	Opt<LSPAny> data = {};
};

/*
 * CompletionList
 *
 * Represents a collection of {@link CompletionItem completion items} to be presented
 * in the editor.
 */
struct CompletionList{
	/*
	 * This list it not complete. Further typing results in recomputing this list.
	 * 
	 * Recomputed lists have all their items replaced (not appended) in the
	 * incomplete completion sessions.
	 */
	bool isIncomplete;
	/*
	 * The completion items.
	 */
	Array<CompletionItem> items;
	/*
	 * In many cases the items of an actual completion result share the same
	 * value for properties like `commitCharacters` or the range of a text
	 * edit. A completion list can therefore define item defaults which will
	 * be used if a completion item itself doesn't specify the value.
	 * 
	 * If a completion list specifies a default value and a completion item
	 * also specifies a corresponding value the one from the item is used.
	 * 
	 * Servers are only allowed to return default values if the client
	 * signals support for this via the `completionList.itemDefaults`
	 * capability.
	 * 
	 * @since 3.17.0
	 */
	Opt<CompletionListItemDefaults> itemDefaults = {};
};

/*
 * CompletionRegistrationOptions
 *
 * Registration options for a {@link CompletionRequest}.
 */
struct CompletionRegistrationOptions : TextDocumentRegistrationOptions, CompletionOptions{
};

/*
 * HoverParams
 *
 * Parameters for a {@link HoverRequest}.
 */
struct HoverParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
};

/*
 * MarkedString_Language_Value
 */
struct MarkedString_Language_Value{
	String language;
	String value;
};

/*
 * MarkedString
 *
 * MarkedString can be used to render human readable text. It is either a markdown string
 * or a code-block that provides a language and a code snippet. The language identifier
 * is semantically equal to the optional language identifier in fenced code blocks in GitHub
 * issues. See https://help.github.com/articles/creating-and-highlighting-code-blocks/#syntax-highlighting
 * 
 * The pair of a language and a value is an equivalent to markdown:
 * ```${language}
 * ${value}
 * ```
 * 
 * Note that markdown strings will be sanitized - that means html will be escaped.
 * @deprecated use MarkupContent instead.
 */
using MarkedString = OneOf<String, MarkedString_Language_Value>;

/*
 * Hover
 *
 * The result of a hover request.
 */
struct Hover{
	/*
	 * The hover's content
	 */
	OneOf<MarkupContent, MarkedString, Array<MarkedString>> contents;
	/*
	 * An optional range inside the text document that is used to
	 * visualize the hover, e.g. by changing the background color.
	 */
	Opt<Range> range = {};
};

/*
 * HoverRegistrationOptions
 *
 * Registration options for a {@link HoverRequest}.
 */
struct HoverRegistrationOptions : TextDocumentRegistrationOptions, HoverOptions{
};

/*
 * ParameterInformation
 *
 * Represents a parameter of a callable-signature. A parameter can
 * have a label and a doc-comment.
 */
struct ParameterInformation{
	/*
	 * The label of this parameter information.
	 * 
	 * Either a string or an inclusive start and exclusive end offsets within its containing
	 * signature label. (see SignatureInformation.label). The offsets are based on a UTF-16
	 * string representation as `Position` and `Range` does.
	 * 
	 * *Note*: a label of type string should be a substring of its containing signature label.
	 * Its intended use case is to highlight the parameter label part in the `SignatureInformation.label`.
	 */
	OneOf<String, Tuple<uint, uint>> label;
	/*
	 * The human-readable doc-comment of this parameter. Will be shown
	 * in the UI but can be omitted.
	 */
	Opt<OneOf<String, MarkupContent>> documentation = {};
};

/*
 * SignatureInformation
 *
 * Represents the signature of something callable. A signature
 * can have a label, like a function-name, a doc-comment, and
 * a set of parameters.
 */
struct SignatureInformation{
	/*
	 * The label of this signature. Will be shown in
	 * the UI.
	 */
	String label;
	/*
	 * The human-readable doc-comment of this signature. Will be shown
	 * in the UI but can be omitted.
	 */
	Opt<OneOf<String, MarkupContent>> documentation = {};
	/*
	 * The parameters of this signature.
	 */
	Opt<Array<ParameterInformation>> parameters = {};
	/*
	 * The index of the active parameter.
	 * 
	 * If provided, this is used in place of `SignatureHelp.activeParameter`.
	 * 
	 * @since 3.16.0
	 */
	Opt<uint> activeParameter = {};
};

/*
 * SignatureHelp
 *
 * Signature help represents the signature of something
 * callable. There can be multiple signature but only one
 * active and only one active parameter.
 */
struct SignatureHelp{
	/*
	 * One or more signatures.
	 */
	Array<SignatureInformation> signatures;
	/*
	 * The active signature. If omitted or the value lies outside the
	 * range of `signatures` the value defaults to zero or is ignored if
	 * the `SignatureHelp` has no signatures.
	 * 
	 * Whenever possible implementors should make an active decision about
	 * the active signature and shouldn't rely on a default value.
	 * 
	 * In future version of the protocol this property might become
	 * mandatory to better express this.
	 */
	Opt<uint> activeSignature = {};
	/*
	 * The active parameter of the active signature. If omitted or the value
	 * lies outside the range of `signatures[activeSignature].parameters`
	 * defaults to 0 if the active signature has parameters. If
	 * the active signature has no parameters it is ignored.
	 * In future version of the protocol this property might become
	 * mandatory to better express the active parameter if the
	 * active signature does have any.
	 */
	Opt<uint> activeParameter = {};
};

/*
 * SignatureHelpContext
 *
 * Additional information about the context in which a signature help request was triggered.
 * 
 * @since 3.15.0
 */
struct SignatureHelpContext{
	/*
	 * Action that caused signature help to be triggered.
	 */
	SignatureHelpTriggerKindEnum triggerKind;
	/*
	 * `true` if signature help was already showing when it was triggered.
	 * 
	 * Retriggers occurs when the signature help is already active and can be caused by actions such as
	 * typing a trigger character, a cursor move, or document content changes.
	 */
	bool isRetrigger;
	/*
	 * Character that caused signature help to be triggered.
	 * 
	 * This is undefined when `triggerKind !== SignatureHelpTriggerKind.TriggerCharacter`
	 */
	Opt<String> triggerCharacter = {};
	/*
	 * The currently active `SignatureHelp`.
	 * 
	 * The `activeSignatureHelp` has its `SignatureHelp.activeSignature` field updated based on
	 * the user navigating through available signatures.
	 */
	Opt<SignatureHelp> activeSignatureHelp = {};
};

/*
 * SignatureHelpParams
 *
 * Parameters for a {@link SignatureHelpRequest}.
 */
struct SignatureHelpParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The signature help context. This is only available if the client specifies
	 * to send this using the client capability `textDocument.signatureHelp.contextSupport === true`
	 * 
	 * @since 3.15.0
	 */
	Opt<SignatureHelpContext> context = {};
};

/*
 * SignatureHelpRegistrationOptions
 *
 * Registration options for a {@link SignatureHelpRequest}.
 */
struct SignatureHelpRegistrationOptions : TextDocumentRegistrationOptions, SignatureHelpOptions{
};

/*
 * DefinitionParams
 *
 * Parameters for a {@link DefinitionRequest}.
 */
struct DefinitionParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
};

/*
 * DefinitionRegistrationOptions
 *
 * Registration options for a {@link DefinitionRequest}.
 */
struct DefinitionRegistrationOptions : TextDocumentRegistrationOptions, DefinitionOptions{
};

/*
 * ReferenceContext
 *
 * Value-object that contains additional information when
 * requesting references.
 */
struct ReferenceContext{
	/*
	 * Include the declaration of the current symbol.
	 */
	bool includeDeclaration;
};

/*
 * ReferenceParams
 *
 * Parameters for a {@link ReferencesRequest}.
 */
struct ReferenceParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	ReferenceContext context;
};

/*
 * ReferenceRegistrationOptions
 *
 * Registration options for a {@link ReferencesRequest}.
 */
struct ReferenceRegistrationOptions : TextDocumentRegistrationOptions, ReferenceOptions{
};

/*
 * DocumentHighlightParams
 *
 * Parameters for a {@link DocumentHighlightRequest}.
 */
struct DocumentHighlightParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
};

/*
 * DocumentHighlight
 *
 * A document highlight is a range inside a text document which deserves
 * special attention. Usually a document highlight is visualized by changing
 * the background color of its range.
 */
struct DocumentHighlight{
	/*
	 * The range this highlight applies to.
	 */
	Range range;
	/*
	 * The highlight kind, default is {@link DocumentHighlightKind.Text text}.
	 */
	Opt<DocumentHighlightKindEnum> kind = {};
};

/*
 * DocumentHighlightRegistrationOptions
 *
 * Registration options for a {@link DocumentHighlightRequest}.
 */
struct DocumentHighlightRegistrationOptions : TextDocumentRegistrationOptions, DocumentHighlightOptions{
};

/*
 * DocumentSymbolParams
 *
 * Parameters for a {@link DocumentSymbolRequest}.
 */
struct DocumentSymbolParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The text document.
	 */
	TextDocumentIdentifier textDocument;
};

/*
 * BaseSymbolInformation
 *
 * A base for all symbol information.
 */
struct BaseSymbolInformation{
	/*
	 * The name of this symbol.
	 */
	String name;
	/*
	 * The kind of this symbol.
	 */
	SymbolKindEnum kind;
	/*
	 * Tags for this symbol.
	 * 
	 * @since 3.16.0
	 */
	Opt<Array<SymbolTagEnum>> tags = {};
	/*
	 * The name of the symbol containing this symbol. This information is for
	 * user interface purposes (e.g. to render a qualifier in the user interface
	 * if necessary). It can't be used to re-infer a hierarchy for the document
	 * symbols.
	 */
	Opt<String> containerName = {};
};

/*
 * SymbolInformation
 *
 * Represents information about programming constructs like variables, classes,
 * interfaces etc.
 */
struct SymbolInformation : BaseSymbolInformation{
	/*
	 * The location of this symbol. The location's range is used by a tool
	 * to reveal the location in the editor. If the symbol is selected in the
	 * tool the range's start information is used to position the cursor. So
	 * the range usually spans more than the actual symbol's name and does
	 * normally include things like visibility modifiers.
	 * 
	 * The range doesn't have to denote a node range in the sense of an abstract
	 * syntax tree. It can therefore not be used to re-construct a hierarchy of
	 * the symbols.
	 */
	Location location;
	/*
	 * Indicates if this symbol is deprecated.
	 * 
	 * @deprecated Use tags instead
	 */
	Opt<bool> deprecated = {};
};

/*
 * DocumentSymbol
 *
 * Represents programming constructs like variables, classes, interfaces etc.
 * that appear in a document. Document symbols can be hierarchical and they
 * have two ranges: one that encloses its definition and one that points to
 * its most interesting range, e.g. the range of an identifier.
 */
struct DocumentSymbol{
	/*
	 * The name of this symbol. Will be displayed in the user interface and therefore must not be
	 * an empty string or a string only consisting of white spaces.
	 */
	String name;
	/*
	 * The kind of this symbol.
	 */
	SymbolKindEnum kind;
	/*
	 * The range enclosing this symbol not including leading/trailing whitespace but everything else
	 * like comments. This information is typically used to determine if the clients cursor is
	 * inside the symbol to reveal in the symbol in the UI.
	 */
	Range range;
	/*
	 * The range that should be selected and revealed when this symbol is being picked, e.g the name of a function.
	 * Must be contained by the `range`.
	 */
	Range selectionRange;
	/*
	 * More detail for this symbol, e.g the signature of a function.
	 */
	Opt<String> detail = {};
	/*
	 * Tags for this document symbol.
	 * 
	 * @since 3.16.0
	 */
	Opt<Array<SymbolTagEnum>> tags = {};
	/*
	 * Indicates if this symbol is deprecated.
	 * 
	 * @deprecated Use tags instead
	 */
	Opt<bool> deprecated = {};
	/*
	 * Children of this symbol, e.g. properties of a class.
	 */
	Opt<Array<DocumentSymbol>> children = {};
};

/*
 * DocumentSymbolRegistrationOptions
 *
 * Registration options for a {@link DocumentSymbolRequest}.
 */
struct DocumentSymbolRegistrationOptions : TextDocumentRegistrationOptions, DocumentSymbolOptions{
};

/*
 * CodeActionContext
 *
 * Contains additional diagnostic information about the context in which
 * a {@link CodeActionProvider.provideCodeActions code action} is run.
 */
struct CodeActionContext{
	/*
	 * An array of diagnostics known on the client side overlapping the range provided to the
	 * `textDocument/codeAction` request. They are provided so that the server knows which
	 * errors are currently presented to the user for the given range. There is no guarantee
	 * that these accurately reflect the error state of the resource. The primary parameter
	 * to compute code actions is the provided range.
	 */
	Array<Diagnostic> diagnostics;
	/*
	 * Requested kind of actions to return.
	 * 
	 * Actions not of this kind are filtered out by the client before being shown. So servers
	 * can omit computing them.
	 */
	Opt<Array<CodeActionKindEnum>> only = {};
	/*
	 * The reason why code actions were requested.
	 * 
	 * @since 3.17.0
	 */
	Opt<CodeActionTriggerKindEnum> triggerKind = {};
};

/*
 * CodeActionParams
 *
 * The parameters of a {@link CodeActionRequest}.
 */
struct CodeActionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The document in which the command was invoked.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The range for which the command was invoked.
	 */
	Range range;
	/*
	 * Context carrying additional information.
	 */
	CodeActionContext context;
};

/*
 * CodeActionDisabled
 */
struct CodeActionDisabled{
	/*
	 * Human readable description of why the code action is currently disabled.
	 * 
	 * This is displayed in the code actions UI.
	 */
	String reason;
};

/*
 * CodeAction
 *
 * A code action represents a change that can be performed in code, e.g. to fix a problem or
 * to refactor code.
 * 
 * A CodeAction must set either `edit` and/or a `command`. If both are supplied, the `edit` is applied first, then the `command` is executed.
 */
struct CodeAction{
	/*
	 * A short, human-readable, title for this code action.
	 */
	String title;
	/*
	 * The kind of the code action.
	 * 
	 * Used to filter code actions.
	 */
	Opt<CodeActionKindEnum> kind = {};
	/*
	 * The diagnostics that this code action resolves.
	 */
	Opt<Array<Diagnostic>> diagnostics = {};
	/*
	 * Marks this as a preferred action. Preferred actions are used by the `auto fix` command and can be targeted
	 * by keybindings.
	 * 
	 * A quick fix should be marked preferred if it properly addresses the underlying error.
	 * A refactoring should be marked preferred if it is the most reasonable choice of actions to take.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> isPreferred = {};
	/*
	 * Marks that the code action cannot currently be applied.
	 * 
	 * Clients should follow the following guidelines regarding disabled code actions:
	 * 
	 *   - Disabled code actions are not shown in automatic [lightbulbs](https://code.visualstudio.com/docs/editor/editingevolved#_code-action)
	 *     code action menus.
	 * 
	 *   - Disabled actions are shown as faded out in the code action menu when the user requests a more specific type
	 *     of code action, such as refactorings.
	 * 
	 *   - If the user has a [keybinding](https://code.visualstudio.com/docs/editor/refactoring#_keybindings-for-code-actions)
	 *     that auto applies a code action and only disabled code actions are returned, the client should show the user an
	 *     error message with `reason` in the editor.
	 * 
	 * @since 3.16.0
	 */
	Opt<CodeActionDisabled> disabled = {};
	/*
	 * The workspace edit this code action performs.
	 */
	Opt<WorkspaceEdit> edit = {};
	/*
	 * A command this code action executes. If a code action
	 * provides an edit and a command, first the edit is
	 * executed and then the command.
	 */
	Opt<Command> command = {};
	/*
	 * A data entry field that is preserved on a code action between
	 * a `textDocument/codeAction` and a `codeAction/resolve` request.
	 * 
	 * @since 3.16.0
	 */
	Opt<LSPAny> data = {};
};

/*
 * CodeActionRegistrationOptions
 *
 * Registration options for a {@link CodeActionRequest}.
 */
struct CodeActionRegistrationOptions : TextDocumentRegistrationOptions, CodeActionOptions{
};

/*
 * WorkspaceSymbolParams
 *
 * The parameters of a {@link WorkspaceSymbolRequest}.
 */
struct WorkspaceSymbolParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * A query string to filter symbols by. Clients may send an empty
	 * string here to request all symbols.
	 */
	String query;
};

/*
 * WorkspaceSymbolLocation_Uri
 */
struct WorkspaceSymbolLocation_Uri{
	DocumentUri uri;
};

/*
 * WorkspaceSymbol
 *
 * A special workspace symbol that supports locations without a range.
 * 
 * See also SymbolInformation.
 * 
 * @since 3.17.0
 */
struct WorkspaceSymbol : BaseSymbolInformation{
	/*
	 * The location of the symbol. Whether a server is allowed to
	 * return a location without a range depends on the client
	 * capability `workspace.symbol.resolveSupport`.
	 * 
	 * See SymbolInformation#location for more details.
	 */
	OneOf<Location, WorkspaceSymbolLocation_Uri> location;
	/*
	 * A data entry field that is preserved on a workspace symbol between a
	 * workspace symbol request and a workspace symbol resolve request.
	 */
	Opt<LSPAny> data = {};
};

/*
 * WorkspaceSymbolRegistrationOptions
 *
 * Registration options for a {@link WorkspaceSymbolRequest}.
 */
struct WorkspaceSymbolRegistrationOptions : WorkspaceSymbolOptions{
};

/*
 * CodeLensParams
 *
 * The parameters of a {@link CodeLensRequest}.
 */
struct CodeLensParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The document to request code lens for.
	 */
	TextDocumentIdentifier textDocument;
};

/*
 * CodeLens
 *
 * A code lens represents a {@link Command command} that should be shown along with
 * source text, like the number of references, a way to run tests, etc.
 * 
 * A code lens is _unresolved_ when no command is associated to it. For performance
 * reasons the creation of a code lens and resolving should be done in two stages.
 */
struct CodeLens{
	/*
	 * The range in which this code lens is valid. Should only span a single line.
	 */
	Range range;
	/*
	 * The command this code lens represents.
	 */
	Opt<Command> command = {};
	/*
	 * A data entry field that is preserved on a code lens item between
	 * a {@link CodeLensRequest} and a {@link CodeLensResolveRequest}
	 */
	Opt<LSPAny> data = {};
};

/*
 * CodeLensRegistrationOptions
 *
 * Registration options for a {@link CodeLensRequest}.
 */
struct CodeLensRegistrationOptions : TextDocumentRegistrationOptions, CodeLensOptions{
};

/*
 * DocumentLinkParams
 *
 * The parameters of a {@link DocumentLinkRequest}.
 */
struct DocumentLinkParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * An optional token that a server can use to report partial results (e.g. streaming) to
	 * the client.
	 */
	Opt<ProgressToken> partialResultToken = {};
	/*
	 * The document to provide document links for.
	 */
	TextDocumentIdentifier textDocument;
};

/*
 * DocumentLink
 *
 * A document link is a range in a text document that links to an internal or external resource, like another
 * text document or a web site.
 */
struct DocumentLink{
	/*
	 * The range this link applies to.
	 */
	Range range;
	/*
	 * The uri this link points to. If missing a resolve request is sent later.
	 */
	Opt<Uri> target = {};
	/*
	 * The tooltip text when you hover over this link.
	 * 
	 * If a tooltip is provided, is will be displayed in a string that includes instructions on how to
	 * trigger the link, such as `{0} (ctrl + click)`. The specific instructions vary depending on OS,
	 * user settings, and localization.
	 * 
	 * @since 3.15.0
	 */
	Opt<String> tooltip = {};
	/*
	 * A data entry field that is preserved on a document link between a
	 * DocumentLinkRequest and a DocumentLinkResolveRequest.
	 */
	Opt<LSPAny> data = {};
};

/*
 * DocumentLinkRegistrationOptions
 *
 * Registration options for a {@link DocumentLinkRequest}.
 */
struct DocumentLinkRegistrationOptions : TextDocumentRegistrationOptions, DocumentLinkOptions{
};

/*
 * FormattingOptions
 *
 * Value-object describing what options formatting should use.
 */
struct FormattingOptions{
	/*
	 * Size of a tab in spaces.
	 */
	uint tabSize;
	/*
	 * Prefer spaces over tabs.
	 */
	bool insertSpaces;
	/*
	 * Trim trailing whitespace on a line.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> trimTrailingWhitespace = {};
	/*
	 * Insert a newline character at the end of the file if one does not exist.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> insertFinalNewline = {};
	/*
	 * Trim all newlines after the final newline at the end of the file.
	 * 
	 * @since 3.15.0
	 */
	Opt<bool> trimFinalNewlines = {};
};

/*
 * DocumentFormattingParams
 *
 * The parameters of a {@link DocumentFormattingRequest}.
 */
struct DocumentFormattingParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The document to format.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The format options.
	 */
	FormattingOptions options;
};

/*
 * DocumentFormattingRegistrationOptions
 *
 * Registration options for a {@link DocumentFormattingRequest}.
 */
struct DocumentFormattingRegistrationOptions : TextDocumentRegistrationOptions, DocumentFormattingOptions{
};

/*
 * DocumentRangeFormattingParams
 *
 * The parameters of a {@link DocumentRangeFormattingRequest}.
 */
struct DocumentRangeFormattingParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The document to format.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The range to format
	 */
	Range range;
	/*
	 * The format options
	 */
	FormattingOptions options;
};

/*
 * DocumentRangeFormattingRegistrationOptions
 *
 * Registration options for a {@link DocumentRangeFormattingRequest}.
 */
struct DocumentRangeFormattingRegistrationOptions : TextDocumentRegistrationOptions, DocumentRangeFormattingOptions{
};

/*
 * DocumentRangesFormattingParams
 *
 * The parameters of a {@link DocumentRangesFormattingRequest}.
 * 
 * @since 3.18.0
 * @proposed
 */
struct DocumentRangesFormattingParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The document to format.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The ranges to format
	 */
	Array<Range> ranges;
	/*
	 * The format options
	 */
	FormattingOptions options;
};

/*
 * DocumentOnTypeFormattingParams
 *
 * The parameters of a {@link DocumentOnTypeFormattingRequest}.
 */
struct DocumentOnTypeFormattingParams{
	/*
	 * The document to format.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The position around which the on type formatting should happen.
	 * This is not necessarily the exact position where the character denoted
	 * by the property `ch` got typed.
	 */
	Position position;
	/*
	 * The character that has been typed that triggered the formatting
	 * on type request. That is not necessarily the last character that
	 * got inserted into the document since the client could auto insert
	 * characters as well (e.g. like automatic brace completion).
	 */
	String ch;
	/*
	 * The formatting options.
	 */
	FormattingOptions options;
};

/*
 * DocumentOnTypeFormattingRegistrationOptions
 *
 * Registration options for a {@link DocumentOnTypeFormattingRequest}.
 */
struct DocumentOnTypeFormattingRegistrationOptions : TextDocumentRegistrationOptions, DocumentOnTypeFormattingOptions{
};

/*
 * RenameParams
 *
 * The parameters of a {@link RenameRequest}.
 */
struct RenameParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The document to rename.
	 */
	TextDocumentIdentifier textDocument;
	/*
	 * The position at which this request was sent.
	 */
	Position position;
	/*
	 * The new name of the symbol. If the given name is not valid the
	 * request must return a {@link ResponseError} with an
	 * appropriate message set.
	 */
	String newName;
};

/*
 * RenameRegistrationOptions
 *
 * Registration options for a {@link RenameRequest}.
 */
struct RenameRegistrationOptions : TextDocumentRegistrationOptions, RenameOptions{
};

/*
 * PrepareRenameParams
 */
struct PrepareRenameParams : TextDocumentPositionParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
};

/*
 * ExecuteCommandParams
 *
 * The parameters of a {@link ExecuteCommandRequest}.
 */
struct ExecuteCommandParams{
	/*
	 * An optional token that a server can use to report work done progress.
	 */
	Opt<ProgressToken> workDoneToken = {};
	/*
	 * The identifier of the actual command handler.
	 */
	String command;
	/*
	 * Arguments that the command should be invoked with.
	 */
	Opt<LSPArray> arguments = {};
};

/*
 * ExecuteCommandRegistrationOptions
 *
 * Registration options for a {@link ExecuteCommandRequest}.
 */
struct ExecuteCommandRegistrationOptions : ExecuteCommandOptions{
};

/*
 * ApplyWorkspaceEditParams
 *
 * The parameters passed via an apply workspace edit request.
 */
struct ApplyWorkspaceEditParams{
	/*
	 * The edits to apply.
	 */
	WorkspaceEdit edit;
	/*
	 * An optional label of the workspace edit. This label is
	 * presented in the user interface for example on an undo
	 * stack to undo the workspace edit.
	 */
	Opt<String> label = {};
};

/*
 * ApplyWorkspaceEditResult
 *
 * The result returned from the apply workspace edit request.
 * 
 * @since 3.17 renamed from ApplyWorkspaceEditResponse
 */
struct ApplyWorkspaceEditResult{
	/*
	 * Indicates whether the edit was applied or not.
	 */
	bool applied;
	/*
	 * An optional textual description for why the edit was not applied.
	 * This may be used by the server for diagnostic logging or to provide
	 * a suitable error for a request that triggered the edit.
	 */
	Opt<String> failureReason = {};
	/*
	 * Depending on the client's failure handling strategy `failedChange` might
	 * contain the index of the change that failed. This property is only available
	 * if the client signals a `failureHandlingStrategy` in its client capabilities.
	 */
	Opt<uint> failedChange = {};
};

/*
 * WorkDoneProgressBegin
 */
struct WorkDoneProgressBegin{
	String kind = "begin";
	/*
	 * Mandatory title of the progress operation. Used to briefly inform about
	 * the kind of operation being performed.
	 * 
	 * Examples: "Indexing" or "Linking dependencies".
	 */
	String title;
	/*
	 * Controls if a cancel button should show to allow the user to cancel the
	 * long running operation. Clients that don't support cancellation are allowed
	 * to ignore the setting.
	 */
	Opt<bool> cancellable = {};
	/*
	 * Optional, more detailed associated progress message. Contains
	 * complementary information to the `title`.
	 * 
	 * Examples: "3/25 files", "project/src/module2", "node_modules/some_dep".
	 * If unset, the previous progress message (if any) is still valid.
	 */
	Opt<String> message = {};
	/*
	 * Optional progress percentage to display (value 100 is considered 100%).
	 * If not provided infinite progress is assumed and clients are allowed
	 * to ignore the `percentage` value in subsequent report notifications.
	 * 
	 * The value should be steadily rising. Clients are free to ignore values
	 * that are not following this rule. The value range is [0, 100].
	 */
	Opt<uint> percentage = {};
};

/*
 * WorkDoneProgressReport
 */
struct WorkDoneProgressReport{
	String kind = "report";
	/*
	 * Controls enablement state of a cancel button.
	 * 
	 * Clients that don't support cancellation or don't support controlling the button's
	 * enablement state are allowed to ignore the property.
	 */
	Opt<bool> cancellable = {};
	/*
	 * Optional, more detailed associated progress message. Contains
	 * complementary information to the `title`.
	 * 
	 * Examples: "3/25 files", "project/src/module2", "node_modules/some_dep".
	 * If unset, the previous progress message (if any) is still valid.
	 */
	Opt<String> message = {};
	/*
	 * Optional progress percentage to display (value 100 is considered 100%).
	 * If not provided infinite progress is assumed and clients are allowed
	 * to ignore the `percentage` value in subsequent report notifications.
	 * 
	 * The value should be steadily rising. Clients are free to ignore values
	 * that are not following this rule. The value range is [0, 100].
	 */
	Opt<uint> percentage = {};
};

/*
 * WorkDoneProgressEnd
 */
struct WorkDoneProgressEnd{
	String kind = "end";
	/*
	 * Optional, a final message indicating to for example indicate the outcome
	 * of the operation.
	 */
	Opt<String> message = {};
};

/*
 * SetTraceParams
 */
struct SetTraceParams{
	TraceValuesEnum value;
};

/*
 * LogTraceParams
 */
struct LogTraceParams{
	String message;
	Opt<String> verbose = {};
};

/*
 * CancelParams
 */
struct CancelParams{
	/*
	 * The request id to cancel.
	 */
	OneOf<int, String> id;
};

/*
 * ProgressParams
 */
struct ProgressParams{
	/*
	 * The progress token provided by the client or server.
	 */
	ProgressToken token;
	/*
	 * The progress data.
	 */
	LSPAny value;
};

/*
 * LocationLink
 *
 * Represents the connection of two locations. Provides additional metadata over normal {@link Location locations},
 * including an origin range.
 */
struct LocationLink{
	/*
	 * The target resource identifier of this link.
	 */
	DocumentUri targetUri;
	/*
	 * The full target range of this link. If the target for example is a symbol then target range is the
	 * range enclosing this symbol not including leading/trailing whitespace but everything else
	 * like comments. This information is typically used to highlight the range in the editor.
	 */
	Range targetRange;
	/*
	 * The range that should be selected and revealed when this link is being followed, e.g the name of a function.
	 * Must be contained by the `targetRange`. See also `DocumentSymbol#range`
	 */
	Range targetSelectionRange;
	/*
	 * Span of the origin of this link.
	 * 
	 * Used as the underlined span for mouse interaction. Defaults to the word range at
	 * the definition position.
	 */
	Opt<Range> originSelectionRange = {};
};

/*
 * InlineValueText
 *
 * Provide inline value as text.
 * 
 * @since 3.17.0
 */
struct InlineValueText{
	/*
	 * The document range for which the inline value applies.
	 */
	Range range;
	/*
	 * The text of the inline value.
	 */
	String text;
};

/*
 * InlineValueVariableLookup
 *
 * Provide inline value through a variable lookup.
 * If only a range is specified, the variable name will be extracted from the underlying document.
 * An optional variable name can be used to override the extracted name.
 * 
 * @since 3.17.0
 */
struct InlineValueVariableLookup{
	/*
	 * The document range for which the inline value applies.
	 * The range is used to extract the variable name from the underlying document.
	 */
	Range range;
	/*
	 * How to perform the lookup.
	 */
	bool caseSensitiveLookup;
	/*
	 * If specified the name of the variable to look up.
	 */
	Opt<String> variableName = {};
};

/*
 * InlineValueEvaluatableExpression
 *
 * Provide an inline value through an expression evaluation.
 * If only a range is specified, the expression will be extracted from the underlying document.
 * An optional expression can be used to override the extracted expression.
 * 
 * @since 3.17.0
 */
struct InlineValueEvaluatableExpression{
	/*
	 * The document range for which the inline value applies.
	 * The range is used to extract the evaluatable expression from the underlying document.
	 */
	Range range;
	/*
	 * If specified the expression overrides the extracted expression.
	 */
	Opt<String> expression = {};
};

/*
 * RelatedFullDocumentDiagnosticReport
 *
 * A full diagnostic report with a set of related documents.
 * 
 * @since 3.17.0
 */
struct RelatedFullDocumentDiagnosticReport : FullDocumentDiagnosticReport{
	/*
	 * Diagnostics of related documents. This information is useful
	 * in programming languages where code in a file A can generate
	 * diagnostics in a file B which A depends on. An example of
	 * such a language is C/C++ where marco definitions in a file
	 * a.cpp and result in errors in a header file b.hpp.
	 * 
	 * @since 3.17.0
	 */
	Opt<Map<DocumentUri, OneOf<FullDocumentDiagnosticReport, UnchangedDocumentDiagnosticReport>>> relatedDocuments = {};
};

/*
 * RelatedUnchangedDocumentDiagnosticReport
 *
 * An unchanged diagnostic report with a set of related documents.
 * 
 * @since 3.17.0
 */
struct RelatedUnchangedDocumentDiagnosticReport : UnchangedDocumentDiagnosticReport{
	/*
	 * Diagnostics of related documents. This information is useful
	 * in programming languages where code in a file A can generate
	 * diagnostics in a file B which A depends on. An example of
	 * such a language is C/C++ where marco definitions in a file
	 * a.cpp and result in errors in a header file b.hpp.
	 * 
	 * @since 3.17.0
	 */
	Opt<Map<DocumentUri, OneOf<FullDocumentDiagnosticReport, UnchangedDocumentDiagnosticReport>>> relatedDocuments = {};
};

/*
 * Definition
 *
 * The definition of a symbol represented as one or many {@link Location locations}.
 * For most programming languages there is only one location at which a symbol is
 * defined.
 * 
 * Servers should prefer returning `DefinitionLink` over `Definition` if supported
 * by the client.
 */
using Definition = OneOf<Location, Array<Location>>;

/*
 * DefinitionLink
 *
 * Information about where a symbol is defined.
 * 
 * Provides additional metadata over normal {@link Location location} definitions, including the range of
 * the defining symbol
 */
using DefinitionLink = LocationLink;

/*
 * Declaration
 *
 * The declaration of a symbol representation as one or many {@link Location locations}.
 */
using Declaration = OneOf<Location, Array<Location>>;

/*
 * DeclarationLink
 *
 * Information about where a symbol is declared.
 * 
 * Provides additional metadata over normal {@link Location location} declarations, including the range of
 * the declaring symbol.
 * 
 * Servers should prefer returning `DeclarationLink` over `Declaration` if supported
 * by the client.
 */
using DeclarationLink = LocationLink;

/*
 * InlineValue
 *
 * Inline value information can be provided by different means:
 * - directly as a text value (class InlineValueText).
 * - as a name to use for a variable lookup (class InlineValueVariableLookup)
 * - as an evaluatable expression (class InlineValueEvaluatableExpression)
 * The InlineValue types combines all inline value types into one type.
 * 
 * @since 3.17.0
 */
using InlineValue = OneOf<InlineValueText, InlineValueVariableLookup, InlineValueEvaluatableExpression>;

/*
 * DocumentDiagnosticReport
 *
 * The result of a document diagnostic pull request. A report can
 * either be a full report containing all diagnostics for the
 * requested document or an unchanged report indicating that nothing
 * has changed in terms of diagnostics in comparison to the last
 * pull request.
 * 
 * @since 3.17.0
 */
using DocumentDiagnosticReport = OneOf<RelatedFullDocumentDiagnosticReport, RelatedUnchangedDocumentDiagnosticReport>;

/*
 * PrepareRenameResult_Range_Placeholder
 */
struct PrepareRenameResult_Range_Placeholder{
	Range range;
	String placeholder;
};

/*
 * PrepareRenameResult_DefaultBehavior
 */
struct PrepareRenameResult_DefaultBehavior{
	bool defaultBehavior;
};

/*
 * PrepareRenameResult
 */
using PrepareRenameResult = OneOf<Range, PrepareRenameResult_Range_Placeholder, PrepareRenameResult_DefaultBehavior>;

/*
 * TextDocument_ImplementationResult
 */
using TextDocument_ImplementationResult = NullOrOneOf<Definition, Array<DefinitionLink>>;

/*
 * TextDocument_ImplementationPartialResult
 */
using TextDocument_ImplementationPartialResult = OneOf<Array<Location>, Array<DefinitionLink>>;

/*
 * TextDocument_TypeDefinitionResult
 */
using TextDocument_TypeDefinitionResult = NullOrOneOf<Definition, Array<DefinitionLink>>;

/*
 * TextDocument_TypeDefinitionPartialResult
 */
using TextDocument_TypeDefinitionPartialResult = OneOf<Array<Location>, Array<DefinitionLink>>;

/*
 * Workspace_WorkspaceFoldersResult
 */
using Workspace_WorkspaceFoldersResult = NullOr<Array<WorkspaceFolder>>;

/*
 * Workspace_ConfigurationResult
 */
using Workspace_ConfigurationResult = LSPArray;

/*
 * TextDocument_DocumentColorResult
 */
using TextDocument_DocumentColorResult = Array<ColorInformation>;

/*
 * TextDocument_DocumentColorPartialResult
 */
using TextDocument_DocumentColorPartialResult = Array<ColorInformation>;

/*
 * TextDocument_ColorPresentationResult
 */
using TextDocument_ColorPresentationResult = Array<ColorPresentation>;

/*
 * TextDocument_ColorPresentationPartialResult
 */
using TextDocument_ColorPresentationPartialResult = Array<ColorPresentation>;

/*
 * TextDocument_ColorPresentationRegistrationOptions
 */
using TextDocument_ColorPresentationRegistrationOptions = LSPObject;

/*
 * TextDocument_FoldingRangeResult
 */
using TextDocument_FoldingRangeResult = NullOr<Array<FoldingRange>>;

/*
 * TextDocument_FoldingRangePartialResult
 */
using TextDocument_FoldingRangePartialResult = Array<FoldingRange>;

/*
 * Workspace_FoldingRange_RefreshResult
 */
using Workspace_FoldingRange_RefreshResult = Null;

/*
 * TextDocument_DeclarationResult
 */
using TextDocument_DeclarationResult = NullOrOneOf<Declaration, Array<DeclarationLink>>;

/*
 * TextDocument_DeclarationPartialResult
 */
using TextDocument_DeclarationPartialResult = OneOf<Array<Location>, Array<DeclarationLink>>;

/*
 * TextDocument_SelectionRangeResult
 */
using TextDocument_SelectionRangeResult = NullOr<Array<SelectionRange>>;

/*
 * TextDocument_SelectionRangePartialResult
 */
using TextDocument_SelectionRangePartialResult = Array<SelectionRange>;

/*
 * Window_WorkDoneProgress_CreateResult
 */
using Window_WorkDoneProgress_CreateResult = Null;

/*
 * TextDocument_PrepareCallHierarchyResult
 */
using TextDocument_PrepareCallHierarchyResult = NullOr<Array<CallHierarchyItem>>;

/*
 * CallHierarchy_IncomingCallsResult
 */
using CallHierarchy_IncomingCallsResult = NullOr<Array<CallHierarchyIncomingCall>>;

/*
 * CallHierarchy_IncomingCallsPartialResult
 */
using CallHierarchy_IncomingCallsPartialResult = Array<CallHierarchyIncomingCall>;

/*
 * CallHierarchy_OutgoingCallsResult
 */
using CallHierarchy_OutgoingCallsResult = NullOr<Array<CallHierarchyOutgoingCall>>;

/*
 * CallHierarchy_OutgoingCallsPartialResult
 */
using CallHierarchy_OutgoingCallsPartialResult = Array<CallHierarchyOutgoingCall>;

/*
 * TextDocument_SemanticTokens_FullResult
 */
using TextDocument_SemanticTokens_FullResult = NullOr<SemanticTokens>;

/*
 * TextDocument_SemanticTokens_Full_DeltaResult
 */
using TextDocument_SemanticTokens_Full_DeltaResult = NullOrOneOf<SemanticTokens, SemanticTokensDelta>;

/*
 * TextDocument_SemanticTokens_Full_DeltaPartialResult
 */
using TextDocument_SemanticTokens_Full_DeltaPartialResult = OneOf<SemanticTokensPartialResult, SemanticTokensDeltaPartialResult>;

/*
 * TextDocument_SemanticTokens_RangeResult
 */
using TextDocument_SemanticTokens_RangeResult = NullOr<SemanticTokens>;

/*
 * Workspace_SemanticTokens_RefreshResult
 */
using Workspace_SemanticTokens_RefreshResult = Null;

/*
 * TextDocument_LinkedEditingRangeResult
 */
using TextDocument_LinkedEditingRangeResult = NullOr<LinkedEditingRanges>;

/*
 * Workspace_WillCreateFilesResult
 */
using Workspace_WillCreateFilesResult = NullOr<WorkspaceEdit>;

/*
 * Workspace_WillRenameFilesResult
 */
using Workspace_WillRenameFilesResult = NullOr<WorkspaceEdit>;

/*
 * Workspace_WillDeleteFilesResult
 */
using Workspace_WillDeleteFilesResult = NullOr<WorkspaceEdit>;

/*
 * TextDocument_MonikerResult
 */
using TextDocument_MonikerResult = NullOr<Array<Moniker>>;

/*
 * TextDocument_MonikerPartialResult
 */
using TextDocument_MonikerPartialResult = Array<Moniker>;

/*
 * TextDocument_PrepareTypeHierarchyResult
 */
using TextDocument_PrepareTypeHierarchyResult = NullOr<Array<TypeHierarchyItem>>;

/*
 * TypeHierarchy_SupertypesResult
 */
using TypeHierarchy_SupertypesResult = NullOr<Array<TypeHierarchyItem>>;

/*
 * TypeHierarchy_SupertypesPartialResult
 */
using TypeHierarchy_SupertypesPartialResult = Array<TypeHierarchyItem>;

/*
 * TypeHierarchy_SubtypesResult
 */
using TypeHierarchy_SubtypesResult = NullOr<Array<TypeHierarchyItem>>;

/*
 * TypeHierarchy_SubtypesPartialResult
 */
using TypeHierarchy_SubtypesPartialResult = Array<TypeHierarchyItem>;

/*
 * TextDocument_InlineValueResult
 */
using TextDocument_InlineValueResult = NullOr<Array<InlineValue>>;

/*
 * TextDocument_InlineValuePartialResult
 */
using TextDocument_InlineValuePartialResult = Array<InlineValue>;

/*
 * Workspace_InlineValue_RefreshResult
 */
using Workspace_InlineValue_RefreshResult = Null;

/*
 * TextDocument_InlayHintResult
 */
using TextDocument_InlayHintResult = NullOr<Array<InlayHint>>;

/*
 * TextDocument_InlayHintPartialResult
 */
using TextDocument_InlayHintPartialResult = Array<InlayHint>;

/*
 * Workspace_InlayHint_RefreshResult
 */
using Workspace_InlayHint_RefreshResult = Null;

/*
 * Workspace_Diagnostic_RefreshResult
 */
using Workspace_Diagnostic_RefreshResult = Null;

/*
 * TextDocument_InlineCompletionResult
 */
using TextDocument_InlineCompletionResult = NullOrOneOf<InlineCompletionList, Array<InlineCompletionItem>>;

/*
 * TextDocument_InlineCompletionPartialResult
 */
using TextDocument_InlineCompletionPartialResult = Array<InlineCompletionItem>;

/*
 * Client_RegisterCapabilityResult
 */
using Client_RegisterCapabilityResult = Null;

/*
 * Client_UnregisterCapabilityResult
 */
using Client_UnregisterCapabilityResult = Null;

/*
 * ShutdownResult
 */
using ShutdownResult = Null;

/*
 * Window_ShowMessageRequestResult
 */
using Window_ShowMessageRequestResult = NullOr<MessageActionItem>;

/*
 * TextDocument_WillSaveWaitUntilResult
 */
using TextDocument_WillSaveWaitUntilResult = NullOr<Array<TextEdit>>;

/*
 * TextDocument_CompletionResult
 */
using TextDocument_CompletionResult = NullOrOneOf<Array<CompletionItem>, CompletionList>;

/*
 * TextDocument_CompletionPartialResult
 */
using TextDocument_CompletionPartialResult = Array<CompletionItem>;

/*
 * TextDocument_HoverResult
 */
using TextDocument_HoverResult = NullOr<Hover>;

/*
 * TextDocument_SignatureHelpResult
 */
using TextDocument_SignatureHelpResult = NullOr<SignatureHelp>;

/*
 * TextDocument_DefinitionResult
 */
using TextDocument_DefinitionResult = NullOrOneOf<Definition, Array<DefinitionLink>>;

/*
 * TextDocument_DefinitionPartialResult
 */
using TextDocument_DefinitionPartialResult = OneOf<Array<Location>, Array<DefinitionLink>>;

/*
 * TextDocument_ReferencesResult
 */
using TextDocument_ReferencesResult = NullOr<Array<Location>>;

/*
 * TextDocument_ReferencesPartialResult
 */
using TextDocument_ReferencesPartialResult = Array<Location>;

/*
 * TextDocument_DocumentHighlightResult
 */
using TextDocument_DocumentHighlightResult = NullOr<Array<DocumentHighlight>>;

/*
 * TextDocument_DocumentHighlightPartialResult
 */
using TextDocument_DocumentHighlightPartialResult = Array<DocumentHighlight>;

/*
 * TextDocument_DocumentSymbolResult
 */
using TextDocument_DocumentSymbolResult = NullOrOneOf<Array<SymbolInformation>, Array<DocumentSymbol>>;

/*
 * TextDocument_DocumentSymbolPartialResult
 */
using TextDocument_DocumentSymbolPartialResult = OneOf<Array<SymbolInformation>, Array<DocumentSymbol>>;

/*
 * TextDocument_CodeActionResult
 */
using TextDocument_CodeActionResult = NullOr<Array<OneOf<Command, CodeAction>>>;

/*
 * TextDocument_CodeActionPartialResult
 */
using TextDocument_CodeActionPartialResult = Array<OneOf<Command, CodeAction>>;

/*
 * Workspace_SymbolResult
 */
using Workspace_SymbolResult = NullOrOneOf<Array<SymbolInformation>, Array<WorkspaceSymbol>>;

/*
 * Workspace_SymbolPartialResult
 */
using Workspace_SymbolPartialResult = OneOf<Array<SymbolInformation>, Array<WorkspaceSymbol>>;

/*
 * TextDocument_CodeLensResult
 */
using TextDocument_CodeLensResult = NullOr<Array<CodeLens>>;

/*
 * TextDocument_CodeLensPartialResult
 */
using TextDocument_CodeLensPartialResult = Array<CodeLens>;

/*
 * Workspace_CodeLens_RefreshResult
 */
using Workspace_CodeLens_RefreshResult = Null;

/*
 * TextDocument_DocumentLinkResult
 */
using TextDocument_DocumentLinkResult = NullOr<Array<DocumentLink>>;

/*
 * TextDocument_DocumentLinkPartialResult
 */
using TextDocument_DocumentLinkPartialResult = Array<DocumentLink>;

/*
 * TextDocument_FormattingResult
 */
using TextDocument_FormattingResult = NullOr<Array<TextEdit>>;

/*
 * TextDocument_RangeFormattingResult
 */
using TextDocument_RangeFormattingResult = NullOr<Array<TextEdit>>;

/*
 * TextDocument_RangesFormattingResult
 */
using TextDocument_RangesFormattingResult = NullOr<Array<TextEdit>>;

/*
 * TextDocument_OnTypeFormattingResult
 */
using TextDocument_OnTypeFormattingResult = NullOr<Array<TextEdit>>;

/*
 * TextDocument_RenameResult
 */
using TextDocument_RenameResult = NullOr<WorkspaceEdit>;

/*
 * TextDocument_PrepareRenameResult
 */
using TextDocument_PrepareRenameResult = NullOr<PrepareRenameResult>;

/*
 * Workspace_ExecuteCommandResult
 */
using Workspace_ExecuteCommandResult = NullOr<LSPAny>;

/*
 * Serialization boilerplate
 */

template<>
const char** requiredProperties<TextDocumentIdentifier>();
json::Any toJson(TextDocumentIdentifier&& value);
void fromJson(json::Any&& json, TextDocumentIdentifier& value);
template<>
const char** requiredProperties<Position>();
json::Any toJson(Position&& value);
void fromJson(json::Any&& json, Position& value);
template<>
const char** requiredProperties<TextDocumentPositionParams>();
json::Any toJson(TextDocumentPositionParams&& value);
void fromJson(json::Any&& json, TextDocumentPositionParams& value);
json::Any toJson(WorkDoneProgressParams&& value);
void fromJson(json::Any&& json, WorkDoneProgressParams& value);
json::Any toJson(PartialResultParams&& value);
void fromJson(json::Any&& json, PartialResultParams& value);
template<>
const char** requiredProperties<ImplementationParams>();
json::Any toJson(ImplementationParams&& value);
void fromJson(json::Any&& json, ImplementationParams& value);
template<>
const char** requiredProperties<Range>();
json::Any toJson(Range&& value);
void fromJson(json::Any&& json, Range& value);
template<>
const char** requiredProperties<Location>();
json::Any toJson(Location&& value);
void fromJson(json::Any&& json, Location& value);
template<>
const char** requiredProperties<TextDocumentFilter_Language>();
json::Any toJson(TextDocumentFilter_Language&& value);
void fromJson(json::Any&& json, TextDocumentFilter_Language& value);
template<>
const char** requiredProperties<TextDocumentFilter_Scheme>();
json::Any toJson(TextDocumentFilter_Scheme&& value);
void fromJson(json::Any&& json, TextDocumentFilter_Scheme& value);
template<>
const char** requiredProperties<TextDocumentFilter_Pattern>();
json::Any toJson(TextDocumentFilter_Pattern&& value);
void fromJson(json::Any&& json, TextDocumentFilter_Pattern& value);
template<>
const char** requiredProperties<NotebookDocumentFilter_NotebookType>();
json::Any toJson(NotebookDocumentFilter_NotebookType&& value);
void fromJson(json::Any&& json, NotebookDocumentFilter_NotebookType& value);
template<>
const char** requiredProperties<NotebookDocumentFilter_Scheme>();
json::Any toJson(NotebookDocumentFilter_Scheme&& value);
void fromJson(json::Any&& json, NotebookDocumentFilter_Scheme& value);
template<>
const char** requiredProperties<NotebookDocumentFilter_Pattern>();
json::Any toJson(NotebookDocumentFilter_Pattern&& value);
void fromJson(json::Any&& json, NotebookDocumentFilter_Pattern& value);
template<>
const char** requiredProperties<NotebookCellTextDocumentFilter>();
json::Any toJson(NotebookCellTextDocumentFilter&& value);
void fromJson(json::Any&& json, NotebookCellTextDocumentFilter& value);
template<>
const char** requiredProperties<TextDocumentRegistrationOptions>();
json::Any toJson(TextDocumentRegistrationOptions&& value);
void fromJson(json::Any&& json, TextDocumentRegistrationOptions& value);
json::Any toJson(WorkDoneProgressOptions&& value);
void fromJson(json::Any&& json, WorkDoneProgressOptions& value);
json::Any toJson(ImplementationOptions&& value);
void fromJson(json::Any&& json, ImplementationOptions& value);
json::Any toJson(StaticRegistrationOptions&& value);
void fromJson(json::Any&& json, StaticRegistrationOptions& value);
template<>
const char** requiredProperties<ImplementationRegistrationOptions>();
json::Any toJson(ImplementationRegistrationOptions&& value);
void fromJson(json::Any&& json, ImplementationRegistrationOptions& value);
template<>
const char** requiredProperties<TypeDefinitionParams>();
json::Any toJson(TypeDefinitionParams&& value);
void fromJson(json::Any&& json, TypeDefinitionParams& value);
json::Any toJson(TypeDefinitionOptions&& value);
void fromJson(json::Any&& json, TypeDefinitionOptions& value);
template<>
const char** requiredProperties<TypeDefinitionRegistrationOptions>();
json::Any toJson(TypeDefinitionRegistrationOptions&& value);
void fromJson(json::Any&& json, TypeDefinitionRegistrationOptions& value);
template<>
const char** requiredProperties<WorkspaceFolder>();
json::Any toJson(WorkspaceFolder&& value);
void fromJson(json::Any&& json, WorkspaceFolder& value);
template<>
const char** requiredProperties<WorkspaceFoldersChangeEvent>();
json::Any toJson(WorkspaceFoldersChangeEvent&& value);
void fromJson(json::Any&& json, WorkspaceFoldersChangeEvent& value);
template<>
const char** requiredProperties<DidChangeWorkspaceFoldersParams>();
json::Any toJson(DidChangeWorkspaceFoldersParams&& value);
void fromJson(json::Any&& json, DidChangeWorkspaceFoldersParams& value);
json::Any toJson(ConfigurationItem&& value);
void fromJson(json::Any&& json, ConfigurationItem& value);
template<>
const char** requiredProperties<ConfigurationParams>();
json::Any toJson(ConfigurationParams&& value);
void fromJson(json::Any&& json, ConfigurationParams& value);
template<>
const char** requiredProperties<DocumentColorParams>();
json::Any toJson(DocumentColorParams&& value);
void fromJson(json::Any&& json, DocumentColorParams& value);
template<>
const char** requiredProperties<Color>();
json::Any toJson(Color&& value);
void fromJson(json::Any&& json, Color& value);
template<>
const char** requiredProperties<ColorInformation>();
json::Any toJson(ColorInformation&& value);
void fromJson(json::Any&& json, ColorInformation& value);
json::Any toJson(DocumentColorOptions&& value);
void fromJson(json::Any&& json, DocumentColorOptions& value);
template<>
const char** requiredProperties<DocumentColorRegistrationOptions>();
json::Any toJson(DocumentColorRegistrationOptions&& value);
void fromJson(json::Any&& json, DocumentColorRegistrationOptions& value);
template<>
const char** requiredProperties<ColorPresentationParams>();
json::Any toJson(ColorPresentationParams&& value);
void fromJson(json::Any&& json, ColorPresentationParams& value);
template<>
const char** requiredProperties<TextEdit>();
json::Any toJson(TextEdit&& value);
void fromJson(json::Any&& json, TextEdit& value);
template<>
const char** requiredProperties<ColorPresentation>();
json::Any toJson(ColorPresentation&& value);
void fromJson(json::Any&& json, ColorPresentation& value);
template<>
const char** requiredProperties<FoldingRangeParams>();
json::Any toJson(FoldingRangeParams&& value);
void fromJson(json::Any&& json, FoldingRangeParams& value);
template<>
const char** requiredProperties<FoldingRange>();
json::Any toJson(FoldingRange&& value);
void fromJson(json::Any&& json, FoldingRange& value);
json::Any toJson(FoldingRangeOptions&& value);
void fromJson(json::Any&& json, FoldingRangeOptions& value);
template<>
const char** requiredProperties<FoldingRangeRegistrationOptions>();
json::Any toJson(FoldingRangeRegistrationOptions&& value);
void fromJson(json::Any&& json, FoldingRangeRegistrationOptions& value);
template<>
const char** requiredProperties<DeclarationParams>();
json::Any toJson(DeclarationParams&& value);
void fromJson(json::Any&& json, DeclarationParams& value);
json::Any toJson(DeclarationOptions&& value);
void fromJson(json::Any&& json, DeclarationOptions& value);
template<>
const char** requiredProperties<DeclarationRegistrationOptions>();
json::Any toJson(DeclarationRegistrationOptions&& value);
void fromJson(json::Any&& json, DeclarationRegistrationOptions& value);
template<>
const char** requiredProperties<SelectionRangeParams>();
json::Any toJson(SelectionRangeParams&& value);
void fromJson(json::Any&& json, SelectionRangeParams& value);
template<>
const char** requiredProperties<SelectionRange>();
json::Any toJson(SelectionRange&& value);
void fromJson(json::Any&& json, SelectionRange& value);
json::Any toJson(SelectionRangeOptions&& value);
void fromJson(json::Any&& json, SelectionRangeOptions& value);
template<>
const char** requiredProperties<SelectionRangeRegistrationOptions>();
json::Any toJson(SelectionRangeRegistrationOptions&& value);
void fromJson(json::Any&& json, SelectionRangeRegistrationOptions& value);
template<>
const char** requiredProperties<WorkDoneProgressCreateParams>();
json::Any toJson(WorkDoneProgressCreateParams&& value);
void fromJson(json::Any&& json, WorkDoneProgressCreateParams& value);
template<>
const char** requiredProperties<WorkDoneProgressCancelParams>();
json::Any toJson(WorkDoneProgressCancelParams&& value);
void fromJson(json::Any&& json, WorkDoneProgressCancelParams& value);
template<>
const char** requiredProperties<CallHierarchyPrepareParams>();
json::Any toJson(CallHierarchyPrepareParams&& value);
void fromJson(json::Any&& json, CallHierarchyPrepareParams& value);
template<>
const char** requiredProperties<CallHierarchyItem>();
json::Any toJson(CallHierarchyItem&& value);
void fromJson(json::Any&& json, CallHierarchyItem& value);
json::Any toJson(CallHierarchyOptions&& value);
void fromJson(json::Any&& json, CallHierarchyOptions& value);
template<>
const char** requiredProperties<CallHierarchyRegistrationOptions>();
json::Any toJson(CallHierarchyRegistrationOptions&& value);
void fromJson(json::Any&& json, CallHierarchyRegistrationOptions& value);
template<>
const char** requiredProperties<CallHierarchyIncomingCallsParams>();
json::Any toJson(CallHierarchyIncomingCallsParams&& value);
void fromJson(json::Any&& json, CallHierarchyIncomingCallsParams& value);
template<>
const char** requiredProperties<CallHierarchyIncomingCall>();
json::Any toJson(CallHierarchyIncomingCall&& value);
void fromJson(json::Any&& json, CallHierarchyIncomingCall& value);
template<>
const char** requiredProperties<CallHierarchyOutgoingCallsParams>();
json::Any toJson(CallHierarchyOutgoingCallsParams&& value);
void fromJson(json::Any&& json, CallHierarchyOutgoingCallsParams& value);
template<>
const char** requiredProperties<CallHierarchyOutgoingCall>();
json::Any toJson(CallHierarchyOutgoingCall&& value);
void fromJson(json::Any&& json, CallHierarchyOutgoingCall& value);
template<>
const char** requiredProperties<SemanticTokensParams>();
json::Any toJson(SemanticTokensParams&& value);
void fromJson(json::Any&& json, SemanticTokensParams& value);
template<>
const char** requiredProperties<SemanticTokens>();
json::Any toJson(SemanticTokens&& value);
void fromJson(json::Any&& json, SemanticTokens& value);
template<>
const char** requiredProperties<SemanticTokensPartialResult>();
json::Any toJson(SemanticTokensPartialResult&& value);
void fromJson(json::Any&& json, SemanticTokensPartialResult& value);
template<>
const char** requiredProperties<SemanticTokensLegend>();
json::Any toJson(SemanticTokensLegend&& value);
void fromJson(json::Any&& json, SemanticTokensLegend& value);
json::Any toJson(SemanticTokensOptionsRange&& value);
void fromJson(json::Any&& json, SemanticTokensOptionsRange& value);
json::Any toJson(SemanticTokensOptionsFull&& value);
void fromJson(json::Any&& json, SemanticTokensOptionsFull& value);
template<>
const char** requiredProperties<SemanticTokensOptions>();
json::Any toJson(SemanticTokensOptions&& value);
void fromJson(json::Any&& json, SemanticTokensOptions& value);
template<>
const char** requiredProperties<SemanticTokensRegistrationOptions>();
json::Any toJson(SemanticTokensRegistrationOptions&& value);
void fromJson(json::Any&& json, SemanticTokensRegistrationOptions& value);
template<>
const char** requiredProperties<SemanticTokensDeltaParams>();
json::Any toJson(SemanticTokensDeltaParams&& value);
void fromJson(json::Any&& json, SemanticTokensDeltaParams& value);
template<>
const char** requiredProperties<SemanticTokensEdit>();
json::Any toJson(SemanticTokensEdit&& value);
void fromJson(json::Any&& json, SemanticTokensEdit& value);
template<>
const char** requiredProperties<SemanticTokensDelta>();
json::Any toJson(SemanticTokensDelta&& value);
void fromJson(json::Any&& json, SemanticTokensDelta& value);
template<>
const char** requiredProperties<SemanticTokensDeltaPartialResult>();
json::Any toJson(SemanticTokensDeltaPartialResult&& value);
void fromJson(json::Any&& json, SemanticTokensDeltaPartialResult& value);
template<>
const char** requiredProperties<SemanticTokensRangeParams>();
json::Any toJson(SemanticTokensRangeParams&& value);
void fromJson(json::Any&& json, SemanticTokensRangeParams& value);
template<>
const char** requiredProperties<ShowDocumentParams>();
json::Any toJson(ShowDocumentParams&& value);
void fromJson(json::Any&& json, ShowDocumentParams& value);
template<>
const char** requiredProperties<ShowDocumentResult>();
json::Any toJson(ShowDocumentResult&& value);
void fromJson(json::Any&& json, ShowDocumentResult& value);
template<>
const char** requiredProperties<LinkedEditingRangeParams>();
json::Any toJson(LinkedEditingRangeParams&& value);
void fromJson(json::Any&& json, LinkedEditingRangeParams& value);
template<>
const char** requiredProperties<LinkedEditingRanges>();
json::Any toJson(LinkedEditingRanges&& value);
void fromJson(json::Any&& json, LinkedEditingRanges& value);
json::Any toJson(LinkedEditingRangeOptions&& value);
void fromJson(json::Any&& json, LinkedEditingRangeOptions& value);
template<>
const char** requiredProperties<LinkedEditingRangeRegistrationOptions>();
json::Any toJson(LinkedEditingRangeRegistrationOptions&& value);
void fromJson(json::Any&& json, LinkedEditingRangeRegistrationOptions& value);
template<>
const char** requiredProperties<FileCreate>();
json::Any toJson(FileCreate&& value);
void fromJson(json::Any&& json, FileCreate& value);
template<>
const char** requiredProperties<CreateFilesParams>();
json::Any toJson(CreateFilesParams&& value);
void fromJson(json::Any&& json, CreateFilesParams& value);
template<>
const char** requiredProperties<OptionalVersionedTextDocumentIdentifier>();
json::Any toJson(OptionalVersionedTextDocumentIdentifier&& value);
void fromJson(json::Any&& json, OptionalVersionedTextDocumentIdentifier& value);
template<>
const char** requiredProperties<AnnotatedTextEdit>();
json::Any toJson(AnnotatedTextEdit&& value);
void fromJson(json::Any&& json, AnnotatedTextEdit& value);
template<>
const char** requiredProperties<TextDocumentEdit>();
json::Any toJson(TextDocumentEdit&& value);
void fromJson(json::Any&& json, TextDocumentEdit& value);
template<>
const char** requiredProperties<ResourceOperation>();
json::Any toJson(ResourceOperation&& value);
void fromJson(json::Any&& json, ResourceOperation& value);
json::Any toJson(CreateFileOptions&& value);
void fromJson(json::Any&& json, CreateFileOptions& value);
template<>
const char** requiredProperties<CreateFile>();
template<>
const std::pair<const char*, json::Any>* literalProperties<CreateFile>();
json::Any toJson(CreateFile&& value);
void fromJson(json::Any&& json, CreateFile& value);
json::Any toJson(RenameFileOptions&& value);
void fromJson(json::Any&& json, RenameFileOptions& value);
template<>
const char** requiredProperties<RenameFile>();
template<>
const std::pair<const char*, json::Any>* literalProperties<RenameFile>();
json::Any toJson(RenameFile&& value);
void fromJson(json::Any&& json, RenameFile& value);
json::Any toJson(DeleteFileOptions&& value);
void fromJson(json::Any&& json, DeleteFileOptions& value);
template<>
const char** requiredProperties<DeleteFile>();
template<>
const std::pair<const char*, json::Any>* literalProperties<DeleteFile>();
json::Any toJson(DeleteFile&& value);
void fromJson(json::Any&& json, DeleteFile& value);
template<>
const char** requiredProperties<ChangeAnnotation>();
json::Any toJson(ChangeAnnotation&& value);
void fromJson(json::Any&& json, ChangeAnnotation& value);
json::Any toJson(WorkspaceEdit&& value);
void fromJson(json::Any&& json, WorkspaceEdit& value);
json::Any toJson(FileOperationPatternOptions&& value);
void fromJson(json::Any&& json, FileOperationPatternOptions& value);
template<>
const char** requiredProperties<FileOperationPattern>();
json::Any toJson(FileOperationPattern&& value);
void fromJson(json::Any&& json, FileOperationPattern& value);
template<>
const char** requiredProperties<FileOperationFilter>();
json::Any toJson(FileOperationFilter&& value);
void fromJson(json::Any&& json, FileOperationFilter& value);
template<>
const char** requiredProperties<FileOperationRegistrationOptions>();
json::Any toJson(FileOperationRegistrationOptions&& value);
void fromJson(json::Any&& json, FileOperationRegistrationOptions& value);
template<>
const char** requiredProperties<FileRename>();
json::Any toJson(FileRename&& value);
void fromJson(json::Any&& json, FileRename& value);
template<>
const char** requiredProperties<RenameFilesParams>();
json::Any toJson(RenameFilesParams&& value);
void fromJson(json::Any&& json, RenameFilesParams& value);
template<>
const char** requiredProperties<FileDelete>();
json::Any toJson(FileDelete&& value);
void fromJson(json::Any&& json, FileDelete& value);
template<>
const char** requiredProperties<DeleteFilesParams>();
json::Any toJson(DeleteFilesParams&& value);
void fromJson(json::Any&& json, DeleteFilesParams& value);
template<>
const char** requiredProperties<MonikerParams>();
json::Any toJson(MonikerParams&& value);
void fromJson(json::Any&& json, MonikerParams& value);
template<>
const char** requiredProperties<Moniker>();
json::Any toJson(Moniker&& value);
void fromJson(json::Any&& json, Moniker& value);
json::Any toJson(MonikerOptions&& value);
void fromJson(json::Any&& json, MonikerOptions& value);
template<>
const char** requiredProperties<MonikerRegistrationOptions>();
json::Any toJson(MonikerRegistrationOptions&& value);
void fromJson(json::Any&& json, MonikerRegistrationOptions& value);
template<>
const char** requiredProperties<TypeHierarchyPrepareParams>();
json::Any toJson(TypeHierarchyPrepareParams&& value);
void fromJson(json::Any&& json, TypeHierarchyPrepareParams& value);
template<>
const char** requiredProperties<TypeHierarchyItem>();
json::Any toJson(TypeHierarchyItem&& value);
void fromJson(json::Any&& json, TypeHierarchyItem& value);
json::Any toJson(TypeHierarchyOptions&& value);
void fromJson(json::Any&& json, TypeHierarchyOptions& value);
template<>
const char** requiredProperties<TypeHierarchyRegistrationOptions>();
json::Any toJson(TypeHierarchyRegistrationOptions&& value);
void fromJson(json::Any&& json, TypeHierarchyRegistrationOptions& value);
template<>
const char** requiredProperties<TypeHierarchySupertypesParams>();
json::Any toJson(TypeHierarchySupertypesParams&& value);
void fromJson(json::Any&& json, TypeHierarchySupertypesParams& value);
template<>
const char** requiredProperties<TypeHierarchySubtypesParams>();
json::Any toJson(TypeHierarchySubtypesParams&& value);
void fromJson(json::Any&& json, TypeHierarchySubtypesParams& value);
template<>
const char** requiredProperties<InlineValueContext>();
json::Any toJson(InlineValueContext&& value);
void fromJson(json::Any&& json, InlineValueContext& value);
template<>
const char** requiredProperties<InlineValueParams>();
json::Any toJson(InlineValueParams&& value);
void fromJson(json::Any&& json, InlineValueParams& value);
json::Any toJson(InlineValueOptions&& value);
void fromJson(json::Any&& json, InlineValueOptions& value);
template<>
const char** requiredProperties<InlineValueRegistrationOptions>();
json::Any toJson(InlineValueRegistrationOptions&& value);
void fromJson(json::Any&& json, InlineValueRegistrationOptions& value);
template<>
const char** requiredProperties<InlayHintParams>();
json::Any toJson(InlayHintParams&& value);
void fromJson(json::Any&& json, InlayHintParams& value);
template<>
const char** requiredProperties<MarkupContent>();
json::Any toJson(MarkupContent&& value);
void fromJson(json::Any&& json, MarkupContent& value);
template<>
const char** requiredProperties<Command>();
json::Any toJson(Command&& value);
void fromJson(json::Any&& json, Command& value);
template<>
const char** requiredProperties<InlayHintLabelPart>();
json::Any toJson(InlayHintLabelPart&& value);
void fromJson(json::Any&& json, InlayHintLabelPart& value);
template<>
const char** requiredProperties<InlayHint>();
json::Any toJson(InlayHint&& value);
void fromJson(json::Any&& json, InlayHint& value);
json::Any toJson(InlayHintOptions&& value);
void fromJson(json::Any&& json, InlayHintOptions& value);
template<>
const char** requiredProperties<InlayHintRegistrationOptions>();
json::Any toJson(InlayHintRegistrationOptions&& value);
void fromJson(json::Any&& json, InlayHintRegistrationOptions& value);
template<>
const char** requiredProperties<DocumentDiagnosticParams>();
json::Any toJson(DocumentDiagnosticParams&& value);
void fromJson(json::Any&& json, DocumentDiagnosticParams& value);
template<>
const char** requiredProperties<CodeDescription>();
json::Any toJson(CodeDescription&& value);
void fromJson(json::Any&& json, CodeDescription& value);
template<>
const char** requiredProperties<DiagnosticRelatedInformation>();
json::Any toJson(DiagnosticRelatedInformation&& value);
void fromJson(json::Any&& json, DiagnosticRelatedInformation& value);
template<>
const char** requiredProperties<Diagnostic>();
json::Any toJson(Diagnostic&& value);
void fromJson(json::Any&& json, Diagnostic& value);
template<>
const char** requiredProperties<FullDocumentDiagnosticReport>();
template<>
const std::pair<const char*, json::Any>* literalProperties<FullDocumentDiagnosticReport>();
json::Any toJson(FullDocumentDiagnosticReport&& value);
void fromJson(json::Any&& json, FullDocumentDiagnosticReport& value);
template<>
const char** requiredProperties<UnchangedDocumentDiagnosticReport>();
template<>
const std::pair<const char*, json::Any>* literalProperties<UnchangedDocumentDiagnosticReport>();
json::Any toJson(UnchangedDocumentDiagnosticReport&& value);
void fromJson(json::Any&& json, UnchangedDocumentDiagnosticReport& value);
template<>
const char** requiredProperties<DocumentDiagnosticReportPartialResult>();
json::Any toJson(DocumentDiagnosticReportPartialResult&& value);
void fromJson(json::Any&& json, DocumentDiagnosticReportPartialResult& value);
template<>
const char** requiredProperties<DiagnosticServerCancellationData>();
json::Any toJson(DiagnosticServerCancellationData&& value);
void fromJson(json::Any&& json, DiagnosticServerCancellationData& value);
template<>
const char** requiredProperties<DiagnosticOptions>();
json::Any toJson(DiagnosticOptions&& value);
void fromJson(json::Any&& json, DiagnosticOptions& value);
template<>
const char** requiredProperties<DiagnosticRegistrationOptions>();
json::Any toJson(DiagnosticRegistrationOptions&& value);
void fromJson(json::Any&& json, DiagnosticRegistrationOptions& value);
template<>
const char** requiredProperties<PreviousResultId>();
json::Any toJson(PreviousResultId&& value);
void fromJson(json::Any&& json, PreviousResultId& value);
template<>
const char** requiredProperties<WorkspaceDiagnosticParams>();
json::Any toJson(WorkspaceDiagnosticParams&& value);
void fromJson(json::Any&& json, WorkspaceDiagnosticParams& value);
template<>
const char** requiredProperties<WorkspaceFullDocumentDiagnosticReport>();
json::Any toJson(WorkspaceFullDocumentDiagnosticReport&& value);
void fromJson(json::Any&& json, WorkspaceFullDocumentDiagnosticReport& value);
template<>
const char** requiredProperties<WorkspaceUnchangedDocumentDiagnosticReport>();
json::Any toJson(WorkspaceUnchangedDocumentDiagnosticReport&& value);
void fromJson(json::Any&& json, WorkspaceUnchangedDocumentDiagnosticReport& value);
template<>
const char** requiredProperties<WorkspaceDiagnosticReport>();
json::Any toJson(WorkspaceDiagnosticReport&& value);
void fromJson(json::Any&& json, WorkspaceDiagnosticReport& value);
template<>
const char** requiredProperties<WorkspaceDiagnosticReportPartialResult>();
json::Any toJson(WorkspaceDiagnosticReportPartialResult&& value);
void fromJson(json::Any&& json, WorkspaceDiagnosticReportPartialResult& value);
template<>
const char** requiredProperties<ExecutionSummary>();
json::Any toJson(ExecutionSummary&& value);
void fromJson(json::Any&& json, ExecutionSummary& value);
template<>
const char** requiredProperties<NotebookCell>();
json::Any toJson(NotebookCell&& value);
void fromJson(json::Any&& json, NotebookCell& value);
template<>
const char** requiredProperties<NotebookDocument>();
json::Any toJson(NotebookDocument&& value);
void fromJson(json::Any&& json, NotebookDocument& value);
template<>
const char** requiredProperties<TextDocumentItem>();
json::Any toJson(TextDocumentItem&& value);
void fromJson(json::Any&& json, TextDocumentItem& value);
template<>
const char** requiredProperties<DidOpenNotebookDocumentParams>();
json::Any toJson(DidOpenNotebookDocumentParams&& value);
void fromJson(json::Any&& json, DidOpenNotebookDocumentParams& value);
template<>
const char** requiredProperties<VersionedNotebookDocumentIdentifier>();
json::Any toJson(VersionedNotebookDocumentIdentifier&& value);
void fromJson(json::Any&& json, VersionedNotebookDocumentIdentifier& value);
template<>
const char** requiredProperties<NotebookCellArrayChange>();
json::Any toJson(NotebookCellArrayChange&& value);
void fromJson(json::Any&& json, NotebookCellArrayChange& value);
template<>
const char** requiredProperties<NotebookDocumentChangeEventCellsStructure>();
json::Any toJson(NotebookDocumentChangeEventCellsStructure&& value);
void fromJson(json::Any&& json, NotebookDocumentChangeEventCellsStructure& value);
template<>
const char** requiredProperties<VersionedTextDocumentIdentifier>();
json::Any toJson(VersionedTextDocumentIdentifier&& value);
void fromJson(json::Any&& json, VersionedTextDocumentIdentifier& value);
template<>
const char** requiredProperties<TextDocumentContentChangeEvent_Range_Text>();
json::Any toJson(TextDocumentContentChangeEvent_Range_Text&& value);
void fromJson(json::Any&& json, TextDocumentContentChangeEvent_Range_Text& value);
template<>
const char** requiredProperties<TextDocumentContentChangeEvent_Text>();
json::Any toJson(TextDocumentContentChangeEvent_Text&& value);
void fromJson(json::Any&& json, TextDocumentContentChangeEvent_Text& value);
template<>
const char** requiredProperties<NotebookDocumentChangeEventCellsTextContent>();
json::Any toJson(NotebookDocumentChangeEventCellsTextContent&& value);
void fromJson(json::Any&& json, NotebookDocumentChangeEventCellsTextContent& value);
json::Any toJson(NotebookDocumentChangeEventCells&& value);
void fromJson(json::Any&& json, NotebookDocumentChangeEventCells& value);
json::Any toJson(NotebookDocumentChangeEvent&& value);
void fromJson(json::Any&& json, NotebookDocumentChangeEvent& value);
template<>
const char** requiredProperties<DidChangeNotebookDocumentParams>();
json::Any toJson(DidChangeNotebookDocumentParams&& value);
void fromJson(json::Any&& json, DidChangeNotebookDocumentParams& value);
template<>
const char** requiredProperties<NotebookDocumentIdentifier>();
json::Any toJson(NotebookDocumentIdentifier&& value);
void fromJson(json::Any&& json, NotebookDocumentIdentifier& value);
template<>
const char** requiredProperties<DidSaveNotebookDocumentParams>();
json::Any toJson(DidSaveNotebookDocumentParams&& value);
void fromJson(json::Any&& json, DidSaveNotebookDocumentParams& value);
template<>
const char** requiredProperties<DidCloseNotebookDocumentParams>();
json::Any toJson(DidCloseNotebookDocumentParams&& value);
void fromJson(json::Any&& json, DidCloseNotebookDocumentParams& value);
template<>
const char** requiredProperties<SelectedCompletionInfo>();
json::Any toJson(SelectedCompletionInfo&& value);
void fromJson(json::Any&& json, SelectedCompletionInfo& value);
template<>
const char** requiredProperties<InlineCompletionContext>();
json::Any toJson(InlineCompletionContext&& value);
void fromJson(json::Any&& json, InlineCompletionContext& value);
template<>
const char** requiredProperties<InlineCompletionParams>();
json::Any toJson(InlineCompletionParams&& value);
void fromJson(json::Any&& json, InlineCompletionParams& value);
template<>
const char** requiredProperties<StringValue>();
template<>
const std::pair<const char*, json::Any>* literalProperties<StringValue>();
json::Any toJson(StringValue&& value);
void fromJson(json::Any&& json, StringValue& value);
template<>
const char** requiredProperties<InlineCompletionItem>();
json::Any toJson(InlineCompletionItem&& value);
void fromJson(json::Any&& json, InlineCompletionItem& value);
template<>
const char** requiredProperties<InlineCompletionList>();
json::Any toJson(InlineCompletionList&& value);
void fromJson(json::Any&& json, InlineCompletionList& value);
json::Any toJson(InlineCompletionOptions&& value);
void fromJson(json::Any&& json, InlineCompletionOptions& value);
template<>
const char** requiredProperties<InlineCompletionRegistrationOptions>();
json::Any toJson(InlineCompletionRegistrationOptions&& value);
void fromJson(json::Any&& json, InlineCompletionRegistrationOptions& value);
template<>
const char** requiredProperties<Registration>();
json::Any toJson(Registration&& value);
void fromJson(json::Any&& json, Registration& value);
template<>
const char** requiredProperties<RegistrationParams>();
json::Any toJson(RegistrationParams&& value);
void fromJson(json::Any&& json, RegistrationParams& value);
template<>
const char** requiredProperties<Unregistration>();
json::Any toJson(Unregistration&& value);
void fromJson(json::Any&& json, Unregistration& value);
template<>
const char** requiredProperties<UnregistrationParams>();
json::Any toJson(UnregistrationParams&& value);
void fromJson(json::Any&& json, UnregistrationParams& value);
json::Any toJson(WorkspaceEditClientCapabilitiesChangeAnnotationSupport&& value);
void fromJson(json::Any&& json, WorkspaceEditClientCapabilitiesChangeAnnotationSupport& value);
json::Any toJson(WorkspaceEditClientCapabilities&& value);
void fromJson(json::Any&& json, WorkspaceEditClientCapabilities& value);
json::Any toJson(DidChangeConfigurationClientCapabilities&& value);
void fromJson(json::Any&& json, DidChangeConfigurationClientCapabilities& value);
json::Any toJson(DidChangeWatchedFilesClientCapabilities&& value);
void fromJson(json::Any&& json, DidChangeWatchedFilesClientCapabilities& value);
json::Any toJson(WorkspaceSymbolClientCapabilitiesSymbolKind&& value);
void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilitiesSymbolKind& value);
template<>
const char** requiredProperties<WorkspaceSymbolClientCapabilitiesTagSupport>();
json::Any toJson(WorkspaceSymbolClientCapabilitiesTagSupport&& value);
void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilitiesTagSupport& value);
template<>
const char** requiredProperties<WorkspaceSymbolClientCapabilitiesResolveSupport>();
json::Any toJson(WorkspaceSymbolClientCapabilitiesResolveSupport&& value);
void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilitiesResolveSupport& value);
json::Any toJson(WorkspaceSymbolClientCapabilities&& value);
void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilities& value);
json::Any toJson(ExecuteCommandClientCapabilities&& value);
void fromJson(json::Any&& json, ExecuteCommandClientCapabilities& value);
json::Any toJson(SemanticTokensWorkspaceClientCapabilities&& value);
void fromJson(json::Any&& json, SemanticTokensWorkspaceClientCapabilities& value);
json::Any toJson(CodeLensWorkspaceClientCapabilities&& value);
void fromJson(json::Any&& json, CodeLensWorkspaceClientCapabilities& value);
json::Any toJson(FileOperationClientCapabilities&& value);
void fromJson(json::Any&& json, FileOperationClientCapabilities& value);
json::Any toJson(InlineValueWorkspaceClientCapabilities&& value);
void fromJson(json::Any&& json, InlineValueWorkspaceClientCapabilities& value);
json::Any toJson(InlayHintWorkspaceClientCapabilities&& value);
void fromJson(json::Any&& json, InlayHintWorkspaceClientCapabilities& value);
json::Any toJson(DiagnosticWorkspaceClientCapabilities&& value);
void fromJson(json::Any&& json, DiagnosticWorkspaceClientCapabilities& value);
json::Any toJson(FoldingRangeWorkspaceClientCapabilities&& value);
void fromJson(json::Any&& json, FoldingRangeWorkspaceClientCapabilities& value);
json::Any toJson(WorkspaceClientCapabilities&& value);
void fromJson(json::Any&& json, WorkspaceClientCapabilities& value);
json::Any toJson(TextDocumentSyncClientCapabilities&& value);
void fromJson(json::Any&& json, TextDocumentSyncClientCapabilities& value);
template<>
const char** requiredProperties<CompletionClientCapabilitiesCompletionItemTagSupport>();
json::Any toJson(CompletionClientCapabilitiesCompletionItemTagSupport&& value);
void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemTagSupport& value);
template<>
const char** requiredProperties<CompletionClientCapabilitiesCompletionItemResolveSupport>();
json::Any toJson(CompletionClientCapabilitiesCompletionItemResolveSupport&& value);
void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemResolveSupport& value);
template<>
const char** requiredProperties<CompletionClientCapabilitiesCompletionItemInsertTextModeSupport>();
json::Any toJson(CompletionClientCapabilitiesCompletionItemInsertTextModeSupport&& value);
void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemInsertTextModeSupport& value);
json::Any toJson(CompletionClientCapabilitiesCompletionItem&& value);
void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItem& value);
json::Any toJson(CompletionClientCapabilitiesCompletionItemKind&& value);
void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemKind& value);
json::Any toJson(CompletionClientCapabilitiesCompletionList&& value);
void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionList& value);
json::Any toJson(CompletionClientCapabilities&& value);
void fromJson(json::Any&& json, CompletionClientCapabilities& value);
json::Any toJson(HoverClientCapabilities&& value);
void fromJson(json::Any&& json, HoverClientCapabilities& value);
json::Any toJson(SignatureHelpClientCapabilitiesSignatureInformationParameterInformation&& value);
void fromJson(json::Any&& json, SignatureHelpClientCapabilitiesSignatureInformationParameterInformation& value);
json::Any toJson(SignatureHelpClientCapabilitiesSignatureInformation&& value);
void fromJson(json::Any&& json, SignatureHelpClientCapabilitiesSignatureInformation& value);
json::Any toJson(SignatureHelpClientCapabilities&& value);
void fromJson(json::Any&& json, SignatureHelpClientCapabilities& value);
json::Any toJson(DeclarationClientCapabilities&& value);
void fromJson(json::Any&& json, DeclarationClientCapabilities& value);
json::Any toJson(DefinitionClientCapabilities&& value);
void fromJson(json::Any&& json, DefinitionClientCapabilities& value);
json::Any toJson(TypeDefinitionClientCapabilities&& value);
void fromJson(json::Any&& json, TypeDefinitionClientCapabilities& value);
json::Any toJson(ImplementationClientCapabilities&& value);
void fromJson(json::Any&& json, ImplementationClientCapabilities& value);
json::Any toJson(ReferenceClientCapabilities&& value);
void fromJson(json::Any&& json, ReferenceClientCapabilities& value);
json::Any toJson(DocumentHighlightClientCapabilities&& value);
void fromJson(json::Any&& json, DocumentHighlightClientCapabilities& value);
json::Any toJson(DocumentSymbolClientCapabilitiesSymbolKind&& value);
void fromJson(json::Any&& json, DocumentSymbolClientCapabilitiesSymbolKind& value);
template<>
const char** requiredProperties<DocumentSymbolClientCapabilitiesTagSupport>();
json::Any toJson(DocumentSymbolClientCapabilitiesTagSupport&& value);
void fromJson(json::Any&& json, DocumentSymbolClientCapabilitiesTagSupport& value);
json::Any toJson(DocumentSymbolClientCapabilities&& value);
void fromJson(json::Any&& json, DocumentSymbolClientCapabilities& value);
template<>
const char** requiredProperties<CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind>();
json::Any toJson(CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind&& value);
void fromJson(json::Any&& json, CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind& value);
template<>
const char** requiredProperties<CodeActionClientCapabilitiesCodeActionLiteralSupport>();
json::Any toJson(CodeActionClientCapabilitiesCodeActionLiteralSupport&& value);
void fromJson(json::Any&& json, CodeActionClientCapabilitiesCodeActionLiteralSupport& value);
template<>
const char** requiredProperties<CodeActionClientCapabilitiesResolveSupport>();
json::Any toJson(CodeActionClientCapabilitiesResolveSupport&& value);
void fromJson(json::Any&& json, CodeActionClientCapabilitiesResolveSupport& value);
json::Any toJson(CodeActionClientCapabilities&& value);
void fromJson(json::Any&& json, CodeActionClientCapabilities& value);
json::Any toJson(CodeLensClientCapabilities&& value);
void fromJson(json::Any&& json, CodeLensClientCapabilities& value);
json::Any toJson(DocumentLinkClientCapabilities&& value);
void fromJson(json::Any&& json, DocumentLinkClientCapabilities& value);
json::Any toJson(DocumentColorClientCapabilities&& value);
void fromJson(json::Any&& json, DocumentColorClientCapabilities& value);
json::Any toJson(DocumentFormattingClientCapabilities&& value);
void fromJson(json::Any&& json, DocumentFormattingClientCapabilities& value);
json::Any toJson(DocumentRangeFormattingClientCapabilities&& value);
void fromJson(json::Any&& json, DocumentRangeFormattingClientCapabilities& value);
json::Any toJson(DocumentOnTypeFormattingClientCapabilities&& value);
void fromJson(json::Any&& json, DocumentOnTypeFormattingClientCapabilities& value);
json::Any toJson(RenameClientCapabilities&& value);
void fromJson(json::Any&& json, RenameClientCapabilities& value);
json::Any toJson(FoldingRangeClientCapabilitiesFoldingRangeKind&& value);
void fromJson(json::Any&& json, FoldingRangeClientCapabilitiesFoldingRangeKind& value);
json::Any toJson(FoldingRangeClientCapabilitiesFoldingRange&& value);
void fromJson(json::Any&& json, FoldingRangeClientCapabilitiesFoldingRange& value);
json::Any toJson(FoldingRangeClientCapabilities&& value);
void fromJson(json::Any&& json, FoldingRangeClientCapabilities& value);
json::Any toJson(SelectionRangeClientCapabilities&& value);
void fromJson(json::Any&& json, SelectionRangeClientCapabilities& value);
template<>
const char** requiredProperties<PublishDiagnosticsClientCapabilitiesTagSupport>();
json::Any toJson(PublishDiagnosticsClientCapabilitiesTagSupport&& value);
void fromJson(json::Any&& json, PublishDiagnosticsClientCapabilitiesTagSupport& value);
json::Any toJson(PublishDiagnosticsClientCapabilities&& value);
void fromJson(json::Any&& json, PublishDiagnosticsClientCapabilities& value);
json::Any toJson(CallHierarchyClientCapabilities&& value);
void fromJson(json::Any&& json, CallHierarchyClientCapabilities& value);
json::Any toJson(SemanticTokensClientCapabilitiesRequestsRange&& value);
void fromJson(json::Any&& json, SemanticTokensClientCapabilitiesRequestsRange& value);
json::Any toJson(SemanticTokensClientCapabilitiesRequestsFull&& value);
void fromJson(json::Any&& json, SemanticTokensClientCapabilitiesRequestsFull& value);
json::Any toJson(SemanticTokensClientCapabilitiesRequests&& value);
void fromJson(json::Any&& json, SemanticTokensClientCapabilitiesRequests& value);
template<>
const char** requiredProperties<SemanticTokensClientCapabilities>();
json::Any toJson(SemanticTokensClientCapabilities&& value);
void fromJson(json::Any&& json, SemanticTokensClientCapabilities& value);
json::Any toJson(LinkedEditingRangeClientCapabilities&& value);
void fromJson(json::Any&& json, LinkedEditingRangeClientCapabilities& value);
json::Any toJson(MonikerClientCapabilities&& value);
void fromJson(json::Any&& json, MonikerClientCapabilities& value);
json::Any toJson(TypeHierarchyClientCapabilities&& value);
void fromJson(json::Any&& json, TypeHierarchyClientCapabilities& value);
json::Any toJson(InlineValueClientCapabilities&& value);
void fromJson(json::Any&& json, InlineValueClientCapabilities& value);
template<>
const char** requiredProperties<InlayHintClientCapabilitiesResolveSupport>();
json::Any toJson(InlayHintClientCapabilitiesResolveSupport&& value);
void fromJson(json::Any&& json, InlayHintClientCapabilitiesResolveSupport& value);
json::Any toJson(InlayHintClientCapabilities&& value);
void fromJson(json::Any&& json, InlayHintClientCapabilities& value);
json::Any toJson(DiagnosticClientCapabilities&& value);
void fromJson(json::Any&& json, DiagnosticClientCapabilities& value);
json::Any toJson(InlineCompletionClientCapabilities&& value);
void fromJson(json::Any&& json, InlineCompletionClientCapabilities& value);
json::Any toJson(TextDocumentClientCapabilities&& value);
void fromJson(json::Any&& json, TextDocumentClientCapabilities& value);
json::Any toJson(NotebookDocumentSyncClientCapabilities&& value);
void fromJson(json::Any&& json, NotebookDocumentSyncClientCapabilities& value);
template<>
const char** requiredProperties<NotebookDocumentClientCapabilities>();
json::Any toJson(NotebookDocumentClientCapabilities&& value);
void fromJson(json::Any&& json, NotebookDocumentClientCapabilities& value);
json::Any toJson(ShowMessageRequestClientCapabilitiesMessageActionItem&& value);
void fromJson(json::Any&& json, ShowMessageRequestClientCapabilitiesMessageActionItem& value);
json::Any toJson(ShowMessageRequestClientCapabilities&& value);
void fromJson(json::Any&& json, ShowMessageRequestClientCapabilities& value);
template<>
const char** requiredProperties<ShowDocumentClientCapabilities>();
json::Any toJson(ShowDocumentClientCapabilities&& value);
void fromJson(json::Any&& json, ShowDocumentClientCapabilities& value);
json::Any toJson(WindowClientCapabilities&& value);
void fromJson(json::Any&& json, WindowClientCapabilities& value);
template<>
const char** requiredProperties<GeneralClientCapabilitiesStaleRequestSupport>();
json::Any toJson(GeneralClientCapabilitiesStaleRequestSupport&& value);
void fromJson(json::Any&& json, GeneralClientCapabilitiesStaleRequestSupport& value);
template<>
const char** requiredProperties<RegularExpressionsClientCapabilities>();
json::Any toJson(RegularExpressionsClientCapabilities&& value);
void fromJson(json::Any&& json, RegularExpressionsClientCapabilities& value);
template<>
const char** requiredProperties<MarkdownClientCapabilities>();
json::Any toJson(MarkdownClientCapabilities&& value);
void fromJson(json::Any&& json, MarkdownClientCapabilities& value);
json::Any toJson(GeneralClientCapabilities&& value);
void fromJson(json::Any&& json, GeneralClientCapabilities& value);
json::Any toJson(ClientCapabilities&& value);
void fromJson(json::Any&& json, ClientCapabilities& value);
template<>
const char** requiredProperties<_InitializeParamsClientInfo>();
json::Any toJson(_InitializeParamsClientInfo&& value);
void fromJson(json::Any&& json, _InitializeParamsClientInfo& value);
template<>
const char** requiredProperties<_InitializeParams>();
json::Any toJson(_InitializeParams&& value);
void fromJson(json::Any&& json, _InitializeParams& value);
json::Any toJson(WorkspaceFoldersInitializeParams&& value);
void fromJson(json::Any&& json, WorkspaceFoldersInitializeParams& value);
template<>
const char** requiredProperties<InitializeParams>();
json::Any toJson(InitializeParams&& value);
void fromJson(json::Any&& json, InitializeParams& value);
json::Any toJson(SaveOptions&& value);
void fromJson(json::Any&& json, SaveOptions& value);
json::Any toJson(TextDocumentSyncOptions&& value);
void fromJson(json::Any&& json, TextDocumentSyncOptions& value);
template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_NotebookCells>();
json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_NotebookCells&& value);
void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_NotebookCells& value);
template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_Notebook>();
json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_Notebook&& value);
void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_Notebook& value);
template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_CellsCells>();
json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_CellsCells&& value);
void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_CellsCells& value);
template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_Cells>();
json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_Cells&& value);
void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_Cells& value);
template<>
const char** requiredProperties<NotebookDocumentSyncOptions>();
json::Any toJson(NotebookDocumentSyncOptions&& value);
void fromJson(json::Any&& json, NotebookDocumentSyncOptions& value);
template<>
const char** requiredProperties<NotebookDocumentSyncRegistrationOptions>();
json::Any toJson(NotebookDocumentSyncRegistrationOptions&& value);
void fromJson(json::Any&& json, NotebookDocumentSyncRegistrationOptions& value);
json::Any toJson(CompletionOptionsCompletionItem&& value);
void fromJson(json::Any&& json, CompletionOptionsCompletionItem& value);
json::Any toJson(CompletionOptions&& value);
void fromJson(json::Any&& json, CompletionOptions& value);
json::Any toJson(HoverOptions&& value);
void fromJson(json::Any&& json, HoverOptions& value);
json::Any toJson(SignatureHelpOptions&& value);
void fromJson(json::Any&& json, SignatureHelpOptions& value);
json::Any toJson(DefinitionOptions&& value);
void fromJson(json::Any&& json, DefinitionOptions& value);
json::Any toJson(ReferenceOptions&& value);
void fromJson(json::Any&& json, ReferenceOptions& value);
json::Any toJson(DocumentHighlightOptions&& value);
void fromJson(json::Any&& json, DocumentHighlightOptions& value);
json::Any toJson(DocumentSymbolOptions&& value);
void fromJson(json::Any&& json, DocumentSymbolOptions& value);
json::Any toJson(CodeActionOptions&& value);
void fromJson(json::Any&& json, CodeActionOptions& value);
json::Any toJson(CodeLensOptions&& value);
void fromJson(json::Any&& json, CodeLensOptions& value);
json::Any toJson(DocumentLinkOptions&& value);
void fromJson(json::Any&& json, DocumentLinkOptions& value);
json::Any toJson(WorkspaceSymbolOptions&& value);
void fromJson(json::Any&& json, WorkspaceSymbolOptions& value);
json::Any toJson(DocumentFormattingOptions&& value);
void fromJson(json::Any&& json, DocumentFormattingOptions& value);
json::Any toJson(DocumentRangeFormattingOptions&& value);
void fromJson(json::Any&& json, DocumentRangeFormattingOptions& value);
template<>
const char** requiredProperties<DocumentOnTypeFormattingOptions>();
json::Any toJson(DocumentOnTypeFormattingOptions&& value);
void fromJson(json::Any&& json, DocumentOnTypeFormattingOptions& value);
json::Any toJson(RenameOptions&& value);
void fromJson(json::Any&& json, RenameOptions& value);
template<>
const char** requiredProperties<ExecuteCommandOptions>();
json::Any toJson(ExecuteCommandOptions&& value);
void fromJson(json::Any&& json, ExecuteCommandOptions& value);
json::Any toJson(WorkspaceFoldersServerCapabilities&& value);
void fromJson(json::Any&& json, WorkspaceFoldersServerCapabilities& value);
json::Any toJson(FileOperationOptions&& value);
void fromJson(json::Any&& json, FileOperationOptions& value);
json::Any toJson(ServerCapabilitiesWorkspace&& value);
void fromJson(json::Any&& json, ServerCapabilitiesWorkspace& value);
json::Any toJson(ServerCapabilities&& value);
void fromJson(json::Any&& json, ServerCapabilities& value);
template<>
const char** requiredProperties<InitializeResultServerInfo>();
json::Any toJson(InitializeResultServerInfo&& value);
void fromJson(json::Any&& json, InitializeResultServerInfo& value);
template<>
const char** requiredProperties<InitializeResult>();
json::Any toJson(InitializeResult&& value);
void fromJson(json::Any&& json, InitializeResult& value);
template<>
const char** requiredProperties<InitializeError>();
json::Any toJson(InitializeError&& value);
void fromJson(json::Any&& json, InitializeError& value);
json::Any toJson(InitializedParams&& value);
void fromJson(json::Any&& json, InitializedParams& value);
template<>
const char** requiredProperties<DidChangeConfigurationParams>();
json::Any toJson(DidChangeConfigurationParams&& value);
void fromJson(json::Any&& json, DidChangeConfigurationParams& value);
json::Any toJson(DidChangeConfigurationRegistrationOptions&& value);
void fromJson(json::Any&& json, DidChangeConfigurationRegistrationOptions& value);
template<>
const char** requiredProperties<ShowMessageParams>();
json::Any toJson(ShowMessageParams&& value);
void fromJson(json::Any&& json, ShowMessageParams& value);
template<>
const char** requiredProperties<MessageActionItem>();
json::Any toJson(MessageActionItem&& value);
void fromJson(json::Any&& json, MessageActionItem& value);
template<>
const char** requiredProperties<ShowMessageRequestParams>();
json::Any toJson(ShowMessageRequestParams&& value);
void fromJson(json::Any&& json, ShowMessageRequestParams& value);
template<>
const char** requiredProperties<LogMessageParams>();
json::Any toJson(LogMessageParams&& value);
void fromJson(json::Any&& json, LogMessageParams& value);
template<>
const char** requiredProperties<DidOpenTextDocumentParams>();
json::Any toJson(DidOpenTextDocumentParams&& value);
void fromJson(json::Any&& json, DidOpenTextDocumentParams& value);
template<>
const char** requiredProperties<DidChangeTextDocumentParams>();
json::Any toJson(DidChangeTextDocumentParams&& value);
void fromJson(json::Any&& json, DidChangeTextDocumentParams& value);
template<>
const char** requiredProperties<TextDocumentChangeRegistrationOptions>();
json::Any toJson(TextDocumentChangeRegistrationOptions&& value);
void fromJson(json::Any&& json, TextDocumentChangeRegistrationOptions& value);
template<>
const char** requiredProperties<DidCloseTextDocumentParams>();
json::Any toJson(DidCloseTextDocumentParams&& value);
void fromJson(json::Any&& json, DidCloseTextDocumentParams& value);
template<>
const char** requiredProperties<DidSaveTextDocumentParams>();
json::Any toJson(DidSaveTextDocumentParams&& value);
void fromJson(json::Any&& json, DidSaveTextDocumentParams& value);
template<>
const char** requiredProperties<TextDocumentSaveRegistrationOptions>();
json::Any toJson(TextDocumentSaveRegistrationOptions&& value);
void fromJson(json::Any&& json, TextDocumentSaveRegistrationOptions& value);
template<>
const char** requiredProperties<WillSaveTextDocumentParams>();
json::Any toJson(WillSaveTextDocumentParams&& value);
void fromJson(json::Any&& json, WillSaveTextDocumentParams& value);
template<>
const char** requiredProperties<FileEvent>();
json::Any toJson(FileEvent&& value);
void fromJson(json::Any&& json, FileEvent& value);
template<>
const char** requiredProperties<DidChangeWatchedFilesParams>();
json::Any toJson(DidChangeWatchedFilesParams&& value);
void fromJson(json::Any&& json, DidChangeWatchedFilesParams& value);
template<>
const char** requiredProperties<RelativePattern>();
json::Any toJson(RelativePattern&& value);
void fromJson(json::Any&& json, RelativePattern& value);
template<>
const char** requiredProperties<FileSystemWatcher>();
json::Any toJson(FileSystemWatcher&& value);
void fromJson(json::Any&& json, FileSystemWatcher& value);
template<>
const char** requiredProperties<DidChangeWatchedFilesRegistrationOptions>();
json::Any toJson(DidChangeWatchedFilesRegistrationOptions&& value);
void fromJson(json::Any&& json, DidChangeWatchedFilesRegistrationOptions& value);
template<>
const char** requiredProperties<PublishDiagnosticsParams>();
json::Any toJson(PublishDiagnosticsParams&& value);
void fromJson(json::Any&& json, PublishDiagnosticsParams& value);
template<>
const char** requiredProperties<CompletionContext>();
json::Any toJson(CompletionContext&& value);
void fromJson(json::Any&& json, CompletionContext& value);
template<>
const char** requiredProperties<CompletionParams>();
json::Any toJson(CompletionParams&& value);
void fromJson(json::Any&& json, CompletionParams& value);
json::Any toJson(CompletionItemLabelDetails&& value);
void fromJson(json::Any&& json, CompletionItemLabelDetails& value);
template<>
const char** requiredProperties<InsertReplaceEdit>();
json::Any toJson(InsertReplaceEdit&& value);
void fromJson(json::Any&& json, InsertReplaceEdit& value);
template<>
const char** requiredProperties<CompletionItem>();
json::Any toJson(CompletionItem&& value);
void fromJson(json::Any&& json, CompletionItem& value);
template<>
const char** requiredProperties<CompletionListItemDefaultsEditRange_Insert_Replace>();
json::Any toJson(CompletionListItemDefaultsEditRange_Insert_Replace&& value);
void fromJson(json::Any&& json, CompletionListItemDefaultsEditRange_Insert_Replace& value);
json::Any toJson(CompletionListItemDefaults&& value);
void fromJson(json::Any&& json, CompletionListItemDefaults& value);
template<>
const char** requiredProperties<CompletionList>();
json::Any toJson(CompletionList&& value);
void fromJson(json::Any&& json, CompletionList& value);
template<>
const char** requiredProperties<CompletionRegistrationOptions>();
json::Any toJson(CompletionRegistrationOptions&& value);
void fromJson(json::Any&& json, CompletionRegistrationOptions& value);
template<>
const char** requiredProperties<HoverParams>();
json::Any toJson(HoverParams&& value);
void fromJson(json::Any&& json, HoverParams& value);
template<>
const char** requiredProperties<MarkedString_Language_Value>();
json::Any toJson(MarkedString_Language_Value&& value);
void fromJson(json::Any&& json, MarkedString_Language_Value& value);
template<>
const char** requiredProperties<Hover>();
json::Any toJson(Hover&& value);
void fromJson(json::Any&& json, Hover& value);
template<>
const char** requiredProperties<HoverRegistrationOptions>();
json::Any toJson(HoverRegistrationOptions&& value);
void fromJson(json::Any&& json, HoverRegistrationOptions& value);
template<>
const char** requiredProperties<ParameterInformation>();
json::Any toJson(ParameterInformation&& value);
void fromJson(json::Any&& json, ParameterInformation& value);
template<>
const char** requiredProperties<SignatureInformation>();
json::Any toJson(SignatureInformation&& value);
void fromJson(json::Any&& json, SignatureInformation& value);
template<>
const char** requiredProperties<SignatureHelp>();
json::Any toJson(SignatureHelp&& value);
void fromJson(json::Any&& json, SignatureHelp& value);
template<>
const char** requiredProperties<SignatureHelpContext>();
json::Any toJson(SignatureHelpContext&& value);
void fromJson(json::Any&& json, SignatureHelpContext& value);
template<>
const char** requiredProperties<SignatureHelpParams>();
json::Any toJson(SignatureHelpParams&& value);
void fromJson(json::Any&& json, SignatureHelpParams& value);
template<>
const char** requiredProperties<SignatureHelpRegistrationOptions>();
json::Any toJson(SignatureHelpRegistrationOptions&& value);
void fromJson(json::Any&& json, SignatureHelpRegistrationOptions& value);
template<>
const char** requiredProperties<DefinitionParams>();
json::Any toJson(DefinitionParams&& value);
void fromJson(json::Any&& json, DefinitionParams& value);
template<>
const char** requiredProperties<DefinitionRegistrationOptions>();
json::Any toJson(DefinitionRegistrationOptions&& value);
void fromJson(json::Any&& json, DefinitionRegistrationOptions& value);
template<>
const char** requiredProperties<ReferenceContext>();
json::Any toJson(ReferenceContext&& value);
void fromJson(json::Any&& json, ReferenceContext& value);
template<>
const char** requiredProperties<ReferenceParams>();
json::Any toJson(ReferenceParams&& value);
void fromJson(json::Any&& json, ReferenceParams& value);
template<>
const char** requiredProperties<ReferenceRegistrationOptions>();
json::Any toJson(ReferenceRegistrationOptions&& value);
void fromJson(json::Any&& json, ReferenceRegistrationOptions& value);
template<>
const char** requiredProperties<DocumentHighlightParams>();
json::Any toJson(DocumentHighlightParams&& value);
void fromJson(json::Any&& json, DocumentHighlightParams& value);
template<>
const char** requiredProperties<DocumentHighlight>();
json::Any toJson(DocumentHighlight&& value);
void fromJson(json::Any&& json, DocumentHighlight& value);
template<>
const char** requiredProperties<DocumentHighlightRegistrationOptions>();
json::Any toJson(DocumentHighlightRegistrationOptions&& value);
void fromJson(json::Any&& json, DocumentHighlightRegistrationOptions& value);
template<>
const char** requiredProperties<DocumentSymbolParams>();
json::Any toJson(DocumentSymbolParams&& value);
void fromJson(json::Any&& json, DocumentSymbolParams& value);
template<>
const char** requiredProperties<BaseSymbolInformation>();
json::Any toJson(BaseSymbolInformation&& value);
void fromJson(json::Any&& json, BaseSymbolInformation& value);
template<>
const char** requiredProperties<SymbolInformation>();
json::Any toJson(SymbolInformation&& value);
void fromJson(json::Any&& json, SymbolInformation& value);
template<>
const char** requiredProperties<DocumentSymbol>();
json::Any toJson(DocumentSymbol&& value);
void fromJson(json::Any&& json, DocumentSymbol& value);
template<>
const char** requiredProperties<DocumentSymbolRegistrationOptions>();
json::Any toJson(DocumentSymbolRegistrationOptions&& value);
void fromJson(json::Any&& json, DocumentSymbolRegistrationOptions& value);
template<>
const char** requiredProperties<CodeActionContext>();
json::Any toJson(CodeActionContext&& value);
void fromJson(json::Any&& json, CodeActionContext& value);
template<>
const char** requiredProperties<CodeActionParams>();
json::Any toJson(CodeActionParams&& value);
void fromJson(json::Any&& json, CodeActionParams& value);
template<>
const char** requiredProperties<CodeActionDisabled>();
json::Any toJson(CodeActionDisabled&& value);
void fromJson(json::Any&& json, CodeActionDisabled& value);
template<>
const char** requiredProperties<CodeAction>();
json::Any toJson(CodeAction&& value);
void fromJson(json::Any&& json, CodeAction& value);
template<>
const char** requiredProperties<CodeActionRegistrationOptions>();
json::Any toJson(CodeActionRegistrationOptions&& value);
void fromJson(json::Any&& json, CodeActionRegistrationOptions& value);
template<>
const char** requiredProperties<WorkspaceSymbolParams>();
json::Any toJson(WorkspaceSymbolParams&& value);
void fromJson(json::Any&& json, WorkspaceSymbolParams& value);
template<>
const char** requiredProperties<WorkspaceSymbolLocation_Uri>();
json::Any toJson(WorkspaceSymbolLocation_Uri&& value);
void fromJson(json::Any&& json, WorkspaceSymbolLocation_Uri& value);
template<>
const char** requiredProperties<WorkspaceSymbol>();
json::Any toJson(WorkspaceSymbol&& value);
void fromJson(json::Any&& json, WorkspaceSymbol& value);
json::Any toJson(WorkspaceSymbolRegistrationOptions&& value);
void fromJson(json::Any&& json, WorkspaceSymbolRegistrationOptions& value);
template<>
const char** requiredProperties<CodeLensParams>();
json::Any toJson(CodeLensParams&& value);
void fromJson(json::Any&& json, CodeLensParams& value);
template<>
const char** requiredProperties<CodeLens>();
json::Any toJson(CodeLens&& value);
void fromJson(json::Any&& json, CodeLens& value);
template<>
const char** requiredProperties<CodeLensRegistrationOptions>();
json::Any toJson(CodeLensRegistrationOptions&& value);
void fromJson(json::Any&& json, CodeLensRegistrationOptions& value);
template<>
const char** requiredProperties<DocumentLinkParams>();
json::Any toJson(DocumentLinkParams&& value);
void fromJson(json::Any&& json, DocumentLinkParams& value);
template<>
const char** requiredProperties<DocumentLink>();
json::Any toJson(DocumentLink&& value);
void fromJson(json::Any&& json, DocumentLink& value);
template<>
const char** requiredProperties<DocumentLinkRegistrationOptions>();
json::Any toJson(DocumentLinkRegistrationOptions&& value);
void fromJson(json::Any&& json, DocumentLinkRegistrationOptions& value);
template<>
const char** requiredProperties<FormattingOptions>();
json::Any toJson(FormattingOptions&& value);
void fromJson(json::Any&& json, FormattingOptions& value);
template<>
const char** requiredProperties<DocumentFormattingParams>();
json::Any toJson(DocumentFormattingParams&& value);
void fromJson(json::Any&& json, DocumentFormattingParams& value);
template<>
const char** requiredProperties<DocumentFormattingRegistrationOptions>();
json::Any toJson(DocumentFormattingRegistrationOptions&& value);
void fromJson(json::Any&& json, DocumentFormattingRegistrationOptions& value);
template<>
const char** requiredProperties<DocumentRangeFormattingParams>();
json::Any toJson(DocumentRangeFormattingParams&& value);
void fromJson(json::Any&& json, DocumentRangeFormattingParams& value);
template<>
const char** requiredProperties<DocumentRangeFormattingRegistrationOptions>();
json::Any toJson(DocumentRangeFormattingRegistrationOptions&& value);
void fromJson(json::Any&& json, DocumentRangeFormattingRegistrationOptions& value);
template<>
const char** requiredProperties<DocumentRangesFormattingParams>();
json::Any toJson(DocumentRangesFormattingParams&& value);
void fromJson(json::Any&& json, DocumentRangesFormattingParams& value);
template<>
const char** requiredProperties<DocumentOnTypeFormattingParams>();
json::Any toJson(DocumentOnTypeFormattingParams&& value);
void fromJson(json::Any&& json, DocumentOnTypeFormattingParams& value);
template<>
const char** requiredProperties<DocumentOnTypeFormattingRegistrationOptions>();
json::Any toJson(DocumentOnTypeFormattingRegistrationOptions&& value);
void fromJson(json::Any&& json, DocumentOnTypeFormattingRegistrationOptions& value);
template<>
const char** requiredProperties<RenameParams>();
json::Any toJson(RenameParams&& value);
void fromJson(json::Any&& json, RenameParams& value);
template<>
const char** requiredProperties<RenameRegistrationOptions>();
json::Any toJson(RenameRegistrationOptions&& value);
void fromJson(json::Any&& json, RenameRegistrationOptions& value);
template<>
const char** requiredProperties<PrepareRenameParams>();
json::Any toJson(PrepareRenameParams&& value);
void fromJson(json::Any&& json, PrepareRenameParams& value);
template<>
const char** requiredProperties<ExecuteCommandParams>();
json::Any toJson(ExecuteCommandParams&& value);
void fromJson(json::Any&& json, ExecuteCommandParams& value);
template<>
const char** requiredProperties<ExecuteCommandRegistrationOptions>();
json::Any toJson(ExecuteCommandRegistrationOptions&& value);
void fromJson(json::Any&& json, ExecuteCommandRegistrationOptions& value);
template<>
const char** requiredProperties<ApplyWorkspaceEditParams>();
json::Any toJson(ApplyWorkspaceEditParams&& value);
void fromJson(json::Any&& json, ApplyWorkspaceEditParams& value);
template<>
const char** requiredProperties<ApplyWorkspaceEditResult>();
json::Any toJson(ApplyWorkspaceEditResult&& value);
void fromJson(json::Any&& json, ApplyWorkspaceEditResult& value);
template<>
const char** requiredProperties<WorkDoneProgressBegin>();
template<>
const std::pair<const char*, json::Any>* literalProperties<WorkDoneProgressBegin>();
json::Any toJson(WorkDoneProgressBegin&& value);
void fromJson(json::Any&& json, WorkDoneProgressBegin& value);
template<>
const std::pair<const char*, json::Any>* literalProperties<WorkDoneProgressReport>();
json::Any toJson(WorkDoneProgressReport&& value);
void fromJson(json::Any&& json, WorkDoneProgressReport& value);
template<>
const std::pair<const char*, json::Any>* literalProperties<WorkDoneProgressEnd>();
json::Any toJson(WorkDoneProgressEnd&& value);
void fromJson(json::Any&& json, WorkDoneProgressEnd& value);
template<>
const char** requiredProperties<SetTraceParams>();
json::Any toJson(SetTraceParams&& value);
void fromJson(json::Any&& json, SetTraceParams& value);
template<>
const char** requiredProperties<LogTraceParams>();
json::Any toJson(LogTraceParams&& value);
void fromJson(json::Any&& json, LogTraceParams& value);
template<>
const char** requiredProperties<CancelParams>();
json::Any toJson(CancelParams&& value);
void fromJson(json::Any&& json, CancelParams& value);
template<>
const char** requiredProperties<ProgressParams>();
json::Any toJson(ProgressParams&& value);
void fromJson(json::Any&& json, ProgressParams& value);
template<>
const char** requiredProperties<LocationLink>();
json::Any toJson(LocationLink&& value);
void fromJson(json::Any&& json, LocationLink& value);
template<>
const char** requiredProperties<InlineValueText>();
json::Any toJson(InlineValueText&& value);
void fromJson(json::Any&& json, InlineValueText& value);
template<>
const char** requiredProperties<InlineValueVariableLookup>();
json::Any toJson(InlineValueVariableLookup&& value);
void fromJson(json::Any&& json, InlineValueVariableLookup& value);
template<>
const char** requiredProperties<InlineValueEvaluatableExpression>();
json::Any toJson(InlineValueEvaluatableExpression&& value);
void fromJson(json::Any&& json, InlineValueEvaluatableExpression& value);
template<>
const char** requiredProperties<RelatedFullDocumentDiagnosticReport>();
json::Any toJson(RelatedFullDocumentDiagnosticReport&& value);
void fromJson(json::Any&& json, RelatedFullDocumentDiagnosticReport& value);
template<>
const char** requiredProperties<RelatedUnchangedDocumentDiagnosticReport>();
json::Any toJson(RelatedUnchangedDocumentDiagnosticReport&& value);
void fromJson(json::Any&& json, RelatedUnchangedDocumentDiagnosticReport& value);
template<>
const char** requiredProperties<PrepareRenameResult_Range_Placeholder>();
json::Any toJson(PrepareRenameResult_Range_Placeholder&& value);
void fromJson(json::Any&& json, PrepareRenameResult_Range_Placeholder& value);
template<>
const char** requiredProperties<PrepareRenameResult_DefaultBehavior>();
json::Any toJson(PrepareRenameResult_DefaultBehavior&& value);
void fromJson(json::Any&& json, PrepareRenameResult_DefaultBehavior& value);

} // namespace lsp
