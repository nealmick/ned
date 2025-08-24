#include "types.h"

/*#############################################################
 * NOTE: This is a generated file and it shouldn't be modified!
 *#############################################################*/

namespace lsp{

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced formal parameter
#endif

template<>
const SemanticTokenTypesEnum::ConstInitType SemanticTokenTypesEnum::s_values[] = {
	"namespace",
	"type",
	"class",
	"enum",
	"interface",
	"struct",
	"typeParameter",
	"parameter",
	"variable",
	"property",
	"enumMember",
	"event",
	"function",
	"method",
	"macro",
	"keyword",
	"modifier",
	"comment",
	"string",
	"number",
	"regexp",
	"operator",
	"decorator"
};

template<>
const SemanticTokenModifiersEnum::ConstInitType SemanticTokenModifiersEnum::s_values[] = {
	"declaration",
	"definition",
	"readonly",
	"static",
	"deprecated",
	"abstract",
	"async",
	"modification",
	"documentation",
	"defaultLibrary"
};

template<>
const DocumentDiagnosticReportKindEnum::ConstInitType DocumentDiagnosticReportKindEnum::s_values[] = {
	"full",
	"unchanged"
};

template<>
const ErrorCodesEnum::ConstInitType ErrorCodesEnum::s_values[] = {
	-32700,
	-32600,
	-32601,
	-32602,
	-32603,
	-32002,
	-32001
};

template<>
const LSPErrorCodesEnum::ConstInitType LSPErrorCodesEnum::s_values[] = {
	-32803,
	-32802,
	-32801,
	-32800
};

template<>
const FoldingRangeKindEnum::ConstInitType FoldingRangeKindEnum::s_values[] = {
	"comment",
	"imports",
	"region"
};

template<>
const SymbolKindEnum::ConstInitType SymbolKindEnum::s_values[] = {
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26
};

template<>
const SymbolTagEnum::ConstInitType SymbolTagEnum::s_values[] = {
	1
};

template<>
const UniquenessLevelEnum::ConstInitType UniquenessLevelEnum::s_values[] = {
	"document",
	"project",
	"group",
	"scheme",
	"global"
};

template<>
const MonikerKindEnum::ConstInitType MonikerKindEnum::s_values[] = {
	"import",
	"export",
	"local"
};

template<>
const InlayHintKindEnum::ConstInitType InlayHintKindEnum::s_values[] = {
	1,
	2
};

template<>
const MessageTypeEnum::ConstInitType MessageTypeEnum::s_values[] = {
	1,
	2,
	3,
	4,
	5
};

template<>
const TextDocumentSyncKindEnum::ConstInitType TextDocumentSyncKindEnum::s_values[] = {
	0,
	1,
	2
};

template<>
const TextDocumentSaveReasonEnum::ConstInitType TextDocumentSaveReasonEnum::s_values[] = {
	1,
	2,
	3
};

template<>
const CompletionItemKindEnum::ConstInitType CompletionItemKindEnum::s_values[] = {
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25
};

template<>
const CompletionItemTagEnum::ConstInitType CompletionItemTagEnum::s_values[] = {
	1
};

template<>
const InsertTextFormatEnum::ConstInitType InsertTextFormatEnum::s_values[] = {
	1,
	2
};

template<>
const InsertTextModeEnum::ConstInitType InsertTextModeEnum::s_values[] = {
	1,
	2
};

template<>
const DocumentHighlightKindEnum::ConstInitType DocumentHighlightKindEnum::s_values[] = {
	1,
	2,
	3
};

template<>
const CodeActionKindEnum::ConstInitType CodeActionKindEnum::s_values[] = {
	"",
	"quickfix",
	"refactor",
	"refactor.extract",
	"refactor.inline",
	"refactor.rewrite",
	"source",
	"source.organizeImports",
	"source.fixAll"
};

template<>
const TraceValuesEnum::ConstInitType TraceValuesEnum::s_values[] = {
	"off",
	"messages",
	"verbose"
};

template<>
const MarkupKindEnum::ConstInitType MarkupKindEnum::s_values[] = {
	"plaintext",
	"markdown"
};

template<>
const InlineCompletionTriggerKindEnum::ConstInitType InlineCompletionTriggerKindEnum::s_values[] = {
	0,
	1
};

template<>
const PositionEncodingKindEnum::ConstInitType PositionEncodingKindEnum::s_values[] = {
	"utf-8",
	"utf-16",
	"utf-32"
};

template<>
const FileChangeTypeEnum::ConstInitType FileChangeTypeEnum::s_values[] = {
	1,
	2,
	3
};

template<>
const WatchKindEnum::ConstInitType WatchKindEnum::s_values[] = {
	1,
	2,
	4
};

template<>
const DiagnosticSeverityEnum::ConstInitType DiagnosticSeverityEnum::s_values[] = {
	1,
	2,
	3,
	4
};

template<>
const DiagnosticTagEnum::ConstInitType DiagnosticTagEnum::s_values[] = {
	1,
	2
};

template<>
const CompletionTriggerKindEnum::ConstInitType CompletionTriggerKindEnum::s_values[] = {
	1,
	2,
	3
};

template<>
const SignatureHelpTriggerKindEnum::ConstInitType SignatureHelpTriggerKindEnum::s_values[] = {
	1,
	2,
	3
};

template<>
const CodeActionTriggerKindEnum::ConstInitType CodeActionTriggerKindEnum::s_values[] = {
	1,
	2
};

template<>
const FileOperationPatternKindEnum::ConstInitType FileOperationPatternKindEnum::s_values[] = {
	"file",
	"folder"
};

template<>
const NotebookCellKindEnum::ConstInitType NotebookCellKindEnum::s_values[] = {
	1,
	2
};

template<>
const ResourceOperationKindEnum::ConstInitType ResourceOperationKindEnum::s_values[] = {
	"create",
	"rename",
	"delete"
};

template<>
const FailureHandlingKindEnum::ConstInitType FailureHandlingKindEnum::s_values[] = {
	"abort",
	"transactional",
	"textOnlyTransactional",
	"undo"
};

template<>
const PrepareSupportDefaultBehaviorEnum::ConstInitType PrepareSupportDefaultBehaviorEnum::s_values[] = {
	1
};

template<>
const TokenFormatEnum::ConstInitType TokenFormatEnum::s_values[] = {
	"relative"
};

static void textDocumentIdentifierToJson(TextDocumentIdentifier& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
}

static void textDocumentIdentifierFromJson(json::Object& json, TextDocumentIdentifier& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
}

static void positionToJson(Position& value, json::Object& json)
{
	json["line"] = toJson(value.line);
	json["character"] = toJson(value.character);
}

static void positionFromJson(json::Object& json, Position& value)
{
	fromJson(std::move(json.get("line")), value.line);
	fromJson(std::move(json.get("character")), value.character);
}

static void textDocumentPositionParamsToJson(TextDocumentPositionParams& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["position"] = toJson(std::move(value.position));
}

static void textDocumentPositionParamsFromJson(json::Object& json, TextDocumentPositionParams& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("position")), value.position);
}

static void workDoneProgressParamsToJson(WorkDoneProgressParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
}

static void workDoneProgressParamsFromJson(json::Object& json, WorkDoneProgressParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
}

static void partialResultParamsToJson(PartialResultParams& value, json::Object& json)
{
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
}

static void partialResultParamsFromJson(json::Object& json, PartialResultParams& value)
{
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
}

static void implementationParamsToJson(ImplementationParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
}

static void implementationParamsFromJson(json::Object& json, ImplementationParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
}

static void rangeToJson(Range& value, json::Object& json)
{
	json["start"] = toJson(std::move(value.start));
	json["end"] = toJson(std::move(value.end));
}

static void rangeFromJson(json::Object& json, Range& value)
{
	fromJson(std::move(json.get("start")), value.start);
	fromJson(std::move(json.get("end")), value.end);
}

static void locationToJson(Location& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	json["range"] = toJson(std::move(value.range));
}

static void locationFromJson(json::Object& json, Location& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("range")), value.range);
}

static void textDocumentFilter_LanguageToJson(TextDocumentFilter_Language& value, json::Object& json)
{
	json["language"] = toJson(std::move(value.language));
	if(value.scheme)
		json["scheme"] = toJson(std::move(value.scheme));
	if(value.pattern)
		json["pattern"] = toJson(std::move(value.pattern));
}

static void textDocumentFilter_LanguageFromJson(json::Object& json, TextDocumentFilter_Language& value)
{
	fromJson(std::move(json.get("language")), value.language);
	if(const auto it = json.find("scheme"); it != json.end())
		fromJson(std::move(it->second), value.scheme);
	if(const auto it = json.find("pattern"); it != json.end())
		fromJson(std::move(it->second), value.pattern);
}

static void textDocumentFilter_SchemeToJson(TextDocumentFilter_Scheme& value, json::Object& json)
{
	json["scheme"] = toJson(std::move(value.scheme));
	if(value.language)
		json["language"] = toJson(std::move(value.language));
	if(value.pattern)
		json["pattern"] = toJson(std::move(value.pattern));
}

static void textDocumentFilter_SchemeFromJson(json::Object& json, TextDocumentFilter_Scheme& value)
{
	fromJson(std::move(json.get("scheme")), value.scheme);
	if(const auto it = json.find("language"); it != json.end())
		fromJson(std::move(it->second), value.language);
	if(const auto it = json.find("pattern"); it != json.end())
		fromJson(std::move(it->second), value.pattern);
}

static void textDocumentFilter_PatternToJson(TextDocumentFilter_Pattern& value, json::Object& json)
{
	json["pattern"] = toJson(std::move(value.pattern));
	if(value.language)
		json["language"] = toJson(std::move(value.language));
	if(value.scheme)
		json["scheme"] = toJson(std::move(value.scheme));
}

static void textDocumentFilter_PatternFromJson(json::Object& json, TextDocumentFilter_Pattern& value)
{
	fromJson(std::move(json.get("pattern")), value.pattern);
	if(const auto it = json.find("language"); it != json.end())
		fromJson(std::move(it->second), value.language);
	if(const auto it = json.find("scheme"); it != json.end())
		fromJson(std::move(it->second), value.scheme);
}

static void notebookDocumentFilter_NotebookTypeToJson(NotebookDocumentFilter_NotebookType& value, json::Object& json)
{
	json["notebookType"] = toJson(std::move(value.notebookType));
	if(value.scheme)
		json["scheme"] = toJson(std::move(value.scheme));
	if(value.pattern)
		json["pattern"] = toJson(std::move(value.pattern));
}

static void notebookDocumentFilter_NotebookTypeFromJson(json::Object& json, NotebookDocumentFilter_NotebookType& value)
{
	fromJson(std::move(json.get("notebookType")), value.notebookType);
	if(const auto it = json.find("scheme"); it != json.end())
		fromJson(std::move(it->second), value.scheme);
	if(const auto it = json.find("pattern"); it != json.end())
		fromJson(std::move(it->second), value.pattern);
}

static void notebookDocumentFilter_SchemeToJson(NotebookDocumentFilter_Scheme& value, json::Object& json)
{
	json["scheme"] = toJson(std::move(value.scheme));
	if(value.notebookType)
		json["notebookType"] = toJson(std::move(value.notebookType));
	if(value.pattern)
		json["pattern"] = toJson(std::move(value.pattern));
}

static void notebookDocumentFilter_SchemeFromJson(json::Object& json, NotebookDocumentFilter_Scheme& value)
{
	fromJson(std::move(json.get("scheme")), value.scheme);
	if(const auto it = json.find("notebookType"); it != json.end())
		fromJson(std::move(it->second), value.notebookType);
	if(const auto it = json.find("pattern"); it != json.end())
		fromJson(std::move(it->second), value.pattern);
}

static void notebookDocumentFilter_PatternToJson(NotebookDocumentFilter_Pattern& value, json::Object& json)
{
	json["pattern"] = toJson(std::move(value.pattern));
	if(value.notebookType)
		json["notebookType"] = toJson(std::move(value.notebookType));
	if(value.scheme)
		json["scheme"] = toJson(std::move(value.scheme));
}

static void notebookDocumentFilter_PatternFromJson(json::Object& json, NotebookDocumentFilter_Pattern& value)
{
	fromJson(std::move(json.get("pattern")), value.pattern);
	if(const auto it = json.find("notebookType"); it != json.end())
		fromJson(std::move(it->second), value.notebookType);
	if(const auto it = json.find("scheme"); it != json.end())
		fromJson(std::move(it->second), value.scheme);
}

static void notebookCellTextDocumentFilterToJson(NotebookCellTextDocumentFilter& value, json::Object& json)
{
	json["notebook"] = toJson(std::move(value.notebook));
	if(value.language)
		json["language"] = toJson(std::move(value.language));
}

static void notebookCellTextDocumentFilterFromJson(json::Object& json, NotebookCellTextDocumentFilter& value)
{
	fromJson(std::move(json.get("notebook")), value.notebook);
	if(const auto it = json.find("language"); it != json.end())
		fromJson(std::move(it->second), value.language);
}

static void textDocumentRegistrationOptionsToJson(TextDocumentRegistrationOptions& value, json::Object& json)
{
	json["documentSelector"] = toJson(std::move(value.documentSelector));
}

static void textDocumentRegistrationOptionsFromJson(json::Object& json, TextDocumentRegistrationOptions& value)
{
	fromJson(std::move(json.get("documentSelector")), value.documentSelector);
}

static void workDoneProgressOptionsToJson(WorkDoneProgressOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void workDoneProgressOptionsFromJson(json::Object& json, WorkDoneProgressOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void implementationOptionsToJson(ImplementationOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void implementationOptionsFromJson(json::Object& json, ImplementationOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void staticRegistrationOptionsToJson(StaticRegistrationOptions& value, json::Object& json)
{
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void staticRegistrationOptionsFromJson(json::Object& json, StaticRegistrationOptions& value)
{
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void implementationRegistrationOptionsToJson(ImplementationRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	implementationOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void implementationRegistrationOptionsFromJson(json::Object& json, ImplementationRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	implementationOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void typeDefinitionParamsToJson(TypeDefinitionParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
}

static void typeDefinitionParamsFromJson(json::Object& json, TypeDefinitionParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
}

static void typeDefinitionOptionsToJson(TypeDefinitionOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void typeDefinitionOptionsFromJson(json::Object& json, TypeDefinitionOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void typeDefinitionRegistrationOptionsToJson(TypeDefinitionRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	typeDefinitionOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void typeDefinitionRegistrationOptionsFromJson(json::Object& json, TypeDefinitionRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	typeDefinitionOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void workspaceFolderToJson(WorkspaceFolder& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	json["name"] = toJson(std::move(value.name));
}

static void workspaceFolderFromJson(json::Object& json, WorkspaceFolder& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("name")), value.name);
}

static void workspaceFoldersChangeEventToJson(WorkspaceFoldersChangeEvent& value, json::Object& json)
{
	json["added"] = toJson(std::move(value.added));
	json["removed"] = toJson(std::move(value.removed));
}

static void workspaceFoldersChangeEventFromJson(json::Object& json, WorkspaceFoldersChangeEvent& value)
{
	fromJson(std::move(json.get("added")), value.added);
	fromJson(std::move(json.get("removed")), value.removed);
}

static void didChangeWorkspaceFoldersParamsToJson(DidChangeWorkspaceFoldersParams& value, json::Object& json)
{
	json["event"] = toJson(std::move(value.event));
}

static void didChangeWorkspaceFoldersParamsFromJson(json::Object& json, DidChangeWorkspaceFoldersParams& value)
{
	fromJson(std::move(json.get("event")), value.event);
}

static void configurationItemToJson(ConfigurationItem& value, json::Object& json)
{
	if(value.scopeUri)
		json["scopeUri"] = toJson(std::move(value.scopeUri));
	if(value.section)
		json["section"] = toJson(std::move(value.section));
}

static void configurationItemFromJson(json::Object& json, ConfigurationItem& value)
{
	if(const auto it = json.find("scopeUri"); it != json.end())
		fromJson(std::move(it->second), value.scopeUri);
	if(const auto it = json.find("section"); it != json.end())
		fromJson(std::move(it->second), value.section);
}

static void configurationParamsToJson(ConfigurationParams& value, json::Object& json)
{
	json["items"] = toJson(std::move(value.items));
}

static void configurationParamsFromJson(json::Object& json, ConfigurationParams& value)
{
	fromJson(std::move(json.get("items")), value.items);
}

static void documentColorParamsToJson(DocumentColorParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void documentColorParamsFromJson(json::Object& json, DocumentColorParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void colorToJson(Color& value, json::Object& json)
{
	json["red"] = toJson(value.red);
	json["green"] = toJson(value.green);
	json["blue"] = toJson(value.blue);
	json["alpha"] = toJson(value.alpha);
}

static void colorFromJson(json::Object& json, Color& value)
{
	fromJson(std::move(json.get("red")), value.red);
	fromJson(std::move(json.get("green")), value.green);
	fromJson(std::move(json.get("blue")), value.blue);
	fromJson(std::move(json.get("alpha")), value.alpha);
}

static void colorInformationToJson(ColorInformation& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["color"] = toJson(std::move(value.color));
}

static void colorInformationFromJson(json::Object& json, ColorInformation& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("color")), value.color);
}

static void documentColorOptionsToJson(DocumentColorOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void documentColorOptionsFromJson(json::Object& json, DocumentColorOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void documentColorRegistrationOptionsToJson(DocumentColorRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	documentColorOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void documentColorRegistrationOptionsFromJson(json::Object& json, DocumentColorRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	documentColorOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void colorPresentationParamsToJson(ColorPresentationParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["color"] = toJson(std::move(value.color));
	json["range"] = toJson(std::move(value.range));
}

static void colorPresentationParamsFromJson(json::Object& json, ColorPresentationParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("color")), value.color);
	fromJson(std::move(json.get("range")), value.range);
}

static void textEditToJson(TextEdit& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["newText"] = toJson(std::move(value.newText));
}

static void textEditFromJson(json::Object& json, TextEdit& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("newText")), value.newText);
}

static void colorPresentationToJson(ColorPresentation& value, json::Object& json)
{
	json["label"] = toJson(std::move(value.label));
	if(value.textEdit)
		json["textEdit"] = toJson(std::move(value.textEdit));
	if(value.additionalTextEdits)
		json["additionalTextEdits"] = toJson(std::move(value.additionalTextEdits));
}

static void colorPresentationFromJson(json::Object& json, ColorPresentation& value)
{
	fromJson(std::move(json.get("label")), value.label);
	if(const auto it = json.find("textEdit"); it != json.end())
		fromJson(std::move(it->second), value.textEdit);
	if(const auto it = json.find("additionalTextEdits"); it != json.end())
		fromJson(std::move(it->second), value.additionalTextEdits);
}

static void foldingRangeParamsToJson(FoldingRangeParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void foldingRangeParamsFromJson(json::Object& json, FoldingRangeParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void foldingRangeToJson(FoldingRange& value, json::Object& json)
{
	json["startLine"] = toJson(value.startLine);
	json["endLine"] = toJson(value.endLine);
	if(value.startCharacter)
		json["startCharacter"] = toJson(std::move(value.startCharacter));
	if(value.endCharacter)
		json["endCharacter"] = toJson(std::move(value.endCharacter));
	if(value.kind)
		json["kind"] = toJson(std::move(value.kind));
	if(value.collapsedText)
		json["collapsedText"] = toJson(std::move(value.collapsedText));
}

static void foldingRangeFromJson(json::Object& json, FoldingRange& value)
{
	fromJson(std::move(json.get("startLine")), value.startLine);
	fromJson(std::move(json.get("endLine")), value.endLine);
	if(const auto it = json.find("startCharacter"); it != json.end())
		fromJson(std::move(it->second), value.startCharacter);
	if(const auto it = json.find("endCharacter"); it != json.end())
		fromJson(std::move(it->second), value.endCharacter);
	if(const auto it = json.find("kind"); it != json.end())
		fromJson(std::move(it->second), value.kind);
	if(const auto it = json.find("collapsedText"); it != json.end())
		fromJson(std::move(it->second), value.collapsedText);
}

static void foldingRangeOptionsToJson(FoldingRangeOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void foldingRangeOptionsFromJson(json::Object& json, FoldingRangeOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void foldingRangeRegistrationOptionsToJson(FoldingRangeRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	foldingRangeOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void foldingRangeRegistrationOptionsFromJson(json::Object& json, FoldingRangeRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	foldingRangeOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void declarationParamsToJson(DeclarationParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
}

static void declarationParamsFromJson(json::Object& json, DeclarationParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
}

static void declarationOptionsToJson(DeclarationOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void declarationOptionsFromJson(json::Object& json, DeclarationOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void declarationRegistrationOptionsToJson(DeclarationRegistrationOptions& value, json::Object& json)
{
	declarationOptionsToJson(value, json);
	textDocumentRegistrationOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void declarationRegistrationOptionsFromJson(json::Object& json, DeclarationRegistrationOptions& value)
{
	declarationOptionsFromJson(json, value);
	textDocumentRegistrationOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void selectionRangeParamsToJson(SelectionRangeParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["positions"] = toJson(std::move(value.positions));
}

static void selectionRangeParamsFromJson(json::Object& json, SelectionRangeParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("positions")), value.positions);
}

static void selectionRangeToJson(SelectionRange& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	if(value.parent)
		json["parent"] = toJson(std::move(value.parent));
}

static void selectionRangeFromJson(json::Object& json, SelectionRange& value)
{
	fromJson(std::move(json.get("range")), value.range);
	if(const auto it = json.find("parent"); it != json.end())
		fromJson(std::move(it->second), value.parent);
}

static void selectionRangeOptionsToJson(SelectionRangeOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void selectionRangeOptionsFromJson(json::Object& json, SelectionRangeOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void selectionRangeRegistrationOptionsToJson(SelectionRangeRegistrationOptions& value, json::Object& json)
{
	selectionRangeOptionsToJson(value, json);
	textDocumentRegistrationOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void selectionRangeRegistrationOptionsFromJson(json::Object& json, SelectionRangeRegistrationOptions& value)
{
	selectionRangeOptionsFromJson(json, value);
	textDocumentRegistrationOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void workDoneProgressCreateParamsToJson(WorkDoneProgressCreateParams& value, json::Object& json)
{
	json["token"] = toJson(std::move(value.token));
}

static void workDoneProgressCreateParamsFromJson(json::Object& json, WorkDoneProgressCreateParams& value)
{
	fromJson(std::move(json.get("token")), value.token);
}

static void workDoneProgressCancelParamsToJson(WorkDoneProgressCancelParams& value, json::Object& json)
{
	json["token"] = toJson(std::move(value.token));
}

static void workDoneProgressCancelParamsFromJson(json::Object& json, WorkDoneProgressCancelParams& value)
{
	fromJson(std::move(json.get("token")), value.token);
}

static void callHierarchyPrepareParamsToJson(CallHierarchyPrepareParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
}

static void callHierarchyPrepareParamsFromJson(json::Object& json, CallHierarchyPrepareParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
}

static void callHierarchyItemToJson(CallHierarchyItem& value, json::Object& json)
{
	json["name"] = toJson(std::move(value.name));
	json["kind"] = toJson(std::move(value.kind));
	json["uri"] = toJson(std::move(value.uri));
	json["range"] = toJson(std::move(value.range));
	json["selectionRange"] = toJson(std::move(value.selectionRange));
	if(value.tags)
		json["tags"] = toJson(std::move(value.tags));
	if(value.detail)
		json["detail"] = toJson(std::move(value.detail));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void callHierarchyItemFromJson(json::Object& json, CallHierarchyItem& value)
{
	fromJson(std::move(json.get("name")), value.name);
	fromJson(std::move(json.get("kind")), value.kind);
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("selectionRange")), value.selectionRange);
	if(const auto it = json.find("tags"); it != json.end())
		fromJson(std::move(it->second), value.tags);
	if(const auto it = json.find("detail"); it != json.end())
		fromJson(std::move(it->second), value.detail);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void callHierarchyOptionsToJson(CallHierarchyOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void callHierarchyOptionsFromJson(json::Object& json, CallHierarchyOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void callHierarchyRegistrationOptionsToJson(CallHierarchyRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	callHierarchyOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void callHierarchyRegistrationOptionsFromJson(json::Object& json, CallHierarchyRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	callHierarchyOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void callHierarchyIncomingCallsParamsToJson(CallHierarchyIncomingCallsParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["item"] = toJson(std::move(value.item));
}

static void callHierarchyIncomingCallsParamsFromJson(json::Object& json, CallHierarchyIncomingCallsParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("item")), value.item);
}

static void callHierarchyIncomingCallToJson(CallHierarchyIncomingCall& value, json::Object& json)
{
	json["from"] = toJson(std::move(value.from));
	json["fromRanges"] = toJson(std::move(value.fromRanges));
}

static void callHierarchyIncomingCallFromJson(json::Object& json, CallHierarchyIncomingCall& value)
{
	fromJson(std::move(json.get("from")), value.from);
	fromJson(std::move(json.get("fromRanges")), value.fromRanges);
}

static void callHierarchyOutgoingCallsParamsToJson(CallHierarchyOutgoingCallsParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["item"] = toJson(std::move(value.item));
}

static void callHierarchyOutgoingCallsParamsFromJson(json::Object& json, CallHierarchyOutgoingCallsParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("item")), value.item);
}

static void callHierarchyOutgoingCallToJson(CallHierarchyOutgoingCall& value, json::Object& json)
{
	json["to"] = toJson(std::move(value.to));
	json["fromRanges"] = toJson(std::move(value.fromRanges));
}

static void callHierarchyOutgoingCallFromJson(json::Object& json, CallHierarchyOutgoingCall& value)
{
	fromJson(std::move(json.get("to")), value.to);
	fromJson(std::move(json.get("fromRanges")), value.fromRanges);
}

static void semanticTokensParamsToJson(SemanticTokensParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void semanticTokensParamsFromJson(json::Object& json, SemanticTokensParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void semanticTokensToJson(SemanticTokens& value, json::Object& json)
{
	json["data"] = toJson(std::move(value.data));
	if(value.resultId)
		json["resultId"] = toJson(std::move(value.resultId));
}

static void semanticTokensFromJson(json::Object& json, SemanticTokens& value)
{
	fromJson(std::move(json.get("data")), value.data);
	if(const auto it = json.find("resultId"); it != json.end())
		fromJson(std::move(it->second), value.resultId);
}

static void semanticTokensPartialResultToJson(SemanticTokensPartialResult& value, json::Object& json)
{
	json["data"] = toJson(std::move(value.data));
}

static void semanticTokensPartialResultFromJson(json::Object& json, SemanticTokensPartialResult& value)
{
	fromJson(std::move(json.get("data")), value.data);
}

static void semanticTokensLegendToJson(SemanticTokensLegend& value, json::Object& json)
{
	json["tokenTypes"] = toJson(std::move(value.tokenTypes));
	json["tokenModifiers"] = toJson(std::move(value.tokenModifiers));
}

static void semanticTokensLegendFromJson(json::Object& json, SemanticTokensLegend& value)
{
	fromJson(std::move(json.get("tokenTypes")), value.tokenTypes);
	fromJson(std::move(json.get("tokenModifiers")), value.tokenModifiers);
}

static void semanticTokensOptionsRangeToJson(SemanticTokensOptionsRange& value, json::Object& json)
{
}

static void semanticTokensOptionsRangeFromJson(json::Object& json, SemanticTokensOptionsRange& value)
{
}

static void semanticTokensOptionsFullToJson(SemanticTokensOptionsFull& value, json::Object& json)
{
	if(value.delta)
		json["delta"] = toJson(std::move(value.delta));
}

static void semanticTokensOptionsFullFromJson(json::Object& json, SemanticTokensOptionsFull& value)
{
	if(const auto it = json.find("delta"); it != json.end())
		fromJson(std::move(it->second), value.delta);
}

static void semanticTokensOptionsToJson(SemanticTokensOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	json["legend"] = toJson(std::move(value.legend));
	if(value.range)
		json["range"] = toJson(std::move(value.range));
	if(value.full)
		json["full"] = toJson(std::move(value.full));
}

static void semanticTokensOptionsFromJson(json::Object& json, SemanticTokensOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	fromJson(std::move(json.get("legend")), value.legend);
	if(const auto it = json.find("range"); it != json.end())
		fromJson(std::move(it->second), value.range);
	if(const auto it = json.find("full"); it != json.end())
		fromJson(std::move(it->second), value.full);
}

static void semanticTokensRegistrationOptionsToJson(SemanticTokensRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	semanticTokensOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void semanticTokensRegistrationOptionsFromJson(json::Object& json, SemanticTokensRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	semanticTokensOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void semanticTokensDeltaParamsToJson(SemanticTokensDeltaParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["previousResultId"] = toJson(std::move(value.previousResultId));
}

static void semanticTokensDeltaParamsFromJson(json::Object& json, SemanticTokensDeltaParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("previousResultId")), value.previousResultId);
}

static void semanticTokensEditToJson(SemanticTokensEdit& value, json::Object& json)
{
	json["start"] = toJson(value.start);
	json["deleteCount"] = toJson(value.deleteCount);
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void semanticTokensEditFromJson(json::Object& json, SemanticTokensEdit& value)
{
	fromJson(std::move(json.get("start")), value.start);
	fromJson(std::move(json.get("deleteCount")), value.deleteCount);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void semanticTokensDeltaToJson(SemanticTokensDelta& value, json::Object& json)
{
	json["edits"] = toJson(std::move(value.edits));
	if(value.resultId)
		json["resultId"] = toJson(std::move(value.resultId));
}

static void semanticTokensDeltaFromJson(json::Object& json, SemanticTokensDelta& value)
{
	fromJson(std::move(json.get("edits")), value.edits);
	if(const auto it = json.find("resultId"); it != json.end())
		fromJson(std::move(it->second), value.resultId);
}

static void semanticTokensDeltaPartialResultToJson(SemanticTokensDeltaPartialResult& value, json::Object& json)
{
	json["edits"] = toJson(std::move(value.edits));
}

static void semanticTokensDeltaPartialResultFromJson(json::Object& json, SemanticTokensDeltaPartialResult& value)
{
	fromJson(std::move(json.get("edits")), value.edits);
}

static void semanticTokensRangeParamsToJson(SemanticTokensRangeParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["range"] = toJson(std::move(value.range));
}

static void semanticTokensRangeParamsFromJson(json::Object& json, SemanticTokensRangeParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("range")), value.range);
}

static void showDocumentParamsToJson(ShowDocumentParams& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	if(value.external)
		json["external"] = toJson(std::move(value.external));
	if(value.takeFocus)
		json["takeFocus"] = toJson(std::move(value.takeFocus));
	if(value.selection)
		json["selection"] = toJson(std::move(value.selection));
}

static void showDocumentParamsFromJson(json::Object& json, ShowDocumentParams& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	if(const auto it = json.find("external"); it != json.end())
		fromJson(std::move(it->second), value.external);
	if(const auto it = json.find("takeFocus"); it != json.end())
		fromJson(std::move(it->second), value.takeFocus);
	if(const auto it = json.find("selection"); it != json.end())
		fromJson(std::move(it->second), value.selection);
}

static void showDocumentResultToJson(ShowDocumentResult& value, json::Object& json)
{
	json["success"] = toJson(value.success);
}

static void showDocumentResultFromJson(json::Object& json, ShowDocumentResult& value)
{
	fromJson(std::move(json.get("success")), value.success);
}

static void linkedEditingRangeParamsToJson(LinkedEditingRangeParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
}

static void linkedEditingRangeParamsFromJson(json::Object& json, LinkedEditingRangeParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
}

static void linkedEditingRangesToJson(LinkedEditingRanges& value, json::Object& json)
{
	json["ranges"] = toJson(std::move(value.ranges));
	if(value.wordPattern)
		json["wordPattern"] = toJson(std::move(value.wordPattern));
}

static void linkedEditingRangesFromJson(json::Object& json, LinkedEditingRanges& value)
{
	fromJson(std::move(json.get("ranges")), value.ranges);
	if(const auto it = json.find("wordPattern"); it != json.end())
		fromJson(std::move(it->second), value.wordPattern);
}

static void linkedEditingRangeOptionsToJson(LinkedEditingRangeOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void linkedEditingRangeOptionsFromJson(json::Object& json, LinkedEditingRangeOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void linkedEditingRangeRegistrationOptionsToJson(LinkedEditingRangeRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	linkedEditingRangeOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void linkedEditingRangeRegistrationOptionsFromJson(json::Object& json, LinkedEditingRangeRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	linkedEditingRangeOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void fileCreateToJson(FileCreate& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
}

static void fileCreateFromJson(json::Object& json, FileCreate& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
}

static void createFilesParamsToJson(CreateFilesParams& value, json::Object& json)
{
	json["files"] = toJson(std::move(value.files));
}

static void createFilesParamsFromJson(json::Object& json, CreateFilesParams& value)
{
	fromJson(std::move(json.get("files")), value.files);
}

static void optionalVersionedTextDocumentIdentifierToJson(OptionalVersionedTextDocumentIdentifier& value, json::Object& json)
{
	textDocumentIdentifierToJson(value, json);
	json["version"] = toJson(std::move(value.version));
}

static void optionalVersionedTextDocumentIdentifierFromJson(json::Object& json, OptionalVersionedTextDocumentIdentifier& value)
{
	textDocumentIdentifierFromJson(json, value);
	fromJson(std::move(json.get("version")), value.version);
}

static void annotatedTextEditToJson(AnnotatedTextEdit& value, json::Object& json)
{
	textEditToJson(value, json);
	json["annotationId"] = toJson(std::move(value.annotationId));
}

static void annotatedTextEditFromJson(json::Object& json, AnnotatedTextEdit& value)
{
	textEditFromJson(json, value);
	fromJson(std::move(json.get("annotationId")), value.annotationId);
}

static void textDocumentEditToJson(TextDocumentEdit& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["edits"] = toJson(std::move(value.edits));
}

static void textDocumentEditFromJson(json::Object& json, TextDocumentEdit& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("edits")), value.edits);
}

static void resourceOperationToJson(ResourceOperation& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	if(value.annotationId)
		json["annotationId"] = toJson(std::move(value.annotationId));
}

static void resourceOperationFromJson(json::Object& json, ResourceOperation& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	if(const auto it = json.find("annotationId"); it != json.end())
		fromJson(std::move(it->second), value.annotationId);
}

static void createFileOptionsToJson(CreateFileOptions& value, json::Object& json)
{
	if(value.overwrite)
		json["overwrite"] = toJson(std::move(value.overwrite));
	if(value.ignoreIfExists)
		json["ignoreIfExists"] = toJson(std::move(value.ignoreIfExists));
}

static void createFileOptionsFromJson(json::Object& json, CreateFileOptions& value)
{
	if(const auto it = json.find("overwrite"); it != json.end())
		fromJson(std::move(it->second), value.overwrite);
	if(const auto it = json.find("ignoreIfExists"); it != json.end())
		fromJson(std::move(it->second), value.ignoreIfExists);
}

static void createFileToJson(CreateFile& value, json::Object& json)
{
	resourceOperationToJson(value, json);
	json["uri"] = toJson(std::move(value.uri));
	if(value.options)
		json["options"] = toJson(std::move(value.options));
}

static void createFileFromJson(json::Object& json, CreateFile& value)
{
	resourceOperationFromJson(json, value);
	if(value.kind != "create")
		throw json::TypeError("Unexpected value for literal 'kind'");
	fromJson(std::move(json.get("uri")), value.uri);
	if(const auto it = json.find("options"); it != json.end())
		fromJson(std::move(it->second), value.options);
}

static void renameFileOptionsToJson(RenameFileOptions& value, json::Object& json)
{
	if(value.overwrite)
		json["overwrite"] = toJson(std::move(value.overwrite));
	if(value.ignoreIfExists)
		json["ignoreIfExists"] = toJson(std::move(value.ignoreIfExists));
}

static void renameFileOptionsFromJson(json::Object& json, RenameFileOptions& value)
{
	if(const auto it = json.find("overwrite"); it != json.end())
		fromJson(std::move(it->second), value.overwrite);
	if(const auto it = json.find("ignoreIfExists"); it != json.end())
		fromJson(std::move(it->second), value.ignoreIfExists);
}

static void renameFileToJson(RenameFile& value, json::Object& json)
{
	resourceOperationToJson(value, json);
	json["oldUri"] = toJson(std::move(value.oldUri));
	json["newUri"] = toJson(std::move(value.newUri));
	if(value.options)
		json["options"] = toJson(std::move(value.options));
}

static void renameFileFromJson(json::Object& json, RenameFile& value)
{
	resourceOperationFromJson(json, value);
	if(value.kind != "rename")
		throw json::TypeError("Unexpected value for literal 'kind'");
	fromJson(std::move(json.get("oldUri")), value.oldUri);
	fromJson(std::move(json.get("newUri")), value.newUri);
	if(const auto it = json.find("options"); it != json.end())
		fromJson(std::move(it->second), value.options);
}

static void deleteFileOptionsToJson(DeleteFileOptions& value, json::Object& json)
{
	if(value.recursive)
		json["recursive"] = toJson(std::move(value.recursive));
	if(value.ignoreIfNotExists)
		json["ignoreIfNotExists"] = toJson(std::move(value.ignoreIfNotExists));
}

static void deleteFileOptionsFromJson(json::Object& json, DeleteFileOptions& value)
{
	if(const auto it = json.find("recursive"); it != json.end())
		fromJson(std::move(it->second), value.recursive);
	if(const auto it = json.find("ignoreIfNotExists"); it != json.end())
		fromJson(std::move(it->second), value.ignoreIfNotExists);
}

static void deleteFileToJson(DeleteFile& value, json::Object& json)
{
	resourceOperationToJson(value, json);
	json["uri"] = toJson(std::move(value.uri));
	if(value.options)
		json["options"] = toJson(std::move(value.options));
}

static void deleteFileFromJson(json::Object& json, DeleteFile& value)
{
	resourceOperationFromJson(json, value);
	if(value.kind != "delete")
		throw json::TypeError("Unexpected value for literal 'kind'");
	fromJson(std::move(json.get("uri")), value.uri);
	if(const auto it = json.find("options"); it != json.end())
		fromJson(std::move(it->second), value.options);
}

static void changeAnnotationToJson(ChangeAnnotation& value, json::Object& json)
{
	json["label"] = toJson(std::move(value.label));
	if(value.needsConfirmation)
		json["needsConfirmation"] = toJson(std::move(value.needsConfirmation));
	if(value.description)
		json["description"] = toJson(std::move(value.description));
}

static void changeAnnotationFromJson(json::Object& json, ChangeAnnotation& value)
{
	fromJson(std::move(json.get("label")), value.label);
	if(const auto it = json.find("needsConfirmation"); it != json.end())
		fromJson(std::move(it->second), value.needsConfirmation);
	if(const auto it = json.find("description"); it != json.end())
		fromJson(std::move(it->second), value.description);
}

static void workspaceEditToJson(WorkspaceEdit& value, json::Object& json)
{
	if(value.changes)
		json["changes"] = toJson(std::move(value.changes));
	if(value.documentChanges)
		json["documentChanges"] = toJson(std::move(value.documentChanges));
	if(value.changeAnnotations)
		json["changeAnnotations"] = toJson(std::move(value.changeAnnotations));
}

static void workspaceEditFromJson(json::Object& json, WorkspaceEdit& value)
{
	if(const auto it = json.find("changes"); it != json.end())
		fromJson(std::move(it->second), value.changes);
	if(const auto it = json.find("documentChanges"); it != json.end())
		fromJson(std::move(it->second), value.documentChanges);
	if(const auto it = json.find("changeAnnotations"); it != json.end())
		fromJson(std::move(it->second), value.changeAnnotations);
}

static void fileOperationPatternOptionsToJson(FileOperationPatternOptions& value, json::Object& json)
{
	if(value.ignoreCase)
		json["ignoreCase"] = toJson(std::move(value.ignoreCase));
}

static void fileOperationPatternOptionsFromJson(json::Object& json, FileOperationPatternOptions& value)
{
	if(const auto it = json.find("ignoreCase"); it != json.end())
		fromJson(std::move(it->second), value.ignoreCase);
}

static void fileOperationPatternToJson(FileOperationPattern& value, json::Object& json)
{
	json["glob"] = toJson(std::move(value.glob));
	if(value.matches)
		json["matches"] = toJson(std::move(value.matches));
	if(value.options)
		json["options"] = toJson(std::move(value.options));
}

static void fileOperationPatternFromJson(json::Object& json, FileOperationPattern& value)
{
	fromJson(std::move(json.get("glob")), value.glob);
	if(const auto it = json.find("matches"); it != json.end())
		fromJson(std::move(it->second), value.matches);
	if(const auto it = json.find("options"); it != json.end())
		fromJson(std::move(it->second), value.options);
}

static void fileOperationFilterToJson(FileOperationFilter& value, json::Object& json)
{
	json["pattern"] = toJson(std::move(value.pattern));
	if(value.scheme)
		json["scheme"] = toJson(std::move(value.scheme));
}

static void fileOperationFilterFromJson(json::Object& json, FileOperationFilter& value)
{
	fromJson(std::move(json.get("pattern")), value.pattern);
	if(const auto it = json.find("scheme"); it != json.end())
		fromJson(std::move(it->second), value.scheme);
}

static void fileOperationRegistrationOptionsToJson(FileOperationRegistrationOptions& value, json::Object& json)
{
	json["filters"] = toJson(std::move(value.filters));
}

static void fileOperationRegistrationOptionsFromJson(json::Object& json, FileOperationRegistrationOptions& value)
{
	fromJson(std::move(json.get("filters")), value.filters);
}

static void fileRenameToJson(FileRename& value, json::Object& json)
{
	json["oldUri"] = toJson(std::move(value.oldUri));
	json["newUri"] = toJson(std::move(value.newUri));
}

static void fileRenameFromJson(json::Object& json, FileRename& value)
{
	fromJson(std::move(json.get("oldUri")), value.oldUri);
	fromJson(std::move(json.get("newUri")), value.newUri);
}

static void renameFilesParamsToJson(RenameFilesParams& value, json::Object& json)
{
	json["files"] = toJson(std::move(value.files));
}

static void renameFilesParamsFromJson(json::Object& json, RenameFilesParams& value)
{
	fromJson(std::move(json.get("files")), value.files);
}

static void fileDeleteToJson(FileDelete& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
}

static void fileDeleteFromJson(json::Object& json, FileDelete& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
}

static void deleteFilesParamsToJson(DeleteFilesParams& value, json::Object& json)
{
	json["files"] = toJson(std::move(value.files));
}

static void deleteFilesParamsFromJson(json::Object& json, DeleteFilesParams& value)
{
	fromJson(std::move(json.get("files")), value.files);
}

static void monikerParamsToJson(MonikerParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
}

static void monikerParamsFromJson(json::Object& json, MonikerParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
}

static void monikerToJson(Moniker& value, json::Object& json)
{
	json["scheme"] = toJson(std::move(value.scheme));
	json["identifier"] = toJson(std::move(value.identifier));
	json["unique"] = toJson(std::move(value.unique));
	if(value.kind)
		json["kind"] = toJson(std::move(value.kind));
}

static void monikerFromJson(json::Object& json, Moniker& value)
{
	fromJson(std::move(json.get("scheme")), value.scheme);
	fromJson(std::move(json.get("identifier")), value.identifier);
	fromJson(std::move(json.get("unique")), value.unique);
	if(const auto it = json.find("kind"); it != json.end())
		fromJson(std::move(it->second), value.kind);
}

static void monikerOptionsToJson(MonikerOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void monikerOptionsFromJson(json::Object& json, MonikerOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void monikerRegistrationOptionsToJson(MonikerRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	monikerOptionsToJson(value, json);
}

static void monikerRegistrationOptionsFromJson(json::Object& json, MonikerRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	monikerOptionsFromJson(json, value);
}

static void typeHierarchyPrepareParamsToJson(TypeHierarchyPrepareParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
}

static void typeHierarchyPrepareParamsFromJson(json::Object& json, TypeHierarchyPrepareParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
}

static void typeHierarchyItemToJson(TypeHierarchyItem& value, json::Object& json)
{
	json["name"] = toJson(std::move(value.name));
	json["kind"] = toJson(std::move(value.kind));
	json["uri"] = toJson(std::move(value.uri));
	json["range"] = toJson(std::move(value.range));
	json["selectionRange"] = toJson(std::move(value.selectionRange));
	if(value.tags)
		json["tags"] = toJson(std::move(value.tags));
	if(value.detail)
		json["detail"] = toJson(std::move(value.detail));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void typeHierarchyItemFromJson(json::Object& json, TypeHierarchyItem& value)
{
	fromJson(std::move(json.get("name")), value.name);
	fromJson(std::move(json.get("kind")), value.kind);
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("selectionRange")), value.selectionRange);
	if(const auto it = json.find("tags"); it != json.end())
		fromJson(std::move(it->second), value.tags);
	if(const auto it = json.find("detail"); it != json.end())
		fromJson(std::move(it->second), value.detail);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void typeHierarchyOptionsToJson(TypeHierarchyOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void typeHierarchyOptionsFromJson(json::Object& json, TypeHierarchyOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void typeHierarchyRegistrationOptionsToJson(TypeHierarchyRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	typeHierarchyOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void typeHierarchyRegistrationOptionsFromJson(json::Object& json, TypeHierarchyRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	typeHierarchyOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void typeHierarchySupertypesParamsToJson(TypeHierarchySupertypesParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["item"] = toJson(std::move(value.item));
}

static void typeHierarchySupertypesParamsFromJson(json::Object& json, TypeHierarchySupertypesParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("item")), value.item);
}

static void typeHierarchySubtypesParamsToJson(TypeHierarchySubtypesParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["item"] = toJson(std::move(value.item));
}

static void typeHierarchySubtypesParamsFromJson(json::Object& json, TypeHierarchySubtypesParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("item")), value.item);
}

static void inlineValueContextToJson(InlineValueContext& value, json::Object& json)
{
	json["frameId"] = toJson(value.frameId);
	json["stoppedLocation"] = toJson(std::move(value.stoppedLocation));
}

static void inlineValueContextFromJson(json::Object& json, InlineValueContext& value)
{
	fromJson(std::move(json.get("frameId")), value.frameId);
	fromJson(std::move(json.get("stoppedLocation")), value.stoppedLocation);
}

static void inlineValueParamsToJson(InlineValueParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["range"] = toJson(std::move(value.range));
	json["context"] = toJson(std::move(value.context));
}

static void inlineValueParamsFromJson(json::Object& json, InlineValueParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("context")), value.context);
}

static void inlineValueOptionsToJson(InlineValueOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void inlineValueOptionsFromJson(json::Object& json, InlineValueOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void inlineValueRegistrationOptionsToJson(InlineValueRegistrationOptions& value, json::Object& json)
{
	inlineValueOptionsToJson(value, json);
	textDocumentRegistrationOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void inlineValueRegistrationOptionsFromJson(json::Object& json, InlineValueRegistrationOptions& value)
{
	inlineValueOptionsFromJson(json, value);
	textDocumentRegistrationOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void inlayHintParamsToJson(InlayHintParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["range"] = toJson(std::move(value.range));
}

static void inlayHintParamsFromJson(json::Object& json, InlayHintParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("range")), value.range);
}

static void markupContentToJson(MarkupContent& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	json["value"] = toJson(std::move(value.value));
}

static void markupContentFromJson(json::Object& json, MarkupContent& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	fromJson(std::move(json.get("value")), value.value);
}

static void commandToJson(Command& value, json::Object& json)
{
	json["title"] = toJson(std::move(value.title));
	json["command"] = toJson(std::move(value.command));
	if(value.arguments)
		json["arguments"] = toJson(std::move(value.arguments));
}

static void commandFromJson(json::Object& json, Command& value)
{
	fromJson(std::move(json.get("title")), value.title);
	fromJson(std::move(json.get("command")), value.command);
	if(const auto it = json.find("arguments"); it != json.end())
		fromJson(std::move(it->second), value.arguments);
}

static void inlayHintLabelPartToJson(InlayHintLabelPart& value, json::Object& json)
{
	json["value"] = toJson(std::move(value.value));
	if(value.tooltip)
		json["tooltip"] = toJson(std::move(value.tooltip));
	if(value.location)
		json["location"] = toJson(std::move(value.location));
	if(value.command)
		json["command"] = toJson(std::move(value.command));
}

static void inlayHintLabelPartFromJson(json::Object& json, InlayHintLabelPart& value)
{
	fromJson(std::move(json.get("value")), value.value);
	if(const auto it = json.find("tooltip"); it != json.end())
		fromJson(std::move(it->second), value.tooltip);
	if(const auto it = json.find("location"); it != json.end())
		fromJson(std::move(it->second), value.location);
	if(const auto it = json.find("command"); it != json.end())
		fromJson(std::move(it->second), value.command);
}

static void inlayHintToJson(InlayHint& value, json::Object& json)
{
	json["position"] = toJson(std::move(value.position));
	json["label"] = toJson(std::move(value.label));
	if(value.kind)
		json["kind"] = toJson(std::move(value.kind));
	if(value.textEdits)
		json["textEdits"] = toJson(std::move(value.textEdits));
	if(value.tooltip)
		json["tooltip"] = toJson(std::move(value.tooltip));
	if(value.paddingLeft)
		json["paddingLeft"] = toJson(std::move(value.paddingLeft));
	if(value.paddingRight)
		json["paddingRight"] = toJson(std::move(value.paddingRight));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void inlayHintFromJson(json::Object& json, InlayHint& value)
{
	fromJson(std::move(json.get("position")), value.position);
	fromJson(std::move(json.get("label")), value.label);
	if(const auto it = json.find("kind"); it != json.end())
		fromJson(std::move(it->second), value.kind);
	if(const auto it = json.find("textEdits"); it != json.end())
		fromJson(std::move(it->second), value.textEdits);
	if(const auto it = json.find("tooltip"); it != json.end())
		fromJson(std::move(it->second), value.tooltip);
	if(const auto it = json.find("paddingLeft"); it != json.end())
		fromJson(std::move(it->second), value.paddingLeft);
	if(const auto it = json.find("paddingRight"); it != json.end())
		fromJson(std::move(it->second), value.paddingRight);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void inlayHintOptionsToJson(InlayHintOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.resolveProvider)
		json["resolveProvider"] = toJson(std::move(value.resolveProvider));
}

static void inlayHintOptionsFromJson(json::Object& json, InlayHintOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("resolveProvider"); it != json.end())
		fromJson(std::move(it->second), value.resolveProvider);
}

static void inlayHintRegistrationOptionsToJson(InlayHintRegistrationOptions& value, json::Object& json)
{
	inlayHintOptionsToJson(value, json);
	textDocumentRegistrationOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void inlayHintRegistrationOptionsFromJson(json::Object& json, InlayHintRegistrationOptions& value)
{
	inlayHintOptionsFromJson(json, value);
	textDocumentRegistrationOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void documentDiagnosticParamsToJson(DocumentDiagnosticParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	if(value.identifier)
		json["identifier"] = toJson(std::move(value.identifier));
	if(value.previousResultId)
		json["previousResultId"] = toJson(std::move(value.previousResultId));
}

static void documentDiagnosticParamsFromJson(json::Object& json, DocumentDiagnosticParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	if(const auto it = json.find("identifier"); it != json.end())
		fromJson(std::move(it->second), value.identifier);
	if(const auto it = json.find("previousResultId"); it != json.end())
		fromJson(std::move(it->second), value.previousResultId);
}

static void codeDescriptionToJson(CodeDescription& value, json::Object& json)
{
	json["href"] = toJson(std::move(value.href));
}

static void codeDescriptionFromJson(json::Object& json, CodeDescription& value)
{
	fromJson(std::move(json.get("href")), value.href);
}

static void diagnosticRelatedInformationToJson(DiagnosticRelatedInformation& value, json::Object& json)
{
	json["location"] = toJson(std::move(value.location));
	json["message"] = toJson(std::move(value.message));
}

static void diagnosticRelatedInformationFromJson(json::Object& json, DiagnosticRelatedInformation& value)
{
	fromJson(std::move(json.get("location")), value.location);
	fromJson(std::move(json.get("message")), value.message);
}

static void diagnosticToJson(Diagnostic& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["message"] = toJson(std::move(value.message));
	if(value.severity)
		json["severity"] = toJson(std::move(value.severity));
	if(value.code)
		json["code"] = toJson(std::move(value.code));
	if(value.codeDescription)
		json["codeDescription"] = toJson(std::move(value.codeDescription));
	if(value.source)
		json["source"] = toJson(std::move(value.source));
	if(value.tags)
		json["tags"] = toJson(std::move(value.tags));
	if(value.relatedInformation)
		json["relatedInformation"] = toJson(std::move(value.relatedInformation));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void diagnosticFromJson(json::Object& json, Diagnostic& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("message")), value.message);
	if(const auto it = json.find("severity"); it != json.end())
		fromJson(std::move(it->second), value.severity);
	if(const auto it = json.find("code"); it != json.end())
		fromJson(std::move(it->second), value.code);
	if(const auto it = json.find("codeDescription"); it != json.end())
		fromJson(std::move(it->second), value.codeDescription);
	if(const auto it = json.find("source"); it != json.end())
		fromJson(std::move(it->second), value.source);
	if(const auto it = json.find("tags"); it != json.end())
		fromJson(std::move(it->second), value.tags);
	if(const auto it = json.find("relatedInformation"); it != json.end())
		fromJson(std::move(it->second), value.relatedInformation);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void fullDocumentDiagnosticReportToJson(FullDocumentDiagnosticReport& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	json["items"] = toJson(std::move(value.items));
	if(value.resultId)
		json["resultId"] = toJson(std::move(value.resultId));
}

static void fullDocumentDiagnosticReportFromJson(json::Object& json, FullDocumentDiagnosticReport& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	if(value.kind != "full")
		throw json::TypeError("Unexpected value for literal 'kind'");
	fromJson(std::move(json.get("items")), value.items);
	if(const auto it = json.find("resultId"); it != json.end())
		fromJson(std::move(it->second), value.resultId);
}

static void unchangedDocumentDiagnosticReportToJson(UnchangedDocumentDiagnosticReport& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	json["resultId"] = toJson(std::move(value.resultId));
}

static void unchangedDocumentDiagnosticReportFromJson(json::Object& json, UnchangedDocumentDiagnosticReport& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	if(value.kind != "unchanged")
		throw json::TypeError("Unexpected value for literal 'kind'");
	fromJson(std::move(json.get("resultId")), value.resultId);
}

static void documentDiagnosticReportPartialResultToJson(DocumentDiagnosticReportPartialResult& value, json::Object& json)
{
	json["relatedDocuments"] = toJson(std::move(value.relatedDocuments));
}

static void documentDiagnosticReportPartialResultFromJson(json::Object& json, DocumentDiagnosticReportPartialResult& value)
{
	fromJson(std::move(json.get("relatedDocuments")), value.relatedDocuments);
}

static void diagnosticServerCancellationDataToJson(DiagnosticServerCancellationData& value, json::Object& json)
{
	json["retriggerRequest"] = toJson(value.retriggerRequest);
}

static void diagnosticServerCancellationDataFromJson(json::Object& json, DiagnosticServerCancellationData& value)
{
	fromJson(std::move(json.get("retriggerRequest")), value.retriggerRequest);
}

static void diagnosticOptionsToJson(DiagnosticOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	json["interFileDependencies"] = toJson(value.interFileDependencies);
	json["workspaceDiagnostics"] = toJson(value.workspaceDiagnostics);
	if(value.identifier)
		json["identifier"] = toJson(std::move(value.identifier));
}

static void diagnosticOptionsFromJson(json::Object& json, DiagnosticOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	fromJson(std::move(json.get("interFileDependencies")), value.interFileDependencies);
	fromJson(std::move(json.get("workspaceDiagnostics")), value.workspaceDiagnostics);
	if(const auto it = json.find("identifier"); it != json.end())
		fromJson(std::move(it->second), value.identifier);
}

static void diagnosticRegistrationOptionsToJson(DiagnosticRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	diagnosticOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void diagnosticRegistrationOptionsFromJson(json::Object& json, DiagnosticRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	diagnosticOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void previousResultIdToJson(PreviousResultId& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	json["value"] = toJson(std::move(value.value));
}

static void previousResultIdFromJson(json::Object& json, PreviousResultId& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("value")), value.value);
}

static void workspaceDiagnosticParamsToJson(WorkspaceDiagnosticParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["previousResultIds"] = toJson(std::move(value.previousResultIds));
	if(value.identifier)
		json["identifier"] = toJson(std::move(value.identifier));
}

static void workspaceDiagnosticParamsFromJson(json::Object& json, WorkspaceDiagnosticParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("previousResultIds")), value.previousResultIds);
	if(const auto it = json.find("identifier"); it != json.end())
		fromJson(std::move(it->second), value.identifier);
}

static void workspaceFullDocumentDiagnosticReportToJson(WorkspaceFullDocumentDiagnosticReport& value, json::Object& json)
{
	fullDocumentDiagnosticReportToJson(value, json);
	json["uri"] = toJson(std::move(value.uri));
	json["version"] = toJson(std::move(value.version));
}

static void workspaceFullDocumentDiagnosticReportFromJson(json::Object& json, WorkspaceFullDocumentDiagnosticReport& value)
{
	fullDocumentDiagnosticReportFromJson(json, value);
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("version")), value.version);
}

static void workspaceUnchangedDocumentDiagnosticReportToJson(WorkspaceUnchangedDocumentDiagnosticReport& value, json::Object& json)
{
	unchangedDocumentDiagnosticReportToJson(value, json);
	json["uri"] = toJson(std::move(value.uri));
	json["version"] = toJson(std::move(value.version));
}

static void workspaceUnchangedDocumentDiagnosticReportFromJson(json::Object& json, WorkspaceUnchangedDocumentDiagnosticReport& value)
{
	unchangedDocumentDiagnosticReportFromJson(json, value);
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("version")), value.version);
}

static void workspaceDiagnosticReportToJson(WorkspaceDiagnosticReport& value, json::Object& json)
{
	json["items"] = toJson(std::move(value.items));
}

static void workspaceDiagnosticReportFromJson(json::Object& json, WorkspaceDiagnosticReport& value)
{
	fromJson(std::move(json.get("items")), value.items);
}

static void workspaceDiagnosticReportPartialResultToJson(WorkspaceDiagnosticReportPartialResult& value, json::Object& json)
{
	json["items"] = toJson(std::move(value.items));
}

static void workspaceDiagnosticReportPartialResultFromJson(json::Object& json, WorkspaceDiagnosticReportPartialResult& value)
{
	fromJson(std::move(json.get("items")), value.items);
}

static void executionSummaryToJson(ExecutionSummary& value, json::Object& json)
{
	json["executionOrder"] = toJson(value.executionOrder);
	if(value.success)
		json["success"] = toJson(std::move(value.success));
}

static void executionSummaryFromJson(json::Object& json, ExecutionSummary& value)
{
	fromJson(std::move(json.get("executionOrder")), value.executionOrder);
	if(const auto it = json.find("success"); it != json.end())
		fromJson(std::move(it->second), value.success);
}

static void notebookCellToJson(NotebookCell& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	json["document"] = toJson(std::move(value.document));
	if(value.metadata)
		json["metadata"] = toJson(std::move(value.metadata));
	if(value.executionSummary)
		json["executionSummary"] = toJson(std::move(value.executionSummary));
}

static void notebookCellFromJson(json::Object& json, NotebookCell& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	fromJson(std::move(json.get("document")), value.document);
	if(const auto it = json.find("metadata"); it != json.end())
		fromJson(std::move(it->second), value.metadata);
	if(const auto it = json.find("executionSummary"); it != json.end())
		fromJson(std::move(it->second), value.executionSummary);
}

static void notebookDocumentToJson(NotebookDocument& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	json["notebookType"] = toJson(std::move(value.notebookType));
	json["version"] = toJson(value.version);
	json["cells"] = toJson(std::move(value.cells));
	if(value.metadata)
		json["metadata"] = toJson(std::move(value.metadata));
}

static void notebookDocumentFromJson(json::Object& json, NotebookDocument& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("notebookType")), value.notebookType);
	fromJson(std::move(json.get("version")), value.version);
	fromJson(std::move(json.get("cells")), value.cells);
	if(const auto it = json.find("metadata"); it != json.end())
		fromJson(std::move(it->second), value.metadata);
}

static void textDocumentItemToJson(TextDocumentItem& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	json["languageId"] = toJson(std::move(value.languageId));
	json["version"] = toJson(value.version);
	json["text"] = toJson(std::move(value.text));
}

static void textDocumentItemFromJson(json::Object& json, TextDocumentItem& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("languageId")), value.languageId);
	fromJson(std::move(json.get("version")), value.version);
	fromJson(std::move(json.get("text")), value.text);
}

static void didOpenNotebookDocumentParamsToJson(DidOpenNotebookDocumentParams& value, json::Object& json)
{
	json["notebookDocument"] = toJson(std::move(value.notebookDocument));
	json["cellTextDocuments"] = toJson(std::move(value.cellTextDocuments));
}

static void didOpenNotebookDocumentParamsFromJson(json::Object& json, DidOpenNotebookDocumentParams& value)
{
	fromJson(std::move(json.get("notebookDocument")), value.notebookDocument);
	fromJson(std::move(json.get("cellTextDocuments")), value.cellTextDocuments);
}

static void versionedNotebookDocumentIdentifierToJson(VersionedNotebookDocumentIdentifier& value, json::Object& json)
{
	json["version"] = toJson(value.version);
	json["uri"] = toJson(std::move(value.uri));
}

static void versionedNotebookDocumentIdentifierFromJson(json::Object& json, VersionedNotebookDocumentIdentifier& value)
{
	fromJson(std::move(json.get("version")), value.version);
	fromJson(std::move(json.get("uri")), value.uri);
}

static void notebookCellArrayChangeToJson(NotebookCellArrayChange& value, json::Object& json)
{
	json["start"] = toJson(value.start);
	json["deleteCount"] = toJson(value.deleteCount);
	if(value.cells)
		json["cells"] = toJson(std::move(value.cells));
}

static void notebookCellArrayChangeFromJson(json::Object& json, NotebookCellArrayChange& value)
{
	fromJson(std::move(json.get("start")), value.start);
	fromJson(std::move(json.get("deleteCount")), value.deleteCount);
	if(const auto it = json.find("cells"); it != json.end())
		fromJson(std::move(it->second), value.cells);
}

static void notebookDocumentChangeEventCellsStructureToJson(NotebookDocumentChangeEventCellsStructure& value, json::Object& json)
{
	json["array"] = toJson(std::move(value.array));
	if(value.didOpen)
		json["didOpen"] = toJson(std::move(value.didOpen));
	if(value.didClose)
		json["didClose"] = toJson(std::move(value.didClose));
}

static void notebookDocumentChangeEventCellsStructureFromJson(json::Object& json, NotebookDocumentChangeEventCellsStructure& value)
{
	fromJson(std::move(json.get("array")), value.array);
	if(const auto it = json.find("didOpen"); it != json.end())
		fromJson(std::move(it->second), value.didOpen);
	if(const auto it = json.find("didClose"); it != json.end())
		fromJson(std::move(it->second), value.didClose);
}

static void versionedTextDocumentIdentifierToJson(VersionedTextDocumentIdentifier& value, json::Object& json)
{
	textDocumentIdentifierToJson(value, json);
	json["version"] = toJson(value.version);
}

static void versionedTextDocumentIdentifierFromJson(json::Object& json, VersionedTextDocumentIdentifier& value)
{
	textDocumentIdentifierFromJson(json, value);
	fromJson(std::move(json.get("version")), value.version);
}

static void textDocumentContentChangeEvent_Range_TextToJson(TextDocumentContentChangeEvent_Range_Text& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["text"] = toJson(std::move(value.text));
	if(value.rangeLength)
		json["rangeLength"] = toJson(std::move(value.rangeLength));
}

static void textDocumentContentChangeEvent_Range_TextFromJson(json::Object& json, TextDocumentContentChangeEvent_Range_Text& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("text")), value.text);
	if(const auto it = json.find("rangeLength"); it != json.end())
		fromJson(std::move(it->second), value.rangeLength);
}

static void textDocumentContentChangeEvent_TextToJson(TextDocumentContentChangeEvent_Text& value, json::Object& json)
{
	json["text"] = toJson(std::move(value.text));
}

static void textDocumentContentChangeEvent_TextFromJson(json::Object& json, TextDocumentContentChangeEvent_Text& value)
{
	fromJson(std::move(json.get("text")), value.text);
}

static void notebookDocumentChangeEventCellsTextContentToJson(NotebookDocumentChangeEventCellsTextContent& value, json::Object& json)
{
	json["document"] = toJson(std::move(value.document));
	json["changes"] = toJson(std::move(value.changes));
}

static void notebookDocumentChangeEventCellsTextContentFromJson(json::Object& json, NotebookDocumentChangeEventCellsTextContent& value)
{
	fromJson(std::move(json.get("document")), value.document);
	fromJson(std::move(json.get("changes")), value.changes);
}

static void notebookDocumentChangeEventCellsToJson(NotebookDocumentChangeEventCells& value, json::Object& json)
{
	if(value.structure)
		json["structure"] = toJson(std::move(value.structure));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
	if(value.textContent)
		json["textContent"] = toJson(std::move(value.textContent));
}

static void notebookDocumentChangeEventCellsFromJson(json::Object& json, NotebookDocumentChangeEventCells& value)
{
	if(const auto it = json.find("structure"); it != json.end())
		fromJson(std::move(it->second), value.structure);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
	if(const auto it = json.find("textContent"); it != json.end())
		fromJson(std::move(it->second), value.textContent);
}

static void notebookDocumentChangeEventToJson(NotebookDocumentChangeEvent& value, json::Object& json)
{
	if(value.metadata)
		json["metadata"] = toJson(std::move(value.metadata));
	if(value.cells)
		json["cells"] = toJson(std::move(value.cells));
}

static void notebookDocumentChangeEventFromJson(json::Object& json, NotebookDocumentChangeEvent& value)
{
	if(const auto it = json.find("metadata"); it != json.end())
		fromJson(std::move(it->second), value.metadata);
	if(const auto it = json.find("cells"); it != json.end())
		fromJson(std::move(it->second), value.cells);
}

static void didChangeNotebookDocumentParamsToJson(DidChangeNotebookDocumentParams& value, json::Object& json)
{
	json["notebookDocument"] = toJson(std::move(value.notebookDocument));
	json["change"] = toJson(std::move(value.change));
}

static void didChangeNotebookDocumentParamsFromJson(json::Object& json, DidChangeNotebookDocumentParams& value)
{
	fromJson(std::move(json.get("notebookDocument")), value.notebookDocument);
	fromJson(std::move(json.get("change")), value.change);
}

static void notebookDocumentIdentifierToJson(NotebookDocumentIdentifier& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
}

static void notebookDocumentIdentifierFromJson(json::Object& json, NotebookDocumentIdentifier& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
}

static void didSaveNotebookDocumentParamsToJson(DidSaveNotebookDocumentParams& value, json::Object& json)
{
	json["notebookDocument"] = toJson(std::move(value.notebookDocument));
}

static void didSaveNotebookDocumentParamsFromJson(json::Object& json, DidSaveNotebookDocumentParams& value)
{
	fromJson(std::move(json.get("notebookDocument")), value.notebookDocument);
}

static void didCloseNotebookDocumentParamsToJson(DidCloseNotebookDocumentParams& value, json::Object& json)
{
	json["notebookDocument"] = toJson(std::move(value.notebookDocument));
	json["cellTextDocuments"] = toJson(std::move(value.cellTextDocuments));
}

static void didCloseNotebookDocumentParamsFromJson(json::Object& json, DidCloseNotebookDocumentParams& value)
{
	fromJson(std::move(json.get("notebookDocument")), value.notebookDocument);
	fromJson(std::move(json.get("cellTextDocuments")), value.cellTextDocuments);
}

static void selectedCompletionInfoToJson(SelectedCompletionInfo& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["text"] = toJson(std::move(value.text));
}

static void selectedCompletionInfoFromJson(json::Object& json, SelectedCompletionInfo& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("text")), value.text);
}

static void inlineCompletionContextToJson(InlineCompletionContext& value, json::Object& json)
{
	json["triggerKind"] = toJson(std::move(value.triggerKind));
	if(value.selectedCompletionInfo)
		json["selectedCompletionInfo"] = toJson(std::move(value.selectedCompletionInfo));
}

static void inlineCompletionContextFromJson(json::Object& json, InlineCompletionContext& value)
{
	fromJson(std::move(json.get("triggerKind")), value.triggerKind);
	if(const auto it = json.find("selectedCompletionInfo"); it != json.end())
		fromJson(std::move(it->second), value.selectedCompletionInfo);
}

static void inlineCompletionParamsToJson(InlineCompletionParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["context"] = toJson(std::move(value.context));
}

static void inlineCompletionParamsFromJson(json::Object& json, InlineCompletionParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("context")), value.context);
}

static void stringValueToJson(StringValue& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	json["value"] = toJson(std::move(value.value));
}

static void stringValueFromJson(json::Object& json, StringValue& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	if(value.kind != "snippet")
		throw json::TypeError("Unexpected value for literal 'kind'");
	fromJson(std::move(json.get("value")), value.value);
}

static void inlineCompletionItemToJson(InlineCompletionItem& value, json::Object& json)
{
	json["insertText"] = toJson(std::move(value.insertText));
	if(value.filterText)
		json["filterText"] = toJson(std::move(value.filterText));
	if(value.range)
		json["range"] = toJson(std::move(value.range));
	if(value.command)
		json["command"] = toJson(std::move(value.command));
}

static void inlineCompletionItemFromJson(json::Object& json, InlineCompletionItem& value)
{
	fromJson(std::move(json.get("insertText")), value.insertText);
	if(const auto it = json.find("filterText"); it != json.end())
		fromJson(std::move(it->second), value.filterText);
	if(const auto it = json.find("range"); it != json.end())
		fromJson(std::move(it->second), value.range);
	if(const auto it = json.find("command"); it != json.end())
		fromJson(std::move(it->second), value.command);
}

static void inlineCompletionListToJson(InlineCompletionList& value, json::Object& json)
{
	json["items"] = toJson(std::move(value.items));
}

static void inlineCompletionListFromJson(json::Object& json, InlineCompletionList& value)
{
	fromJson(std::move(json.get("items")), value.items);
}

static void inlineCompletionOptionsToJson(InlineCompletionOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void inlineCompletionOptionsFromJson(json::Object& json, InlineCompletionOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void inlineCompletionRegistrationOptionsToJson(InlineCompletionRegistrationOptions& value, json::Object& json)
{
	inlineCompletionOptionsToJson(value, json);
	textDocumentRegistrationOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void inlineCompletionRegistrationOptionsFromJson(json::Object& json, InlineCompletionRegistrationOptions& value)
{
	inlineCompletionOptionsFromJson(json, value);
	textDocumentRegistrationOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void registrationToJson(Registration& value, json::Object& json)
{
	json["id"] = toJson(std::move(value.id));
	json["method"] = toJson(std::move(value.method));
	if(value.registerOptions)
		json["registerOptions"] = toJson(std::move(value.registerOptions));
}

static void registrationFromJson(json::Object& json, Registration& value)
{
	fromJson(std::move(json.get("id")), value.id);
	fromJson(std::move(json.get("method")), value.method);
	if(const auto it = json.find("registerOptions"); it != json.end())
		fromJson(std::move(it->second), value.registerOptions);
}

static void registrationParamsToJson(RegistrationParams& value, json::Object& json)
{
	json["registrations"] = toJson(std::move(value.registrations));
}

static void registrationParamsFromJson(json::Object& json, RegistrationParams& value)
{
	fromJson(std::move(json.get("registrations")), value.registrations);
}

static void unregistrationToJson(Unregistration& value, json::Object& json)
{
	json["id"] = toJson(std::move(value.id));
	json["method"] = toJson(std::move(value.method));
}

static void unregistrationFromJson(json::Object& json, Unregistration& value)
{
	fromJson(std::move(json.get("id")), value.id);
	fromJson(std::move(json.get("method")), value.method);
}

static void unregistrationParamsToJson(UnregistrationParams& value, json::Object& json)
{
	json["unregisterations"] = toJson(std::move(value.unregisterations));
}

static void unregistrationParamsFromJson(json::Object& json, UnregistrationParams& value)
{
	fromJson(std::move(json.get("unregisterations")), value.unregisterations);
}

static void workspaceEditClientCapabilitiesChangeAnnotationSupportToJson(WorkspaceEditClientCapabilitiesChangeAnnotationSupport& value, json::Object& json)
{
	if(value.groupsOnLabel)
		json["groupsOnLabel"] = toJson(std::move(value.groupsOnLabel));
}

static void workspaceEditClientCapabilitiesChangeAnnotationSupportFromJson(json::Object& json, WorkspaceEditClientCapabilitiesChangeAnnotationSupport& value)
{
	if(const auto it = json.find("groupsOnLabel"); it != json.end())
		fromJson(std::move(it->second), value.groupsOnLabel);
}

static void workspaceEditClientCapabilitiesToJson(WorkspaceEditClientCapabilities& value, json::Object& json)
{
	if(value.documentChanges)
		json["documentChanges"] = toJson(std::move(value.documentChanges));
	if(value.resourceOperations)
		json["resourceOperations"] = toJson(std::move(value.resourceOperations));
	if(value.failureHandling)
		json["failureHandling"] = toJson(std::move(value.failureHandling));
	if(value.normalizesLineEndings)
		json["normalizesLineEndings"] = toJson(std::move(value.normalizesLineEndings));
	if(value.changeAnnotationSupport)
		json["changeAnnotationSupport"] = toJson(std::move(value.changeAnnotationSupport));
}

static void workspaceEditClientCapabilitiesFromJson(json::Object& json, WorkspaceEditClientCapabilities& value)
{
	if(const auto it = json.find("documentChanges"); it != json.end())
		fromJson(std::move(it->second), value.documentChanges);
	if(const auto it = json.find("resourceOperations"); it != json.end())
		fromJson(std::move(it->second), value.resourceOperations);
	if(const auto it = json.find("failureHandling"); it != json.end())
		fromJson(std::move(it->second), value.failureHandling);
	if(const auto it = json.find("normalizesLineEndings"); it != json.end())
		fromJson(std::move(it->second), value.normalizesLineEndings);
	if(const auto it = json.find("changeAnnotationSupport"); it != json.end())
		fromJson(std::move(it->second), value.changeAnnotationSupport);
}

static void didChangeConfigurationClientCapabilitiesToJson(DidChangeConfigurationClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void didChangeConfigurationClientCapabilitiesFromJson(json::Object& json, DidChangeConfigurationClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void didChangeWatchedFilesClientCapabilitiesToJson(DidChangeWatchedFilesClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.relativePatternSupport)
		json["relativePatternSupport"] = toJson(std::move(value.relativePatternSupport));
}

static void didChangeWatchedFilesClientCapabilitiesFromJson(json::Object& json, DidChangeWatchedFilesClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("relativePatternSupport"); it != json.end())
		fromJson(std::move(it->second), value.relativePatternSupport);
}

static void workspaceSymbolClientCapabilitiesSymbolKindToJson(WorkspaceSymbolClientCapabilitiesSymbolKind& value, json::Object& json)
{
	if(value.valueSet)
		json["valueSet"] = toJson(std::move(value.valueSet));
}

static void workspaceSymbolClientCapabilitiesSymbolKindFromJson(json::Object& json, WorkspaceSymbolClientCapabilitiesSymbolKind& value)
{
	if(const auto it = json.find("valueSet"); it != json.end())
		fromJson(std::move(it->second), value.valueSet);
}

static void workspaceSymbolClientCapabilitiesTagSupportToJson(WorkspaceSymbolClientCapabilitiesTagSupport& value, json::Object& json)
{
	json["valueSet"] = toJson(std::move(value.valueSet));
}

static void workspaceSymbolClientCapabilitiesTagSupportFromJson(json::Object& json, WorkspaceSymbolClientCapabilitiesTagSupport& value)
{
	fromJson(std::move(json.get("valueSet")), value.valueSet);
}

static void workspaceSymbolClientCapabilitiesResolveSupportToJson(WorkspaceSymbolClientCapabilitiesResolveSupport& value, json::Object& json)
{
	json["properties"] = toJson(std::move(value.properties));
}

static void workspaceSymbolClientCapabilitiesResolveSupportFromJson(json::Object& json, WorkspaceSymbolClientCapabilitiesResolveSupport& value)
{
	fromJson(std::move(json.get("properties")), value.properties);
}

static void workspaceSymbolClientCapabilitiesToJson(WorkspaceSymbolClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.symbolKind)
		json["symbolKind"] = toJson(std::move(value.symbolKind));
	if(value.tagSupport)
		json["tagSupport"] = toJson(std::move(value.tagSupport));
	if(value.resolveSupport)
		json["resolveSupport"] = toJson(std::move(value.resolveSupport));
}

static void workspaceSymbolClientCapabilitiesFromJson(json::Object& json, WorkspaceSymbolClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("symbolKind"); it != json.end())
		fromJson(std::move(it->second), value.symbolKind);
	if(const auto it = json.find("tagSupport"); it != json.end())
		fromJson(std::move(it->second), value.tagSupport);
	if(const auto it = json.find("resolveSupport"); it != json.end())
		fromJson(std::move(it->second), value.resolveSupport);
}

static void executeCommandClientCapabilitiesToJson(ExecuteCommandClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void executeCommandClientCapabilitiesFromJson(json::Object& json, ExecuteCommandClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void semanticTokensWorkspaceClientCapabilitiesToJson(SemanticTokensWorkspaceClientCapabilities& value, json::Object& json)
{
	if(value.refreshSupport)
		json["refreshSupport"] = toJson(std::move(value.refreshSupport));
}

static void semanticTokensWorkspaceClientCapabilitiesFromJson(json::Object& json, SemanticTokensWorkspaceClientCapabilities& value)
{
	if(const auto it = json.find("refreshSupport"); it != json.end())
		fromJson(std::move(it->second), value.refreshSupport);
}

static void codeLensWorkspaceClientCapabilitiesToJson(CodeLensWorkspaceClientCapabilities& value, json::Object& json)
{
	if(value.refreshSupport)
		json["refreshSupport"] = toJson(std::move(value.refreshSupport));
}

static void codeLensWorkspaceClientCapabilitiesFromJson(json::Object& json, CodeLensWorkspaceClientCapabilities& value)
{
	if(const auto it = json.find("refreshSupport"); it != json.end())
		fromJson(std::move(it->second), value.refreshSupport);
}

static void fileOperationClientCapabilitiesToJson(FileOperationClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.didCreate)
		json["didCreate"] = toJson(std::move(value.didCreate));
	if(value.willCreate)
		json["willCreate"] = toJson(std::move(value.willCreate));
	if(value.didRename)
		json["didRename"] = toJson(std::move(value.didRename));
	if(value.willRename)
		json["willRename"] = toJson(std::move(value.willRename));
	if(value.didDelete)
		json["didDelete"] = toJson(std::move(value.didDelete));
	if(value.willDelete)
		json["willDelete"] = toJson(std::move(value.willDelete));
}

static void fileOperationClientCapabilitiesFromJson(json::Object& json, FileOperationClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("didCreate"); it != json.end())
		fromJson(std::move(it->second), value.didCreate);
	if(const auto it = json.find("willCreate"); it != json.end())
		fromJson(std::move(it->second), value.willCreate);
	if(const auto it = json.find("didRename"); it != json.end())
		fromJson(std::move(it->second), value.didRename);
	if(const auto it = json.find("willRename"); it != json.end())
		fromJson(std::move(it->second), value.willRename);
	if(const auto it = json.find("didDelete"); it != json.end())
		fromJson(std::move(it->second), value.didDelete);
	if(const auto it = json.find("willDelete"); it != json.end())
		fromJson(std::move(it->second), value.willDelete);
}

static void inlineValueWorkspaceClientCapabilitiesToJson(InlineValueWorkspaceClientCapabilities& value, json::Object& json)
{
	if(value.refreshSupport)
		json["refreshSupport"] = toJson(std::move(value.refreshSupport));
}

static void inlineValueWorkspaceClientCapabilitiesFromJson(json::Object& json, InlineValueWorkspaceClientCapabilities& value)
{
	if(const auto it = json.find("refreshSupport"); it != json.end())
		fromJson(std::move(it->second), value.refreshSupport);
}

static void inlayHintWorkspaceClientCapabilitiesToJson(InlayHintWorkspaceClientCapabilities& value, json::Object& json)
{
	if(value.refreshSupport)
		json["refreshSupport"] = toJson(std::move(value.refreshSupport));
}

static void inlayHintWorkspaceClientCapabilitiesFromJson(json::Object& json, InlayHintWorkspaceClientCapabilities& value)
{
	if(const auto it = json.find("refreshSupport"); it != json.end())
		fromJson(std::move(it->second), value.refreshSupport);
}

static void diagnosticWorkspaceClientCapabilitiesToJson(DiagnosticWorkspaceClientCapabilities& value, json::Object& json)
{
	if(value.refreshSupport)
		json["refreshSupport"] = toJson(std::move(value.refreshSupport));
}

static void diagnosticWorkspaceClientCapabilitiesFromJson(json::Object& json, DiagnosticWorkspaceClientCapabilities& value)
{
	if(const auto it = json.find("refreshSupport"); it != json.end())
		fromJson(std::move(it->second), value.refreshSupport);
}

static void foldingRangeWorkspaceClientCapabilitiesToJson(FoldingRangeWorkspaceClientCapabilities& value, json::Object& json)
{
	if(value.refreshSupport)
		json["refreshSupport"] = toJson(std::move(value.refreshSupport));
}

static void foldingRangeWorkspaceClientCapabilitiesFromJson(json::Object& json, FoldingRangeWorkspaceClientCapabilities& value)
{
	if(const auto it = json.find("refreshSupport"); it != json.end())
		fromJson(std::move(it->second), value.refreshSupport);
}

static void workspaceClientCapabilitiesToJson(WorkspaceClientCapabilities& value, json::Object& json)
{
	if(value.applyEdit)
		json["applyEdit"] = toJson(std::move(value.applyEdit));
	if(value.workspaceEdit)
		json["workspaceEdit"] = toJson(std::move(value.workspaceEdit));
	if(value.didChangeConfiguration)
		json["didChangeConfiguration"] = toJson(std::move(value.didChangeConfiguration));
	if(value.didChangeWatchedFiles)
		json["didChangeWatchedFiles"] = toJson(std::move(value.didChangeWatchedFiles));
	if(value.symbol)
		json["symbol"] = toJson(std::move(value.symbol));
	if(value.executeCommand)
		json["executeCommand"] = toJson(std::move(value.executeCommand));
	if(value.workspaceFolders)
		json["workspaceFolders"] = toJson(std::move(value.workspaceFolders));
	if(value.configuration)
		json["configuration"] = toJson(std::move(value.configuration));
	if(value.semanticTokens)
		json["semanticTokens"] = toJson(std::move(value.semanticTokens));
	if(value.codeLens)
		json["codeLens"] = toJson(std::move(value.codeLens));
	if(value.fileOperations)
		json["fileOperations"] = toJson(std::move(value.fileOperations));
	if(value.inlineValue)
		json["inlineValue"] = toJson(std::move(value.inlineValue));
	if(value.inlayHint)
		json["inlayHint"] = toJson(std::move(value.inlayHint));
	if(value.diagnostics)
		json["diagnostics"] = toJson(std::move(value.diagnostics));
	if(value.foldingRange)
		json["foldingRange"] = toJson(std::move(value.foldingRange));
}

static void workspaceClientCapabilitiesFromJson(json::Object& json, WorkspaceClientCapabilities& value)
{
	if(const auto it = json.find("applyEdit"); it != json.end())
		fromJson(std::move(it->second), value.applyEdit);
	if(const auto it = json.find("workspaceEdit"); it != json.end())
		fromJson(std::move(it->second), value.workspaceEdit);
	if(const auto it = json.find("didChangeConfiguration"); it != json.end())
		fromJson(std::move(it->second), value.didChangeConfiguration);
	if(const auto it = json.find("didChangeWatchedFiles"); it != json.end())
		fromJson(std::move(it->second), value.didChangeWatchedFiles);
	if(const auto it = json.find("symbol"); it != json.end())
		fromJson(std::move(it->second), value.symbol);
	if(const auto it = json.find("executeCommand"); it != json.end())
		fromJson(std::move(it->second), value.executeCommand);
	if(const auto it = json.find("workspaceFolders"); it != json.end())
		fromJson(std::move(it->second), value.workspaceFolders);
	if(const auto it = json.find("configuration"); it != json.end())
		fromJson(std::move(it->second), value.configuration);
	if(const auto it = json.find("semanticTokens"); it != json.end())
		fromJson(std::move(it->second), value.semanticTokens);
	if(const auto it = json.find("codeLens"); it != json.end())
		fromJson(std::move(it->second), value.codeLens);
	if(const auto it = json.find("fileOperations"); it != json.end())
		fromJson(std::move(it->second), value.fileOperations);
	if(const auto it = json.find("inlineValue"); it != json.end())
		fromJson(std::move(it->second), value.inlineValue);
	if(const auto it = json.find("inlayHint"); it != json.end())
		fromJson(std::move(it->second), value.inlayHint);
	if(const auto it = json.find("diagnostics"); it != json.end())
		fromJson(std::move(it->second), value.diagnostics);
	if(const auto it = json.find("foldingRange"); it != json.end())
		fromJson(std::move(it->second), value.foldingRange);
}

static void textDocumentSyncClientCapabilitiesToJson(TextDocumentSyncClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.willSave)
		json["willSave"] = toJson(std::move(value.willSave));
	if(value.willSaveWaitUntil)
		json["willSaveWaitUntil"] = toJson(std::move(value.willSaveWaitUntil));
	if(value.didSave)
		json["didSave"] = toJson(std::move(value.didSave));
}

static void textDocumentSyncClientCapabilitiesFromJson(json::Object& json, TextDocumentSyncClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("willSave"); it != json.end())
		fromJson(std::move(it->second), value.willSave);
	if(const auto it = json.find("willSaveWaitUntil"); it != json.end())
		fromJson(std::move(it->second), value.willSaveWaitUntil);
	if(const auto it = json.find("didSave"); it != json.end())
		fromJson(std::move(it->second), value.didSave);
}

static void completionClientCapabilitiesCompletionItemTagSupportToJson(CompletionClientCapabilitiesCompletionItemTagSupport& value, json::Object& json)
{
	json["valueSet"] = toJson(std::move(value.valueSet));
}

static void completionClientCapabilitiesCompletionItemTagSupportFromJson(json::Object& json, CompletionClientCapabilitiesCompletionItemTagSupport& value)
{
	fromJson(std::move(json.get("valueSet")), value.valueSet);
}

static void completionClientCapabilitiesCompletionItemResolveSupportToJson(CompletionClientCapabilitiesCompletionItemResolveSupport& value, json::Object& json)
{
	json["properties"] = toJson(std::move(value.properties));
}

static void completionClientCapabilitiesCompletionItemResolveSupportFromJson(json::Object& json, CompletionClientCapabilitiesCompletionItemResolveSupport& value)
{
	fromJson(std::move(json.get("properties")), value.properties);
}

static void completionClientCapabilitiesCompletionItemInsertTextModeSupportToJson(CompletionClientCapabilitiesCompletionItemInsertTextModeSupport& value, json::Object& json)
{
	json["valueSet"] = toJson(std::move(value.valueSet));
}

static void completionClientCapabilitiesCompletionItemInsertTextModeSupportFromJson(json::Object& json, CompletionClientCapabilitiesCompletionItemInsertTextModeSupport& value)
{
	fromJson(std::move(json.get("valueSet")), value.valueSet);
}

static void completionClientCapabilitiesCompletionItemToJson(CompletionClientCapabilitiesCompletionItem& value, json::Object& json)
{
	if(value.snippetSupport)
		json["snippetSupport"] = toJson(std::move(value.snippetSupport));
	if(value.commitCharactersSupport)
		json["commitCharactersSupport"] = toJson(std::move(value.commitCharactersSupport));
	if(value.documentationFormat)
		json["documentationFormat"] = toJson(std::move(value.documentationFormat));
	if(value.deprecatedSupport)
		json["deprecatedSupport"] = toJson(std::move(value.deprecatedSupport));
	if(value.preselectSupport)
		json["preselectSupport"] = toJson(std::move(value.preselectSupport));
	if(value.tagSupport)
		json["tagSupport"] = toJson(std::move(value.tagSupport));
	if(value.insertReplaceSupport)
		json["insertReplaceSupport"] = toJson(std::move(value.insertReplaceSupport));
	if(value.resolveSupport)
		json["resolveSupport"] = toJson(std::move(value.resolveSupport));
	if(value.insertTextModeSupport)
		json["insertTextModeSupport"] = toJson(std::move(value.insertTextModeSupport));
	if(value.labelDetailsSupport)
		json["labelDetailsSupport"] = toJson(std::move(value.labelDetailsSupport));
}

static void completionClientCapabilitiesCompletionItemFromJson(json::Object& json, CompletionClientCapabilitiesCompletionItem& value)
{
	if(const auto it = json.find("snippetSupport"); it != json.end())
		fromJson(std::move(it->second), value.snippetSupport);
	if(const auto it = json.find("commitCharactersSupport"); it != json.end())
		fromJson(std::move(it->second), value.commitCharactersSupport);
	if(const auto it = json.find("documentationFormat"); it != json.end())
		fromJson(std::move(it->second), value.documentationFormat);
	if(const auto it = json.find("deprecatedSupport"); it != json.end())
		fromJson(std::move(it->second), value.deprecatedSupport);
	if(const auto it = json.find("preselectSupport"); it != json.end())
		fromJson(std::move(it->second), value.preselectSupport);
	if(const auto it = json.find("tagSupport"); it != json.end())
		fromJson(std::move(it->second), value.tagSupport);
	if(const auto it = json.find("insertReplaceSupport"); it != json.end())
		fromJson(std::move(it->second), value.insertReplaceSupport);
	if(const auto it = json.find("resolveSupport"); it != json.end())
		fromJson(std::move(it->second), value.resolveSupport);
	if(const auto it = json.find("insertTextModeSupport"); it != json.end())
		fromJson(std::move(it->second), value.insertTextModeSupport);
	if(const auto it = json.find("labelDetailsSupport"); it != json.end())
		fromJson(std::move(it->second), value.labelDetailsSupport);
}

static void completionClientCapabilitiesCompletionItemKindToJson(CompletionClientCapabilitiesCompletionItemKind& value, json::Object& json)
{
	if(value.valueSet)
		json["valueSet"] = toJson(std::move(value.valueSet));
}

static void completionClientCapabilitiesCompletionItemKindFromJson(json::Object& json, CompletionClientCapabilitiesCompletionItemKind& value)
{
	if(const auto it = json.find("valueSet"); it != json.end())
		fromJson(std::move(it->second), value.valueSet);
}

static void completionClientCapabilitiesCompletionListToJson(CompletionClientCapabilitiesCompletionList& value, json::Object& json)
{
	if(value.itemDefaults)
		json["itemDefaults"] = toJson(std::move(value.itemDefaults));
}

static void completionClientCapabilitiesCompletionListFromJson(json::Object& json, CompletionClientCapabilitiesCompletionList& value)
{
	if(const auto it = json.find("itemDefaults"); it != json.end())
		fromJson(std::move(it->second), value.itemDefaults);
}

static void completionClientCapabilitiesToJson(CompletionClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.completionItem)
		json["completionItem"] = toJson(std::move(value.completionItem));
	if(value.completionItemKind)
		json["completionItemKind"] = toJson(std::move(value.completionItemKind));
	if(value.insertTextMode)
		json["insertTextMode"] = toJson(std::move(value.insertTextMode));
	if(value.contextSupport)
		json["contextSupport"] = toJson(std::move(value.contextSupport));
	if(value.completionList)
		json["completionList"] = toJson(std::move(value.completionList));
}

static void completionClientCapabilitiesFromJson(json::Object& json, CompletionClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("completionItem"); it != json.end())
		fromJson(std::move(it->second), value.completionItem);
	if(const auto it = json.find("completionItemKind"); it != json.end())
		fromJson(std::move(it->second), value.completionItemKind);
	if(const auto it = json.find("insertTextMode"); it != json.end())
		fromJson(std::move(it->second), value.insertTextMode);
	if(const auto it = json.find("contextSupport"); it != json.end())
		fromJson(std::move(it->second), value.contextSupport);
	if(const auto it = json.find("completionList"); it != json.end())
		fromJson(std::move(it->second), value.completionList);
}

static void hoverClientCapabilitiesToJson(HoverClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.contentFormat)
		json["contentFormat"] = toJson(std::move(value.contentFormat));
}

static void hoverClientCapabilitiesFromJson(json::Object& json, HoverClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("contentFormat"); it != json.end())
		fromJson(std::move(it->second), value.contentFormat);
}

static void signatureHelpClientCapabilitiesSignatureInformationParameterInformationToJson(SignatureHelpClientCapabilitiesSignatureInformationParameterInformation& value, json::Object& json)
{
	if(value.labelOffsetSupport)
		json["labelOffsetSupport"] = toJson(std::move(value.labelOffsetSupport));
}

static void signatureHelpClientCapabilitiesSignatureInformationParameterInformationFromJson(json::Object& json, SignatureHelpClientCapabilitiesSignatureInformationParameterInformation& value)
{
	if(const auto it = json.find("labelOffsetSupport"); it != json.end())
		fromJson(std::move(it->second), value.labelOffsetSupport);
}

static void signatureHelpClientCapabilitiesSignatureInformationToJson(SignatureHelpClientCapabilitiesSignatureInformation& value, json::Object& json)
{
	if(value.documentationFormat)
		json["documentationFormat"] = toJson(std::move(value.documentationFormat));
	if(value.parameterInformation)
		json["parameterInformation"] = toJson(std::move(value.parameterInformation));
	if(value.activeParameterSupport)
		json["activeParameterSupport"] = toJson(std::move(value.activeParameterSupport));
}

static void signatureHelpClientCapabilitiesSignatureInformationFromJson(json::Object& json, SignatureHelpClientCapabilitiesSignatureInformation& value)
{
	if(const auto it = json.find("documentationFormat"); it != json.end())
		fromJson(std::move(it->second), value.documentationFormat);
	if(const auto it = json.find("parameterInformation"); it != json.end())
		fromJson(std::move(it->second), value.parameterInformation);
	if(const auto it = json.find("activeParameterSupport"); it != json.end())
		fromJson(std::move(it->second), value.activeParameterSupport);
}

static void signatureHelpClientCapabilitiesToJson(SignatureHelpClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.signatureInformation)
		json["signatureInformation"] = toJson(std::move(value.signatureInformation));
	if(value.contextSupport)
		json["contextSupport"] = toJson(std::move(value.contextSupport));
}

static void signatureHelpClientCapabilitiesFromJson(json::Object& json, SignatureHelpClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("signatureInformation"); it != json.end())
		fromJson(std::move(it->second), value.signatureInformation);
	if(const auto it = json.find("contextSupport"); it != json.end())
		fromJson(std::move(it->second), value.contextSupport);
}

static void declarationClientCapabilitiesToJson(DeclarationClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.linkSupport)
		json["linkSupport"] = toJson(std::move(value.linkSupport));
}

static void declarationClientCapabilitiesFromJson(json::Object& json, DeclarationClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("linkSupport"); it != json.end())
		fromJson(std::move(it->second), value.linkSupport);
}

static void definitionClientCapabilitiesToJson(DefinitionClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.linkSupport)
		json["linkSupport"] = toJson(std::move(value.linkSupport));
}

static void definitionClientCapabilitiesFromJson(json::Object& json, DefinitionClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("linkSupport"); it != json.end())
		fromJson(std::move(it->second), value.linkSupport);
}

static void typeDefinitionClientCapabilitiesToJson(TypeDefinitionClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.linkSupport)
		json["linkSupport"] = toJson(std::move(value.linkSupport));
}

static void typeDefinitionClientCapabilitiesFromJson(json::Object& json, TypeDefinitionClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("linkSupport"); it != json.end())
		fromJson(std::move(it->second), value.linkSupport);
}

static void implementationClientCapabilitiesToJson(ImplementationClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.linkSupport)
		json["linkSupport"] = toJson(std::move(value.linkSupport));
}

static void implementationClientCapabilitiesFromJson(json::Object& json, ImplementationClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("linkSupport"); it != json.end())
		fromJson(std::move(it->second), value.linkSupport);
}

static void referenceClientCapabilitiesToJson(ReferenceClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void referenceClientCapabilitiesFromJson(json::Object& json, ReferenceClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void documentHighlightClientCapabilitiesToJson(DocumentHighlightClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void documentHighlightClientCapabilitiesFromJson(json::Object& json, DocumentHighlightClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void documentSymbolClientCapabilitiesSymbolKindToJson(DocumentSymbolClientCapabilitiesSymbolKind& value, json::Object& json)
{
	if(value.valueSet)
		json["valueSet"] = toJson(std::move(value.valueSet));
}

static void documentSymbolClientCapabilitiesSymbolKindFromJson(json::Object& json, DocumentSymbolClientCapabilitiesSymbolKind& value)
{
	if(const auto it = json.find("valueSet"); it != json.end())
		fromJson(std::move(it->second), value.valueSet);
}

static void documentSymbolClientCapabilitiesTagSupportToJson(DocumentSymbolClientCapabilitiesTagSupport& value, json::Object& json)
{
	json["valueSet"] = toJson(std::move(value.valueSet));
}

static void documentSymbolClientCapabilitiesTagSupportFromJson(json::Object& json, DocumentSymbolClientCapabilitiesTagSupport& value)
{
	fromJson(std::move(json.get("valueSet")), value.valueSet);
}

static void documentSymbolClientCapabilitiesToJson(DocumentSymbolClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.symbolKind)
		json["symbolKind"] = toJson(std::move(value.symbolKind));
	if(value.hierarchicalDocumentSymbolSupport)
		json["hierarchicalDocumentSymbolSupport"] = toJson(std::move(value.hierarchicalDocumentSymbolSupport));
	if(value.tagSupport)
		json["tagSupport"] = toJson(std::move(value.tagSupport));
	if(value.labelSupport)
		json["labelSupport"] = toJson(std::move(value.labelSupport));
}

static void documentSymbolClientCapabilitiesFromJson(json::Object& json, DocumentSymbolClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("symbolKind"); it != json.end())
		fromJson(std::move(it->second), value.symbolKind);
	if(const auto it = json.find("hierarchicalDocumentSymbolSupport"); it != json.end())
		fromJson(std::move(it->second), value.hierarchicalDocumentSymbolSupport);
	if(const auto it = json.find("tagSupport"); it != json.end())
		fromJson(std::move(it->second), value.tagSupport);
	if(const auto it = json.find("labelSupport"); it != json.end())
		fromJson(std::move(it->second), value.labelSupport);
}

static void codeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKindToJson(CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind& value, json::Object& json)
{
	json["valueSet"] = toJson(std::move(value.valueSet));
}

static void codeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKindFromJson(json::Object& json, CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind& value)
{
	fromJson(std::move(json.get("valueSet")), value.valueSet);
}

static void codeActionClientCapabilitiesCodeActionLiteralSupportToJson(CodeActionClientCapabilitiesCodeActionLiteralSupport& value, json::Object& json)
{
	json["codeActionKind"] = toJson(std::move(value.codeActionKind));
}

static void codeActionClientCapabilitiesCodeActionLiteralSupportFromJson(json::Object& json, CodeActionClientCapabilitiesCodeActionLiteralSupport& value)
{
	fromJson(std::move(json.get("codeActionKind")), value.codeActionKind);
}

static void codeActionClientCapabilitiesResolveSupportToJson(CodeActionClientCapabilitiesResolveSupport& value, json::Object& json)
{
	json["properties"] = toJson(std::move(value.properties));
}

static void codeActionClientCapabilitiesResolveSupportFromJson(json::Object& json, CodeActionClientCapabilitiesResolveSupport& value)
{
	fromJson(std::move(json.get("properties")), value.properties);
}

static void codeActionClientCapabilitiesToJson(CodeActionClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.codeActionLiteralSupport)
		json["codeActionLiteralSupport"] = toJson(std::move(value.codeActionLiteralSupport));
	if(value.isPreferredSupport)
		json["isPreferredSupport"] = toJson(std::move(value.isPreferredSupport));
	if(value.disabledSupport)
		json["disabledSupport"] = toJson(std::move(value.disabledSupport));
	if(value.dataSupport)
		json["dataSupport"] = toJson(std::move(value.dataSupport));
	if(value.resolveSupport)
		json["resolveSupport"] = toJson(std::move(value.resolveSupport));
	if(value.honorsChangeAnnotations)
		json["honorsChangeAnnotations"] = toJson(std::move(value.honorsChangeAnnotations));
}

static void codeActionClientCapabilitiesFromJson(json::Object& json, CodeActionClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("codeActionLiteralSupport"); it != json.end())
		fromJson(std::move(it->second), value.codeActionLiteralSupport);
	if(const auto it = json.find("isPreferredSupport"); it != json.end())
		fromJson(std::move(it->second), value.isPreferredSupport);
	if(const auto it = json.find("disabledSupport"); it != json.end())
		fromJson(std::move(it->second), value.disabledSupport);
	if(const auto it = json.find("dataSupport"); it != json.end())
		fromJson(std::move(it->second), value.dataSupport);
	if(const auto it = json.find("resolveSupport"); it != json.end())
		fromJson(std::move(it->second), value.resolveSupport);
	if(const auto it = json.find("honorsChangeAnnotations"); it != json.end())
		fromJson(std::move(it->second), value.honorsChangeAnnotations);
}

static void codeLensClientCapabilitiesToJson(CodeLensClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void codeLensClientCapabilitiesFromJson(json::Object& json, CodeLensClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void documentLinkClientCapabilitiesToJson(DocumentLinkClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.tooltipSupport)
		json["tooltipSupport"] = toJson(std::move(value.tooltipSupport));
}

static void documentLinkClientCapabilitiesFromJson(json::Object& json, DocumentLinkClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("tooltipSupport"); it != json.end())
		fromJson(std::move(it->second), value.tooltipSupport);
}

static void documentColorClientCapabilitiesToJson(DocumentColorClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void documentColorClientCapabilitiesFromJson(json::Object& json, DocumentColorClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void documentFormattingClientCapabilitiesToJson(DocumentFormattingClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void documentFormattingClientCapabilitiesFromJson(json::Object& json, DocumentFormattingClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void documentRangeFormattingClientCapabilitiesToJson(DocumentRangeFormattingClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.rangesSupport)
		json["rangesSupport"] = toJson(std::move(value.rangesSupport));
}

static void documentRangeFormattingClientCapabilitiesFromJson(json::Object& json, DocumentRangeFormattingClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("rangesSupport"); it != json.end())
		fromJson(std::move(it->second), value.rangesSupport);
}

static void documentOnTypeFormattingClientCapabilitiesToJson(DocumentOnTypeFormattingClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void documentOnTypeFormattingClientCapabilitiesFromJson(json::Object& json, DocumentOnTypeFormattingClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void renameClientCapabilitiesToJson(RenameClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.prepareSupport)
		json["prepareSupport"] = toJson(std::move(value.prepareSupport));
	if(value.prepareSupportDefaultBehavior)
		json["prepareSupportDefaultBehavior"] = toJson(std::move(value.prepareSupportDefaultBehavior));
	if(value.honorsChangeAnnotations)
		json["honorsChangeAnnotations"] = toJson(std::move(value.honorsChangeAnnotations));
}

static void renameClientCapabilitiesFromJson(json::Object& json, RenameClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("prepareSupport"); it != json.end())
		fromJson(std::move(it->second), value.prepareSupport);
	if(const auto it = json.find("prepareSupportDefaultBehavior"); it != json.end())
		fromJson(std::move(it->second), value.prepareSupportDefaultBehavior);
	if(const auto it = json.find("honorsChangeAnnotations"); it != json.end())
		fromJson(std::move(it->second), value.honorsChangeAnnotations);
}

static void foldingRangeClientCapabilitiesFoldingRangeKindToJson(FoldingRangeClientCapabilitiesFoldingRangeKind& value, json::Object& json)
{
	if(value.valueSet)
		json["valueSet"] = toJson(std::move(value.valueSet));
}

static void foldingRangeClientCapabilitiesFoldingRangeKindFromJson(json::Object& json, FoldingRangeClientCapabilitiesFoldingRangeKind& value)
{
	if(const auto it = json.find("valueSet"); it != json.end())
		fromJson(std::move(it->second), value.valueSet);
}

static void foldingRangeClientCapabilitiesFoldingRangeToJson(FoldingRangeClientCapabilitiesFoldingRange& value, json::Object& json)
{
	if(value.collapsedText)
		json["collapsedText"] = toJson(std::move(value.collapsedText));
}

static void foldingRangeClientCapabilitiesFoldingRangeFromJson(json::Object& json, FoldingRangeClientCapabilitiesFoldingRange& value)
{
	if(const auto it = json.find("collapsedText"); it != json.end())
		fromJson(std::move(it->second), value.collapsedText);
}

static void foldingRangeClientCapabilitiesToJson(FoldingRangeClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.rangeLimit)
		json["rangeLimit"] = toJson(std::move(value.rangeLimit));
	if(value.lineFoldingOnly)
		json["lineFoldingOnly"] = toJson(std::move(value.lineFoldingOnly));
	if(value.foldingRangeKind)
		json["foldingRangeKind"] = toJson(std::move(value.foldingRangeKind));
	if(value.foldingRange)
		json["foldingRange"] = toJson(std::move(value.foldingRange));
}

static void foldingRangeClientCapabilitiesFromJson(json::Object& json, FoldingRangeClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("rangeLimit"); it != json.end())
		fromJson(std::move(it->second), value.rangeLimit);
	if(const auto it = json.find("lineFoldingOnly"); it != json.end())
		fromJson(std::move(it->second), value.lineFoldingOnly);
	if(const auto it = json.find("foldingRangeKind"); it != json.end())
		fromJson(std::move(it->second), value.foldingRangeKind);
	if(const auto it = json.find("foldingRange"); it != json.end())
		fromJson(std::move(it->second), value.foldingRange);
}

static void selectionRangeClientCapabilitiesToJson(SelectionRangeClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void selectionRangeClientCapabilitiesFromJson(json::Object& json, SelectionRangeClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void publishDiagnosticsClientCapabilitiesTagSupportToJson(PublishDiagnosticsClientCapabilitiesTagSupport& value, json::Object& json)
{
	json["valueSet"] = toJson(std::move(value.valueSet));
}

static void publishDiagnosticsClientCapabilitiesTagSupportFromJson(json::Object& json, PublishDiagnosticsClientCapabilitiesTagSupport& value)
{
	fromJson(std::move(json.get("valueSet")), value.valueSet);
}

static void publishDiagnosticsClientCapabilitiesToJson(PublishDiagnosticsClientCapabilities& value, json::Object& json)
{
	if(value.relatedInformation)
		json["relatedInformation"] = toJson(std::move(value.relatedInformation));
	if(value.tagSupport)
		json["tagSupport"] = toJson(std::move(value.tagSupport));
	if(value.versionSupport)
		json["versionSupport"] = toJson(std::move(value.versionSupport));
	if(value.codeDescriptionSupport)
		json["codeDescriptionSupport"] = toJson(std::move(value.codeDescriptionSupport));
	if(value.dataSupport)
		json["dataSupport"] = toJson(std::move(value.dataSupport));
}

static void publishDiagnosticsClientCapabilitiesFromJson(json::Object& json, PublishDiagnosticsClientCapabilities& value)
{
	if(const auto it = json.find("relatedInformation"); it != json.end())
		fromJson(std::move(it->second), value.relatedInformation);
	if(const auto it = json.find("tagSupport"); it != json.end())
		fromJson(std::move(it->second), value.tagSupport);
	if(const auto it = json.find("versionSupport"); it != json.end())
		fromJson(std::move(it->second), value.versionSupport);
	if(const auto it = json.find("codeDescriptionSupport"); it != json.end())
		fromJson(std::move(it->second), value.codeDescriptionSupport);
	if(const auto it = json.find("dataSupport"); it != json.end())
		fromJson(std::move(it->second), value.dataSupport);
}

static void callHierarchyClientCapabilitiesToJson(CallHierarchyClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void callHierarchyClientCapabilitiesFromJson(json::Object& json, CallHierarchyClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void semanticTokensClientCapabilitiesRequestsRangeToJson(SemanticTokensClientCapabilitiesRequestsRange& value, json::Object& json)
{
}

static void semanticTokensClientCapabilitiesRequestsRangeFromJson(json::Object& json, SemanticTokensClientCapabilitiesRequestsRange& value)
{
}

static void semanticTokensClientCapabilitiesRequestsFullToJson(SemanticTokensClientCapabilitiesRequestsFull& value, json::Object& json)
{
	if(value.delta)
		json["delta"] = toJson(std::move(value.delta));
}

static void semanticTokensClientCapabilitiesRequestsFullFromJson(json::Object& json, SemanticTokensClientCapabilitiesRequestsFull& value)
{
	if(const auto it = json.find("delta"); it != json.end())
		fromJson(std::move(it->second), value.delta);
}

static void semanticTokensClientCapabilitiesRequestsToJson(SemanticTokensClientCapabilitiesRequests& value, json::Object& json)
{
	if(value.range)
		json["range"] = toJson(std::move(value.range));
	if(value.full)
		json["full"] = toJson(std::move(value.full));
}

static void semanticTokensClientCapabilitiesRequestsFromJson(json::Object& json, SemanticTokensClientCapabilitiesRequests& value)
{
	if(const auto it = json.find("range"); it != json.end())
		fromJson(std::move(it->second), value.range);
	if(const auto it = json.find("full"); it != json.end())
		fromJson(std::move(it->second), value.full);
}

static void semanticTokensClientCapabilitiesToJson(SemanticTokensClientCapabilities& value, json::Object& json)
{
	json["requests"] = toJson(std::move(value.requests));
	json["tokenTypes"] = toJson(std::move(value.tokenTypes));
	json["tokenModifiers"] = toJson(std::move(value.tokenModifiers));
	json["formats"] = toJson(std::move(value.formats));
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.overlappingTokenSupport)
		json["overlappingTokenSupport"] = toJson(std::move(value.overlappingTokenSupport));
	if(value.multilineTokenSupport)
		json["multilineTokenSupport"] = toJson(std::move(value.multilineTokenSupport));
	if(value.serverCancelSupport)
		json["serverCancelSupport"] = toJson(std::move(value.serverCancelSupport));
	if(value.augmentsSyntaxTokens)
		json["augmentsSyntaxTokens"] = toJson(std::move(value.augmentsSyntaxTokens));
}

static void semanticTokensClientCapabilitiesFromJson(json::Object& json, SemanticTokensClientCapabilities& value)
{
	fromJson(std::move(json.get("requests")), value.requests);
	fromJson(std::move(json.get("tokenTypes")), value.tokenTypes);
	fromJson(std::move(json.get("tokenModifiers")), value.tokenModifiers);
	fromJson(std::move(json.get("formats")), value.formats);
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("overlappingTokenSupport"); it != json.end())
		fromJson(std::move(it->second), value.overlappingTokenSupport);
	if(const auto it = json.find("multilineTokenSupport"); it != json.end())
		fromJson(std::move(it->second), value.multilineTokenSupport);
	if(const auto it = json.find("serverCancelSupport"); it != json.end())
		fromJson(std::move(it->second), value.serverCancelSupport);
	if(const auto it = json.find("augmentsSyntaxTokens"); it != json.end())
		fromJson(std::move(it->second), value.augmentsSyntaxTokens);
}

static void linkedEditingRangeClientCapabilitiesToJson(LinkedEditingRangeClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void linkedEditingRangeClientCapabilitiesFromJson(json::Object& json, LinkedEditingRangeClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void monikerClientCapabilitiesToJson(MonikerClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void monikerClientCapabilitiesFromJson(json::Object& json, MonikerClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void typeHierarchyClientCapabilitiesToJson(TypeHierarchyClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void typeHierarchyClientCapabilitiesFromJson(json::Object& json, TypeHierarchyClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void inlineValueClientCapabilitiesToJson(InlineValueClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void inlineValueClientCapabilitiesFromJson(json::Object& json, InlineValueClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void inlayHintClientCapabilitiesResolveSupportToJson(InlayHintClientCapabilitiesResolveSupport& value, json::Object& json)
{
	json["properties"] = toJson(std::move(value.properties));
}

static void inlayHintClientCapabilitiesResolveSupportFromJson(json::Object& json, InlayHintClientCapabilitiesResolveSupport& value)
{
	fromJson(std::move(json.get("properties")), value.properties);
}

static void inlayHintClientCapabilitiesToJson(InlayHintClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.resolveSupport)
		json["resolveSupport"] = toJson(std::move(value.resolveSupport));
}

static void inlayHintClientCapabilitiesFromJson(json::Object& json, InlayHintClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("resolveSupport"); it != json.end())
		fromJson(std::move(it->second), value.resolveSupport);
}

static void diagnosticClientCapabilitiesToJson(DiagnosticClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.relatedDocumentSupport)
		json["relatedDocumentSupport"] = toJson(std::move(value.relatedDocumentSupport));
}

static void diagnosticClientCapabilitiesFromJson(json::Object& json, DiagnosticClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("relatedDocumentSupport"); it != json.end())
		fromJson(std::move(it->second), value.relatedDocumentSupport);
}

static void inlineCompletionClientCapabilitiesToJson(InlineCompletionClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
}

static void inlineCompletionClientCapabilitiesFromJson(json::Object& json, InlineCompletionClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
}

static void textDocumentClientCapabilitiesToJson(TextDocumentClientCapabilities& value, json::Object& json)
{
	if(value.synchronization)
		json["synchronization"] = toJson(std::move(value.synchronization));
	if(value.completion)
		json["completion"] = toJson(std::move(value.completion));
	if(value.hover)
		json["hover"] = toJson(std::move(value.hover));
	if(value.signatureHelp)
		json["signatureHelp"] = toJson(std::move(value.signatureHelp));
	if(value.declaration)
		json["declaration"] = toJson(std::move(value.declaration));
	if(value.definition)
		json["definition"] = toJson(std::move(value.definition));
	if(value.typeDefinition)
		json["typeDefinition"] = toJson(std::move(value.typeDefinition));
	if(value.implementation)
		json["implementation"] = toJson(std::move(value.implementation));
	if(value.references)
		json["references"] = toJson(std::move(value.references));
	if(value.documentHighlight)
		json["documentHighlight"] = toJson(std::move(value.documentHighlight));
	if(value.documentSymbol)
		json["documentSymbol"] = toJson(std::move(value.documentSymbol));
	if(value.codeAction)
		json["codeAction"] = toJson(std::move(value.codeAction));
	if(value.codeLens)
		json["codeLens"] = toJson(std::move(value.codeLens));
	if(value.documentLink)
		json["documentLink"] = toJson(std::move(value.documentLink));
	if(value.colorProvider)
		json["colorProvider"] = toJson(std::move(value.colorProvider));
	if(value.formatting)
		json["formatting"] = toJson(std::move(value.formatting));
	if(value.rangeFormatting)
		json["rangeFormatting"] = toJson(std::move(value.rangeFormatting));
	if(value.onTypeFormatting)
		json["onTypeFormatting"] = toJson(std::move(value.onTypeFormatting));
	if(value.rename)
		json["rename"] = toJson(std::move(value.rename));
	if(value.foldingRange)
		json["foldingRange"] = toJson(std::move(value.foldingRange));
	if(value.selectionRange)
		json["selectionRange"] = toJson(std::move(value.selectionRange));
	if(value.publishDiagnostics)
		json["publishDiagnostics"] = toJson(std::move(value.publishDiagnostics));
	if(value.callHierarchy)
		json["callHierarchy"] = toJson(std::move(value.callHierarchy));
	if(value.semanticTokens)
		json["semanticTokens"] = toJson(std::move(value.semanticTokens));
	if(value.linkedEditingRange)
		json["linkedEditingRange"] = toJson(std::move(value.linkedEditingRange));
	if(value.moniker)
		json["moniker"] = toJson(std::move(value.moniker));
	if(value.typeHierarchy)
		json["typeHierarchy"] = toJson(std::move(value.typeHierarchy));
	if(value.inlineValue)
		json["inlineValue"] = toJson(std::move(value.inlineValue));
	if(value.inlayHint)
		json["inlayHint"] = toJson(std::move(value.inlayHint));
	if(value.diagnostic)
		json["diagnostic"] = toJson(std::move(value.diagnostic));
	if(value.inlineCompletion)
		json["inlineCompletion"] = toJson(std::move(value.inlineCompletion));
}

static void textDocumentClientCapabilitiesFromJson(json::Object& json, TextDocumentClientCapabilities& value)
{
	if(const auto it = json.find("synchronization"); it != json.end())
		fromJson(std::move(it->second), value.synchronization);
	if(const auto it = json.find("completion"); it != json.end())
		fromJson(std::move(it->second), value.completion);
	if(const auto it = json.find("hover"); it != json.end())
		fromJson(std::move(it->second), value.hover);
	if(const auto it = json.find("signatureHelp"); it != json.end())
		fromJson(std::move(it->second), value.signatureHelp);
	if(const auto it = json.find("declaration"); it != json.end())
		fromJson(std::move(it->second), value.declaration);
	if(const auto it = json.find("definition"); it != json.end())
		fromJson(std::move(it->second), value.definition);
	if(const auto it = json.find("typeDefinition"); it != json.end())
		fromJson(std::move(it->second), value.typeDefinition);
	if(const auto it = json.find("implementation"); it != json.end())
		fromJson(std::move(it->second), value.implementation);
	if(const auto it = json.find("references"); it != json.end())
		fromJson(std::move(it->second), value.references);
	if(const auto it = json.find("documentHighlight"); it != json.end())
		fromJson(std::move(it->second), value.documentHighlight);
	if(const auto it = json.find("documentSymbol"); it != json.end())
		fromJson(std::move(it->second), value.documentSymbol);
	if(const auto it = json.find("codeAction"); it != json.end())
		fromJson(std::move(it->second), value.codeAction);
	if(const auto it = json.find("codeLens"); it != json.end())
		fromJson(std::move(it->second), value.codeLens);
	if(const auto it = json.find("documentLink"); it != json.end())
		fromJson(std::move(it->second), value.documentLink);
	if(const auto it = json.find("colorProvider"); it != json.end())
		fromJson(std::move(it->second), value.colorProvider);
	if(const auto it = json.find("formatting"); it != json.end())
		fromJson(std::move(it->second), value.formatting);
	if(const auto it = json.find("rangeFormatting"); it != json.end())
		fromJson(std::move(it->second), value.rangeFormatting);
	if(const auto it = json.find("onTypeFormatting"); it != json.end())
		fromJson(std::move(it->second), value.onTypeFormatting);
	if(const auto it = json.find("rename"); it != json.end())
		fromJson(std::move(it->second), value.rename);
	if(const auto it = json.find("foldingRange"); it != json.end())
		fromJson(std::move(it->second), value.foldingRange);
	if(const auto it = json.find("selectionRange"); it != json.end())
		fromJson(std::move(it->second), value.selectionRange);
	if(const auto it = json.find("publishDiagnostics"); it != json.end())
		fromJson(std::move(it->second), value.publishDiagnostics);
	if(const auto it = json.find("callHierarchy"); it != json.end())
		fromJson(std::move(it->second), value.callHierarchy);
	if(const auto it = json.find("semanticTokens"); it != json.end())
		fromJson(std::move(it->second), value.semanticTokens);
	if(const auto it = json.find("linkedEditingRange"); it != json.end())
		fromJson(std::move(it->second), value.linkedEditingRange);
	if(const auto it = json.find("moniker"); it != json.end())
		fromJson(std::move(it->second), value.moniker);
	if(const auto it = json.find("typeHierarchy"); it != json.end())
		fromJson(std::move(it->second), value.typeHierarchy);
	if(const auto it = json.find("inlineValue"); it != json.end())
		fromJson(std::move(it->second), value.inlineValue);
	if(const auto it = json.find("inlayHint"); it != json.end())
		fromJson(std::move(it->second), value.inlayHint);
	if(const auto it = json.find("diagnostic"); it != json.end())
		fromJson(std::move(it->second), value.diagnostic);
	if(const auto it = json.find("inlineCompletion"); it != json.end())
		fromJson(std::move(it->second), value.inlineCompletion);
}

static void notebookDocumentSyncClientCapabilitiesToJson(NotebookDocumentSyncClientCapabilities& value, json::Object& json)
{
	if(value.dynamicRegistration)
		json["dynamicRegistration"] = toJson(std::move(value.dynamicRegistration));
	if(value.executionSummarySupport)
		json["executionSummarySupport"] = toJson(std::move(value.executionSummarySupport));
}

static void notebookDocumentSyncClientCapabilitiesFromJson(json::Object& json, NotebookDocumentSyncClientCapabilities& value)
{
	if(const auto it = json.find("dynamicRegistration"); it != json.end())
		fromJson(std::move(it->second), value.dynamicRegistration);
	if(const auto it = json.find("executionSummarySupport"); it != json.end())
		fromJson(std::move(it->second), value.executionSummarySupport);
}

static void notebookDocumentClientCapabilitiesToJson(NotebookDocumentClientCapabilities& value, json::Object& json)
{
	json["synchronization"] = toJson(std::move(value.synchronization));
}

static void notebookDocumentClientCapabilitiesFromJson(json::Object& json, NotebookDocumentClientCapabilities& value)
{
	fromJson(std::move(json.get("synchronization")), value.synchronization);
}

static void showMessageRequestClientCapabilitiesMessageActionItemToJson(ShowMessageRequestClientCapabilitiesMessageActionItem& value, json::Object& json)
{
	if(value.additionalPropertiesSupport)
		json["additionalPropertiesSupport"] = toJson(std::move(value.additionalPropertiesSupport));
}

static void showMessageRequestClientCapabilitiesMessageActionItemFromJson(json::Object& json, ShowMessageRequestClientCapabilitiesMessageActionItem& value)
{
	if(const auto it = json.find("additionalPropertiesSupport"); it != json.end())
		fromJson(std::move(it->second), value.additionalPropertiesSupport);
}

static void showMessageRequestClientCapabilitiesToJson(ShowMessageRequestClientCapabilities& value, json::Object& json)
{
	if(value.messageActionItem)
		json["messageActionItem"] = toJson(std::move(value.messageActionItem));
}

static void showMessageRequestClientCapabilitiesFromJson(json::Object& json, ShowMessageRequestClientCapabilities& value)
{
	if(const auto it = json.find("messageActionItem"); it != json.end())
		fromJson(std::move(it->second), value.messageActionItem);
}

static void showDocumentClientCapabilitiesToJson(ShowDocumentClientCapabilities& value, json::Object& json)
{
	json["support"] = toJson(value.support);
}

static void showDocumentClientCapabilitiesFromJson(json::Object& json, ShowDocumentClientCapabilities& value)
{
	fromJson(std::move(json.get("support")), value.support);
}

static void windowClientCapabilitiesToJson(WindowClientCapabilities& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.showMessage)
		json["showMessage"] = toJson(std::move(value.showMessage));
	if(value.showDocument)
		json["showDocument"] = toJson(std::move(value.showDocument));
}

static void windowClientCapabilitiesFromJson(json::Object& json, WindowClientCapabilities& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("showMessage"); it != json.end())
		fromJson(std::move(it->second), value.showMessage);
	if(const auto it = json.find("showDocument"); it != json.end())
		fromJson(std::move(it->second), value.showDocument);
}

static void generalClientCapabilitiesStaleRequestSupportToJson(GeneralClientCapabilitiesStaleRequestSupport& value, json::Object& json)
{
	json["cancel"] = toJson(value.cancel);
	json["retryOnContentModified"] = toJson(std::move(value.retryOnContentModified));
}

static void generalClientCapabilitiesStaleRequestSupportFromJson(json::Object& json, GeneralClientCapabilitiesStaleRequestSupport& value)
{
	fromJson(std::move(json.get("cancel")), value.cancel);
	fromJson(std::move(json.get("retryOnContentModified")), value.retryOnContentModified);
}

static void regularExpressionsClientCapabilitiesToJson(RegularExpressionsClientCapabilities& value, json::Object& json)
{
	json["engine"] = toJson(std::move(value.engine));
	if(value.version)
		json["version"] = toJson(std::move(value.version));
}

static void regularExpressionsClientCapabilitiesFromJson(json::Object& json, RegularExpressionsClientCapabilities& value)
{
	fromJson(std::move(json.get("engine")), value.engine);
	if(const auto it = json.find("version"); it != json.end())
		fromJson(std::move(it->second), value.version);
}

static void markdownClientCapabilitiesToJson(MarkdownClientCapabilities& value, json::Object& json)
{
	json["parser"] = toJson(std::move(value.parser));
	if(value.version)
		json["version"] = toJson(std::move(value.version));
	if(value.allowedTags)
		json["allowedTags"] = toJson(std::move(value.allowedTags));
}

static void markdownClientCapabilitiesFromJson(json::Object& json, MarkdownClientCapabilities& value)
{
	fromJson(std::move(json.get("parser")), value.parser);
	if(const auto it = json.find("version"); it != json.end())
		fromJson(std::move(it->second), value.version);
	if(const auto it = json.find("allowedTags"); it != json.end())
		fromJson(std::move(it->second), value.allowedTags);
}

static void generalClientCapabilitiesToJson(GeneralClientCapabilities& value, json::Object& json)
{
	if(value.staleRequestSupport)
		json["staleRequestSupport"] = toJson(std::move(value.staleRequestSupport));
	if(value.regularExpressions)
		json["regularExpressions"] = toJson(std::move(value.regularExpressions));
	if(value.markdown)
		json["markdown"] = toJson(std::move(value.markdown));
	if(value.positionEncodings)
		json["positionEncodings"] = toJson(std::move(value.positionEncodings));
}

static void generalClientCapabilitiesFromJson(json::Object& json, GeneralClientCapabilities& value)
{
	if(const auto it = json.find("staleRequestSupport"); it != json.end())
		fromJson(std::move(it->second), value.staleRequestSupport);
	if(const auto it = json.find("regularExpressions"); it != json.end())
		fromJson(std::move(it->second), value.regularExpressions);
	if(const auto it = json.find("markdown"); it != json.end())
		fromJson(std::move(it->second), value.markdown);
	if(const auto it = json.find("positionEncodings"); it != json.end())
		fromJson(std::move(it->second), value.positionEncodings);
}

static void clientCapabilitiesToJson(ClientCapabilities& value, json::Object& json)
{
	if(value.workspace)
		json["workspace"] = toJson(std::move(value.workspace));
	if(value.textDocument)
		json["textDocument"] = toJson(std::move(value.textDocument));
	if(value.notebookDocument)
		json["notebookDocument"] = toJson(std::move(value.notebookDocument));
	if(value.window)
		json["window"] = toJson(std::move(value.window));
	if(value.general)
		json["general"] = toJson(std::move(value.general));
	if(value.experimental)
		json["experimental"] = toJson(std::move(value.experimental));
}

static void clientCapabilitiesFromJson(json::Object& json, ClientCapabilities& value)
{
	if(const auto it = json.find("workspace"); it != json.end())
		fromJson(std::move(it->second), value.workspace);
	if(const auto it = json.find("textDocument"); it != json.end())
		fromJson(std::move(it->second), value.textDocument);
	if(const auto it = json.find("notebookDocument"); it != json.end())
		fromJson(std::move(it->second), value.notebookDocument);
	if(const auto it = json.find("window"); it != json.end())
		fromJson(std::move(it->second), value.window);
	if(const auto it = json.find("general"); it != json.end())
		fromJson(std::move(it->second), value.general);
	if(const auto it = json.find("experimental"); it != json.end())
		fromJson(std::move(it->second), value.experimental);
}

static void _InitializeParamsClientInfoToJson(_InitializeParamsClientInfo& value, json::Object& json)
{
	json["name"] = toJson(std::move(value.name));
	if(value.version)
		json["version"] = toJson(std::move(value.version));
}

static void _InitializeParamsClientInfoFromJson(json::Object& json, _InitializeParamsClientInfo& value)
{
	fromJson(std::move(json.get("name")), value.name);
	if(const auto it = json.find("version"); it != json.end())
		fromJson(std::move(it->second), value.version);
}

static void _InitializeParamsToJson(_InitializeParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["processId"] = toJson(std::move(value.processId));
	json["rootUri"] = toJson(std::move(value.rootUri));
	json["capabilities"] = toJson(std::move(value.capabilities));
	if(value.clientInfo)
		json["clientInfo"] = toJson(std::move(value.clientInfo));
	if(value.locale)
		json["locale"] = toJson(std::move(value.locale));
	if(value.rootPath)
		json["rootPath"] = toJson(std::move(value.rootPath));
	if(value.initializationOptions)
		json["initializationOptions"] = toJson(std::move(value.initializationOptions));
	if(value.trace)
		json["trace"] = toJson(std::move(value.trace));
}

static void _InitializeParamsFromJson(json::Object& json, _InitializeParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("processId")), value.processId);
	fromJson(std::move(json.get("rootUri")), value.rootUri);
	fromJson(std::move(json.get("capabilities")), value.capabilities);
	if(const auto it = json.find("clientInfo"); it != json.end())
		fromJson(std::move(it->second), value.clientInfo);
	if(const auto it = json.find("locale"); it != json.end())
		fromJson(std::move(it->second), value.locale);
	if(const auto it = json.find("rootPath"); it != json.end())
		fromJson(std::move(it->second), value.rootPath);
	if(const auto it = json.find("initializationOptions"); it != json.end())
		fromJson(std::move(it->second), value.initializationOptions);
	if(const auto it = json.find("trace"); it != json.end())
		fromJson(std::move(it->second), value.trace);
}

static void workspaceFoldersInitializeParamsToJson(WorkspaceFoldersInitializeParams& value, json::Object& json)
{
	if(value.workspaceFolders)
		json["workspaceFolders"] = toJson(std::move(value.workspaceFolders));
}

static void workspaceFoldersInitializeParamsFromJson(json::Object& json, WorkspaceFoldersInitializeParams& value)
{
	if(const auto it = json.find("workspaceFolders"); it != json.end())
		fromJson(std::move(it->second), value.workspaceFolders);
}

static void initializeParamsToJson(InitializeParams& value, json::Object& json)
{
	_InitializeParamsToJson(value, json);
	workspaceFoldersInitializeParamsToJson(value, json);
}

static void initializeParamsFromJson(json::Object& json, InitializeParams& value)
{
	_InitializeParamsFromJson(json, value);
	workspaceFoldersInitializeParamsFromJson(json, value);
}

static void saveOptionsToJson(SaveOptions& value, json::Object& json)
{
	if(value.includeText)
		json["includeText"] = toJson(std::move(value.includeText));
}

static void saveOptionsFromJson(json::Object& json, SaveOptions& value)
{
	if(const auto it = json.find("includeText"); it != json.end())
		fromJson(std::move(it->second), value.includeText);
}

static void textDocumentSyncOptionsToJson(TextDocumentSyncOptions& value, json::Object& json)
{
	if(value.openClose)
		json["openClose"] = toJson(std::move(value.openClose));
	if(value.change)
		json["change"] = toJson(std::move(value.change));
	if(value.willSave)
		json["willSave"] = toJson(std::move(value.willSave));
	if(value.willSaveWaitUntil)
		json["willSaveWaitUntil"] = toJson(std::move(value.willSaveWaitUntil));
	if(value.save)
		json["save"] = toJson(std::move(value.save));
}

static void textDocumentSyncOptionsFromJson(json::Object& json, TextDocumentSyncOptions& value)
{
	if(const auto it = json.find("openClose"); it != json.end())
		fromJson(std::move(it->second), value.openClose);
	if(const auto it = json.find("change"); it != json.end())
		fromJson(std::move(it->second), value.change);
	if(const auto it = json.find("willSave"); it != json.end())
		fromJson(std::move(it->second), value.willSave);
	if(const auto it = json.find("willSaveWaitUntil"); it != json.end())
		fromJson(std::move(it->second), value.willSaveWaitUntil);
	if(const auto it = json.find("save"); it != json.end())
		fromJson(std::move(it->second), value.save);
}

static void notebookDocumentSyncOptionsNotebookSelector_NotebookCellsToJson(NotebookDocumentSyncOptionsNotebookSelector_NotebookCells& value, json::Object& json)
{
	json["language"] = toJson(std::move(value.language));
}

static void notebookDocumentSyncOptionsNotebookSelector_NotebookCellsFromJson(json::Object& json, NotebookDocumentSyncOptionsNotebookSelector_NotebookCells& value)
{
	fromJson(std::move(json.get("language")), value.language);
}

static void notebookDocumentSyncOptionsNotebookSelector_NotebookToJson(NotebookDocumentSyncOptionsNotebookSelector_Notebook& value, json::Object& json)
{
	json["notebook"] = toJson(std::move(value.notebook));
	if(value.cells)
		json["cells"] = toJson(std::move(value.cells));
}

static void notebookDocumentSyncOptionsNotebookSelector_NotebookFromJson(json::Object& json, NotebookDocumentSyncOptionsNotebookSelector_Notebook& value)
{
	fromJson(std::move(json.get("notebook")), value.notebook);
	if(const auto it = json.find("cells"); it != json.end())
		fromJson(std::move(it->second), value.cells);
}

static void notebookDocumentSyncOptionsNotebookSelector_CellsCellsToJson(NotebookDocumentSyncOptionsNotebookSelector_CellsCells& value, json::Object& json)
{
	json["language"] = toJson(std::move(value.language));
}

static void notebookDocumentSyncOptionsNotebookSelector_CellsCellsFromJson(json::Object& json, NotebookDocumentSyncOptionsNotebookSelector_CellsCells& value)
{
	fromJson(std::move(json.get("language")), value.language);
}

static void notebookDocumentSyncOptionsNotebookSelector_CellsToJson(NotebookDocumentSyncOptionsNotebookSelector_Cells& value, json::Object& json)
{
	json["cells"] = toJson(std::move(value.cells));
	if(value.notebook)
		json["notebook"] = toJson(std::move(value.notebook));
}

static void notebookDocumentSyncOptionsNotebookSelector_CellsFromJson(json::Object& json, NotebookDocumentSyncOptionsNotebookSelector_Cells& value)
{
	fromJson(std::move(json.get("cells")), value.cells);
	if(const auto it = json.find("notebook"); it != json.end())
		fromJson(std::move(it->second), value.notebook);
}

static void notebookDocumentSyncOptionsToJson(NotebookDocumentSyncOptions& value, json::Object& json)
{
	json["notebookSelector"] = toJson(std::move(value.notebookSelector));
	if(value.save)
		json["save"] = toJson(std::move(value.save));
}

static void notebookDocumentSyncOptionsFromJson(json::Object& json, NotebookDocumentSyncOptions& value)
{
	fromJson(std::move(json.get("notebookSelector")), value.notebookSelector);
	if(const auto it = json.find("save"); it != json.end())
		fromJson(std::move(it->second), value.save);
}

static void notebookDocumentSyncRegistrationOptionsToJson(NotebookDocumentSyncRegistrationOptions& value, json::Object& json)
{
	notebookDocumentSyncOptionsToJson(value, json);
	if(value.id)
		json["id"] = toJson(std::move(value.id));
}

static void notebookDocumentSyncRegistrationOptionsFromJson(json::Object& json, NotebookDocumentSyncRegistrationOptions& value)
{
	notebookDocumentSyncOptionsFromJson(json, value);
	if(const auto it = json.find("id"); it != json.end())
		fromJson(std::move(it->second), value.id);
}

static void completionOptionsCompletionItemToJson(CompletionOptionsCompletionItem& value, json::Object& json)
{
	if(value.labelDetailsSupport)
		json["labelDetailsSupport"] = toJson(std::move(value.labelDetailsSupport));
}

static void completionOptionsCompletionItemFromJson(json::Object& json, CompletionOptionsCompletionItem& value)
{
	if(const auto it = json.find("labelDetailsSupport"); it != json.end())
		fromJson(std::move(it->second), value.labelDetailsSupport);
}

static void completionOptionsToJson(CompletionOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.triggerCharacters)
		json["triggerCharacters"] = toJson(std::move(value.triggerCharacters));
	if(value.allCommitCharacters)
		json["allCommitCharacters"] = toJson(std::move(value.allCommitCharacters));
	if(value.resolveProvider)
		json["resolveProvider"] = toJson(std::move(value.resolveProvider));
	if(value.completionItem)
		json["completionItem"] = toJson(std::move(value.completionItem));
}

static void completionOptionsFromJson(json::Object& json, CompletionOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("triggerCharacters"); it != json.end())
		fromJson(std::move(it->second), value.triggerCharacters);
	if(const auto it = json.find("allCommitCharacters"); it != json.end())
		fromJson(std::move(it->second), value.allCommitCharacters);
	if(const auto it = json.find("resolveProvider"); it != json.end())
		fromJson(std::move(it->second), value.resolveProvider);
	if(const auto it = json.find("completionItem"); it != json.end())
		fromJson(std::move(it->second), value.completionItem);
}

static void hoverOptionsToJson(HoverOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void hoverOptionsFromJson(json::Object& json, HoverOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void signatureHelpOptionsToJson(SignatureHelpOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.triggerCharacters)
		json["triggerCharacters"] = toJson(std::move(value.triggerCharacters));
	if(value.retriggerCharacters)
		json["retriggerCharacters"] = toJson(std::move(value.retriggerCharacters));
}

static void signatureHelpOptionsFromJson(json::Object& json, SignatureHelpOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("triggerCharacters"); it != json.end())
		fromJson(std::move(it->second), value.triggerCharacters);
	if(const auto it = json.find("retriggerCharacters"); it != json.end())
		fromJson(std::move(it->second), value.retriggerCharacters);
}

static void definitionOptionsToJson(DefinitionOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void definitionOptionsFromJson(json::Object& json, DefinitionOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void referenceOptionsToJson(ReferenceOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void referenceOptionsFromJson(json::Object& json, ReferenceOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void documentHighlightOptionsToJson(DocumentHighlightOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void documentHighlightOptionsFromJson(json::Object& json, DocumentHighlightOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void documentSymbolOptionsToJson(DocumentSymbolOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.label)
		json["label"] = toJson(std::move(value.label));
}

static void documentSymbolOptionsFromJson(json::Object& json, DocumentSymbolOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("label"); it != json.end())
		fromJson(std::move(it->second), value.label);
}

static void codeActionOptionsToJson(CodeActionOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.codeActionKinds)
		json["codeActionKinds"] = toJson(std::move(value.codeActionKinds));
	if(value.resolveProvider)
		json["resolveProvider"] = toJson(std::move(value.resolveProvider));
}

static void codeActionOptionsFromJson(json::Object& json, CodeActionOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("codeActionKinds"); it != json.end())
		fromJson(std::move(it->second), value.codeActionKinds);
	if(const auto it = json.find("resolveProvider"); it != json.end())
		fromJson(std::move(it->second), value.resolveProvider);
}

static void codeLensOptionsToJson(CodeLensOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.resolveProvider)
		json["resolveProvider"] = toJson(std::move(value.resolveProvider));
}

static void codeLensOptionsFromJson(json::Object& json, CodeLensOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("resolveProvider"); it != json.end())
		fromJson(std::move(it->second), value.resolveProvider);
}

static void documentLinkOptionsToJson(DocumentLinkOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.resolveProvider)
		json["resolveProvider"] = toJson(std::move(value.resolveProvider));
}

static void documentLinkOptionsFromJson(json::Object& json, DocumentLinkOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("resolveProvider"); it != json.end())
		fromJson(std::move(it->second), value.resolveProvider);
}

static void workspaceSymbolOptionsToJson(WorkspaceSymbolOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.resolveProvider)
		json["resolveProvider"] = toJson(std::move(value.resolveProvider));
}

static void workspaceSymbolOptionsFromJson(json::Object& json, WorkspaceSymbolOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("resolveProvider"); it != json.end())
		fromJson(std::move(it->second), value.resolveProvider);
}

static void documentFormattingOptionsToJson(DocumentFormattingOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
}

static void documentFormattingOptionsFromJson(json::Object& json, DocumentFormattingOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
}

static void documentRangeFormattingOptionsToJson(DocumentRangeFormattingOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.rangesSupport)
		json["rangesSupport"] = toJson(std::move(value.rangesSupport));
}

static void documentRangeFormattingOptionsFromJson(json::Object& json, DocumentRangeFormattingOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("rangesSupport"); it != json.end())
		fromJson(std::move(it->second), value.rangesSupport);
}

static void documentOnTypeFormattingOptionsToJson(DocumentOnTypeFormattingOptions& value, json::Object& json)
{
	json["firstTriggerCharacter"] = toJson(std::move(value.firstTriggerCharacter));
	if(value.moreTriggerCharacter)
		json["moreTriggerCharacter"] = toJson(std::move(value.moreTriggerCharacter));
}

static void documentOnTypeFormattingOptionsFromJson(json::Object& json, DocumentOnTypeFormattingOptions& value)
{
	fromJson(std::move(json.get("firstTriggerCharacter")), value.firstTriggerCharacter);
	if(const auto it = json.find("moreTriggerCharacter"); it != json.end())
		fromJson(std::move(it->second), value.moreTriggerCharacter);
}

static void renameOptionsToJson(RenameOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	if(value.prepareProvider)
		json["prepareProvider"] = toJson(std::move(value.prepareProvider));
}

static void renameOptionsFromJson(json::Object& json, RenameOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	if(const auto it = json.find("prepareProvider"); it != json.end())
		fromJson(std::move(it->second), value.prepareProvider);
}

static void executeCommandOptionsToJson(ExecuteCommandOptions& value, json::Object& json)
{
	if(value.workDoneProgress)
		json["workDoneProgress"] = toJson(std::move(value.workDoneProgress));
	json["commands"] = toJson(std::move(value.commands));
}

static void executeCommandOptionsFromJson(json::Object& json, ExecuteCommandOptions& value)
{
	if(const auto it = json.find("workDoneProgress"); it != json.end())
		fromJson(std::move(it->second), value.workDoneProgress);
	fromJson(std::move(json.get("commands")), value.commands);
}

static void workspaceFoldersServerCapabilitiesToJson(WorkspaceFoldersServerCapabilities& value, json::Object& json)
{
	if(value.supported)
		json["supported"] = toJson(std::move(value.supported));
	if(value.changeNotifications)
		json["changeNotifications"] = toJson(std::move(value.changeNotifications));
}

static void workspaceFoldersServerCapabilitiesFromJson(json::Object& json, WorkspaceFoldersServerCapabilities& value)
{
	if(const auto it = json.find("supported"); it != json.end())
		fromJson(std::move(it->second), value.supported);
	if(const auto it = json.find("changeNotifications"); it != json.end())
		fromJson(std::move(it->second), value.changeNotifications);
}

static void fileOperationOptionsToJson(FileOperationOptions& value, json::Object& json)
{
	if(value.didCreate)
		json["didCreate"] = toJson(std::move(value.didCreate));
	if(value.willCreate)
		json["willCreate"] = toJson(std::move(value.willCreate));
	if(value.didRename)
		json["didRename"] = toJson(std::move(value.didRename));
	if(value.willRename)
		json["willRename"] = toJson(std::move(value.willRename));
	if(value.didDelete)
		json["didDelete"] = toJson(std::move(value.didDelete));
	if(value.willDelete)
		json["willDelete"] = toJson(std::move(value.willDelete));
}

static void fileOperationOptionsFromJson(json::Object& json, FileOperationOptions& value)
{
	if(const auto it = json.find("didCreate"); it != json.end())
		fromJson(std::move(it->second), value.didCreate);
	if(const auto it = json.find("willCreate"); it != json.end())
		fromJson(std::move(it->second), value.willCreate);
	if(const auto it = json.find("didRename"); it != json.end())
		fromJson(std::move(it->second), value.didRename);
	if(const auto it = json.find("willRename"); it != json.end())
		fromJson(std::move(it->second), value.willRename);
	if(const auto it = json.find("didDelete"); it != json.end())
		fromJson(std::move(it->second), value.didDelete);
	if(const auto it = json.find("willDelete"); it != json.end())
		fromJson(std::move(it->second), value.willDelete);
}

static void serverCapabilitiesWorkspaceToJson(ServerCapabilitiesWorkspace& value, json::Object& json)
{
	if(value.workspaceFolders)
		json["workspaceFolders"] = toJson(std::move(value.workspaceFolders));
	if(value.fileOperations)
		json["fileOperations"] = toJson(std::move(value.fileOperations));
}

static void serverCapabilitiesWorkspaceFromJson(json::Object& json, ServerCapabilitiesWorkspace& value)
{
	if(const auto it = json.find("workspaceFolders"); it != json.end())
		fromJson(std::move(it->second), value.workspaceFolders);
	if(const auto it = json.find("fileOperations"); it != json.end())
		fromJson(std::move(it->second), value.fileOperations);
}

static void serverCapabilitiesToJson(ServerCapabilities& value, json::Object& json)
{
	if(value.positionEncoding)
		json["positionEncoding"] = toJson(std::move(value.positionEncoding));
	if(value.textDocumentSync)
		json["textDocumentSync"] = toJson(std::move(value.textDocumentSync));
	if(value.notebookDocumentSync)
		json["notebookDocumentSync"] = toJson(std::move(value.notebookDocumentSync));
	if(value.completionProvider)
		json["completionProvider"] = toJson(std::move(value.completionProvider));
	if(value.hoverProvider)
		json["hoverProvider"] = toJson(std::move(value.hoverProvider));
	if(value.signatureHelpProvider)
		json["signatureHelpProvider"] = toJson(std::move(value.signatureHelpProvider));
	if(value.declarationProvider)
		json["declarationProvider"] = toJson(std::move(value.declarationProvider));
	if(value.definitionProvider)
		json["definitionProvider"] = toJson(std::move(value.definitionProvider));
	if(value.typeDefinitionProvider)
		json["typeDefinitionProvider"] = toJson(std::move(value.typeDefinitionProvider));
	if(value.implementationProvider)
		json["implementationProvider"] = toJson(std::move(value.implementationProvider));
	if(value.referencesProvider)
		json["referencesProvider"] = toJson(std::move(value.referencesProvider));
	if(value.documentHighlightProvider)
		json["documentHighlightProvider"] = toJson(std::move(value.documentHighlightProvider));
	if(value.documentSymbolProvider)
		json["documentSymbolProvider"] = toJson(std::move(value.documentSymbolProvider));
	if(value.codeActionProvider)
		json["codeActionProvider"] = toJson(std::move(value.codeActionProvider));
	if(value.codeLensProvider)
		json["codeLensProvider"] = toJson(std::move(value.codeLensProvider));
	if(value.documentLinkProvider)
		json["documentLinkProvider"] = toJson(std::move(value.documentLinkProvider));
	if(value.colorProvider)
		json["colorProvider"] = toJson(std::move(value.colorProvider));
	if(value.workspaceSymbolProvider)
		json["workspaceSymbolProvider"] = toJson(std::move(value.workspaceSymbolProvider));
	if(value.documentFormattingProvider)
		json["documentFormattingProvider"] = toJson(std::move(value.documentFormattingProvider));
	if(value.documentRangeFormattingProvider)
		json["documentRangeFormattingProvider"] = toJson(std::move(value.documentRangeFormattingProvider));
	if(value.documentOnTypeFormattingProvider)
		json["documentOnTypeFormattingProvider"] = toJson(std::move(value.documentOnTypeFormattingProvider));
	if(value.renameProvider)
		json["renameProvider"] = toJson(std::move(value.renameProvider));
	if(value.foldingRangeProvider)
		json["foldingRangeProvider"] = toJson(std::move(value.foldingRangeProvider));
	if(value.selectionRangeProvider)
		json["selectionRangeProvider"] = toJson(std::move(value.selectionRangeProvider));
	if(value.executeCommandProvider)
		json["executeCommandProvider"] = toJson(std::move(value.executeCommandProvider));
	if(value.callHierarchyProvider)
		json["callHierarchyProvider"] = toJson(std::move(value.callHierarchyProvider));
	if(value.linkedEditingRangeProvider)
		json["linkedEditingRangeProvider"] = toJson(std::move(value.linkedEditingRangeProvider));
	if(value.semanticTokensProvider)
		json["semanticTokensProvider"] = toJson(std::move(value.semanticTokensProvider));
	if(value.monikerProvider)
		json["monikerProvider"] = toJson(std::move(value.monikerProvider));
	if(value.typeHierarchyProvider)
		json["typeHierarchyProvider"] = toJson(std::move(value.typeHierarchyProvider));
	if(value.inlineValueProvider)
		json["inlineValueProvider"] = toJson(std::move(value.inlineValueProvider));
	if(value.inlayHintProvider)
		json["inlayHintProvider"] = toJson(std::move(value.inlayHintProvider));
	if(value.diagnosticProvider)
		json["diagnosticProvider"] = toJson(std::move(value.diagnosticProvider));
	if(value.inlineCompletionProvider)
		json["inlineCompletionProvider"] = toJson(std::move(value.inlineCompletionProvider));
	if(value.workspace)
		json["workspace"] = toJson(std::move(value.workspace));
	if(value.experimental)
		json["experimental"] = toJson(std::move(value.experimental));
}

static void serverCapabilitiesFromJson(json::Object& json, ServerCapabilities& value)
{
	if(const auto it = json.find("positionEncoding"); it != json.end())
		fromJson(std::move(it->second), value.positionEncoding);
	if(const auto it = json.find("textDocumentSync"); it != json.end())
		fromJson(std::move(it->second), value.textDocumentSync);
	if(const auto it = json.find("notebookDocumentSync"); it != json.end())
		fromJson(std::move(it->second), value.notebookDocumentSync);
	if(const auto it = json.find("completionProvider"); it != json.end())
		fromJson(std::move(it->second), value.completionProvider);
	if(const auto it = json.find("hoverProvider"); it != json.end())
		fromJson(std::move(it->second), value.hoverProvider);
	if(const auto it = json.find("signatureHelpProvider"); it != json.end())
		fromJson(std::move(it->second), value.signatureHelpProvider);
	if(const auto it = json.find("declarationProvider"); it != json.end())
		fromJson(std::move(it->second), value.declarationProvider);
	if(const auto it = json.find("definitionProvider"); it != json.end())
		fromJson(std::move(it->second), value.definitionProvider);
	if(const auto it = json.find("typeDefinitionProvider"); it != json.end())
		fromJson(std::move(it->second), value.typeDefinitionProvider);
	if(const auto it = json.find("implementationProvider"); it != json.end())
		fromJson(std::move(it->second), value.implementationProvider);
	if(const auto it = json.find("referencesProvider"); it != json.end())
		fromJson(std::move(it->second), value.referencesProvider);
	if(const auto it = json.find("documentHighlightProvider"); it != json.end())
		fromJson(std::move(it->second), value.documentHighlightProvider);
	if(const auto it = json.find("documentSymbolProvider"); it != json.end())
		fromJson(std::move(it->second), value.documentSymbolProvider);
	if(const auto it = json.find("codeActionProvider"); it != json.end())
		fromJson(std::move(it->second), value.codeActionProvider);
	if(const auto it = json.find("codeLensProvider"); it != json.end())
		fromJson(std::move(it->second), value.codeLensProvider);
	if(const auto it = json.find("documentLinkProvider"); it != json.end())
		fromJson(std::move(it->second), value.documentLinkProvider);
	if(const auto it = json.find("colorProvider"); it != json.end())
		fromJson(std::move(it->second), value.colorProvider);
	if(const auto it = json.find("workspaceSymbolProvider"); it != json.end())
		fromJson(std::move(it->second), value.workspaceSymbolProvider);
	if(const auto it = json.find("documentFormattingProvider"); it != json.end())
		fromJson(std::move(it->second), value.documentFormattingProvider);
	if(const auto it = json.find("documentRangeFormattingProvider"); it != json.end())
		fromJson(std::move(it->second), value.documentRangeFormattingProvider);
	if(const auto it = json.find("documentOnTypeFormattingProvider"); it != json.end())
		fromJson(std::move(it->second), value.documentOnTypeFormattingProvider);
	if(const auto it = json.find("renameProvider"); it != json.end())
		fromJson(std::move(it->second), value.renameProvider);
	if(const auto it = json.find("foldingRangeProvider"); it != json.end())
		fromJson(std::move(it->second), value.foldingRangeProvider);
	if(const auto it = json.find("selectionRangeProvider"); it != json.end())
		fromJson(std::move(it->second), value.selectionRangeProvider);
	if(const auto it = json.find("executeCommandProvider"); it != json.end())
		fromJson(std::move(it->second), value.executeCommandProvider);
	if(const auto it = json.find("callHierarchyProvider"); it != json.end())
		fromJson(std::move(it->second), value.callHierarchyProvider);
	if(const auto it = json.find("linkedEditingRangeProvider"); it != json.end())
		fromJson(std::move(it->second), value.linkedEditingRangeProvider);
	if(const auto it = json.find("semanticTokensProvider"); it != json.end())
		fromJson(std::move(it->second), value.semanticTokensProvider);
	if(const auto it = json.find("monikerProvider"); it != json.end())
		fromJson(std::move(it->second), value.monikerProvider);
	if(const auto it = json.find("typeHierarchyProvider"); it != json.end())
		fromJson(std::move(it->second), value.typeHierarchyProvider);
	if(const auto it = json.find("inlineValueProvider"); it != json.end())
		fromJson(std::move(it->second), value.inlineValueProvider);
	if(const auto it = json.find("inlayHintProvider"); it != json.end())
		fromJson(std::move(it->second), value.inlayHintProvider);
	if(const auto it = json.find("diagnosticProvider"); it != json.end())
		fromJson(std::move(it->second), value.diagnosticProvider);
	if(const auto it = json.find("inlineCompletionProvider"); it != json.end())
		fromJson(std::move(it->second), value.inlineCompletionProvider);
	if(const auto it = json.find("workspace"); it != json.end())
		fromJson(std::move(it->second), value.workspace);
	if(const auto it = json.find("experimental"); it != json.end())
		fromJson(std::move(it->second), value.experimental);
}

static void initializeResultServerInfoToJson(InitializeResultServerInfo& value, json::Object& json)
{
	json["name"] = toJson(std::move(value.name));
	if(value.version)
		json["version"] = toJson(std::move(value.version));
}

static void initializeResultServerInfoFromJson(json::Object& json, InitializeResultServerInfo& value)
{
	fromJson(std::move(json.get("name")), value.name);
	if(const auto it = json.find("version"); it != json.end())
		fromJson(std::move(it->second), value.version);
}

static void initializeResultToJson(InitializeResult& value, json::Object& json)
{
	json["capabilities"] = toJson(std::move(value.capabilities));
	if(value.serverInfo)
		json["serverInfo"] = toJson(std::move(value.serverInfo));
}

static void initializeResultFromJson(json::Object& json, InitializeResult& value)
{
	fromJson(std::move(json.get("capabilities")), value.capabilities);
	if(const auto it = json.find("serverInfo"); it != json.end())
		fromJson(std::move(it->second), value.serverInfo);
}

static void initializeErrorToJson(InitializeError& value, json::Object& json)
{
	json["retry"] = toJson(value.retry);
}

static void initializeErrorFromJson(json::Object& json, InitializeError& value)
{
	fromJson(std::move(json.get("retry")), value.retry);
}

static void initializedParamsToJson(InitializedParams& value, json::Object& json)
{
}

static void initializedParamsFromJson(json::Object& json, InitializedParams& value)
{
}

static void didChangeConfigurationParamsToJson(DidChangeConfigurationParams& value, json::Object& json)
{
	json["settings"] = toJson(std::move(value.settings));
}

static void didChangeConfigurationParamsFromJson(json::Object& json, DidChangeConfigurationParams& value)
{
	fromJson(std::move(json.get("settings")), value.settings);
}

static void didChangeConfigurationRegistrationOptionsToJson(DidChangeConfigurationRegistrationOptions& value, json::Object& json)
{
	if(value.section)
		json["section"] = toJson(std::move(value.section));
}

static void didChangeConfigurationRegistrationOptionsFromJson(json::Object& json, DidChangeConfigurationRegistrationOptions& value)
{
	if(const auto it = json.find("section"); it != json.end())
		fromJson(std::move(it->second), value.section);
}

static void showMessageParamsToJson(ShowMessageParams& value, json::Object& json)
{
	json["type"] = toJson(std::move(value.type));
	json["message"] = toJson(std::move(value.message));
}

static void showMessageParamsFromJson(json::Object& json, ShowMessageParams& value)
{
	fromJson(std::move(json.get("type")), value.type);
	fromJson(std::move(json.get("message")), value.message);
}

static void messageActionItemToJson(MessageActionItem& value, json::Object& json)
{
	json["title"] = toJson(std::move(value.title));
}

static void messageActionItemFromJson(json::Object& json, MessageActionItem& value)
{
	fromJson(std::move(json.get("title")), value.title);
}

static void showMessageRequestParamsToJson(ShowMessageRequestParams& value, json::Object& json)
{
	json["type"] = toJson(std::move(value.type));
	json["message"] = toJson(std::move(value.message));
	if(value.actions)
		json["actions"] = toJson(std::move(value.actions));
}

static void showMessageRequestParamsFromJson(json::Object& json, ShowMessageRequestParams& value)
{
	fromJson(std::move(json.get("type")), value.type);
	fromJson(std::move(json.get("message")), value.message);
	if(const auto it = json.find("actions"); it != json.end())
		fromJson(std::move(it->second), value.actions);
}

static void logMessageParamsToJson(LogMessageParams& value, json::Object& json)
{
	json["type"] = toJson(std::move(value.type));
	json["message"] = toJson(std::move(value.message));
}

static void logMessageParamsFromJson(json::Object& json, LogMessageParams& value)
{
	fromJson(std::move(json.get("type")), value.type);
	fromJson(std::move(json.get("message")), value.message);
}

static void didOpenTextDocumentParamsToJson(DidOpenTextDocumentParams& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void didOpenTextDocumentParamsFromJson(json::Object& json, DidOpenTextDocumentParams& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void didChangeTextDocumentParamsToJson(DidChangeTextDocumentParams& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["contentChanges"] = toJson(std::move(value.contentChanges));
}

static void didChangeTextDocumentParamsFromJson(json::Object& json, DidChangeTextDocumentParams& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("contentChanges")), value.contentChanges);
}

static void textDocumentChangeRegistrationOptionsToJson(TextDocumentChangeRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	json["syncKind"] = toJson(std::move(value.syncKind));
}

static void textDocumentChangeRegistrationOptionsFromJson(json::Object& json, TextDocumentChangeRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	fromJson(std::move(json.get("syncKind")), value.syncKind);
}

static void didCloseTextDocumentParamsToJson(DidCloseTextDocumentParams& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void didCloseTextDocumentParamsFromJson(json::Object& json, DidCloseTextDocumentParams& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void didSaveTextDocumentParamsToJson(DidSaveTextDocumentParams& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
	if(value.text)
		json["text"] = toJson(std::move(value.text));
}

static void didSaveTextDocumentParamsFromJson(json::Object& json, DidSaveTextDocumentParams& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	if(const auto it = json.find("text"); it != json.end())
		fromJson(std::move(it->second), value.text);
}

static void textDocumentSaveRegistrationOptionsToJson(TextDocumentSaveRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	saveOptionsToJson(value, json);
}

static void textDocumentSaveRegistrationOptionsFromJson(json::Object& json, TextDocumentSaveRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	saveOptionsFromJson(json, value);
}

static void willSaveTextDocumentParamsToJson(WillSaveTextDocumentParams& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["reason"] = toJson(std::move(value.reason));
}

static void willSaveTextDocumentParamsFromJson(json::Object& json, WillSaveTextDocumentParams& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("reason")), value.reason);
}

static void fileEventToJson(FileEvent& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	json["type"] = toJson(std::move(value.type));
}

static void fileEventFromJson(json::Object& json, FileEvent& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("type")), value.type);
}

static void didChangeWatchedFilesParamsToJson(DidChangeWatchedFilesParams& value, json::Object& json)
{
	json["changes"] = toJson(std::move(value.changes));
}

static void didChangeWatchedFilesParamsFromJson(json::Object& json, DidChangeWatchedFilesParams& value)
{
	fromJson(std::move(json.get("changes")), value.changes);
}

static void relativePatternToJson(RelativePattern& value, json::Object& json)
{
	json["baseUri"] = toJson(std::move(value.baseUri));
	json["pattern"] = toJson(std::move(value.pattern));
}

static void relativePatternFromJson(json::Object& json, RelativePattern& value)
{
	fromJson(std::move(json.get("baseUri")), value.baseUri);
	fromJson(std::move(json.get("pattern")), value.pattern);
}

static void fileSystemWatcherToJson(FileSystemWatcher& value, json::Object& json)
{
	json["globPattern"] = toJson(std::move(value.globPattern));
	if(value.kind)
		json["kind"] = toJson(std::move(value.kind));
}

static void fileSystemWatcherFromJson(json::Object& json, FileSystemWatcher& value)
{
	fromJson(std::move(json.get("globPattern")), value.globPattern);
	if(const auto it = json.find("kind"); it != json.end())
		fromJson(std::move(it->second), value.kind);
}

static void didChangeWatchedFilesRegistrationOptionsToJson(DidChangeWatchedFilesRegistrationOptions& value, json::Object& json)
{
	json["watchers"] = toJson(std::move(value.watchers));
}

static void didChangeWatchedFilesRegistrationOptionsFromJson(json::Object& json, DidChangeWatchedFilesRegistrationOptions& value)
{
	fromJson(std::move(json.get("watchers")), value.watchers);
}

static void publishDiagnosticsParamsToJson(PublishDiagnosticsParams& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
	json["diagnostics"] = toJson(std::move(value.diagnostics));
	if(value.version)
		json["version"] = toJson(std::move(value.version));
}

static void publishDiagnosticsParamsFromJson(json::Object& json, PublishDiagnosticsParams& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
	fromJson(std::move(json.get("diagnostics")), value.diagnostics);
	if(const auto it = json.find("version"); it != json.end())
		fromJson(std::move(it->second), value.version);
}

static void completionContextToJson(CompletionContext& value, json::Object& json)
{
	json["triggerKind"] = toJson(std::move(value.triggerKind));
	if(value.triggerCharacter)
		json["triggerCharacter"] = toJson(std::move(value.triggerCharacter));
}

static void completionContextFromJson(json::Object& json, CompletionContext& value)
{
	fromJson(std::move(json.get("triggerKind")), value.triggerKind);
	if(const auto it = json.find("triggerCharacter"); it != json.end())
		fromJson(std::move(it->second), value.triggerCharacter);
}

static void completionParamsToJson(CompletionParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	if(value.context)
		json["context"] = toJson(std::move(value.context));
}

static void completionParamsFromJson(json::Object& json, CompletionParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	if(const auto it = json.find("context"); it != json.end())
		fromJson(std::move(it->second), value.context);
}

static void completionItemLabelDetailsToJson(CompletionItemLabelDetails& value, json::Object& json)
{
	if(value.detail)
		json["detail"] = toJson(std::move(value.detail));
	if(value.description)
		json["description"] = toJson(std::move(value.description));
}

static void completionItemLabelDetailsFromJson(json::Object& json, CompletionItemLabelDetails& value)
{
	if(const auto it = json.find("detail"); it != json.end())
		fromJson(std::move(it->second), value.detail);
	if(const auto it = json.find("description"); it != json.end())
		fromJson(std::move(it->second), value.description);
}

static void insertReplaceEditToJson(InsertReplaceEdit& value, json::Object& json)
{
	json["newText"] = toJson(std::move(value.newText));
	json["insert"] = toJson(std::move(value.insert));
	json["replace"] = toJson(std::move(value.replace));
}

static void insertReplaceEditFromJson(json::Object& json, InsertReplaceEdit& value)
{
	fromJson(std::move(json.get("newText")), value.newText);
	fromJson(std::move(json.get("insert")), value.insert);
	fromJson(std::move(json.get("replace")), value.replace);
}

static void completionItemToJson(CompletionItem& value, json::Object& json)
{
	json["label"] = toJson(std::move(value.label));
	if(value.labelDetails)
		json["labelDetails"] = toJson(std::move(value.labelDetails));
	if(value.kind)
		json["kind"] = toJson(std::move(value.kind));
	if(value.tags)
		json["tags"] = toJson(std::move(value.tags));
	if(value.detail)
		json["detail"] = toJson(std::move(value.detail));
	if(value.documentation)
		json["documentation"] = toJson(std::move(value.documentation));
	if(value.deprecated)
		json["deprecated"] = toJson(std::move(value.deprecated));
	if(value.preselect)
		json["preselect"] = toJson(std::move(value.preselect));
	if(value.sortText)
		json["sortText"] = toJson(std::move(value.sortText));
	if(value.filterText)
		json["filterText"] = toJson(std::move(value.filterText));
	if(value.insertText)
		json["insertText"] = toJson(std::move(value.insertText));
	if(value.insertTextFormat)
		json["insertTextFormat"] = toJson(std::move(value.insertTextFormat));
	if(value.insertTextMode)
		json["insertTextMode"] = toJson(std::move(value.insertTextMode));
	if(value.textEdit)
		json["textEdit"] = toJson(std::move(value.textEdit));
	if(value.textEditText)
		json["textEditText"] = toJson(std::move(value.textEditText));
	if(value.additionalTextEdits)
		json["additionalTextEdits"] = toJson(std::move(value.additionalTextEdits));
	if(value.commitCharacters)
		json["commitCharacters"] = toJson(std::move(value.commitCharacters));
	if(value.command)
		json["command"] = toJson(std::move(value.command));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void completionItemFromJson(json::Object& json, CompletionItem& value)
{
	fromJson(std::move(json.get("label")), value.label);
	if(const auto it = json.find("labelDetails"); it != json.end())
		fromJson(std::move(it->second), value.labelDetails);
	if(const auto it = json.find("kind"); it != json.end())
		fromJson(std::move(it->second), value.kind);
	if(const auto it = json.find("tags"); it != json.end())
		fromJson(std::move(it->second), value.tags);
	if(const auto it = json.find("detail"); it != json.end())
		fromJson(std::move(it->second), value.detail);
	if(const auto it = json.find("documentation"); it != json.end())
		fromJson(std::move(it->second), value.documentation);
	if(const auto it = json.find("deprecated"); it != json.end())
		fromJson(std::move(it->second), value.deprecated);
	if(const auto it = json.find("preselect"); it != json.end())
		fromJson(std::move(it->second), value.preselect);
	if(const auto it = json.find("sortText"); it != json.end())
		fromJson(std::move(it->second), value.sortText);
	if(const auto it = json.find("filterText"); it != json.end())
		fromJson(std::move(it->second), value.filterText);
	if(const auto it = json.find("insertText"); it != json.end())
		fromJson(std::move(it->second), value.insertText);
	if(const auto it = json.find("insertTextFormat"); it != json.end())
		fromJson(std::move(it->second), value.insertTextFormat);
	if(const auto it = json.find("insertTextMode"); it != json.end())
		fromJson(std::move(it->second), value.insertTextMode);
	if(const auto it = json.find("textEdit"); it != json.end())
		fromJson(std::move(it->second), value.textEdit);
	if(const auto it = json.find("textEditText"); it != json.end())
		fromJson(std::move(it->second), value.textEditText);
	if(const auto it = json.find("additionalTextEdits"); it != json.end())
		fromJson(std::move(it->second), value.additionalTextEdits);
	if(const auto it = json.find("commitCharacters"); it != json.end())
		fromJson(std::move(it->second), value.commitCharacters);
	if(const auto it = json.find("command"); it != json.end())
		fromJson(std::move(it->second), value.command);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void completionListItemDefaultsEditRange_Insert_ReplaceToJson(CompletionListItemDefaultsEditRange_Insert_Replace& value, json::Object& json)
{
	json["insert"] = toJson(std::move(value.insert));
	json["replace"] = toJson(std::move(value.replace));
}

static void completionListItemDefaultsEditRange_Insert_ReplaceFromJson(json::Object& json, CompletionListItemDefaultsEditRange_Insert_Replace& value)
{
	fromJson(std::move(json.get("insert")), value.insert);
	fromJson(std::move(json.get("replace")), value.replace);
}

static void completionListItemDefaultsToJson(CompletionListItemDefaults& value, json::Object& json)
{
	if(value.commitCharacters)
		json["commitCharacters"] = toJson(std::move(value.commitCharacters));
	if(value.editRange)
		json["editRange"] = toJson(std::move(value.editRange));
	if(value.insertTextFormat)
		json["insertTextFormat"] = toJson(std::move(value.insertTextFormat));
	if(value.insertTextMode)
		json["insertTextMode"] = toJson(std::move(value.insertTextMode));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void completionListItemDefaultsFromJson(json::Object& json, CompletionListItemDefaults& value)
{
	if(const auto it = json.find("commitCharacters"); it != json.end())
		fromJson(std::move(it->second), value.commitCharacters);
	if(const auto it = json.find("editRange"); it != json.end())
		fromJson(std::move(it->second), value.editRange);
	if(const auto it = json.find("insertTextFormat"); it != json.end())
		fromJson(std::move(it->second), value.insertTextFormat);
	if(const auto it = json.find("insertTextMode"); it != json.end())
		fromJson(std::move(it->second), value.insertTextMode);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void completionListToJson(CompletionList& value, json::Object& json)
{
	json["isIncomplete"] = toJson(value.isIncomplete);
	json["items"] = toJson(std::move(value.items));
	if(value.itemDefaults)
		json["itemDefaults"] = toJson(std::move(value.itemDefaults));
}

static void completionListFromJson(json::Object& json, CompletionList& value)
{
	fromJson(std::move(json.get("isIncomplete")), value.isIncomplete);
	fromJson(std::move(json.get("items")), value.items);
	if(const auto it = json.find("itemDefaults"); it != json.end())
		fromJson(std::move(it->second), value.itemDefaults);
}

static void completionRegistrationOptionsToJson(CompletionRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	completionOptionsToJson(value, json);
}

static void completionRegistrationOptionsFromJson(json::Object& json, CompletionRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	completionOptionsFromJson(json, value);
}

static void hoverParamsToJson(HoverParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
}

static void hoverParamsFromJson(json::Object& json, HoverParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
}

static void markedString_Language_ValueToJson(MarkedString_Language_Value& value, json::Object& json)
{
	json["language"] = toJson(std::move(value.language));
	json["value"] = toJson(std::move(value.value));
}

static void markedString_Language_ValueFromJson(json::Object& json, MarkedString_Language_Value& value)
{
	fromJson(std::move(json.get("language")), value.language);
	fromJson(std::move(json.get("value")), value.value);
}

static void hoverToJson(Hover& value, json::Object& json)
{
	json["contents"] = toJson(std::move(value.contents));
	if(value.range)
		json["range"] = toJson(std::move(value.range));
}

static void hoverFromJson(json::Object& json, Hover& value)
{
	fromJson(std::move(json.get("contents")), value.contents);
	if(const auto it = json.find("range"); it != json.end())
		fromJson(std::move(it->second), value.range);
}

static void hoverRegistrationOptionsToJson(HoverRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	hoverOptionsToJson(value, json);
}

static void hoverRegistrationOptionsFromJson(json::Object& json, HoverRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	hoverOptionsFromJson(json, value);
}

static void parameterInformationToJson(ParameterInformation& value, json::Object& json)
{
	json["label"] = toJson(std::move(value.label));
	if(value.documentation)
		json["documentation"] = toJson(std::move(value.documentation));
}

static void parameterInformationFromJson(json::Object& json, ParameterInformation& value)
{
	fromJson(std::move(json.get("label")), value.label);
	if(const auto it = json.find("documentation"); it != json.end())
		fromJson(std::move(it->second), value.documentation);
}

static void signatureInformationToJson(SignatureInformation& value, json::Object& json)
{
	json["label"] = toJson(std::move(value.label));
	if(value.documentation)
		json["documentation"] = toJson(std::move(value.documentation));
	if(value.parameters)
		json["parameters"] = toJson(std::move(value.parameters));
	if(value.activeParameter)
		json["activeParameter"] = toJson(std::move(value.activeParameter));
}

static void signatureInformationFromJson(json::Object& json, SignatureInformation& value)
{
	fromJson(std::move(json.get("label")), value.label);
	if(const auto it = json.find("documentation"); it != json.end())
		fromJson(std::move(it->second), value.documentation);
	if(const auto it = json.find("parameters"); it != json.end())
		fromJson(std::move(it->second), value.parameters);
	if(const auto it = json.find("activeParameter"); it != json.end())
		fromJson(std::move(it->second), value.activeParameter);
}

static void signatureHelpToJson(SignatureHelp& value, json::Object& json)
{
	json["signatures"] = toJson(std::move(value.signatures));
	if(value.activeSignature)
		json["activeSignature"] = toJson(std::move(value.activeSignature));
	if(value.activeParameter)
		json["activeParameter"] = toJson(std::move(value.activeParameter));
}

static void signatureHelpFromJson(json::Object& json, SignatureHelp& value)
{
	fromJson(std::move(json.get("signatures")), value.signatures);
	if(const auto it = json.find("activeSignature"); it != json.end())
		fromJson(std::move(it->second), value.activeSignature);
	if(const auto it = json.find("activeParameter"); it != json.end())
		fromJson(std::move(it->second), value.activeParameter);
}

static void signatureHelpContextToJson(SignatureHelpContext& value, json::Object& json)
{
	json["triggerKind"] = toJson(std::move(value.triggerKind));
	json["isRetrigger"] = toJson(value.isRetrigger);
	if(value.triggerCharacter)
		json["triggerCharacter"] = toJson(std::move(value.triggerCharacter));
	if(value.activeSignatureHelp)
		json["activeSignatureHelp"] = toJson(std::move(value.activeSignatureHelp));
}

static void signatureHelpContextFromJson(json::Object& json, SignatureHelpContext& value)
{
	fromJson(std::move(json.get("triggerKind")), value.triggerKind);
	fromJson(std::move(json.get("isRetrigger")), value.isRetrigger);
	if(const auto it = json.find("triggerCharacter"); it != json.end())
		fromJson(std::move(it->second), value.triggerCharacter);
	if(const auto it = json.find("activeSignatureHelp"); it != json.end())
		fromJson(std::move(it->second), value.activeSignatureHelp);
}

static void signatureHelpParamsToJson(SignatureHelpParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.context)
		json["context"] = toJson(std::move(value.context));
}

static void signatureHelpParamsFromJson(json::Object& json, SignatureHelpParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("context"); it != json.end())
		fromJson(std::move(it->second), value.context);
}

static void signatureHelpRegistrationOptionsToJson(SignatureHelpRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	signatureHelpOptionsToJson(value, json);
}

static void signatureHelpRegistrationOptionsFromJson(json::Object& json, SignatureHelpRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	signatureHelpOptionsFromJson(json, value);
}

static void definitionParamsToJson(DefinitionParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
}

static void definitionParamsFromJson(json::Object& json, DefinitionParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
}

static void definitionRegistrationOptionsToJson(DefinitionRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	definitionOptionsToJson(value, json);
}

static void definitionRegistrationOptionsFromJson(json::Object& json, DefinitionRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	definitionOptionsFromJson(json, value);
}

static void referenceContextToJson(ReferenceContext& value, json::Object& json)
{
	json["includeDeclaration"] = toJson(value.includeDeclaration);
}

static void referenceContextFromJson(json::Object& json, ReferenceContext& value)
{
	fromJson(std::move(json.get("includeDeclaration")), value.includeDeclaration);
}

static void referenceParamsToJson(ReferenceParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["context"] = toJson(std::move(value.context));
}

static void referenceParamsFromJson(json::Object& json, ReferenceParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("context")), value.context);
}

static void referenceRegistrationOptionsToJson(ReferenceRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	referenceOptionsToJson(value, json);
}

static void referenceRegistrationOptionsFromJson(json::Object& json, ReferenceRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	referenceOptionsFromJson(json, value);
}

static void documentHighlightParamsToJson(DocumentHighlightParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
}

static void documentHighlightParamsFromJson(json::Object& json, DocumentHighlightParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
}

static void documentHighlightToJson(DocumentHighlight& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	if(value.kind)
		json["kind"] = toJson(std::move(value.kind));
}

static void documentHighlightFromJson(json::Object& json, DocumentHighlight& value)
{
	fromJson(std::move(json.get("range")), value.range);
	if(const auto it = json.find("kind"); it != json.end())
		fromJson(std::move(it->second), value.kind);
}

static void documentHighlightRegistrationOptionsToJson(DocumentHighlightRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	documentHighlightOptionsToJson(value, json);
}

static void documentHighlightRegistrationOptionsFromJson(json::Object& json, DocumentHighlightRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	documentHighlightOptionsFromJson(json, value);
}

static void documentSymbolParamsToJson(DocumentSymbolParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void documentSymbolParamsFromJson(json::Object& json, DocumentSymbolParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void baseSymbolInformationToJson(BaseSymbolInformation& value, json::Object& json)
{
	json["name"] = toJson(std::move(value.name));
	json["kind"] = toJson(std::move(value.kind));
	if(value.tags)
		json["tags"] = toJson(std::move(value.tags));
	if(value.containerName)
		json["containerName"] = toJson(std::move(value.containerName));
}

static void baseSymbolInformationFromJson(json::Object& json, BaseSymbolInformation& value)
{
	fromJson(std::move(json.get("name")), value.name);
	fromJson(std::move(json.get("kind")), value.kind);
	if(const auto it = json.find("tags"); it != json.end())
		fromJson(std::move(it->second), value.tags);
	if(const auto it = json.find("containerName"); it != json.end())
		fromJson(std::move(it->second), value.containerName);
}

static void symbolInformationToJson(SymbolInformation& value, json::Object& json)
{
	baseSymbolInformationToJson(value, json);
	json["location"] = toJson(std::move(value.location));
	if(value.deprecated)
		json["deprecated"] = toJson(std::move(value.deprecated));
}

static void symbolInformationFromJson(json::Object& json, SymbolInformation& value)
{
	baseSymbolInformationFromJson(json, value);
	fromJson(std::move(json.get("location")), value.location);
	if(const auto it = json.find("deprecated"); it != json.end())
		fromJson(std::move(it->second), value.deprecated);
}

static void documentSymbolToJson(DocumentSymbol& value, json::Object& json)
{
	json["name"] = toJson(std::move(value.name));
	json["kind"] = toJson(std::move(value.kind));
	json["range"] = toJson(std::move(value.range));
	json["selectionRange"] = toJson(std::move(value.selectionRange));
	if(value.detail)
		json["detail"] = toJson(std::move(value.detail));
	if(value.tags)
		json["tags"] = toJson(std::move(value.tags));
	if(value.deprecated)
		json["deprecated"] = toJson(std::move(value.deprecated));
	if(value.children)
		json["children"] = toJson(std::move(value.children));
}

static void documentSymbolFromJson(json::Object& json, DocumentSymbol& value)
{
	fromJson(std::move(json.get("name")), value.name);
	fromJson(std::move(json.get("kind")), value.kind);
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("selectionRange")), value.selectionRange);
	if(const auto it = json.find("detail"); it != json.end())
		fromJson(std::move(it->second), value.detail);
	if(const auto it = json.find("tags"); it != json.end())
		fromJson(std::move(it->second), value.tags);
	if(const auto it = json.find("deprecated"); it != json.end())
		fromJson(std::move(it->second), value.deprecated);
	if(const auto it = json.find("children"); it != json.end())
		fromJson(std::move(it->second), value.children);
}

static void documentSymbolRegistrationOptionsToJson(DocumentSymbolRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	documentSymbolOptionsToJson(value, json);
}

static void documentSymbolRegistrationOptionsFromJson(json::Object& json, DocumentSymbolRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	documentSymbolOptionsFromJson(json, value);
}

static void codeActionContextToJson(CodeActionContext& value, json::Object& json)
{
	json["diagnostics"] = toJson(std::move(value.diagnostics));
	if(value.only)
		json["only"] = toJson(std::move(value.only));
	if(value.triggerKind)
		json["triggerKind"] = toJson(std::move(value.triggerKind));
}

static void codeActionContextFromJson(json::Object& json, CodeActionContext& value)
{
	fromJson(std::move(json.get("diagnostics")), value.diagnostics);
	if(const auto it = json.find("only"); it != json.end())
		fromJson(std::move(it->second), value.only);
	if(const auto it = json.find("triggerKind"); it != json.end())
		fromJson(std::move(it->second), value.triggerKind);
}

static void codeActionParamsToJson(CodeActionParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["range"] = toJson(std::move(value.range));
	json["context"] = toJson(std::move(value.context));
}

static void codeActionParamsFromJson(json::Object& json, CodeActionParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("context")), value.context);
}

static void codeActionDisabledToJson(CodeActionDisabled& value, json::Object& json)
{
	json["reason"] = toJson(std::move(value.reason));
}

static void codeActionDisabledFromJson(json::Object& json, CodeActionDisabled& value)
{
	fromJson(std::move(json.get("reason")), value.reason);
}

static void codeActionToJson(CodeAction& value, json::Object& json)
{
	json["title"] = toJson(std::move(value.title));
	if(value.kind)
		json["kind"] = toJson(std::move(value.kind));
	if(value.diagnostics)
		json["diagnostics"] = toJson(std::move(value.diagnostics));
	if(value.isPreferred)
		json["isPreferred"] = toJson(std::move(value.isPreferred));
	if(value.disabled)
		json["disabled"] = toJson(std::move(value.disabled));
	if(value.edit)
		json["edit"] = toJson(std::move(value.edit));
	if(value.command)
		json["command"] = toJson(std::move(value.command));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void codeActionFromJson(json::Object& json, CodeAction& value)
{
	fromJson(std::move(json.get("title")), value.title);
	if(const auto it = json.find("kind"); it != json.end())
		fromJson(std::move(it->second), value.kind);
	if(const auto it = json.find("diagnostics"); it != json.end())
		fromJson(std::move(it->second), value.diagnostics);
	if(const auto it = json.find("isPreferred"); it != json.end())
		fromJson(std::move(it->second), value.isPreferred);
	if(const auto it = json.find("disabled"); it != json.end())
		fromJson(std::move(it->second), value.disabled);
	if(const auto it = json.find("edit"); it != json.end())
		fromJson(std::move(it->second), value.edit);
	if(const auto it = json.find("command"); it != json.end())
		fromJson(std::move(it->second), value.command);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void codeActionRegistrationOptionsToJson(CodeActionRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	codeActionOptionsToJson(value, json);
}

static void codeActionRegistrationOptionsFromJson(json::Object& json, CodeActionRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	codeActionOptionsFromJson(json, value);
}

static void workspaceSymbolParamsToJson(WorkspaceSymbolParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["query"] = toJson(std::move(value.query));
}

static void workspaceSymbolParamsFromJson(json::Object& json, WorkspaceSymbolParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("query")), value.query);
}

static void workspaceSymbolLocation_UriToJson(WorkspaceSymbolLocation_Uri& value, json::Object& json)
{
	json["uri"] = toJson(std::move(value.uri));
}

static void workspaceSymbolLocation_UriFromJson(json::Object& json, WorkspaceSymbolLocation_Uri& value)
{
	fromJson(std::move(json.get("uri")), value.uri);
}

static void workspaceSymbolToJson(WorkspaceSymbol& value, json::Object& json)
{
	baseSymbolInformationToJson(value, json);
	json["location"] = toJson(std::move(value.location));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void workspaceSymbolFromJson(json::Object& json, WorkspaceSymbol& value)
{
	baseSymbolInformationFromJson(json, value);
	fromJson(std::move(json.get("location")), value.location);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void workspaceSymbolRegistrationOptionsToJson(WorkspaceSymbolRegistrationOptions& value, json::Object& json)
{
	workspaceSymbolOptionsToJson(value, json);
}

static void workspaceSymbolRegistrationOptionsFromJson(json::Object& json, WorkspaceSymbolRegistrationOptions& value)
{
	workspaceSymbolOptionsFromJson(json, value);
}

static void codeLensParamsToJson(CodeLensParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void codeLensParamsFromJson(json::Object& json, CodeLensParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void codeLensToJson(CodeLens& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	if(value.command)
		json["command"] = toJson(std::move(value.command));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void codeLensFromJson(json::Object& json, CodeLens& value)
{
	fromJson(std::move(json.get("range")), value.range);
	if(const auto it = json.find("command"); it != json.end())
		fromJson(std::move(it->second), value.command);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void codeLensRegistrationOptionsToJson(CodeLensRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	codeLensOptionsToJson(value, json);
}

static void codeLensRegistrationOptionsFromJson(json::Object& json, CodeLensRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	codeLensOptionsFromJson(json, value);
}

static void documentLinkParamsToJson(DocumentLinkParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	if(value.partialResultToken)
		json["partialResultToken"] = toJson(std::move(value.partialResultToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
}

static void documentLinkParamsFromJson(json::Object& json, DocumentLinkParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	if(const auto it = json.find("partialResultToken"); it != json.end())
		fromJson(std::move(it->second), value.partialResultToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
}

static void documentLinkToJson(DocumentLink& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	if(value.target)
		json["target"] = toJson(std::move(value.target));
	if(value.tooltip)
		json["tooltip"] = toJson(std::move(value.tooltip));
	if(value.data)
		json["data"] = toJson(std::move(value.data));
}

static void documentLinkFromJson(json::Object& json, DocumentLink& value)
{
	fromJson(std::move(json.get("range")), value.range);
	if(const auto it = json.find("target"); it != json.end())
		fromJson(std::move(it->second), value.target);
	if(const auto it = json.find("tooltip"); it != json.end())
		fromJson(std::move(it->second), value.tooltip);
	if(const auto it = json.find("data"); it != json.end())
		fromJson(std::move(it->second), value.data);
}

static void documentLinkRegistrationOptionsToJson(DocumentLinkRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	documentLinkOptionsToJson(value, json);
}

static void documentLinkRegistrationOptionsFromJson(json::Object& json, DocumentLinkRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	documentLinkOptionsFromJson(json, value);
}

static void formattingOptionsToJson(FormattingOptions& value, json::Object& json)
{
	json["tabSize"] = toJson(value.tabSize);
	json["insertSpaces"] = toJson(value.insertSpaces);
	if(value.trimTrailingWhitespace)
		json["trimTrailingWhitespace"] = toJson(std::move(value.trimTrailingWhitespace));
	if(value.insertFinalNewline)
		json["insertFinalNewline"] = toJson(std::move(value.insertFinalNewline));
	if(value.trimFinalNewlines)
		json["trimFinalNewlines"] = toJson(std::move(value.trimFinalNewlines));
}

static void formattingOptionsFromJson(json::Object& json, FormattingOptions& value)
{
	fromJson(std::move(json.get("tabSize")), value.tabSize);
	fromJson(std::move(json.get("insertSpaces")), value.insertSpaces);
	if(const auto it = json.find("trimTrailingWhitespace"); it != json.end())
		fromJson(std::move(it->second), value.trimTrailingWhitespace);
	if(const auto it = json.find("insertFinalNewline"); it != json.end())
		fromJson(std::move(it->second), value.insertFinalNewline);
	if(const auto it = json.find("trimFinalNewlines"); it != json.end())
		fromJson(std::move(it->second), value.trimFinalNewlines);
}

static void documentFormattingParamsToJson(DocumentFormattingParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["options"] = toJson(std::move(value.options));
}

static void documentFormattingParamsFromJson(json::Object& json, DocumentFormattingParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("options")), value.options);
}

static void documentFormattingRegistrationOptionsToJson(DocumentFormattingRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	documentFormattingOptionsToJson(value, json);
}

static void documentFormattingRegistrationOptionsFromJson(json::Object& json, DocumentFormattingRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	documentFormattingOptionsFromJson(json, value);
}

static void documentRangeFormattingParamsToJson(DocumentRangeFormattingParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["range"] = toJson(std::move(value.range));
	json["options"] = toJson(std::move(value.options));
}

static void documentRangeFormattingParamsFromJson(json::Object& json, DocumentRangeFormattingParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("options")), value.options);
}

static void documentRangeFormattingRegistrationOptionsToJson(DocumentRangeFormattingRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	documentRangeFormattingOptionsToJson(value, json);
}

static void documentRangeFormattingRegistrationOptionsFromJson(json::Object& json, DocumentRangeFormattingRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	documentRangeFormattingOptionsFromJson(json, value);
}

static void documentRangesFormattingParamsToJson(DocumentRangesFormattingParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["ranges"] = toJson(std::move(value.ranges));
	json["options"] = toJson(std::move(value.options));
}

static void documentRangesFormattingParamsFromJson(json::Object& json, DocumentRangesFormattingParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("ranges")), value.ranges);
	fromJson(std::move(json.get("options")), value.options);
}

static void documentOnTypeFormattingParamsToJson(DocumentOnTypeFormattingParams& value, json::Object& json)
{
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["position"] = toJson(std::move(value.position));
	json["ch"] = toJson(std::move(value.ch));
	json["options"] = toJson(std::move(value.options));
}

static void documentOnTypeFormattingParamsFromJson(json::Object& json, DocumentOnTypeFormattingParams& value)
{
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("position")), value.position);
	fromJson(std::move(json.get("ch")), value.ch);
	fromJson(std::move(json.get("options")), value.options);
}

static void documentOnTypeFormattingRegistrationOptionsToJson(DocumentOnTypeFormattingRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	documentOnTypeFormattingOptionsToJson(value, json);
}

static void documentOnTypeFormattingRegistrationOptionsFromJson(json::Object& json, DocumentOnTypeFormattingRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	documentOnTypeFormattingOptionsFromJson(json, value);
}

static void renameParamsToJson(RenameParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["textDocument"] = toJson(std::move(value.textDocument));
	json["position"] = toJson(std::move(value.position));
	json["newName"] = toJson(std::move(value.newName));
}

static void renameParamsFromJson(json::Object& json, RenameParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("textDocument")), value.textDocument);
	fromJson(std::move(json.get("position")), value.position);
	fromJson(std::move(json.get("newName")), value.newName);
}

static void renameRegistrationOptionsToJson(RenameRegistrationOptions& value, json::Object& json)
{
	textDocumentRegistrationOptionsToJson(value, json);
	renameOptionsToJson(value, json);
}

static void renameRegistrationOptionsFromJson(json::Object& json, RenameRegistrationOptions& value)
{
	textDocumentRegistrationOptionsFromJson(json, value);
	renameOptionsFromJson(json, value);
}

static void prepareRenameParamsToJson(PrepareRenameParams& value, json::Object& json)
{
	textDocumentPositionParamsToJson(value, json);
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
}

static void prepareRenameParamsFromJson(json::Object& json, PrepareRenameParams& value)
{
	textDocumentPositionParamsFromJson(json, value);
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
}

static void executeCommandParamsToJson(ExecuteCommandParams& value, json::Object& json)
{
	if(value.workDoneToken)
		json["workDoneToken"] = toJson(std::move(value.workDoneToken));
	json["command"] = toJson(std::move(value.command));
	if(value.arguments)
		json["arguments"] = toJson(std::move(value.arguments));
}

static void executeCommandParamsFromJson(json::Object& json, ExecuteCommandParams& value)
{
	if(const auto it = json.find("workDoneToken"); it != json.end())
		fromJson(std::move(it->second), value.workDoneToken);
	fromJson(std::move(json.get("command")), value.command);
	if(const auto it = json.find("arguments"); it != json.end())
		fromJson(std::move(it->second), value.arguments);
}

static void executeCommandRegistrationOptionsToJson(ExecuteCommandRegistrationOptions& value, json::Object& json)
{
	executeCommandOptionsToJson(value, json);
}

static void executeCommandRegistrationOptionsFromJson(json::Object& json, ExecuteCommandRegistrationOptions& value)
{
	executeCommandOptionsFromJson(json, value);
}

static void applyWorkspaceEditParamsToJson(ApplyWorkspaceEditParams& value, json::Object& json)
{
	json["edit"] = toJson(std::move(value.edit));
	if(value.label)
		json["label"] = toJson(std::move(value.label));
}

static void applyWorkspaceEditParamsFromJson(json::Object& json, ApplyWorkspaceEditParams& value)
{
	fromJson(std::move(json.get("edit")), value.edit);
	if(const auto it = json.find("label"); it != json.end())
		fromJson(std::move(it->second), value.label);
}

static void applyWorkspaceEditResultToJson(ApplyWorkspaceEditResult& value, json::Object& json)
{
	json["applied"] = toJson(value.applied);
	if(value.failureReason)
		json["failureReason"] = toJson(std::move(value.failureReason));
	if(value.failedChange)
		json["failedChange"] = toJson(std::move(value.failedChange));
}

static void applyWorkspaceEditResultFromJson(json::Object& json, ApplyWorkspaceEditResult& value)
{
	fromJson(std::move(json.get("applied")), value.applied);
	if(const auto it = json.find("failureReason"); it != json.end())
		fromJson(std::move(it->second), value.failureReason);
	if(const auto it = json.find("failedChange"); it != json.end())
		fromJson(std::move(it->second), value.failedChange);
}

static void workDoneProgressBeginToJson(WorkDoneProgressBegin& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	json["title"] = toJson(std::move(value.title));
	if(value.cancellable)
		json["cancellable"] = toJson(std::move(value.cancellable));
	if(value.message)
		json["message"] = toJson(std::move(value.message));
	if(value.percentage)
		json["percentage"] = toJson(std::move(value.percentage));
}

static void workDoneProgressBeginFromJson(json::Object& json, WorkDoneProgressBegin& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	if(value.kind != "begin")
		throw json::TypeError("Unexpected value for literal 'kind'");
	fromJson(std::move(json.get("title")), value.title);
	if(const auto it = json.find("cancellable"); it != json.end())
		fromJson(std::move(it->second), value.cancellable);
	if(const auto it = json.find("message"); it != json.end())
		fromJson(std::move(it->second), value.message);
	if(const auto it = json.find("percentage"); it != json.end())
		fromJson(std::move(it->second), value.percentage);
}

static void workDoneProgressReportToJson(WorkDoneProgressReport& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	if(value.cancellable)
		json["cancellable"] = toJson(std::move(value.cancellable));
	if(value.message)
		json["message"] = toJson(std::move(value.message));
	if(value.percentage)
		json["percentage"] = toJson(std::move(value.percentage));
}

static void workDoneProgressReportFromJson(json::Object& json, WorkDoneProgressReport& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	if(value.kind != "report")
		throw json::TypeError("Unexpected value for literal 'kind'");
	if(const auto it = json.find("cancellable"); it != json.end())
		fromJson(std::move(it->second), value.cancellable);
	if(const auto it = json.find("message"); it != json.end())
		fromJson(std::move(it->second), value.message);
	if(const auto it = json.find("percentage"); it != json.end())
		fromJson(std::move(it->second), value.percentage);
}

static void workDoneProgressEndToJson(WorkDoneProgressEnd& value, json::Object& json)
{
	json["kind"] = toJson(std::move(value.kind));
	if(value.message)
		json["message"] = toJson(std::move(value.message));
}

static void workDoneProgressEndFromJson(json::Object& json, WorkDoneProgressEnd& value)
{
	fromJson(std::move(json.get("kind")), value.kind);
	if(value.kind != "end")
		throw json::TypeError("Unexpected value for literal 'kind'");
	if(const auto it = json.find("message"); it != json.end())
		fromJson(std::move(it->second), value.message);
}

static void setTraceParamsToJson(SetTraceParams& value, json::Object& json)
{
	json["value"] = toJson(std::move(value.value));
}

static void setTraceParamsFromJson(json::Object& json, SetTraceParams& value)
{
	fromJson(std::move(json.get("value")), value.value);
}

static void logTraceParamsToJson(LogTraceParams& value, json::Object& json)
{
	json["message"] = toJson(std::move(value.message));
	if(value.verbose)
		json["verbose"] = toJson(std::move(value.verbose));
}

static void logTraceParamsFromJson(json::Object& json, LogTraceParams& value)
{
	fromJson(std::move(json.get("message")), value.message);
	if(const auto it = json.find("verbose"); it != json.end())
		fromJson(std::move(it->second), value.verbose);
}

static void cancelParamsToJson(CancelParams& value, json::Object& json)
{
	json["id"] = toJson(std::move(value.id));
}

static void cancelParamsFromJson(json::Object& json, CancelParams& value)
{
	fromJson(std::move(json.get("id")), value.id);
}

static void progressParamsToJson(ProgressParams& value, json::Object& json)
{
	json["token"] = toJson(std::move(value.token));
	json["value"] = toJson(std::move(value.value));
}

static void progressParamsFromJson(json::Object& json, ProgressParams& value)
{
	fromJson(std::move(json.get("token")), value.token);
	fromJson(std::move(json.get("value")), value.value);
}

static void locationLinkToJson(LocationLink& value, json::Object& json)
{
	json["targetUri"] = toJson(std::move(value.targetUri));
	json["targetRange"] = toJson(std::move(value.targetRange));
	json["targetSelectionRange"] = toJson(std::move(value.targetSelectionRange));
	if(value.originSelectionRange)
		json["originSelectionRange"] = toJson(std::move(value.originSelectionRange));
}

static void locationLinkFromJson(json::Object& json, LocationLink& value)
{
	fromJson(std::move(json.get("targetUri")), value.targetUri);
	fromJson(std::move(json.get("targetRange")), value.targetRange);
	fromJson(std::move(json.get("targetSelectionRange")), value.targetSelectionRange);
	if(const auto it = json.find("originSelectionRange"); it != json.end())
		fromJson(std::move(it->second), value.originSelectionRange);
}

static void inlineValueTextToJson(InlineValueText& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["text"] = toJson(std::move(value.text));
}

static void inlineValueTextFromJson(json::Object& json, InlineValueText& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("text")), value.text);
}

static void inlineValueVariableLookupToJson(InlineValueVariableLookup& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["caseSensitiveLookup"] = toJson(value.caseSensitiveLookup);
	if(value.variableName)
		json["variableName"] = toJson(std::move(value.variableName));
}

static void inlineValueVariableLookupFromJson(json::Object& json, InlineValueVariableLookup& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("caseSensitiveLookup")), value.caseSensitiveLookup);
	if(const auto it = json.find("variableName"); it != json.end())
		fromJson(std::move(it->second), value.variableName);
}

static void inlineValueEvaluatableExpressionToJson(InlineValueEvaluatableExpression& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	if(value.expression)
		json["expression"] = toJson(std::move(value.expression));
}

static void inlineValueEvaluatableExpressionFromJson(json::Object& json, InlineValueEvaluatableExpression& value)
{
	fromJson(std::move(json.get("range")), value.range);
	if(const auto it = json.find("expression"); it != json.end())
		fromJson(std::move(it->second), value.expression);
}

static void relatedFullDocumentDiagnosticReportToJson(RelatedFullDocumentDiagnosticReport& value, json::Object& json)
{
	fullDocumentDiagnosticReportToJson(value, json);
	if(value.relatedDocuments)
		json["relatedDocuments"] = toJson(std::move(value.relatedDocuments));
}

static void relatedFullDocumentDiagnosticReportFromJson(json::Object& json, RelatedFullDocumentDiagnosticReport& value)
{
	fullDocumentDiagnosticReportFromJson(json, value);
	if(const auto it = json.find("relatedDocuments"); it != json.end())
		fromJson(std::move(it->second), value.relatedDocuments);
}

static void relatedUnchangedDocumentDiagnosticReportToJson(RelatedUnchangedDocumentDiagnosticReport& value, json::Object& json)
{
	unchangedDocumentDiagnosticReportToJson(value, json);
	if(value.relatedDocuments)
		json["relatedDocuments"] = toJson(std::move(value.relatedDocuments));
}

static void relatedUnchangedDocumentDiagnosticReportFromJson(json::Object& json, RelatedUnchangedDocumentDiagnosticReport& value)
{
	unchangedDocumentDiagnosticReportFromJson(json, value);
	if(const auto it = json.find("relatedDocuments"); it != json.end())
		fromJson(std::move(it->second), value.relatedDocuments);
}

static void prepareRenameResult_Range_PlaceholderToJson(PrepareRenameResult_Range_Placeholder& value, json::Object& json)
{
	json["range"] = toJson(std::move(value.range));
	json["placeholder"] = toJson(std::move(value.placeholder));
}

static void prepareRenameResult_Range_PlaceholderFromJson(json::Object& json, PrepareRenameResult_Range_Placeholder& value)
{
	fromJson(std::move(json.get("range")), value.range);
	fromJson(std::move(json.get("placeholder")), value.placeholder);
}

static void prepareRenameResult_DefaultBehaviorToJson(PrepareRenameResult_DefaultBehavior& value, json::Object& json)
{
	json["defaultBehavior"] = toJson(value.defaultBehavior);
}

static void prepareRenameResult_DefaultBehaviorFromJson(json::Object& json, PrepareRenameResult_DefaultBehavior& value)
{
	fromJson(std::move(json.get("defaultBehavior")), value.defaultBehavior);
}

/*
 * Serialization boilerplate
 */

template<>
const char** requiredProperties<TextDocumentIdentifier>()
{
	static const char* properties[] = {
		"uri",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentIdentifier&& value)
{
	json::Object obj;
	textDocumentIdentifierToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentIdentifier& value)
{
	auto& obj = json.object();
	textDocumentIdentifierFromJson(obj, value);
}

template<>
const char** requiredProperties<Position>()
{
	static const char* properties[] = {
		"line",
		"character",
		nullptr
	};
	return properties;
}

json::Any toJson(Position&& value)
{
	json::Object obj;
	positionToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Position& value)
{
	auto& obj = json.object();
	positionFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentPositionParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentPositionParams&& value)
{
	json::Object obj;
	textDocumentPositionParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentPositionParams& value)
{
	auto& obj = json.object();
	textDocumentPositionParamsFromJson(obj, value);
}

json::Any toJson(WorkDoneProgressParams&& value)
{
	json::Object obj;
	workDoneProgressParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkDoneProgressParams& value)
{
	auto& obj = json.object();
	workDoneProgressParamsFromJson(obj, value);
}

json::Any toJson(PartialResultParams&& value)
{
	json::Object obj;
	partialResultParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PartialResultParams& value)
{
	auto& obj = json.object();
	partialResultParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ImplementationParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(ImplementationParams&& value)
{
	json::Object obj;
	implementationParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ImplementationParams& value)
{
	auto& obj = json.object();
	implementationParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<Range>()
{
	static const char* properties[] = {
		"start",
		"end",
		nullptr
	};
	return properties;
}

json::Any toJson(Range&& value)
{
	json::Object obj;
	rangeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Range& value)
{
	auto& obj = json.object();
	rangeFromJson(obj, value);
}

template<>
const char** requiredProperties<Location>()
{
	static const char* properties[] = {
		"uri",
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(Location&& value)
{
	json::Object obj;
	locationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Location& value)
{
	auto& obj = json.object();
	locationFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentFilter_Language>()
{
	static const char* properties[] = {
		"language",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentFilter_Language&& value)
{
	json::Object obj;
	textDocumentFilter_LanguageToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentFilter_Language& value)
{
	auto& obj = json.object();
	textDocumentFilter_LanguageFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentFilter_Scheme>()
{
	static const char* properties[] = {
		"scheme",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentFilter_Scheme&& value)
{
	json::Object obj;
	textDocumentFilter_SchemeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentFilter_Scheme& value)
{
	auto& obj = json.object();
	textDocumentFilter_SchemeFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentFilter_Pattern>()
{
	static const char* properties[] = {
		"pattern",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentFilter_Pattern&& value)
{
	json::Object obj;
	textDocumentFilter_PatternToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentFilter_Pattern& value)
{
	auto& obj = json.object();
	textDocumentFilter_PatternFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentFilter_NotebookType>()
{
	static const char* properties[] = {
		"notebookType",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentFilter_NotebookType&& value)
{
	json::Object obj;
	notebookDocumentFilter_NotebookTypeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentFilter_NotebookType& value)
{
	auto& obj = json.object();
	notebookDocumentFilter_NotebookTypeFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentFilter_Scheme>()
{
	static const char* properties[] = {
		"scheme",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentFilter_Scheme&& value)
{
	json::Object obj;
	notebookDocumentFilter_SchemeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentFilter_Scheme& value)
{
	auto& obj = json.object();
	notebookDocumentFilter_SchemeFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentFilter_Pattern>()
{
	static const char* properties[] = {
		"pattern",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentFilter_Pattern&& value)
{
	json::Object obj;
	notebookDocumentFilter_PatternToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentFilter_Pattern& value)
{
	auto& obj = json.object();
	notebookDocumentFilter_PatternFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookCellTextDocumentFilter>()
{
	static const char* properties[] = {
		"notebook",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookCellTextDocumentFilter&& value)
{
	json::Object obj;
	notebookCellTextDocumentFilterToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookCellTextDocumentFilter& value)
{
	auto& obj = json.object();
	notebookCellTextDocumentFilterFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentRegistrationOptions&& value)
{
	json::Object obj;
	textDocumentRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentRegistrationOptions& value)
{
	auto& obj = json.object();
	textDocumentRegistrationOptionsFromJson(obj, value);
}

json::Any toJson(WorkDoneProgressOptions&& value)
{
	json::Object obj;
	workDoneProgressOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkDoneProgressOptions& value)
{
	auto& obj = json.object();
	workDoneProgressOptionsFromJson(obj, value);
}

json::Any toJson(ImplementationOptions&& value)
{
	json::Object obj;
	implementationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ImplementationOptions& value)
{
	auto& obj = json.object();
	implementationOptionsFromJson(obj, value);
}

json::Any toJson(StaticRegistrationOptions&& value)
{
	json::Object obj;
	staticRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, StaticRegistrationOptions& value)
{
	auto& obj = json.object();
	staticRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<ImplementationRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(ImplementationRegistrationOptions&& value)
{
	json::Object obj;
	implementationRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ImplementationRegistrationOptions& value)
{
	auto& obj = json.object();
	implementationRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<TypeDefinitionParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(TypeDefinitionParams&& value)
{
	json::Object obj;
	typeDefinitionParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeDefinitionParams& value)
{
	auto& obj = json.object();
	typeDefinitionParamsFromJson(obj, value);
}

json::Any toJson(TypeDefinitionOptions&& value)
{
	json::Object obj;
	typeDefinitionOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeDefinitionOptions& value)
{
	auto& obj = json.object();
	typeDefinitionOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<TypeDefinitionRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(TypeDefinitionRegistrationOptions&& value)
{
	json::Object obj;
	typeDefinitionRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeDefinitionRegistrationOptions& value)
{
	auto& obj = json.object();
	typeDefinitionRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceFolder>()
{
	static const char* properties[] = {
		"uri",
		"name",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceFolder&& value)
{
	json::Object obj;
	workspaceFolderToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceFolder& value)
{
	auto& obj = json.object();
	workspaceFolderFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceFoldersChangeEvent>()
{
	static const char* properties[] = {
		"added",
		"removed",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceFoldersChangeEvent&& value)
{
	json::Object obj;
	workspaceFoldersChangeEventToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceFoldersChangeEvent& value)
{
	auto& obj = json.object();
	workspaceFoldersChangeEventFromJson(obj, value);
}

template<>
const char** requiredProperties<DidChangeWorkspaceFoldersParams>()
{
	static const char* properties[] = {
		"event",
		nullptr
	};
	return properties;
}

json::Any toJson(DidChangeWorkspaceFoldersParams&& value)
{
	json::Object obj;
	didChangeWorkspaceFoldersParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeWorkspaceFoldersParams& value)
{
	auto& obj = json.object();
	didChangeWorkspaceFoldersParamsFromJson(obj, value);
}

json::Any toJson(ConfigurationItem&& value)
{
	json::Object obj;
	configurationItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ConfigurationItem& value)
{
	auto& obj = json.object();
	configurationItemFromJson(obj, value);
}

template<>
const char** requiredProperties<ConfigurationParams>()
{
	static const char* properties[] = {
		"items",
		nullptr
	};
	return properties;
}

json::Any toJson(ConfigurationParams&& value)
{
	json::Object obj;
	configurationParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ConfigurationParams& value)
{
	auto& obj = json.object();
	configurationParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentColorParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentColorParams&& value)
{
	json::Object obj;
	documentColorParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentColorParams& value)
{
	auto& obj = json.object();
	documentColorParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<Color>()
{
	static const char* properties[] = {
		"red",
		"green",
		"blue",
		"alpha",
		nullptr
	};
	return properties;
}

json::Any toJson(Color&& value)
{
	json::Object obj;
	colorToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Color& value)
{
	auto& obj = json.object();
	colorFromJson(obj, value);
}

template<>
const char** requiredProperties<ColorInformation>()
{
	static const char* properties[] = {
		"range",
		"color",
		nullptr
	};
	return properties;
}

json::Any toJson(ColorInformation&& value)
{
	json::Object obj;
	colorInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ColorInformation& value)
{
	auto& obj = json.object();
	colorInformationFromJson(obj, value);
}

json::Any toJson(DocumentColorOptions&& value)
{
	json::Object obj;
	documentColorOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentColorOptions& value)
{
	auto& obj = json.object();
	documentColorOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentColorRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentColorRegistrationOptions&& value)
{
	json::Object obj;
	documentColorRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentColorRegistrationOptions& value)
{
	auto& obj = json.object();
	documentColorRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<ColorPresentationParams>()
{
	static const char* properties[] = {
		"textDocument",
		"color",
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(ColorPresentationParams&& value)
{
	json::Object obj;
	colorPresentationParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ColorPresentationParams& value)
{
	auto& obj = json.object();
	colorPresentationParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<TextEdit>()
{
	static const char* properties[] = {
		"range",
		"newText",
		nullptr
	};
	return properties;
}

json::Any toJson(TextEdit&& value)
{
	json::Object obj;
	textEditToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextEdit& value)
{
	auto& obj = json.object();
	textEditFromJson(obj, value);
}

template<>
const char** requiredProperties<ColorPresentation>()
{
	static const char* properties[] = {
		"label",
		nullptr
	};
	return properties;
}

json::Any toJson(ColorPresentation&& value)
{
	json::Object obj;
	colorPresentationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ColorPresentation& value)
{
	auto& obj = json.object();
	colorPresentationFromJson(obj, value);
}

template<>
const char** requiredProperties<FoldingRangeParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(FoldingRangeParams&& value)
{
	json::Object obj;
	foldingRangeParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRangeParams& value)
{
	auto& obj = json.object();
	foldingRangeParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<FoldingRange>()
{
	static const char* properties[] = {
		"startLine",
		"endLine",
		nullptr
	};
	return properties;
}

json::Any toJson(FoldingRange&& value)
{
	json::Object obj;
	foldingRangeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRange& value)
{
	auto& obj = json.object();
	foldingRangeFromJson(obj, value);
}

json::Any toJson(FoldingRangeOptions&& value)
{
	json::Object obj;
	foldingRangeOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRangeOptions& value)
{
	auto& obj = json.object();
	foldingRangeOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<FoldingRangeRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(FoldingRangeRegistrationOptions&& value)
{
	json::Object obj;
	foldingRangeRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRangeRegistrationOptions& value)
{
	auto& obj = json.object();
	foldingRangeRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DeclarationParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(DeclarationParams&& value)
{
	json::Object obj;
	declarationParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DeclarationParams& value)
{
	auto& obj = json.object();
	declarationParamsFromJson(obj, value);
}

json::Any toJson(DeclarationOptions&& value)
{
	json::Object obj;
	declarationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DeclarationOptions& value)
{
	auto& obj = json.object();
	declarationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DeclarationRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DeclarationRegistrationOptions&& value)
{
	json::Object obj;
	declarationRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DeclarationRegistrationOptions& value)
{
	auto& obj = json.object();
	declarationRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<SelectionRangeParams>()
{
	static const char* properties[] = {
		"textDocument",
		"positions",
		nullptr
	};
	return properties;
}

json::Any toJson(SelectionRangeParams&& value)
{
	json::Object obj;
	selectionRangeParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SelectionRangeParams& value)
{
	auto& obj = json.object();
	selectionRangeParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<SelectionRange>()
{
	static const char* properties[] = {
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(SelectionRange&& value)
{
	json::Object obj;
	selectionRangeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SelectionRange& value)
{
	auto& obj = json.object();
	selectionRangeFromJson(obj, value);
}

json::Any toJson(SelectionRangeOptions&& value)
{
	json::Object obj;
	selectionRangeOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SelectionRangeOptions& value)
{
	auto& obj = json.object();
	selectionRangeOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<SelectionRangeRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(SelectionRangeRegistrationOptions&& value)
{
	json::Object obj;
	selectionRangeRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SelectionRangeRegistrationOptions& value)
{
	auto& obj = json.object();
	selectionRangeRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkDoneProgressCreateParams>()
{
	static const char* properties[] = {
		"token",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkDoneProgressCreateParams&& value)
{
	json::Object obj;
	workDoneProgressCreateParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkDoneProgressCreateParams& value)
{
	auto& obj = json.object();
	workDoneProgressCreateParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkDoneProgressCancelParams>()
{
	static const char* properties[] = {
		"token",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkDoneProgressCancelParams&& value)
{
	json::Object obj;
	workDoneProgressCancelParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkDoneProgressCancelParams& value)
{
	auto& obj = json.object();
	workDoneProgressCancelParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CallHierarchyPrepareParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(CallHierarchyPrepareParams&& value)
{
	json::Object obj;
	callHierarchyPrepareParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyPrepareParams& value)
{
	auto& obj = json.object();
	callHierarchyPrepareParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CallHierarchyItem>()
{
	static const char* properties[] = {
		"name",
		"kind",
		"uri",
		"range",
		"selectionRange",
		nullptr
	};
	return properties;
}

json::Any toJson(CallHierarchyItem&& value)
{
	json::Object obj;
	callHierarchyItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyItem& value)
{
	auto& obj = json.object();
	callHierarchyItemFromJson(obj, value);
}

json::Any toJson(CallHierarchyOptions&& value)
{
	json::Object obj;
	callHierarchyOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyOptions& value)
{
	auto& obj = json.object();
	callHierarchyOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<CallHierarchyRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(CallHierarchyRegistrationOptions&& value)
{
	json::Object obj;
	callHierarchyRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyRegistrationOptions& value)
{
	auto& obj = json.object();
	callHierarchyRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<CallHierarchyIncomingCallsParams>()
{
	static const char* properties[] = {
		"item",
		nullptr
	};
	return properties;
}

json::Any toJson(CallHierarchyIncomingCallsParams&& value)
{
	json::Object obj;
	callHierarchyIncomingCallsParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyIncomingCallsParams& value)
{
	auto& obj = json.object();
	callHierarchyIncomingCallsParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CallHierarchyIncomingCall>()
{
	static const char* properties[] = {
		"from",
		"fromRanges",
		nullptr
	};
	return properties;
}

json::Any toJson(CallHierarchyIncomingCall&& value)
{
	json::Object obj;
	callHierarchyIncomingCallToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyIncomingCall& value)
{
	auto& obj = json.object();
	callHierarchyIncomingCallFromJson(obj, value);
}

template<>
const char** requiredProperties<CallHierarchyOutgoingCallsParams>()
{
	static const char* properties[] = {
		"item",
		nullptr
	};
	return properties;
}

json::Any toJson(CallHierarchyOutgoingCallsParams&& value)
{
	json::Object obj;
	callHierarchyOutgoingCallsParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyOutgoingCallsParams& value)
{
	auto& obj = json.object();
	callHierarchyOutgoingCallsParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CallHierarchyOutgoingCall>()
{
	static const char* properties[] = {
		"to",
		"fromRanges",
		nullptr
	};
	return properties;
}

json::Any toJson(CallHierarchyOutgoingCall&& value)
{
	json::Object obj;
	callHierarchyOutgoingCallToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyOutgoingCall& value)
{
	auto& obj = json.object();
	callHierarchyOutgoingCallFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensParams&& value)
{
	json::Object obj;
	semanticTokensParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensParams& value)
{
	auto& obj = json.object();
	semanticTokensParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokens>()
{
	static const char* properties[] = {
		"data",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokens&& value)
{
	json::Object obj;
	semanticTokensToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokens& value)
{
	auto& obj = json.object();
	semanticTokensFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensPartialResult>()
{
	static const char* properties[] = {
		"data",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensPartialResult&& value)
{
	json::Object obj;
	semanticTokensPartialResultToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensPartialResult& value)
{
	auto& obj = json.object();
	semanticTokensPartialResultFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensLegend>()
{
	static const char* properties[] = {
		"tokenTypes",
		"tokenModifiers",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensLegend&& value)
{
	json::Object obj;
	semanticTokensLegendToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensLegend& value)
{
	auto& obj = json.object();
	semanticTokensLegendFromJson(obj, value);
}

json::Any toJson(SemanticTokensOptionsRange&& value)
{
	json::Object obj;
	semanticTokensOptionsRangeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensOptionsRange& value)
{
	auto& obj = json.object();
	semanticTokensOptionsRangeFromJson(obj, value);
}

json::Any toJson(SemanticTokensOptionsFull&& value)
{
	json::Object obj;
	semanticTokensOptionsFullToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensOptionsFull& value)
{
	auto& obj = json.object();
	semanticTokensOptionsFullFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensOptions>()
{
	static const char* properties[] = {
		"legend",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensOptions&& value)
{
	json::Object obj;
	semanticTokensOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensOptions& value)
{
	auto& obj = json.object();
	semanticTokensOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		"legend",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensRegistrationOptions&& value)
{
	json::Object obj;
	semanticTokensRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensRegistrationOptions& value)
{
	auto& obj = json.object();
	semanticTokensRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensDeltaParams>()
{
	static const char* properties[] = {
		"textDocument",
		"previousResultId",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensDeltaParams&& value)
{
	json::Object obj;
	semanticTokensDeltaParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensDeltaParams& value)
{
	auto& obj = json.object();
	semanticTokensDeltaParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensEdit>()
{
	static const char* properties[] = {
		"start",
		"deleteCount",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensEdit&& value)
{
	json::Object obj;
	semanticTokensEditToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensEdit& value)
{
	auto& obj = json.object();
	semanticTokensEditFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensDelta>()
{
	static const char* properties[] = {
		"edits",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensDelta&& value)
{
	json::Object obj;
	semanticTokensDeltaToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensDelta& value)
{
	auto& obj = json.object();
	semanticTokensDeltaFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensDeltaPartialResult>()
{
	static const char* properties[] = {
		"edits",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensDeltaPartialResult&& value)
{
	json::Object obj;
	semanticTokensDeltaPartialResultToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensDeltaPartialResult& value)
{
	auto& obj = json.object();
	semanticTokensDeltaPartialResultFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensRangeParams>()
{
	static const char* properties[] = {
		"textDocument",
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensRangeParams&& value)
{
	json::Object obj;
	semanticTokensRangeParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensRangeParams& value)
{
	auto& obj = json.object();
	semanticTokensRangeParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ShowDocumentParams>()
{
	static const char* properties[] = {
		"uri",
		nullptr
	};
	return properties;
}

json::Any toJson(ShowDocumentParams&& value)
{
	json::Object obj;
	showDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ShowDocumentParams& value)
{
	auto& obj = json.object();
	showDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ShowDocumentResult>()
{
	static const char* properties[] = {
		"success",
		nullptr
	};
	return properties;
}

json::Any toJson(ShowDocumentResult&& value)
{
	json::Object obj;
	showDocumentResultToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ShowDocumentResult& value)
{
	auto& obj = json.object();
	showDocumentResultFromJson(obj, value);
}

template<>
const char** requiredProperties<LinkedEditingRangeParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(LinkedEditingRangeParams&& value)
{
	json::Object obj;
	linkedEditingRangeParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LinkedEditingRangeParams& value)
{
	auto& obj = json.object();
	linkedEditingRangeParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<LinkedEditingRanges>()
{
	static const char* properties[] = {
		"ranges",
		nullptr
	};
	return properties;
}

json::Any toJson(LinkedEditingRanges&& value)
{
	json::Object obj;
	linkedEditingRangesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LinkedEditingRanges& value)
{
	auto& obj = json.object();
	linkedEditingRangesFromJson(obj, value);
}

json::Any toJson(LinkedEditingRangeOptions&& value)
{
	json::Object obj;
	linkedEditingRangeOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LinkedEditingRangeOptions& value)
{
	auto& obj = json.object();
	linkedEditingRangeOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<LinkedEditingRangeRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(LinkedEditingRangeRegistrationOptions&& value)
{
	json::Object obj;
	linkedEditingRangeRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LinkedEditingRangeRegistrationOptions& value)
{
	auto& obj = json.object();
	linkedEditingRangeRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<FileCreate>()
{
	static const char* properties[] = {
		"uri",
		nullptr
	};
	return properties;
}

json::Any toJson(FileCreate&& value)
{
	json::Object obj;
	fileCreateToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileCreate& value)
{
	auto& obj = json.object();
	fileCreateFromJson(obj, value);
}

template<>
const char** requiredProperties<CreateFilesParams>()
{
	static const char* properties[] = {
		"files",
		nullptr
	};
	return properties;
}

json::Any toJson(CreateFilesParams&& value)
{
	json::Object obj;
	createFilesParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CreateFilesParams& value)
{
	auto& obj = json.object();
	createFilesParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<OptionalVersionedTextDocumentIdentifier>()
{
	static const char* properties[] = {
		"uri",
		"version",
		nullptr
	};
	return properties;
}

json::Any toJson(OptionalVersionedTextDocumentIdentifier&& value)
{
	json::Object obj;
	optionalVersionedTextDocumentIdentifierToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, OptionalVersionedTextDocumentIdentifier& value)
{
	auto& obj = json.object();
	optionalVersionedTextDocumentIdentifierFromJson(obj, value);
}

template<>
const char** requiredProperties<AnnotatedTextEdit>()
{
	static const char* properties[] = {
		"range",
		"newText",
		"annotationId",
		nullptr
	};
	return properties;
}

json::Any toJson(AnnotatedTextEdit&& value)
{
	json::Object obj;
	annotatedTextEditToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, AnnotatedTextEdit& value)
{
	auto& obj = json.object();
	annotatedTextEditFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentEdit>()
{
	static const char* properties[] = {
		"textDocument",
		"edits",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentEdit&& value)
{
	json::Object obj;
	textDocumentEditToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentEdit& value)
{
	auto& obj = json.object();
	textDocumentEditFromJson(obj, value);
}

template<>
const char** requiredProperties<ResourceOperation>()
{
	static const char* properties[] = {
		"kind",
		nullptr
	};
	return properties;
}

json::Any toJson(ResourceOperation&& value)
{
	json::Object obj;
	resourceOperationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ResourceOperation& value)
{
	auto& obj = json.object();
	resourceOperationFromJson(obj, value);
}

json::Any toJson(CreateFileOptions&& value)
{
	json::Object obj;
	createFileOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CreateFileOptions& value)
{
	auto& obj = json.object();
	createFileOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<CreateFile>()
{
	static const char* properties[] = {
		"kind",
		"uri",
		nullptr
	};
	return properties;
}

template<>
const std::pair<const char*, json::Any>* literalProperties<CreateFile>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "create"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(CreateFile&& value)
{
	json::Object obj;
	createFileToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CreateFile& value)
{
	auto& obj = json.object();
	createFileFromJson(obj, value);
}

json::Any toJson(RenameFileOptions&& value)
{
	json::Object obj;
	renameFileOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RenameFileOptions& value)
{
	auto& obj = json.object();
	renameFileOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<RenameFile>()
{
	static const char* properties[] = {
		"kind",
		"oldUri",
		"newUri",
		nullptr
	};
	return properties;
}

template<>
const std::pair<const char*, json::Any>* literalProperties<RenameFile>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "rename"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(RenameFile&& value)
{
	json::Object obj;
	renameFileToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RenameFile& value)
{
	auto& obj = json.object();
	renameFileFromJson(obj, value);
}

json::Any toJson(DeleteFileOptions&& value)
{
	json::Object obj;
	deleteFileOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DeleteFileOptions& value)
{
	auto& obj = json.object();
	deleteFileOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DeleteFile>()
{
	static const char* properties[] = {
		"kind",
		"uri",
		nullptr
	};
	return properties;
}

template<>
const std::pair<const char*, json::Any>* literalProperties<DeleteFile>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "delete"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(DeleteFile&& value)
{
	json::Object obj;
	deleteFileToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DeleteFile& value)
{
	auto& obj = json.object();
	deleteFileFromJson(obj, value);
}

template<>
const char** requiredProperties<ChangeAnnotation>()
{
	static const char* properties[] = {
		"label",
		nullptr
	};
	return properties;
}

json::Any toJson(ChangeAnnotation&& value)
{
	json::Object obj;
	changeAnnotationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ChangeAnnotation& value)
{
	auto& obj = json.object();
	changeAnnotationFromJson(obj, value);
}

json::Any toJson(WorkspaceEdit&& value)
{
	json::Object obj;
	workspaceEditToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceEdit& value)
{
	auto& obj = json.object();
	workspaceEditFromJson(obj, value);
}

json::Any toJson(FileOperationPatternOptions&& value)
{
	json::Object obj;
	fileOperationPatternOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileOperationPatternOptions& value)
{
	auto& obj = json.object();
	fileOperationPatternOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<FileOperationPattern>()
{
	static const char* properties[] = {
		"glob",
		nullptr
	};
	return properties;
}

json::Any toJson(FileOperationPattern&& value)
{
	json::Object obj;
	fileOperationPatternToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileOperationPattern& value)
{
	auto& obj = json.object();
	fileOperationPatternFromJson(obj, value);
}

template<>
const char** requiredProperties<FileOperationFilter>()
{
	static const char* properties[] = {
		"pattern",
		nullptr
	};
	return properties;
}

json::Any toJson(FileOperationFilter&& value)
{
	json::Object obj;
	fileOperationFilterToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileOperationFilter& value)
{
	auto& obj = json.object();
	fileOperationFilterFromJson(obj, value);
}

template<>
const char** requiredProperties<FileOperationRegistrationOptions>()
{
	static const char* properties[] = {
		"filters",
		nullptr
	};
	return properties;
}

json::Any toJson(FileOperationRegistrationOptions&& value)
{
	json::Object obj;
	fileOperationRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileOperationRegistrationOptions& value)
{
	auto& obj = json.object();
	fileOperationRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<FileRename>()
{
	static const char* properties[] = {
		"oldUri",
		"newUri",
		nullptr
	};
	return properties;
}

json::Any toJson(FileRename&& value)
{
	json::Object obj;
	fileRenameToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileRename& value)
{
	auto& obj = json.object();
	fileRenameFromJson(obj, value);
}

template<>
const char** requiredProperties<RenameFilesParams>()
{
	static const char* properties[] = {
		"files",
		nullptr
	};
	return properties;
}

json::Any toJson(RenameFilesParams&& value)
{
	json::Object obj;
	renameFilesParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RenameFilesParams& value)
{
	auto& obj = json.object();
	renameFilesParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<FileDelete>()
{
	static const char* properties[] = {
		"uri",
		nullptr
	};
	return properties;
}

json::Any toJson(FileDelete&& value)
{
	json::Object obj;
	fileDeleteToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileDelete& value)
{
	auto& obj = json.object();
	fileDeleteFromJson(obj, value);
}

template<>
const char** requiredProperties<DeleteFilesParams>()
{
	static const char* properties[] = {
		"files",
		nullptr
	};
	return properties;
}

json::Any toJson(DeleteFilesParams&& value)
{
	json::Object obj;
	deleteFilesParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DeleteFilesParams& value)
{
	auto& obj = json.object();
	deleteFilesParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<MonikerParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(MonikerParams&& value)
{
	json::Object obj;
	monikerParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MonikerParams& value)
{
	auto& obj = json.object();
	monikerParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<Moniker>()
{
	static const char* properties[] = {
		"scheme",
		"identifier",
		"unique",
		nullptr
	};
	return properties;
}

json::Any toJson(Moniker&& value)
{
	json::Object obj;
	monikerToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Moniker& value)
{
	auto& obj = json.object();
	monikerFromJson(obj, value);
}

json::Any toJson(MonikerOptions&& value)
{
	json::Object obj;
	monikerOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MonikerOptions& value)
{
	auto& obj = json.object();
	monikerOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<MonikerRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(MonikerRegistrationOptions&& value)
{
	json::Object obj;
	monikerRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MonikerRegistrationOptions& value)
{
	auto& obj = json.object();
	monikerRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<TypeHierarchyPrepareParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(TypeHierarchyPrepareParams&& value)
{
	json::Object obj;
	typeHierarchyPrepareParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeHierarchyPrepareParams& value)
{
	auto& obj = json.object();
	typeHierarchyPrepareParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<TypeHierarchyItem>()
{
	static const char* properties[] = {
		"name",
		"kind",
		"uri",
		"range",
		"selectionRange",
		nullptr
	};
	return properties;
}

json::Any toJson(TypeHierarchyItem&& value)
{
	json::Object obj;
	typeHierarchyItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeHierarchyItem& value)
{
	auto& obj = json.object();
	typeHierarchyItemFromJson(obj, value);
}

json::Any toJson(TypeHierarchyOptions&& value)
{
	json::Object obj;
	typeHierarchyOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeHierarchyOptions& value)
{
	auto& obj = json.object();
	typeHierarchyOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<TypeHierarchyRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(TypeHierarchyRegistrationOptions&& value)
{
	json::Object obj;
	typeHierarchyRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeHierarchyRegistrationOptions& value)
{
	auto& obj = json.object();
	typeHierarchyRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<TypeHierarchySupertypesParams>()
{
	static const char* properties[] = {
		"item",
		nullptr
	};
	return properties;
}

json::Any toJson(TypeHierarchySupertypesParams&& value)
{
	json::Object obj;
	typeHierarchySupertypesParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeHierarchySupertypesParams& value)
{
	auto& obj = json.object();
	typeHierarchySupertypesParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<TypeHierarchySubtypesParams>()
{
	static const char* properties[] = {
		"item",
		nullptr
	};
	return properties;
}

json::Any toJson(TypeHierarchySubtypesParams&& value)
{
	json::Object obj;
	typeHierarchySubtypesParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeHierarchySubtypesParams& value)
{
	auto& obj = json.object();
	typeHierarchySubtypesParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineValueContext>()
{
	static const char* properties[] = {
		"frameId",
		"stoppedLocation",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineValueContext&& value)
{
	json::Object obj;
	inlineValueContextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueContext& value)
{
	auto& obj = json.object();
	inlineValueContextFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineValueParams>()
{
	static const char* properties[] = {
		"textDocument",
		"range",
		"context",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineValueParams&& value)
{
	json::Object obj;
	inlineValueParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueParams& value)
{
	auto& obj = json.object();
	inlineValueParamsFromJson(obj, value);
}

json::Any toJson(InlineValueOptions&& value)
{
	json::Object obj;
	inlineValueOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueOptions& value)
{
	auto& obj = json.object();
	inlineValueOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineValueRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineValueRegistrationOptions&& value)
{
	json::Object obj;
	inlineValueRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueRegistrationOptions& value)
{
	auto& obj = json.object();
	inlineValueRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<InlayHintParams>()
{
	static const char* properties[] = {
		"textDocument",
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(InlayHintParams&& value)
{
	json::Object obj;
	inlayHintParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHintParams& value)
{
	auto& obj = json.object();
	inlayHintParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<MarkupContent>()
{
	static const char* properties[] = {
		"kind",
		"value",
		nullptr
	};
	return properties;
}

json::Any toJson(MarkupContent&& value)
{
	json::Object obj;
	markupContentToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MarkupContent& value)
{
	auto& obj = json.object();
	markupContentFromJson(obj, value);
}

template<>
const char** requiredProperties<Command>()
{
	static const char* properties[] = {
		"title",
		"command",
		nullptr
	};
	return properties;
}

json::Any toJson(Command&& value)
{
	json::Object obj;
	commandToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Command& value)
{
	auto& obj = json.object();
	commandFromJson(obj, value);
}

template<>
const char** requiredProperties<InlayHintLabelPart>()
{
	static const char* properties[] = {
		"value",
		nullptr
	};
	return properties;
}

json::Any toJson(InlayHintLabelPart&& value)
{
	json::Object obj;
	inlayHintLabelPartToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHintLabelPart& value)
{
	auto& obj = json.object();
	inlayHintLabelPartFromJson(obj, value);
}

template<>
const char** requiredProperties<InlayHint>()
{
	static const char* properties[] = {
		"position",
		"label",
		nullptr
	};
	return properties;
}

json::Any toJson(InlayHint&& value)
{
	json::Object obj;
	inlayHintToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHint& value)
{
	auto& obj = json.object();
	inlayHintFromJson(obj, value);
}

json::Any toJson(InlayHintOptions&& value)
{
	json::Object obj;
	inlayHintOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHintOptions& value)
{
	auto& obj = json.object();
	inlayHintOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<InlayHintRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(InlayHintRegistrationOptions&& value)
{
	json::Object obj;
	inlayHintRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHintRegistrationOptions& value)
{
	auto& obj = json.object();
	inlayHintRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentDiagnosticParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentDiagnosticParams&& value)
{
	json::Object obj;
	documentDiagnosticParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentDiagnosticParams& value)
{
	auto& obj = json.object();
	documentDiagnosticParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeDescription>()
{
	static const char* properties[] = {
		"href",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeDescription&& value)
{
	json::Object obj;
	codeDescriptionToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeDescription& value)
{
	auto& obj = json.object();
	codeDescriptionFromJson(obj, value);
}

template<>
const char** requiredProperties<DiagnosticRelatedInformation>()
{
	static const char* properties[] = {
		"location",
		"message",
		nullptr
	};
	return properties;
}

json::Any toJson(DiagnosticRelatedInformation&& value)
{
	json::Object obj;
	diagnosticRelatedInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DiagnosticRelatedInformation& value)
{
	auto& obj = json.object();
	diagnosticRelatedInformationFromJson(obj, value);
}

template<>
const char** requiredProperties<Diagnostic>()
{
	static const char* properties[] = {
		"range",
		"message",
		nullptr
	};
	return properties;
}

json::Any toJson(Diagnostic&& value)
{
	json::Object obj;
	diagnosticToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Diagnostic& value)
{
	auto& obj = json.object();
	diagnosticFromJson(obj, value);
}

template<>
const char** requiredProperties<FullDocumentDiagnosticReport>()
{
	static const char* properties[] = {
		"items",
		nullptr
	};
	return properties;
}

template<>
const std::pair<const char*, json::Any>* literalProperties<FullDocumentDiagnosticReport>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "full"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(FullDocumentDiagnosticReport&& value)
{
	json::Object obj;
	fullDocumentDiagnosticReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FullDocumentDiagnosticReport& value)
{
	auto& obj = json.object();
	fullDocumentDiagnosticReportFromJson(obj, value);
}

template<>
const char** requiredProperties<UnchangedDocumentDiagnosticReport>()
{
	static const char* properties[] = {
		"resultId",
		nullptr
	};
	return properties;
}

template<>
const std::pair<const char*, json::Any>* literalProperties<UnchangedDocumentDiagnosticReport>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "unchanged"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(UnchangedDocumentDiagnosticReport&& value)
{
	json::Object obj;
	unchangedDocumentDiagnosticReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, UnchangedDocumentDiagnosticReport& value)
{
	auto& obj = json.object();
	unchangedDocumentDiagnosticReportFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentDiagnosticReportPartialResult>()
{
	static const char* properties[] = {
		"relatedDocuments",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentDiagnosticReportPartialResult&& value)
{
	json::Object obj;
	documentDiagnosticReportPartialResultToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentDiagnosticReportPartialResult& value)
{
	auto& obj = json.object();
	documentDiagnosticReportPartialResultFromJson(obj, value);
}

template<>
const char** requiredProperties<DiagnosticServerCancellationData>()
{
	static const char* properties[] = {
		"retriggerRequest",
		nullptr
	};
	return properties;
}

json::Any toJson(DiagnosticServerCancellationData&& value)
{
	json::Object obj;
	diagnosticServerCancellationDataToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DiagnosticServerCancellationData& value)
{
	auto& obj = json.object();
	diagnosticServerCancellationDataFromJson(obj, value);
}

template<>
const char** requiredProperties<DiagnosticOptions>()
{
	static const char* properties[] = {
		"interFileDependencies",
		"workspaceDiagnostics",
		nullptr
	};
	return properties;
}

json::Any toJson(DiagnosticOptions&& value)
{
	json::Object obj;
	diagnosticOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DiagnosticOptions& value)
{
	auto& obj = json.object();
	diagnosticOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DiagnosticRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		"interFileDependencies",
		"workspaceDiagnostics",
		nullptr
	};
	return properties;
}

json::Any toJson(DiagnosticRegistrationOptions&& value)
{
	json::Object obj;
	diagnosticRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DiagnosticRegistrationOptions& value)
{
	auto& obj = json.object();
	diagnosticRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<PreviousResultId>()
{
	static const char* properties[] = {
		"uri",
		"value",
		nullptr
	};
	return properties;
}

json::Any toJson(PreviousResultId&& value)
{
	json::Object obj;
	previousResultIdToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PreviousResultId& value)
{
	auto& obj = json.object();
	previousResultIdFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceDiagnosticParams>()
{
	static const char* properties[] = {
		"previousResultIds",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceDiagnosticParams&& value)
{
	json::Object obj;
	workspaceDiagnosticParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceDiagnosticParams& value)
{
	auto& obj = json.object();
	workspaceDiagnosticParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceFullDocumentDiagnosticReport>()
{
	static const char* properties[] = {
		"kind",
		"items",
		"uri",
		"version",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceFullDocumentDiagnosticReport&& value)
{
	json::Object obj;
	workspaceFullDocumentDiagnosticReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceFullDocumentDiagnosticReport& value)
{
	auto& obj = json.object();
	workspaceFullDocumentDiagnosticReportFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceUnchangedDocumentDiagnosticReport>()
{
	static const char* properties[] = {
		"kind",
		"resultId",
		"uri",
		"version",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceUnchangedDocumentDiagnosticReport&& value)
{
	json::Object obj;
	workspaceUnchangedDocumentDiagnosticReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceUnchangedDocumentDiagnosticReport& value)
{
	auto& obj = json.object();
	workspaceUnchangedDocumentDiagnosticReportFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceDiagnosticReport>()
{
	static const char* properties[] = {
		"items",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceDiagnosticReport&& value)
{
	json::Object obj;
	workspaceDiagnosticReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceDiagnosticReport& value)
{
	auto& obj = json.object();
	workspaceDiagnosticReportFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceDiagnosticReportPartialResult>()
{
	static const char* properties[] = {
		"items",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceDiagnosticReportPartialResult&& value)
{
	json::Object obj;
	workspaceDiagnosticReportPartialResultToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceDiagnosticReportPartialResult& value)
{
	auto& obj = json.object();
	workspaceDiagnosticReportPartialResultFromJson(obj, value);
}

template<>
const char** requiredProperties<ExecutionSummary>()
{
	static const char* properties[] = {
		"executionOrder",
		nullptr
	};
	return properties;
}

json::Any toJson(ExecutionSummary&& value)
{
	json::Object obj;
	executionSummaryToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ExecutionSummary& value)
{
	auto& obj = json.object();
	executionSummaryFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookCell>()
{
	static const char* properties[] = {
		"kind",
		"document",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookCell&& value)
{
	json::Object obj;
	notebookCellToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookCell& value)
{
	auto& obj = json.object();
	notebookCellFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocument>()
{
	static const char* properties[] = {
		"uri",
		"notebookType",
		"version",
		"cells",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocument&& value)
{
	json::Object obj;
	notebookDocumentToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocument& value)
{
	auto& obj = json.object();
	notebookDocumentFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentItem>()
{
	static const char* properties[] = {
		"uri",
		"languageId",
		"version",
		"text",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentItem&& value)
{
	json::Object obj;
	textDocumentItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentItem& value)
{
	auto& obj = json.object();
	textDocumentItemFromJson(obj, value);
}

template<>
const char** requiredProperties<DidOpenNotebookDocumentParams>()
{
	static const char* properties[] = {
		"notebookDocument",
		"cellTextDocuments",
		nullptr
	};
	return properties;
}

json::Any toJson(DidOpenNotebookDocumentParams&& value)
{
	json::Object obj;
	didOpenNotebookDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidOpenNotebookDocumentParams& value)
{
	auto& obj = json.object();
	didOpenNotebookDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<VersionedNotebookDocumentIdentifier>()
{
	static const char* properties[] = {
		"version",
		"uri",
		nullptr
	};
	return properties;
}

json::Any toJson(VersionedNotebookDocumentIdentifier&& value)
{
	json::Object obj;
	versionedNotebookDocumentIdentifierToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, VersionedNotebookDocumentIdentifier& value)
{
	auto& obj = json.object();
	versionedNotebookDocumentIdentifierFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookCellArrayChange>()
{
	static const char* properties[] = {
		"start",
		"deleteCount",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookCellArrayChange&& value)
{
	json::Object obj;
	notebookCellArrayChangeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookCellArrayChange& value)
{
	auto& obj = json.object();
	notebookCellArrayChangeFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentChangeEventCellsStructure>()
{
	static const char* properties[] = {
		"array",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentChangeEventCellsStructure&& value)
{
	json::Object obj;
	notebookDocumentChangeEventCellsStructureToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentChangeEventCellsStructure& value)
{
	auto& obj = json.object();
	notebookDocumentChangeEventCellsStructureFromJson(obj, value);
}

template<>
const char** requiredProperties<VersionedTextDocumentIdentifier>()
{
	static const char* properties[] = {
		"uri",
		"version",
		nullptr
	};
	return properties;
}

json::Any toJson(VersionedTextDocumentIdentifier&& value)
{
	json::Object obj;
	versionedTextDocumentIdentifierToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, VersionedTextDocumentIdentifier& value)
{
	auto& obj = json.object();
	versionedTextDocumentIdentifierFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentContentChangeEvent_Range_Text>()
{
	static const char* properties[] = {
		"range",
		"text",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentContentChangeEvent_Range_Text&& value)
{
	json::Object obj;
	textDocumentContentChangeEvent_Range_TextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentContentChangeEvent_Range_Text& value)
{
	auto& obj = json.object();
	textDocumentContentChangeEvent_Range_TextFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentContentChangeEvent_Text>()
{
	static const char* properties[] = {
		"text",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentContentChangeEvent_Text&& value)
{
	json::Object obj;
	textDocumentContentChangeEvent_TextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentContentChangeEvent_Text& value)
{
	auto& obj = json.object();
	textDocumentContentChangeEvent_TextFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentChangeEventCellsTextContent>()
{
	static const char* properties[] = {
		"document",
		"changes",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentChangeEventCellsTextContent&& value)
{
	json::Object obj;
	notebookDocumentChangeEventCellsTextContentToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentChangeEventCellsTextContent& value)
{
	auto& obj = json.object();
	notebookDocumentChangeEventCellsTextContentFromJson(obj, value);
}

json::Any toJson(NotebookDocumentChangeEventCells&& value)
{
	json::Object obj;
	notebookDocumentChangeEventCellsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentChangeEventCells& value)
{
	auto& obj = json.object();
	notebookDocumentChangeEventCellsFromJson(obj, value);
}

json::Any toJson(NotebookDocumentChangeEvent&& value)
{
	json::Object obj;
	notebookDocumentChangeEventToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentChangeEvent& value)
{
	auto& obj = json.object();
	notebookDocumentChangeEventFromJson(obj, value);
}

template<>
const char** requiredProperties<DidChangeNotebookDocumentParams>()
{
	static const char* properties[] = {
		"notebookDocument",
		"change",
		nullptr
	};
	return properties;
}

json::Any toJson(DidChangeNotebookDocumentParams&& value)
{
	json::Object obj;
	didChangeNotebookDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeNotebookDocumentParams& value)
{
	auto& obj = json.object();
	didChangeNotebookDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentIdentifier>()
{
	static const char* properties[] = {
		"uri",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentIdentifier&& value)
{
	json::Object obj;
	notebookDocumentIdentifierToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentIdentifier& value)
{
	auto& obj = json.object();
	notebookDocumentIdentifierFromJson(obj, value);
}

template<>
const char** requiredProperties<DidSaveNotebookDocumentParams>()
{
	static const char* properties[] = {
		"notebookDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DidSaveNotebookDocumentParams&& value)
{
	json::Object obj;
	didSaveNotebookDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidSaveNotebookDocumentParams& value)
{
	auto& obj = json.object();
	didSaveNotebookDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DidCloseNotebookDocumentParams>()
{
	static const char* properties[] = {
		"notebookDocument",
		"cellTextDocuments",
		nullptr
	};
	return properties;
}

json::Any toJson(DidCloseNotebookDocumentParams&& value)
{
	json::Object obj;
	didCloseNotebookDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidCloseNotebookDocumentParams& value)
{
	auto& obj = json.object();
	didCloseNotebookDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<SelectedCompletionInfo>()
{
	static const char* properties[] = {
		"range",
		"text",
		nullptr
	};
	return properties;
}

json::Any toJson(SelectedCompletionInfo&& value)
{
	json::Object obj;
	selectedCompletionInfoToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SelectedCompletionInfo& value)
{
	auto& obj = json.object();
	selectedCompletionInfoFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineCompletionContext>()
{
	static const char* properties[] = {
		"triggerKind",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineCompletionContext&& value)
{
	json::Object obj;
	inlineCompletionContextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineCompletionContext& value)
{
	auto& obj = json.object();
	inlineCompletionContextFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineCompletionParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		"context",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineCompletionParams&& value)
{
	json::Object obj;
	inlineCompletionParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineCompletionParams& value)
{
	auto& obj = json.object();
	inlineCompletionParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<StringValue>()
{
	static const char* properties[] = {
		"value",
		nullptr
	};
	return properties;
}

template<>
const std::pair<const char*, json::Any>* literalProperties<StringValue>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "snippet"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(StringValue&& value)
{
	json::Object obj;
	stringValueToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, StringValue& value)
{
	auto& obj = json.object();
	stringValueFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineCompletionItem>()
{
	static const char* properties[] = {
		"insertText",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineCompletionItem&& value)
{
	json::Object obj;
	inlineCompletionItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineCompletionItem& value)
{
	auto& obj = json.object();
	inlineCompletionItemFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineCompletionList>()
{
	static const char* properties[] = {
		"items",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineCompletionList&& value)
{
	json::Object obj;
	inlineCompletionListToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineCompletionList& value)
{
	auto& obj = json.object();
	inlineCompletionListFromJson(obj, value);
}

json::Any toJson(InlineCompletionOptions&& value)
{
	json::Object obj;
	inlineCompletionOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineCompletionOptions& value)
{
	auto& obj = json.object();
	inlineCompletionOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineCompletionRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineCompletionRegistrationOptions&& value)
{
	json::Object obj;
	inlineCompletionRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineCompletionRegistrationOptions& value)
{
	auto& obj = json.object();
	inlineCompletionRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<Registration>()
{
	static const char* properties[] = {
		"id",
		"method",
		nullptr
	};
	return properties;
}

json::Any toJson(Registration&& value)
{
	json::Object obj;
	registrationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Registration& value)
{
	auto& obj = json.object();
	registrationFromJson(obj, value);
}

template<>
const char** requiredProperties<RegistrationParams>()
{
	static const char* properties[] = {
		"registrations",
		nullptr
	};
	return properties;
}

json::Any toJson(RegistrationParams&& value)
{
	json::Object obj;
	registrationParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RegistrationParams& value)
{
	auto& obj = json.object();
	registrationParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<Unregistration>()
{
	static const char* properties[] = {
		"id",
		"method",
		nullptr
	};
	return properties;
}

json::Any toJson(Unregistration&& value)
{
	json::Object obj;
	unregistrationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Unregistration& value)
{
	auto& obj = json.object();
	unregistrationFromJson(obj, value);
}

template<>
const char** requiredProperties<UnregistrationParams>()
{
	static const char* properties[] = {
		"unregisterations",
		nullptr
	};
	return properties;
}

json::Any toJson(UnregistrationParams&& value)
{
	json::Object obj;
	unregistrationParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, UnregistrationParams& value)
{
	auto& obj = json.object();
	unregistrationParamsFromJson(obj, value);
}

json::Any toJson(WorkspaceEditClientCapabilitiesChangeAnnotationSupport&& value)
{
	json::Object obj;
	workspaceEditClientCapabilitiesChangeAnnotationSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceEditClientCapabilitiesChangeAnnotationSupport& value)
{
	auto& obj = json.object();
	workspaceEditClientCapabilitiesChangeAnnotationSupportFromJson(obj, value);
}

json::Any toJson(WorkspaceEditClientCapabilities&& value)
{
	json::Object obj;
	workspaceEditClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceEditClientCapabilities& value)
{
	auto& obj = json.object();
	workspaceEditClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DidChangeConfigurationClientCapabilities&& value)
{
	json::Object obj;
	didChangeConfigurationClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeConfigurationClientCapabilities& value)
{
	auto& obj = json.object();
	didChangeConfigurationClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DidChangeWatchedFilesClientCapabilities&& value)
{
	json::Object obj;
	didChangeWatchedFilesClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeWatchedFilesClientCapabilities& value)
{
	auto& obj = json.object();
	didChangeWatchedFilesClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(WorkspaceSymbolClientCapabilitiesSymbolKind&& value)
{
	json::Object obj;
	workspaceSymbolClientCapabilitiesSymbolKindToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilitiesSymbolKind& value)
{
	auto& obj = json.object();
	workspaceSymbolClientCapabilitiesSymbolKindFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceSymbolClientCapabilitiesTagSupport>()
{
	static const char* properties[] = {
		"valueSet",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceSymbolClientCapabilitiesTagSupport&& value)
{
	json::Object obj;
	workspaceSymbolClientCapabilitiesTagSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilitiesTagSupport& value)
{
	auto& obj = json.object();
	workspaceSymbolClientCapabilitiesTagSupportFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceSymbolClientCapabilitiesResolveSupport>()
{
	static const char* properties[] = {
		"properties",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceSymbolClientCapabilitiesResolveSupport&& value)
{
	json::Object obj;
	workspaceSymbolClientCapabilitiesResolveSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilitiesResolveSupport& value)
{
	auto& obj = json.object();
	workspaceSymbolClientCapabilitiesResolveSupportFromJson(obj, value);
}

json::Any toJson(WorkspaceSymbolClientCapabilities&& value)
{
	json::Object obj;
	workspaceSymbolClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolClientCapabilities& value)
{
	auto& obj = json.object();
	workspaceSymbolClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(ExecuteCommandClientCapabilities&& value)
{
	json::Object obj;
	executeCommandClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ExecuteCommandClientCapabilities& value)
{
	auto& obj = json.object();
	executeCommandClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(SemanticTokensWorkspaceClientCapabilities&& value)
{
	json::Object obj;
	semanticTokensWorkspaceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensWorkspaceClientCapabilities& value)
{
	auto& obj = json.object();
	semanticTokensWorkspaceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(CodeLensWorkspaceClientCapabilities&& value)
{
	json::Object obj;
	codeLensWorkspaceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeLensWorkspaceClientCapabilities& value)
{
	auto& obj = json.object();
	codeLensWorkspaceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(FileOperationClientCapabilities&& value)
{
	json::Object obj;
	fileOperationClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileOperationClientCapabilities& value)
{
	auto& obj = json.object();
	fileOperationClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(InlineValueWorkspaceClientCapabilities&& value)
{
	json::Object obj;
	inlineValueWorkspaceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueWorkspaceClientCapabilities& value)
{
	auto& obj = json.object();
	inlineValueWorkspaceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(InlayHintWorkspaceClientCapabilities&& value)
{
	json::Object obj;
	inlayHintWorkspaceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHintWorkspaceClientCapabilities& value)
{
	auto& obj = json.object();
	inlayHintWorkspaceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DiagnosticWorkspaceClientCapabilities&& value)
{
	json::Object obj;
	diagnosticWorkspaceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DiagnosticWorkspaceClientCapabilities& value)
{
	auto& obj = json.object();
	diagnosticWorkspaceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(FoldingRangeWorkspaceClientCapabilities&& value)
{
	json::Object obj;
	foldingRangeWorkspaceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRangeWorkspaceClientCapabilities& value)
{
	auto& obj = json.object();
	foldingRangeWorkspaceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(WorkspaceClientCapabilities&& value)
{
	json::Object obj;
	workspaceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceClientCapabilities& value)
{
	auto& obj = json.object();
	workspaceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(TextDocumentSyncClientCapabilities&& value)
{
	json::Object obj;
	textDocumentSyncClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentSyncClientCapabilities& value)
{
	auto& obj = json.object();
	textDocumentSyncClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionClientCapabilitiesCompletionItemTagSupport>()
{
	static const char* properties[] = {
		"valueSet",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionClientCapabilitiesCompletionItemTagSupport&& value)
{
	json::Object obj;
	completionClientCapabilitiesCompletionItemTagSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemTagSupport& value)
{
	auto& obj = json.object();
	completionClientCapabilitiesCompletionItemTagSupportFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionClientCapabilitiesCompletionItemResolveSupport>()
{
	static const char* properties[] = {
		"properties",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionClientCapabilitiesCompletionItemResolveSupport&& value)
{
	json::Object obj;
	completionClientCapabilitiesCompletionItemResolveSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemResolveSupport& value)
{
	auto& obj = json.object();
	completionClientCapabilitiesCompletionItemResolveSupportFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionClientCapabilitiesCompletionItemInsertTextModeSupport>()
{
	static const char* properties[] = {
		"valueSet",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionClientCapabilitiesCompletionItemInsertTextModeSupport&& value)
{
	json::Object obj;
	completionClientCapabilitiesCompletionItemInsertTextModeSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemInsertTextModeSupport& value)
{
	auto& obj = json.object();
	completionClientCapabilitiesCompletionItemInsertTextModeSupportFromJson(obj, value);
}

json::Any toJson(CompletionClientCapabilitiesCompletionItem&& value)
{
	json::Object obj;
	completionClientCapabilitiesCompletionItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItem& value)
{
	auto& obj = json.object();
	completionClientCapabilitiesCompletionItemFromJson(obj, value);
}

json::Any toJson(CompletionClientCapabilitiesCompletionItemKind&& value)
{
	json::Object obj;
	completionClientCapabilitiesCompletionItemKindToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionItemKind& value)
{
	auto& obj = json.object();
	completionClientCapabilitiesCompletionItemKindFromJson(obj, value);
}

json::Any toJson(CompletionClientCapabilitiesCompletionList&& value)
{
	json::Object obj;
	completionClientCapabilitiesCompletionListToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionClientCapabilitiesCompletionList& value)
{
	auto& obj = json.object();
	completionClientCapabilitiesCompletionListFromJson(obj, value);
}

json::Any toJson(CompletionClientCapabilities&& value)
{
	json::Object obj;
	completionClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionClientCapabilities& value)
{
	auto& obj = json.object();
	completionClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(HoverClientCapabilities&& value)
{
	json::Object obj;
	hoverClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, HoverClientCapabilities& value)
{
	auto& obj = json.object();
	hoverClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(SignatureHelpClientCapabilitiesSignatureInformationParameterInformation&& value)
{
	json::Object obj;
	signatureHelpClientCapabilitiesSignatureInformationParameterInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelpClientCapabilitiesSignatureInformationParameterInformation& value)
{
	auto& obj = json.object();
	signatureHelpClientCapabilitiesSignatureInformationParameterInformationFromJson(obj, value);
}

json::Any toJson(SignatureHelpClientCapabilitiesSignatureInformation&& value)
{
	json::Object obj;
	signatureHelpClientCapabilitiesSignatureInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelpClientCapabilitiesSignatureInformation& value)
{
	auto& obj = json.object();
	signatureHelpClientCapabilitiesSignatureInformationFromJson(obj, value);
}

json::Any toJson(SignatureHelpClientCapabilities&& value)
{
	json::Object obj;
	signatureHelpClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelpClientCapabilities& value)
{
	auto& obj = json.object();
	signatureHelpClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DeclarationClientCapabilities&& value)
{
	json::Object obj;
	declarationClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DeclarationClientCapabilities& value)
{
	auto& obj = json.object();
	declarationClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DefinitionClientCapabilities&& value)
{
	json::Object obj;
	definitionClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DefinitionClientCapabilities& value)
{
	auto& obj = json.object();
	definitionClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(TypeDefinitionClientCapabilities&& value)
{
	json::Object obj;
	typeDefinitionClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeDefinitionClientCapabilities& value)
{
	auto& obj = json.object();
	typeDefinitionClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(ImplementationClientCapabilities&& value)
{
	json::Object obj;
	implementationClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ImplementationClientCapabilities& value)
{
	auto& obj = json.object();
	implementationClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(ReferenceClientCapabilities&& value)
{
	json::Object obj;
	referenceClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ReferenceClientCapabilities& value)
{
	auto& obj = json.object();
	referenceClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DocumentHighlightClientCapabilities&& value)
{
	json::Object obj;
	documentHighlightClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentHighlightClientCapabilities& value)
{
	auto& obj = json.object();
	documentHighlightClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DocumentSymbolClientCapabilitiesSymbolKind&& value)
{
	json::Object obj;
	documentSymbolClientCapabilitiesSymbolKindToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentSymbolClientCapabilitiesSymbolKind& value)
{
	auto& obj = json.object();
	documentSymbolClientCapabilitiesSymbolKindFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentSymbolClientCapabilitiesTagSupport>()
{
	static const char* properties[] = {
		"valueSet",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentSymbolClientCapabilitiesTagSupport&& value)
{
	json::Object obj;
	documentSymbolClientCapabilitiesTagSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentSymbolClientCapabilitiesTagSupport& value)
{
	auto& obj = json.object();
	documentSymbolClientCapabilitiesTagSupportFromJson(obj, value);
}

json::Any toJson(DocumentSymbolClientCapabilities&& value)
{
	json::Object obj;
	documentSymbolClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentSymbolClientCapabilities& value)
{
	auto& obj = json.object();
	documentSymbolClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind>()
{
	static const char* properties[] = {
		"valueSet",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind&& value)
{
	json::Object obj;
	codeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKindToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKind& value)
{
	auto& obj = json.object();
	codeActionClientCapabilitiesCodeActionLiteralSupportCodeActionKindFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeActionClientCapabilitiesCodeActionLiteralSupport>()
{
	static const char* properties[] = {
		"codeActionKind",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeActionClientCapabilitiesCodeActionLiteralSupport&& value)
{
	json::Object obj;
	codeActionClientCapabilitiesCodeActionLiteralSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionClientCapabilitiesCodeActionLiteralSupport& value)
{
	auto& obj = json.object();
	codeActionClientCapabilitiesCodeActionLiteralSupportFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeActionClientCapabilitiesResolveSupport>()
{
	static const char* properties[] = {
		"properties",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeActionClientCapabilitiesResolveSupport&& value)
{
	json::Object obj;
	codeActionClientCapabilitiesResolveSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionClientCapabilitiesResolveSupport& value)
{
	auto& obj = json.object();
	codeActionClientCapabilitiesResolveSupportFromJson(obj, value);
}

json::Any toJson(CodeActionClientCapabilities&& value)
{
	json::Object obj;
	codeActionClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionClientCapabilities& value)
{
	auto& obj = json.object();
	codeActionClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(CodeLensClientCapabilities&& value)
{
	json::Object obj;
	codeLensClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeLensClientCapabilities& value)
{
	auto& obj = json.object();
	codeLensClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DocumentLinkClientCapabilities&& value)
{
	json::Object obj;
	documentLinkClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentLinkClientCapabilities& value)
{
	auto& obj = json.object();
	documentLinkClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DocumentColorClientCapabilities&& value)
{
	json::Object obj;
	documentColorClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentColorClientCapabilities& value)
{
	auto& obj = json.object();
	documentColorClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DocumentFormattingClientCapabilities&& value)
{
	json::Object obj;
	documentFormattingClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentFormattingClientCapabilities& value)
{
	auto& obj = json.object();
	documentFormattingClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DocumentRangeFormattingClientCapabilities&& value)
{
	json::Object obj;
	documentRangeFormattingClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentRangeFormattingClientCapabilities& value)
{
	auto& obj = json.object();
	documentRangeFormattingClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DocumentOnTypeFormattingClientCapabilities&& value)
{
	json::Object obj;
	documentOnTypeFormattingClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentOnTypeFormattingClientCapabilities& value)
{
	auto& obj = json.object();
	documentOnTypeFormattingClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(RenameClientCapabilities&& value)
{
	json::Object obj;
	renameClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RenameClientCapabilities& value)
{
	auto& obj = json.object();
	renameClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(FoldingRangeClientCapabilitiesFoldingRangeKind&& value)
{
	json::Object obj;
	foldingRangeClientCapabilitiesFoldingRangeKindToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRangeClientCapabilitiesFoldingRangeKind& value)
{
	auto& obj = json.object();
	foldingRangeClientCapabilitiesFoldingRangeKindFromJson(obj, value);
}

json::Any toJson(FoldingRangeClientCapabilitiesFoldingRange&& value)
{
	json::Object obj;
	foldingRangeClientCapabilitiesFoldingRangeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRangeClientCapabilitiesFoldingRange& value)
{
	auto& obj = json.object();
	foldingRangeClientCapabilitiesFoldingRangeFromJson(obj, value);
}

json::Any toJson(FoldingRangeClientCapabilities&& value)
{
	json::Object obj;
	foldingRangeClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FoldingRangeClientCapabilities& value)
{
	auto& obj = json.object();
	foldingRangeClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(SelectionRangeClientCapabilities&& value)
{
	json::Object obj;
	selectionRangeClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SelectionRangeClientCapabilities& value)
{
	auto& obj = json.object();
	selectionRangeClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<PublishDiagnosticsClientCapabilitiesTagSupport>()
{
	static const char* properties[] = {
		"valueSet",
		nullptr
	};
	return properties;
}

json::Any toJson(PublishDiagnosticsClientCapabilitiesTagSupport&& value)
{
	json::Object obj;
	publishDiagnosticsClientCapabilitiesTagSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PublishDiagnosticsClientCapabilitiesTagSupport& value)
{
	auto& obj = json.object();
	publishDiagnosticsClientCapabilitiesTagSupportFromJson(obj, value);
}

json::Any toJson(PublishDiagnosticsClientCapabilities&& value)
{
	json::Object obj;
	publishDiagnosticsClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PublishDiagnosticsClientCapabilities& value)
{
	auto& obj = json.object();
	publishDiagnosticsClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(CallHierarchyClientCapabilities&& value)
{
	json::Object obj;
	callHierarchyClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CallHierarchyClientCapabilities& value)
{
	auto& obj = json.object();
	callHierarchyClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(SemanticTokensClientCapabilitiesRequestsRange&& value)
{
	json::Object obj;
	semanticTokensClientCapabilitiesRequestsRangeToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensClientCapabilitiesRequestsRange& value)
{
	auto& obj = json.object();
	semanticTokensClientCapabilitiesRequestsRangeFromJson(obj, value);
}

json::Any toJson(SemanticTokensClientCapabilitiesRequestsFull&& value)
{
	json::Object obj;
	semanticTokensClientCapabilitiesRequestsFullToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensClientCapabilitiesRequestsFull& value)
{
	auto& obj = json.object();
	semanticTokensClientCapabilitiesRequestsFullFromJson(obj, value);
}

json::Any toJson(SemanticTokensClientCapabilitiesRequests&& value)
{
	json::Object obj;
	semanticTokensClientCapabilitiesRequestsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensClientCapabilitiesRequests& value)
{
	auto& obj = json.object();
	semanticTokensClientCapabilitiesRequestsFromJson(obj, value);
}

template<>
const char** requiredProperties<SemanticTokensClientCapabilities>()
{
	static const char* properties[] = {
		"requests",
		"tokenTypes",
		"tokenModifiers",
		"formats",
		nullptr
	};
	return properties;
}

json::Any toJson(SemanticTokensClientCapabilities&& value)
{
	json::Object obj;
	semanticTokensClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SemanticTokensClientCapabilities& value)
{
	auto& obj = json.object();
	semanticTokensClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(LinkedEditingRangeClientCapabilities&& value)
{
	json::Object obj;
	linkedEditingRangeClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LinkedEditingRangeClientCapabilities& value)
{
	auto& obj = json.object();
	linkedEditingRangeClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(MonikerClientCapabilities&& value)
{
	json::Object obj;
	monikerClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MonikerClientCapabilities& value)
{
	auto& obj = json.object();
	monikerClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(TypeHierarchyClientCapabilities&& value)
{
	json::Object obj;
	typeHierarchyClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TypeHierarchyClientCapabilities& value)
{
	auto& obj = json.object();
	typeHierarchyClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(InlineValueClientCapabilities&& value)
{
	json::Object obj;
	inlineValueClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueClientCapabilities& value)
{
	auto& obj = json.object();
	inlineValueClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<InlayHintClientCapabilitiesResolveSupport>()
{
	static const char* properties[] = {
		"properties",
		nullptr
	};
	return properties;
}

json::Any toJson(InlayHintClientCapabilitiesResolveSupport&& value)
{
	json::Object obj;
	inlayHintClientCapabilitiesResolveSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHintClientCapabilitiesResolveSupport& value)
{
	auto& obj = json.object();
	inlayHintClientCapabilitiesResolveSupportFromJson(obj, value);
}

json::Any toJson(InlayHintClientCapabilities&& value)
{
	json::Object obj;
	inlayHintClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlayHintClientCapabilities& value)
{
	auto& obj = json.object();
	inlayHintClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(DiagnosticClientCapabilities&& value)
{
	json::Object obj;
	diagnosticClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DiagnosticClientCapabilities& value)
{
	auto& obj = json.object();
	diagnosticClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(InlineCompletionClientCapabilities&& value)
{
	json::Object obj;
	inlineCompletionClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineCompletionClientCapabilities& value)
{
	auto& obj = json.object();
	inlineCompletionClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(TextDocumentClientCapabilities&& value)
{
	json::Object obj;
	textDocumentClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentClientCapabilities& value)
{
	auto& obj = json.object();
	textDocumentClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(NotebookDocumentSyncClientCapabilities&& value)
{
	json::Object obj;
	notebookDocumentSyncClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentSyncClientCapabilities& value)
{
	auto& obj = json.object();
	notebookDocumentSyncClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentClientCapabilities>()
{
	static const char* properties[] = {
		"synchronization",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentClientCapabilities&& value)
{
	json::Object obj;
	notebookDocumentClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentClientCapabilities& value)
{
	auto& obj = json.object();
	notebookDocumentClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(ShowMessageRequestClientCapabilitiesMessageActionItem&& value)
{
	json::Object obj;
	showMessageRequestClientCapabilitiesMessageActionItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ShowMessageRequestClientCapabilitiesMessageActionItem& value)
{
	auto& obj = json.object();
	showMessageRequestClientCapabilitiesMessageActionItemFromJson(obj, value);
}

json::Any toJson(ShowMessageRequestClientCapabilities&& value)
{
	json::Object obj;
	showMessageRequestClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ShowMessageRequestClientCapabilities& value)
{
	auto& obj = json.object();
	showMessageRequestClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<ShowDocumentClientCapabilities>()
{
	static const char* properties[] = {
		"support",
		nullptr
	};
	return properties;
}

json::Any toJson(ShowDocumentClientCapabilities&& value)
{
	json::Object obj;
	showDocumentClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ShowDocumentClientCapabilities& value)
{
	auto& obj = json.object();
	showDocumentClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(WindowClientCapabilities&& value)
{
	json::Object obj;
	windowClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WindowClientCapabilities& value)
{
	auto& obj = json.object();
	windowClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<GeneralClientCapabilitiesStaleRequestSupport>()
{
	static const char* properties[] = {
		"cancel",
		"retryOnContentModified",
		nullptr
	};
	return properties;
}

json::Any toJson(GeneralClientCapabilitiesStaleRequestSupport&& value)
{
	json::Object obj;
	generalClientCapabilitiesStaleRequestSupportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, GeneralClientCapabilitiesStaleRequestSupport& value)
{
	auto& obj = json.object();
	generalClientCapabilitiesStaleRequestSupportFromJson(obj, value);
}

template<>
const char** requiredProperties<RegularExpressionsClientCapabilities>()
{
	static const char* properties[] = {
		"engine",
		nullptr
	};
	return properties;
}

json::Any toJson(RegularExpressionsClientCapabilities&& value)
{
	json::Object obj;
	regularExpressionsClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RegularExpressionsClientCapabilities& value)
{
	auto& obj = json.object();
	regularExpressionsClientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<MarkdownClientCapabilities>()
{
	static const char* properties[] = {
		"parser",
		nullptr
	};
	return properties;
}

json::Any toJson(MarkdownClientCapabilities&& value)
{
	json::Object obj;
	markdownClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MarkdownClientCapabilities& value)
{
	auto& obj = json.object();
	markdownClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(GeneralClientCapabilities&& value)
{
	json::Object obj;
	generalClientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, GeneralClientCapabilities& value)
{
	auto& obj = json.object();
	generalClientCapabilitiesFromJson(obj, value);
}

json::Any toJson(ClientCapabilities&& value)
{
	json::Object obj;
	clientCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ClientCapabilities& value)
{
	auto& obj = json.object();
	clientCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<_InitializeParamsClientInfo>()
{
	static const char* properties[] = {
		"name",
		nullptr
	};
	return properties;
}

json::Any toJson(_InitializeParamsClientInfo&& value)
{
	json::Object obj;
	_InitializeParamsClientInfoToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, _InitializeParamsClientInfo& value)
{
	auto& obj = json.object();
	_InitializeParamsClientInfoFromJson(obj, value);
}

template<>
const char** requiredProperties<_InitializeParams>()
{
	static const char* properties[] = {
		"processId",
		"rootUri",
		"capabilities",
		nullptr
	};
	return properties;
}

json::Any toJson(_InitializeParams&& value)
{
	json::Object obj;
	_InitializeParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, _InitializeParams& value)
{
	auto& obj = json.object();
	_InitializeParamsFromJson(obj, value);
}

json::Any toJson(WorkspaceFoldersInitializeParams&& value)
{
	json::Object obj;
	workspaceFoldersInitializeParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceFoldersInitializeParams& value)
{
	auto& obj = json.object();
	workspaceFoldersInitializeParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<InitializeParams>()
{
	static const char* properties[] = {
		"processId",
		"rootUri",
		"capabilities",
		nullptr
	};
	return properties;
}

json::Any toJson(InitializeParams&& value)
{
	json::Object obj;
	initializeParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InitializeParams& value)
{
	auto& obj = json.object();
	initializeParamsFromJson(obj, value);
}

json::Any toJson(SaveOptions&& value)
{
	json::Object obj;
	saveOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SaveOptions& value)
{
	auto& obj = json.object();
	saveOptionsFromJson(obj, value);
}

json::Any toJson(TextDocumentSyncOptions&& value)
{
	json::Object obj;
	textDocumentSyncOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentSyncOptions& value)
{
	auto& obj = json.object();
	textDocumentSyncOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_NotebookCells>()
{
	static const char* properties[] = {
		"language",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_NotebookCells&& value)
{
	json::Object obj;
	notebookDocumentSyncOptionsNotebookSelector_NotebookCellsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_NotebookCells& value)
{
	auto& obj = json.object();
	notebookDocumentSyncOptionsNotebookSelector_NotebookCellsFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_Notebook>()
{
	static const char* properties[] = {
		"notebook",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_Notebook&& value)
{
	json::Object obj;
	notebookDocumentSyncOptionsNotebookSelector_NotebookToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_Notebook& value)
{
	auto& obj = json.object();
	notebookDocumentSyncOptionsNotebookSelector_NotebookFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_CellsCells>()
{
	static const char* properties[] = {
		"language",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_CellsCells&& value)
{
	json::Object obj;
	notebookDocumentSyncOptionsNotebookSelector_CellsCellsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_CellsCells& value)
{
	auto& obj = json.object();
	notebookDocumentSyncOptionsNotebookSelector_CellsCellsFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentSyncOptionsNotebookSelector_Cells>()
{
	static const char* properties[] = {
		"cells",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentSyncOptionsNotebookSelector_Cells&& value)
{
	json::Object obj;
	notebookDocumentSyncOptionsNotebookSelector_CellsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentSyncOptionsNotebookSelector_Cells& value)
{
	auto& obj = json.object();
	notebookDocumentSyncOptionsNotebookSelector_CellsFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentSyncOptions>()
{
	static const char* properties[] = {
		"notebookSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentSyncOptions&& value)
{
	json::Object obj;
	notebookDocumentSyncOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentSyncOptions& value)
{
	auto& obj = json.object();
	notebookDocumentSyncOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<NotebookDocumentSyncRegistrationOptions>()
{
	static const char* properties[] = {
		"notebookSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(NotebookDocumentSyncRegistrationOptions&& value)
{
	json::Object obj;
	notebookDocumentSyncRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, NotebookDocumentSyncRegistrationOptions& value)
{
	auto& obj = json.object();
	notebookDocumentSyncRegistrationOptionsFromJson(obj, value);
}

json::Any toJson(CompletionOptionsCompletionItem&& value)
{
	json::Object obj;
	completionOptionsCompletionItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionOptionsCompletionItem& value)
{
	auto& obj = json.object();
	completionOptionsCompletionItemFromJson(obj, value);
}

json::Any toJson(CompletionOptions&& value)
{
	json::Object obj;
	completionOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionOptions& value)
{
	auto& obj = json.object();
	completionOptionsFromJson(obj, value);
}

json::Any toJson(HoverOptions&& value)
{
	json::Object obj;
	hoverOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, HoverOptions& value)
{
	auto& obj = json.object();
	hoverOptionsFromJson(obj, value);
}

json::Any toJson(SignatureHelpOptions&& value)
{
	json::Object obj;
	signatureHelpOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelpOptions& value)
{
	auto& obj = json.object();
	signatureHelpOptionsFromJson(obj, value);
}

json::Any toJson(DefinitionOptions&& value)
{
	json::Object obj;
	definitionOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DefinitionOptions& value)
{
	auto& obj = json.object();
	definitionOptionsFromJson(obj, value);
}

json::Any toJson(ReferenceOptions&& value)
{
	json::Object obj;
	referenceOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ReferenceOptions& value)
{
	auto& obj = json.object();
	referenceOptionsFromJson(obj, value);
}

json::Any toJson(DocumentHighlightOptions&& value)
{
	json::Object obj;
	documentHighlightOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentHighlightOptions& value)
{
	auto& obj = json.object();
	documentHighlightOptionsFromJson(obj, value);
}

json::Any toJson(DocumentSymbolOptions&& value)
{
	json::Object obj;
	documentSymbolOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentSymbolOptions& value)
{
	auto& obj = json.object();
	documentSymbolOptionsFromJson(obj, value);
}

json::Any toJson(CodeActionOptions&& value)
{
	json::Object obj;
	codeActionOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionOptions& value)
{
	auto& obj = json.object();
	codeActionOptionsFromJson(obj, value);
}

json::Any toJson(CodeLensOptions&& value)
{
	json::Object obj;
	codeLensOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeLensOptions& value)
{
	auto& obj = json.object();
	codeLensOptionsFromJson(obj, value);
}

json::Any toJson(DocumentLinkOptions&& value)
{
	json::Object obj;
	documentLinkOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentLinkOptions& value)
{
	auto& obj = json.object();
	documentLinkOptionsFromJson(obj, value);
}

json::Any toJson(WorkspaceSymbolOptions&& value)
{
	json::Object obj;
	workspaceSymbolOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolOptions& value)
{
	auto& obj = json.object();
	workspaceSymbolOptionsFromJson(obj, value);
}

json::Any toJson(DocumentFormattingOptions&& value)
{
	json::Object obj;
	documentFormattingOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentFormattingOptions& value)
{
	auto& obj = json.object();
	documentFormattingOptionsFromJson(obj, value);
}

json::Any toJson(DocumentRangeFormattingOptions&& value)
{
	json::Object obj;
	documentRangeFormattingOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentRangeFormattingOptions& value)
{
	auto& obj = json.object();
	documentRangeFormattingOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentOnTypeFormattingOptions>()
{
	static const char* properties[] = {
		"firstTriggerCharacter",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentOnTypeFormattingOptions&& value)
{
	json::Object obj;
	documentOnTypeFormattingOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentOnTypeFormattingOptions& value)
{
	auto& obj = json.object();
	documentOnTypeFormattingOptionsFromJson(obj, value);
}

json::Any toJson(RenameOptions&& value)
{
	json::Object obj;
	renameOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RenameOptions& value)
{
	auto& obj = json.object();
	renameOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<ExecuteCommandOptions>()
{
	static const char* properties[] = {
		"commands",
		nullptr
	};
	return properties;
}

json::Any toJson(ExecuteCommandOptions&& value)
{
	json::Object obj;
	executeCommandOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ExecuteCommandOptions& value)
{
	auto& obj = json.object();
	executeCommandOptionsFromJson(obj, value);
}

json::Any toJson(WorkspaceFoldersServerCapabilities&& value)
{
	json::Object obj;
	workspaceFoldersServerCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceFoldersServerCapabilities& value)
{
	auto& obj = json.object();
	workspaceFoldersServerCapabilitiesFromJson(obj, value);
}

json::Any toJson(FileOperationOptions&& value)
{
	json::Object obj;
	fileOperationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileOperationOptions& value)
{
	auto& obj = json.object();
	fileOperationOptionsFromJson(obj, value);
}

json::Any toJson(ServerCapabilitiesWorkspace&& value)
{
	json::Object obj;
	serverCapabilitiesWorkspaceToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ServerCapabilitiesWorkspace& value)
{
	auto& obj = json.object();
	serverCapabilitiesWorkspaceFromJson(obj, value);
}

json::Any toJson(ServerCapabilities&& value)
{
	json::Object obj;
	serverCapabilitiesToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ServerCapabilities& value)
{
	auto& obj = json.object();
	serverCapabilitiesFromJson(obj, value);
}

template<>
const char** requiredProperties<InitializeResultServerInfo>()
{
	static const char* properties[] = {
		"name",
		nullptr
	};
	return properties;
}

json::Any toJson(InitializeResultServerInfo&& value)
{
	json::Object obj;
	initializeResultServerInfoToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InitializeResultServerInfo& value)
{
	auto& obj = json.object();
	initializeResultServerInfoFromJson(obj, value);
}

template<>
const char** requiredProperties<InitializeResult>()
{
	static const char* properties[] = {
		"capabilities",
		nullptr
	};
	return properties;
}

json::Any toJson(InitializeResult&& value)
{
	json::Object obj;
	initializeResultToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InitializeResult& value)
{
	auto& obj = json.object();
	initializeResultFromJson(obj, value);
}

template<>
const char** requiredProperties<InitializeError>()
{
	static const char* properties[] = {
		"retry",
		nullptr
	};
	return properties;
}

json::Any toJson(InitializeError&& value)
{
	json::Object obj;
	initializeErrorToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InitializeError& value)
{
	auto& obj = json.object();
	initializeErrorFromJson(obj, value);
}

json::Any toJson(InitializedParams&& value)
{
	json::Object obj;
	initializedParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InitializedParams& value)
{
	auto& obj = json.object();
	initializedParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DidChangeConfigurationParams>()
{
	static const char* properties[] = {
		"settings",
		nullptr
	};
	return properties;
}

json::Any toJson(DidChangeConfigurationParams&& value)
{
	json::Object obj;
	didChangeConfigurationParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeConfigurationParams& value)
{
	auto& obj = json.object();
	didChangeConfigurationParamsFromJson(obj, value);
}

json::Any toJson(DidChangeConfigurationRegistrationOptions&& value)
{
	json::Object obj;
	didChangeConfigurationRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeConfigurationRegistrationOptions& value)
{
	auto& obj = json.object();
	didChangeConfigurationRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<ShowMessageParams>()
{
	static const char* properties[] = {
		"type",
		"message",
		nullptr
	};
	return properties;
}

json::Any toJson(ShowMessageParams&& value)
{
	json::Object obj;
	showMessageParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ShowMessageParams& value)
{
	auto& obj = json.object();
	showMessageParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<MessageActionItem>()
{
	static const char* properties[] = {
		"title",
		nullptr
	};
	return properties;
}

json::Any toJson(MessageActionItem&& value)
{
	json::Object obj;
	messageActionItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MessageActionItem& value)
{
	auto& obj = json.object();
	messageActionItemFromJson(obj, value);
}

template<>
const char** requiredProperties<ShowMessageRequestParams>()
{
	static const char* properties[] = {
		"type",
		"message",
		nullptr
	};
	return properties;
}

json::Any toJson(ShowMessageRequestParams&& value)
{
	json::Object obj;
	showMessageRequestParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ShowMessageRequestParams& value)
{
	auto& obj = json.object();
	showMessageRequestParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<LogMessageParams>()
{
	static const char* properties[] = {
		"type",
		"message",
		nullptr
	};
	return properties;
}

json::Any toJson(LogMessageParams&& value)
{
	json::Object obj;
	logMessageParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LogMessageParams& value)
{
	auto& obj = json.object();
	logMessageParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DidOpenTextDocumentParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DidOpenTextDocumentParams&& value)
{
	json::Object obj;
	didOpenTextDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidOpenTextDocumentParams& value)
{
	auto& obj = json.object();
	didOpenTextDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DidChangeTextDocumentParams>()
{
	static const char* properties[] = {
		"textDocument",
		"contentChanges",
		nullptr
	};
	return properties;
}

json::Any toJson(DidChangeTextDocumentParams&& value)
{
	json::Object obj;
	didChangeTextDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeTextDocumentParams& value)
{
	auto& obj = json.object();
	didChangeTextDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentChangeRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		"syncKind",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentChangeRegistrationOptions&& value)
{
	json::Object obj;
	textDocumentChangeRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentChangeRegistrationOptions& value)
{
	auto& obj = json.object();
	textDocumentChangeRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DidCloseTextDocumentParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DidCloseTextDocumentParams&& value)
{
	json::Object obj;
	didCloseTextDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidCloseTextDocumentParams& value)
{
	auto& obj = json.object();
	didCloseTextDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DidSaveTextDocumentParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DidSaveTextDocumentParams&& value)
{
	json::Object obj;
	didSaveTextDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidSaveTextDocumentParams& value)
{
	auto& obj = json.object();
	didSaveTextDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<TextDocumentSaveRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(TextDocumentSaveRegistrationOptions&& value)
{
	json::Object obj;
	textDocumentSaveRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, TextDocumentSaveRegistrationOptions& value)
{
	auto& obj = json.object();
	textDocumentSaveRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<WillSaveTextDocumentParams>()
{
	static const char* properties[] = {
		"textDocument",
		"reason",
		nullptr
	};
	return properties;
}

json::Any toJson(WillSaveTextDocumentParams&& value)
{
	json::Object obj;
	willSaveTextDocumentParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WillSaveTextDocumentParams& value)
{
	auto& obj = json.object();
	willSaveTextDocumentParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<FileEvent>()
{
	static const char* properties[] = {
		"uri",
		"type",
		nullptr
	};
	return properties;
}

json::Any toJson(FileEvent&& value)
{
	json::Object obj;
	fileEventToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileEvent& value)
{
	auto& obj = json.object();
	fileEventFromJson(obj, value);
}

template<>
const char** requiredProperties<DidChangeWatchedFilesParams>()
{
	static const char* properties[] = {
		"changes",
		nullptr
	};
	return properties;
}

json::Any toJson(DidChangeWatchedFilesParams&& value)
{
	json::Object obj;
	didChangeWatchedFilesParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeWatchedFilesParams& value)
{
	auto& obj = json.object();
	didChangeWatchedFilesParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<RelativePattern>()
{
	static const char* properties[] = {
		"baseUri",
		"pattern",
		nullptr
	};
	return properties;
}

json::Any toJson(RelativePattern&& value)
{
	json::Object obj;
	relativePatternToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RelativePattern& value)
{
	auto& obj = json.object();
	relativePatternFromJson(obj, value);
}

template<>
const char** requiredProperties<FileSystemWatcher>()
{
	static const char* properties[] = {
		"globPattern",
		nullptr
	};
	return properties;
}

json::Any toJson(FileSystemWatcher&& value)
{
	json::Object obj;
	fileSystemWatcherToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FileSystemWatcher& value)
{
	auto& obj = json.object();
	fileSystemWatcherFromJson(obj, value);
}

template<>
const char** requiredProperties<DidChangeWatchedFilesRegistrationOptions>()
{
	static const char* properties[] = {
		"watchers",
		nullptr
	};
	return properties;
}

json::Any toJson(DidChangeWatchedFilesRegistrationOptions&& value)
{
	json::Object obj;
	didChangeWatchedFilesRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DidChangeWatchedFilesRegistrationOptions& value)
{
	auto& obj = json.object();
	didChangeWatchedFilesRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<PublishDiagnosticsParams>()
{
	static const char* properties[] = {
		"uri",
		"diagnostics",
		nullptr
	};
	return properties;
}

json::Any toJson(PublishDiagnosticsParams&& value)
{
	json::Object obj;
	publishDiagnosticsParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PublishDiagnosticsParams& value)
{
	auto& obj = json.object();
	publishDiagnosticsParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionContext>()
{
	static const char* properties[] = {
		"triggerKind",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionContext&& value)
{
	json::Object obj;
	completionContextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionContext& value)
{
	auto& obj = json.object();
	completionContextFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionParams&& value)
{
	json::Object obj;
	completionParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionParams& value)
{
	auto& obj = json.object();
	completionParamsFromJson(obj, value);
}

json::Any toJson(CompletionItemLabelDetails&& value)
{
	json::Object obj;
	completionItemLabelDetailsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionItemLabelDetails& value)
{
	auto& obj = json.object();
	completionItemLabelDetailsFromJson(obj, value);
}

template<>
const char** requiredProperties<InsertReplaceEdit>()
{
	static const char* properties[] = {
		"newText",
		"insert",
		"replace",
		nullptr
	};
	return properties;
}

json::Any toJson(InsertReplaceEdit&& value)
{
	json::Object obj;
	insertReplaceEditToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InsertReplaceEdit& value)
{
	auto& obj = json.object();
	insertReplaceEditFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionItem>()
{
	static const char* properties[] = {
		"label",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionItem&& value)
{
	json::Object obj;
	completionItemToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionItem& value)
{
	auto& obj = json.object();
	completionItemFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionListItemDefaultsEditRange_Insert_Replace>()
{
	static const char* properties[] = {
		"insert",
		"replace",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionListItemDefaultsEditRange_Insert_Replace&& value)
{
	json::Object obj;
	completionListItemDefaultsEditRange_Insert_ReplaceToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionListItemDefaultsEditRange_Insert_Replace& value)
{
	auto& obj = json.object();
	completionListItemDefaultsEditRange_Insert_ReplaceFromJson(obj, value);
}

json::Any toJson(CompletionListItemDefaults&& value)
{
	json::Object obj;
	completionListItemDefaultsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionListItemDefaults& value)
{
	auto& obj = json.object();
	completionListItemDefaultsFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionList>()
{
	static const char* properties[] = {
		"isIncomplete",
		"items",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionList&& value)
{
	json::Object obj;
	completionListToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionList& value)
{
	auto& obj = json.object();
	completionListFromJson(obj, value);
}

template<>
const char** requiredProperties<CompletionRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(CompletionRegistrationOptions&& value)
{
	json::Object obj;
	completionRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CompletionRegistrationOptions& value)
{
	auto& obj = json.object();
	completionRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<HoverParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(HoverParams&& value)
{
	json::Object obj;
	hoverParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, HoverParams& value)
{
	auto& obj = json.object();
	hoverParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<MarkedString_Language_Value>()
{
	static const char* properties[] = {
		"language",
		"value",
		nullptr
	};
	return properties;
}

json::Any toJson(MarkedString_Language_Value&& value)
{
	json::Object obj;
	markedString_Language_ValueToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, MarkedString_Language_Value& value)
{
	auto& obj = json.object();
	markedString_Language_ValueFromJson(obj, value);
}

template<>
const char** requiredProperties<Hover>()
{
	static const char* properties[] = {
		"contents",
		nullptr
	};
	return properties;
}

json::Any toJson(Hover&& value)
{
	json::Object obj;
	hoverToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, Hover& value)
{
	auto& obj = json.object();
	hoverFromJson(obj, value);
}

template<>
const char** requiredProperties<HoverRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(HoverRegistrationOptions&& value)
{
	json::Object obj;
	hoverRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, HoverRegistrationOptions& value)
{
	auto& obj = json.object();
	hoverRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<ParameterInformation>()
{
	static const char* properties[] = {
		"label",
		nullptr
	};
	return properties;
}

json::Any toJson(ParameterInformation&& value)
{
	json::Object obj;
	parameterInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ParameterInformation& value)
{
	auto& obj = json.object();
	parameterInformationFromJson(obj, value);
}

template<>
const char** requiredProperties<SignatureInformation>()
{
	static const char* properties[] = {
		"label",
		nullptr
	};
	return properties;
}

json::Any toJson(SignatureInformation&& value)
{
	json::Object obj;
	signatureInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureInformation& value)
{
	auto& obj = json.object();
	signatureInformationFromJson(obj, value);
}

template<>
const char** requiredProperties<SignatureHelp>()
{
	static const char* properties[] = {
		"signatures",
		nullptr
	};
	return properties;
}

json::Any toJson(SignatureHelp&& value)
{
	json::Object obj;
	signatureHelpToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelp& value)
{
	auto& obj = json.object();
	signatureHelpFromJson(obj, value);
}

template<>
const char** requiredProperties<SignatureHelpContext>()
{
	static const char* properties[] = {
		"triggerKind",
		"isRetrigger",
		nullptr
	};
	return properties;
}

json::Any toJson(SignatureHelpContext&& value)
{
	json::Object obj;
	signatureHelpContextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelpContext& value)
{
	auto& obj = json.object();
	signatureHelpContextFromJson(obj, value);
}

template<>
const char** requiredProperties<SignatureHelpParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(SignatureHelpParams&& value)
{
	json::Object obj;
	signatureHelpParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelpParams& value)
{
	auto& obj = json.object();
	signatureHelpParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<SignatureHelpRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(SignatureHelpRegistrationOptions&& value)
{
	json::Object obj;
	signatureHelpRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SignatureHelpRegistrationOptions& value)
{
	auto& obj = json.object();
	signatureHelpRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DefinitionParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(DefinitionParams&& value)
{
	json::Object obj;
	definitionParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DefinitionParams& value)
{
	auto& obj = json.object();
	definitionParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DefinitionRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DefinitionRegistrationOptions&& value)
{
	json::Object obj;
	definitionRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DefinitionRegistrationOptions& value)
{
	auto& obj = json.object();
	definitionRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<ReferenceContext>()
{
	static const char* properties[] = {
		"includeDeclaration",
		nullptr
	};
	return properties;
}

json::Any toJson(ReferenceContext&& value)
{
	json::Object obj;
	referenceContextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ReferenceContext& value)
{
	auto& obj = json.object();
	referenceContextFromJson(obj, value);
}

template<>
const char** requiredProperties<ReferenceParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		"context",
		nullptr
	};
	return properties;
}

json::Any toJson(ReferenceParams&& value)
{
	json::Object obj;
	referenceParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ReferenceParams& value)
{
	auto& obj = json.object();
	referenceParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ReferenceRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(ReferenceRegistrationOptions&& value)
{
	json::Object obj;
	referenceRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ReferenceRegistrationOptions& value)
{
	auto& obj = json.object();
	referenceRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentHighlightParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentHighlightParams&& value)
{
	json::Object obj;
	documentHighlightParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentHighlightParams& value)
{
	auto& obj = json.object();
	documentHighlightParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentHighlight>()
{
	static const char* properties[] = {
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentHighlight&& value)
{
	json::Object obj;
	documentHighlightToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentHighlight& value)
{
	auto& obj = json.object();
	documentHighlightFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentHighlightRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentHighlightRegistrationOptions&& value)
{
	json::Object obj;
	documentHighlightRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentHighlightRegistrationOptions& value)
{
	auto& obj = json.object();
	documentHighlightRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentSymbolParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentSymbolParams&& value)
{
	json::Object obj;
	documentSymbolParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentSymbolParams& value)
{
	auto& obj = json.object();
	documentSymbolParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<BaseSymbolInformation>()
{
	static const char* properties[] = {
		"name",
		"kind",
		nullptr
	};
	return properties;
}

json::Any toJson(BaseSymbolInformation&& value)
{
	json::Object obj;
	baseSymbolInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, BaseSymbolInformation& value)
{
	auto& obj = json.object();
	baseSymbolInformationFromJson(obj, value);
}

template<>
const char** requiredProperties<SymbolInformation>()
{
	static const char* properties[] = {
		"name",
		"kind",
		"location",
		nullptr
	};
	return properties;
}

json::Any toJson(SymbolInformation&& value)
{
	json::Object obj;
	symbolInformationToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SymbolInformation& value)
{
	auto& obj = json.object();
	symbolInformationFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentSymbol>()
{
	static const char* properties[] = {
		"name",
		"kind",
		"range",
		"selectionRange",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentSymbol&& value)
{
	json::Object obj;
	documentSymbolToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentSymbol& value)
{
	auto& obj = json.object();
	documentSymbolFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentSymbolRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentSymbolRegistrationOptions&& value)
{
	json::Object obj;
	documentSymbolRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentSymbolRegistrationOptions& value)
{
	auto& obj = json.object();
	documentSymbolRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeActionContext>()
{
	static const char* properties[] = {
		"diagnostics",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeActionContext&& value)
{
	json::Object obj;
	codeActionContextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionContext& value)
{
	auto& obj = json.object();
	codeActionContextFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeActionParams>()
{
	static const char* properties[] = {
		"textDocument",
		"range",
		"context",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeActionParams&& value)
{
	json::Object obj;
	codeActionParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionParams& value)
{
	auto& obj = json.object();
	codeActionParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeActionDisabled>()
{
	static const char* properties[] = {
		"reason",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeActionDisabled&& value)
{
	json::Object obj;
	codeActionDisabledToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionDisabled& value)
{
	auto& obj = json.object();
	codeActionDisabledFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeAction>()
{
	static const char* properties[] = {
		"title",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeAction&& value)
{
	json::Object obj;
	codeActionToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeAction& value)
{
	auto& obj = json.object();
	codeActionFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeActionRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeActionRegistrationOptions&& value)
{
	json::Object obj;
	codeActionRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeActionRegistrationOptions& value)
{
	auto& obj = json.object();
	codeActionRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceSymbolParams>()
{
	static const char* properties[] = {
		"query",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceSymbolParams&& value)
{
	json::Object obj;
	workspaceSymbolParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolParams& value)
{
	auto& obj = json.object();
	workspaceSymbolParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceSymbolLocation_Uri>()
{
	static const char* properties[] = {
		"uri",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceSymbolLocation_Uri&& value)
{
	json::Object obj;
	workspaceSymbolLocation_UriToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolLocation_Uri& value)
{
	auto& obj = json.object();
	workspaceSymbolLocation_UriFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkspaceSymbol>()
{
	static const char* properties[] = {
		"name",
		"kind",
		"location",
		nullptr
	};
	return properties;
}

json::Any toJson(WorkspaceSymbol&& value)
{
	json::Object obj;
	workspaceSymbolToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbol& value)
{
	auto& obj = json.object();
	workspaceSymbolFromJson(obj, value);
}

json::Any toJson(WorkspaceSymbolRegistrationOptions&& value)
{
	json::Object obj;
	workspaceSymbolRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkspaceSymbolRegistrationOptions& value)
{
	auto& obj = json.object();
	workspaceSymbolRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeLensParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeLensParams&& value)
{
	json::Object obj;
	codeLensParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeLensParams& value)
{
	auto& obj = json.object();
	codeLensParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeLens>()
{
	static const char* properties[] = {
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeLens&& value)
{
	json::Object obj;
	codeLensToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeLens& value)
{
	auto& obj = json.object();
	codeLensFromJson(obj, value);
}

template<>
const char** requiredProperties<CodeLensRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(CodeLensRegistrationOptions&& value)
{
	json::Object obj;
	codeLensRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CodeLensRegistrationOptions& value)
{
	auto& obj = json.object();
	codeLensRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentLinkParams>()
{
	static const char* properties[] = {
		"textDocument",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentLinkParams&& value)
{
	json::Object obj;
	documentLinkParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentLinkParams& value)
{
	auto& obj = json.object();
	documentLinkParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentLink>()
{
	static const char* properties[] = {
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentLink&& value)
{
	json::Object obj;
	documentLinkToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentLink& value)
{
	auto& obj = json.object();
	documentLinkFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentLinkRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentLinkRegistrationOptions&& value)
{
	json::Object obj;
	documentLinkRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentLinkRegistrationOptions& value)
{
	auto& obj = json.object();
	documentLinkRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<FormattingOptions>()
{
	static const char* properties[] = {
		"tabSize",
		"insertSpaces",
		nullptr
	};
	return properties;
}

json::Any toJson(FormattingOptions&& value)
{
	json::Object obj;
	formattingOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, FormattingOptions& value)
{
	auto& obj = json.object();
	formattingOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentFormattingParams>()
{
	static const char* properties[] = {
		"textDocument",
		"options",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentFormattingParams&& value)
{
	json::Object obj;
	documentFormattingParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentFormattingParams& value)
{
	auto& obj = json.object();
	documentFormattingParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentFormattingRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentFormattingRegistrationOptions&& value)
{
	json::Object obj;
	documentFormattingRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentFormattingRegistrationOptions& value)
{
	auto& obj = json.object();
	documentFormattingRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentRangeFormattingParams>()
{
	static const char* properties[] = {
		"textDocument",
		"range",
		"options",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentRangeFormattingParams&& value)
{
	json::Object obj;
	documentRangeFormattingParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentRangeFormattingParams& value)
{
	auto& obj = json.object();
	documentRangeFormattingParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentRangeFormattingRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentRangeFormattingRegistrationOptions&& value)
{
	json::Object obj;
	documentRangeFormattingRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentRangeFormattingRegistrationOptions& value)
{
	auto& obj = json.object();
	documentRangeFormattingRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentRangesFormattingParams>()
{
	static const char* properties[] = {
		"textDocument",
		"ranges",
		"options",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentRangesFormattingParams&& value)
{
	json::Object obj;
	documentRangesFormattingParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentRangesFormattingParams& value)
{
	auto& obj = json.object();
	documentRangesFormattingParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentOnTypeFormattingParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		"ch",
		"options",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentOnTypeFormattingParams&& value)
{
	json::Object obj;
	documentOnTypeFormattingParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentOnTypeFormattingParams& value)
{
	auto& obj = json.object();
	documentOnTypeFormattingParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<DocumentOnTypeFormattingRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		"firstTriggerCharacter",
		nullptr
	};
	return properties;
}

json::Any toJson(DocumentOnTypeFormattingRegistrationOptions&& value)
{
	json::Object obj;
	documentOnTypeFormattingRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, DocumentOnTypeFormattingRegistrationOptions& value)
{
	auto& obj = json.object();
	documentOnTypeFormattingRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<RenameParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		"newName",
		nullptr
	};
	return properties;
}

json::Any toJson(RenameParams&& value)
{
	json::Object obj;
	renameParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RenameParams& value)
{
	auto& obj = json.object();
	renameParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<RenameRegistrationOptions>()
{
	static const char* properties[] = {
		"documentSelector",
		nullptr
	};
	return properties;
}

json::Any toJson(RenameRegistrationOptions&& value)
{
	json::Object obj;
	renameRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RenameRegistrationOptions& value)
{
	auto& obj = json.object();
	renameRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<PrepareRenameParams>()
{
	static const char* properties[] = {
		"textDocument",
		"position",
		nullptr
	};
	return properties;
}

json::Any toJson(PrepareRenameParams&& value)
{
	json::Object obj;
	prepareRenameParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PrepareRenameParams& value)
{
	auto& obj = json.object();
	prepareRenameParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ExecuteCommandParams>()
{
	static const char* properties[] = {
		"command",
		nullptr
	};
	return properties;
}

json::Any toJson(ExecuteCommandParams&& value)
{
	json::Object obj;
	executeCommandParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ExecuteCommandParams& value)
{
	auto& obj = json.object();
	executeCommandParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ExecuteCommandRegistrationOptions>()
{
	static const char* properties[] = {
		"commands",
		nullptr
	};
	return properties;
}

json::Any toJson(ExecuteCommandRegistrationOptions&& value)
{
	json::Object obj;
	executeCommandRegistrationOptionsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ExecuteCommandRegistrationOptions& value)
{
	auto& obj = json.object();
	executeCommandRegistrationOptionsFromJson(obj, value);
}

template<>
const char** requiredProperties<ApplyWorkspaceEditParams>()
{
	static const char* properties[] = {
		"edit",
		nullptr
	};
	return properties;
}

json::Any toJson(ApplyWorkspaceEditParams&& value)
{
	json::Object obj;
	applyWorkspaceEditParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ApplyWorkspaceEditParams& value)
{
	auto& obj = json.object();
	applyWorkspaceEditParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ApplyWorkspaceEditResult>()
{
	static const char* properties[] = {
		"applied",
		nullptr
	};
	return properties;
}

json::Any toJson(ApplyWorkspaceEditResult&& value)
{
	json::Object obj;
	applyWorkspaceEditResultToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ApplyWorkspaceEditResult& value)
{
	auto& obj = json.object();
	applyWorkspaceEditResultFromJson(obj, value);
}

template<>
const char** requiredProperties<WorkDoneProgressBegin>()
{
	static const char* properties[] = {
		"title",
		nullptr
	};
	return properties;
}

template<>
const std::pair<const char*, json::Any>* literalProperties<WorkDoneProgressBegin>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "begin"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(WorkDoneProgressBegin&& value)
{
	json::Object obj;
	workDoneProgressBeginToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkDoneProgressBegin& value)
{
	auto& obj = json.object();
	workDoneProgressBeginFromJson(obj, value);
}

template<>
const std::pair<const char*, json::Any>* literalProperties<WorkDoneProgressReport>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "report"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(WorkDoneProgressReport&& value)
{
	json::Object obj;
	workDoneProgressReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkDoneProgressReport& value)
{
	auto& obj = json.object();
	workDoneProgressReportFromJson(obj, value);
}

template<>
const std::pair<const char*, json::Any>* literalProperties<WorkDoneProgressEnd>()
{
	static const std::pair<const char*, json::Any> properties[] = {
		{"kind", "end"},
		{nullptr, {}}
	};
	return properties;
}

json::Any toJson(WorkDoneProgressEnd&& value)
{
	json::Object obj;
	workDoneProgressEndToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, WorkDoneProgressEnd& value)
{
	auto& obj = json.object();
	workDoneProgressEndFromJson(obj, value);
}

template<>
const char** requiredProperties<SetTraceParams>()
{
	static const char* properties[] = {
		"value",
		nullptr
	};
	return properties;
}

json::Any toJson(SetTraceParams&& value)
{
	json::Object obj;
	setTraceParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, SetTraceParams& value)
{
	auto& obj = json.object();
	setTraceParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<LogTraceParams>()
{
	static const char* properties[] = {
		"message",
		nullptr
	};
	return properties;
}

json::Any toJson(LogTraceParams&& value)
{
	json::Object obj;
	logTraceParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LogTraceParams& value)
{
	auto& obj = json.object();
	logTraceParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<CancelParams>()
{
	static const char* properties[] = {
		"id",
		nullptr
	};
	return properties;
}

json::Any toJson(CancelParams&& value)
{
	json::Object obj;
	cancelParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, CancelParams& value)
{
	auto& obj = json.object();
	cancelParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<ProgressParams>()
{
	static const char* properties[] = {
		"token",
		"value",
		nullptr
	};
	return properties;
}

json::Any toJson(ProgressParams&& value)
{
	json::Object obj;
	progressParamsToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, ProgressParams& value)
{
	auto& obj = json.object();
	progressParamsFromJson(obj, value);
}

template<>
const char** requiredProperties<LocationLink>()
{
	static const char* properties[] = {
		"targetUri",
		"targetRange",
		"targetSelectionRange",
		nullptr
	};
	return properties;
}

json::Any toJson(LocationLink&& value)
{
	json::Object obj;
	locationLinkToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, LocationLink& value)
{
	auto& obj = json.object();
	locationLinkFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineValueText>()
{
	static const char* properties[] = {
		"range",
		"text",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineValueText&& value)
{
	json::Object obj;
	inlineValueTextToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueText& value)
{
	auto& obj = json.object();
	inlineValueTextFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineValueVariableLookup>()
{
	static const char* properties[] = {
		"range",
		"caseSensitiveLookup",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineValueVariableLookup&& value)
{
	json::Object obj;
	inlineValueVariableLookupToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueVariableLookup& value)
{
	auto& obj = json.object();
	inlineValueVariableLookupFromJson(obj, value);
}

template<>
const char** requiredProperties<InlineValueEvaluatableExpression>()
{
	static const char* properties[] = {
		"range",
		nullptr
	};
	return properties;
}

json::Any toJson(InlineValueEvaluatableExpression&& value)
{
	json::Object obj;
	inlineValueEvaluatableExpressionToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, InlineValueEvaluatableExpression& value)
{
	auto& obj = json.object();
	inlineValueEvaluatableExpressionFromJson(obj, value);
}

template<>
const char** requiredProperties<RelatedFullDocumentDiagnosticReport>()
{
	static const char* properties[] = {
		"kind",
		"items",
		nullptr
	};
	return properties;
}

json::Any toJson(RelatedFullDocumentDiagnosticReport&& value)
{
	json::Object obj;
	relatedFullDocumentDiagnosticReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RelatedFullDocumentDiagnosticReport& value)
{
	auto& obj = json.object();
	relatedFullDocumentDiagnosticReportFromJson(obj, value);
}

template<>
const char** requiredProperties<RelatedUnchangedDocumentDiagnosticReport>()
{
	static const char* properties[] = {
		"kind",
		"resultId",
		nullptr
	};
	return properties;
}

json::Any toJson(RelatedUnchangedDocumentDiagnosticReport&& value)
{
	json::Object obj;
	relatedUnchangedDocumentDiagnosticReportToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, RelatedUnchangedDocumentDiagnosticReport& value)
{
	auto& obj = json.object();
	relatedUnchangedDocumentDiagnosticReportFromJson(obj, value);
}

template<>
const char** requiredProperties<PrepareRenameResult_Range_Placeholder>()
{
	static const char* properties[] = {
		"range",
		"placeholder",
		nullptr
	};
	return properties;
}

json::Any toJson(PrepareRenameResult_Range_Placeholder&& value)
{
	json::Object obj;
	prepareRenameResult_Range_PlaceholderToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PrepareRenameResult_Range_Placeholder& value)
{
	auto& obj = json.object();
	prepareRenameResult_Range_PlaceholderFromJson(obj, value);
}

template<>
const char** requiredProperties<PrepareRenameResult_DefaultBehavior>()
{
	static const char* properties[] = {
		"defaultBehavior",
		nullptr
	};
	return properties;
}

json::Any toJson(PrepareRenameResult_DefaultBehavior&& value)
{
	json::Object obj;
	prepareRenameResult_DefaultBehaviorToJson(value, obj);
	return obj;
}

void fromJson(json::Any&& json, PrepareRenameResult_DefaultBehavior& value)
{
	auto& obj = json.object();
	prepareRenameResult_DefaultBehaviorFromJson(obj, value);
}

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
} // namespace lsp
