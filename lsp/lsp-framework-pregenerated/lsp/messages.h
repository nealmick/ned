#pragma once

/*#############################################################
 * NOTE: This is a generated file and it shouldn't be modified!
 *#############################################################*/

#include <lsp/messagebase.h>
#include "types.h"

namespace lsp{

/*
 * Request messages
 */
namespace requests{

/*
 * callHierarchy/incomingCalls
 *
 * A request to resolve the incoming calls for a given `CallHierarchyItem`.
 * 
 * @since 3.16.0
 */
struct CallHierarchy_IncomingCalls{
	static constexpr auto Method    = std::string_view("callHierarchy/incomingCalls");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using PartialResult = CallHierarchy_IncomingCallsPartialResult;
	using Params = CallHierarchyIncomingCallsParams;
	using Result = CallHierarchy_IncomingCallsResult;
};

/*
 * callHierarchy/outgoingCalls
 *
 * A request to resolve the outgoing calls for a given `CallHierarchyItem`.
 * 
 * @since 3.16.0
 */
struct CallHierarchy_OutgoingCalls{
	static constexpr auto Method    = std::string_view("callHierarchy/outgoingCalls");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using PartialResult = CallHierarchy_OutgoingCallsPartialResult;
	using Params = CallHierarchyOutgoingCallsParams;
	using Result = CallHierarchy_OutgoingCallsResult;
};

/*
 * client/registerCapability
 *
 * The `client/registerCapability` request is sent from the server to the client to register a new capability
 * handler on the client side.
 */
struct Client_RegisterCapability{
	static constexpr auto Method    = std::string_view("client/registerCapability");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Params = RegistrationParams;
	using Result = Client_RegisterCapabilityResult;
};

/*
 * client/unregisterCapability
 *
 * The `client/unregisterCapability` request is sent from the server to the client to unregister a previously registered capability
 * handler on the client side.
 */
struct Client_UnregisterCapability{
	static constexpr auto Method    = std::string_view("client/unregisterCapability");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Params = UnregistrationParams;
	using Result = Client_UnregisterCapabilityResult;
};

/*
 * codeAction/resolve
 *
 * Request to resolve additional information for a given code action.The request's
 * parameter is of type {@link CodeAction} the response
 * is of type {@link CodeAction} or a Thenable that resolves to such.
 */
struct CodeAction_Resolve{
	static constexpr auto Method    = std::string_view("codeAction/resolve");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Params = CodeAction;
	using Result = CodeAction;
};

/*
 * codeLens/resolve
 *
 * A request to resolve a command for a given code lens.
 */
struct CodeLens_Resolve{
	static constexpr auto Method    = std::string_view("codeLens/resolve");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Params = CodeLens;
	using Result = CodeLens;
};

/*
 * completionItem/resolve
 *
 * Request to resolve additional information for a given completion item.The request's
 * parameter is of type {@link CompletionItem} the response
 * is of type {@link CompletionItem} or a Thenable that resolves to such.
 */
struct CompletionItem_Resolve{
	static constexpr auto Method    = std::string_view("completionItem/resolve");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Params = CompletionItem;
	using Result = CompletionItem;
};

/*
 * documentLink/resolve
 *
 * Request to resolve additional information for a given document link. The request's
 * parameter is of type {@link DocumentLink} the response
 * is of type {@link DocumentLink} or a Thenable that resolves to such.
 */
struct DocumentLink_Resolve{
	static constexpr auto Method    = std::string_view("documentLink/resolve");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Params = DocumentLink;
	using Result = DocumentLink;
};

/*
 * initialize
 *
 * The initialize request is sent from the client to the server.
 * It is sent once as the request after starting up the server.
 * The requests parameter is of type {@link InitializeParams}
 * the response if of type {@link InitializeResult} of a Thenable that
 * resolves to such.
 */
struct Initialize{
	static constexpr auto Method    = std::string_view("initialize");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using ErrorData = InitializeError;
	using Params = InitializeParams;
	using Result = InitializeResult;
};

/*
 * inlayHint/resolve
 *
 * A request to resolve additional properties for an inlay hint.
 * The request's parameter is of type {@link InlayHint}, the response is
 * of type {@link InlayHint} or a Thenable that resolves to such.
 * 
 * @since 3.17.0
 */
struct InlayHint_Resolve{
	static constexpr auto Method    = std::string_view("inlayHint/resolve");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Params = InlayHint;
	using Result = InlayHint;
};

/*
 * shutdown
 *
 * A shutdown request is sent from the client to the server.
 * It is sent once when the client decides to shutdown the
 * server. The only notification that is sent after a shutdown request
 * is the exit event.
 */
struct Shutdown{
	static constexpr auto Method    = std::string_view("shutdown");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Result = ShutdownResult;
};

/*
 * textDocument/codeAction
 *
 * A request to provide commands for the given text document and range.
 */
struct TextDocument_CodeAction{
	static constexpr auto Method    = std::string_view("textDocument/codeAction");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = CodeActionRegistrationOptions;
	using PartialResult = TextDocument_CodeActionPartialResult;
	using Params = CodeActionParams;
	using Result = TextDocument_CodeActionResult;
};

/*
 * textDocument/codeLens
 *
 * A request to provide code lens for the given text document.
 */
struct TextDocument_CodeLens{
	static constexpr auto Method    = std::string_view("textDocument/codeLens");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = CodeLensRegistrationOptions;
	using PartialResult = TextDocument_CodeLensPartialResult;
	using Params = CodeLensParams;
	using Result = TextDocument_CodeLensResult;
};

/*
 * textDocument/colorPresentation
 *
 * A request to list all presentation for a color. The request's
 * parameter is of type {@link ColorPresentationParams} the
 * response is of type {@link ColorInformation ColorInformation[]} or a Thenable
 * that resolves to such.
 */
struct TextDocument_ColorPresentation{
	static constexpr auto Method    = std::string_view("textDocument/colorPresentation");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = TextDocument_ColorPresentationRegistrationOptions;
	using PartialResult = TextDocument_ColorPresentationPartialResult;
	using Params = ColorPresentationParams;
	using Result = TextDocument_ColorPresentationResult;
};

/*
 * textDocument/completion
 *
 * Request to request completion at a given text document position. The request's
 * parameter is of type {@link TextDocumentPosition} the response
 * is of type {@link CompletionItem CompletionItem[]} or {@link CompletionList}
 * or a Thenable that resolves to such.
 * 
 * The request can delay the computation of the {@link CompletionItem.detail `detail`}
 * and {@link CompletionItem.documentation `documentation`} properties to the `completionItem/resolve`
 * request. However, properties that are needed for the initial sorting and filtering, like `sortText`,
 * `filterText`, `insertText`, and `textEdit`, must not be changed during resolve.
 */
struct TextDocument_Completion{
	static constexpr auto Method    = std::string_view("textDocument/completion");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = CompletionRegistrationOptions;
	using PartialResult = TextDocument_CompletionPartialResult;
	using Params = CompletionParams;
	using Result = TextDocument_CompletionResult;
};

/*
 * textDocument/declaration
 *
 * A request to resolve the type definition locations of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPositionParams}
 * the response is of type {@link Declaration} or a typed array of {@link DeclarationLink}
 * or a Thenable that resolves to such.
 */
struct TextDocument_Declaration{
	static constexpr auto Method    = std::string_view("textDocument/declaration");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DeclarationRegistrationOptions;
	using PartialResult = TextDocument_DeclarationPartialResult;
	using Params = DeclarationParams;
	using Result = TextDocument_DeclarationResult;
};

/*
 * textDocument/definition
 *
 * A request to resolve the definition location of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPosition}
 * the response is of either type {@link Definition} or a typed array of
 * {@link DefinitionLink} or a Thenable that resolves to such.
 */
struct TextDocument_Definition{
	static constexpr auto Method    = std::string_view("textDocument/definition");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DefinitionRegistrationOptions;
	using PartialResult = TextDocument_DefinitionPartialResult;
	using Params = DefinitionParams;
	using Result = TextDocument_DefinitionResult;
};

/*
 * textDocument/diagnostic
 *
 * The document diagnostic request definition.
 * 
 * @since 3.17.0
 */
struct TextDocument_Diagnostic{
	static constexpr auto Method    = std::string_view("textDocument/diagnostic");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DiagnosticRegistrationOptions;
	using PartialResult = DocumentDiagnosticReportPartialResult;
	using ErrorData = DiagnosticServerCancellationData;
	using Params = DocumentDiagnosticParams;
	using Result = DocumentDiagnosticReport;
};

/*
 * textDocument/documentColor
 *
 * A request to list all color symbols found in a given text document. The request's
 * parameter is of type {@link DocumentColorParams} the
 * response is of type {@link ColorInformation ColorInformation[]} or a Thenable
 * that resolves to such.
 */
struct TextDocument_DocumentColor{
	static constexpr auto Method    = std::string_view("textDocument/documentColor");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentColorRegistrationOptions;
	using PartialResult = TextDocument_DocumentColorPartialResult;
	using Params = DocumentColorParams;
	using Result = TextDocument_DocumentColorResult;
};

/*
 * textDocument/documentHighlight
 *
 * Request to resolve a {@link DocumentHighlight} for a given
 * text document position. The request's parameter is of type {@link TextDocumentPosition}
 * the request response is an array of type {@link DocumentHighlight}
 * or a Thenable that resolves to such.
 */
struct TextDocument_DocumentHighlight{
	static constexpr auto Method    = std::string_view("textDocument/documentHighlight");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentHighlightRegistrationOptions;
	using PartialResult = TextDocument_DocumentHighlightPartialResult;
	using Params = DocumentHighlightParams;
	using Result = TextDocument_DocumentHighlightResult;
};

/*
 * textDocument/documentLink
 *
 * A request to provide document links
 */
struct TextDocument_DocumentLink{
	static constexpr auto Method    = std::string_view("textDocument/documentLink");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentLinkRegistrationOptions;
	using PartialResult = TextDocument_DocumentLinkPartialResult;
	using Params = DocumentLinkParams;
	using Result = TextDocument_DocumentLinkResult;
};

/*
 * textDocument/documentSymbol
 *
 * A request to list all symbols found in a given text document. The request's
 * parameter is of type {@link TextDocumentIdentifier} the
 * response is of type {@link SymbolInformation SymbolInformation[]} or a Thenable
 * that resolves to such.
 */
struct TextDocument_DocumentSymbol{
	static constexpr auto Method    = std::string_view("textDocument/documentSymbol");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentSymbolRegistrationOptions;
	using PartialResult = TextDocument_DocumentSymbolPartialResult;
	using Params = DocumentSymbolParams;
	using Result = TextDocument_DocumentSymbolResult;
};

/*
 * textDocument/foldingRange
 *
 * A request to provide folding ranges in a document. The request's
 * parameter is of type {@link FoldingRangeParams}, the
 * response is of type {@link FoldingRangeList} or a Thenable
 * that resolves to such.
 */
struct TextDocument_FoldingRange{
	static constexpr auto Method    = std::string_view("textDocument/foldingRange");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = FoldingRangeRegistrationOptions;
	using PartialResult = TextDocument_FoldingRangePartialResult;
	using Params = FoldingRangeParams;
	using Result = TextDocument_FoldingRangeResult;
};

/*
 * textDocument/formatting
 *
 * A request to format a whole document.
 */
struct TextDocument_Formatting{
	static constexpr auto Method    = std::string_view("textDocument/formatting");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentFormattingRegistrationOptions;
	using Params = DocumentFormattingParams;
	using Result = TextDocument_FormattingResult;
};

/*
 * textDocument/hover
 *
 * Request to request hover information at a given text document position. The request's
 * parameter is of type {@link TextDocumentPosition} the response is of
 * type {@link Hover} or a Thenable that resolves to such.
 */
struct TextDocument_Hover{
	static constexpr auto Method    = std::string_view("textDocument/hover");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = HoverRegistrationOptions;
	using Params = HoverParams;
	using Result = TextDocument_HoverResult;
};

/*
 * textDocument/implementation
 *
 * A request to resolve the implementation locations of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPositionParams}
 * the response is of type {@link Definition} or a Thenable that resolves to such.
 */
struct TextDocument_Implementation{
	static constexpr auto Method    = std::string_view("textDocument/implementation");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = ImplementationRegistrationOptions;
	using PartialResult = TextDocument_ImplementationPartialResult;
	using Params = ImplementationParams;
	using Result = TextDocument_ImplementationResult;
};

/*
 * textDocument/inlayHint
 *
 * A request to provide inlay hints in a document. The request's parameter is of
 * type {@link InlayHintsParams}, the response is of type
 * {@link InlayHint InlayHint[]} or a Thenable that resolves to such.
 * 
 * @since 3.17.0
 */
struct TextDocument_InlayHint{
	static constexpr auto Method    = std::string_view("textDocument/inlayHint");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = InlayHintRegistrationOptions;
	using PartialResult = TextDocument_InlayHintPartialResult;
	using Params = InlayHintParams;
	using Result = TextDocument_InlayHintResult;
};

/*
 * textDocument/inlineCompletion
 *
 * A request to provide inline completions in a document. The request's parameter is of
 * type {@link InlineCompletionParams}, the response is of type
 * {@link InlineCompletion InlineCompletion[]} or a Thenable that resolves to such.
 * 
 * @since 3.18.0
 * @proposed
 */
struct TextDocument_InlineCompletion{
	static constexpr auto Method    = std::string_view("textDocument/inlineCompletion");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = InlineCompletionRegistrationOptions;
	using PartialResult = TextDocument_InlineCompletionPartialResult;
	using Params = InlineCompletionParams;
	using Result = TextDocument_InlineCompletionResult;
};

/*
 * textDocument/inlineValue
 *
 * A request to provide inline values in a document. The request's parameter is of
 * type {@link InlineValueParams}, the response is of type
 * {@link InlineValue InlineValue[]} or a Thenable that resolves to such.
 * 
 * @since 3.17.0
 */
struct TextDocument_InlineValue{
	static constexpr auto Method    = std::string_view("textDocument/inlineValue");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = InlineValueRegistrationOptions;
	using PartialResult = TextDocument_InlineValuePartialResult;
	using Params = InlineValueParams;
	using Result = TextDocument_InlineValueResult;
};

/*
 * textDocument/linkedEditingRange
 *
 * A request to provide ranges that can be edited together.
 * 
 * @since 3.16.0
 */
struct TextDocument_LinkedEditingRange{
	static constexpr auto Method    = std::string_view("textDocument/linkedEditingRange");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = LinkedEditingRangeRegistrationOptions;
	using Params = LinkedEditingRangeParams;
	using Result = TextDocument_LinkedEditingRangeResult;
};

/*
 * textDocument/moniker
 *
 * A request to get the moniker of a symbol at a given text document position.
 * The request parameter is of type {@link TextDocumentPositionParams}.
 * The response is of type {@link Moniker Moniker[]} or `null`.
 */
struct TextDocument_Moniker{
	static constexpr auto Method    = std::string_view("textDocument/moniker");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = MonikerRegistrationOptions;
	using PartialResult = TextDocument_MonikerPartialResult;
	using Params = MonikerParams;
	using Result = TextDocument_MonikerResult;
};

/*
 * textDocument/onTypeFormatting
 *
 * A request to format a document on type.
 */
struct TextDocument_OnTypeFormatting{
	static constexpr auto Method    = std::string_view("textDocument/onTypeFormatting");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentOnTypeFormattingRegistrationOptions;
	using Params = DocumentOnTypeFormattingParams;
	using Result = TextDocument_OnTypeFormattingResult;
};

/*
 * textDocument/prepareCallHierarchy
 *
 * A request to result a `CallHierarchyItem` in a document at a given position.
 * Can be used as an input to an incoming or outgoing call hierarchy.
 * 
 * @since 3.16.0
 */
struct TextDocument_PrepareCallHierarchy{
	static constexpr auto Method    = std::string_view("textDocument/prepareCallHierarchy");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = CallHierarchyRegistrationOptions;
	using Params = CallHierarchyPrepareParams;
	using Result = TextDocument_PrepareCallHierarchyResult;
};

/*
 * textDocument/prepareRename
 *
 * A request to test and perform the setup necessary for a rename.
 * 
 * @since 3.16 - support for default behavior
 */
struct TextDocument_PrepareRename{
	static constexpr auto Method    = std::string_view("textDocument/prepareRename");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Params = PrepareRenameParams;
	using Result = TextDocument_PrepareRenameResult;
};

/*
 * textDocument/prepareTypeHierarchy
 *
 * A request to result a `TypeHierarchyItem` in a document at a given position.
 * Can be used as an input to a subtypes or supertypes type hierarchy.
 * 
 * @since 3.17.0
 */
struct TextDocument_PrepareTypeHierarchy{
	static constexpr auto Method    = std::string_view("textDocument/prepareTypeHierarchy");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = TypeHierarchyRegistrationOptions;
	using Params = TypeHierarchyPrepareParams;
	using Result = TextDocument_PrepareTypeHierarchyResult;
};

/*
 * textDocument/rangeFormatting
 *
 * A request to format a range in a document.
 */
struct TextDocument_RangeFormatting{
	static constexpr auto Method    = std::string_view("textDocument/rangeFormatting");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentRangeFormattingRegistrationOptions;
	using Params = DocumentRangeFormattingParams;
	using Result = TextDocument_RangeFormattingResult;
};

/*
 * textDocument/rangesFormatting
 *
 * A request to format ranges in a document.
 * 
 * @since 3.18.0
 * @proposed
 */
struct TextDocument_RangesFormatting{
	static constexpr auto Method    = std::string_view("textDocument/rangesFormatting");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = DocumentRangeFormattingRegistrationOptions;
	using Params = DocumentRangesFormattingParams;
	using Result = TextDocument_RangesFormattingResult;
};

/*
 * textDocument/references
 *
 * A request to resolve project-wide references for the symbol denoted
 * by the given text document position. The request's parameter is of
 * type {@link ReferenceParams} the response is of type
 * {@link Location Location[]} or a Thenable that resolves to such.
 */
struct TextDocument_References{
	static constexpr auto Method    = std::string_view("textDocument/references");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = ReferenceRegistrationOptions;
	using PartialResult = TextDocument_ReferencesPartialResult;
	using Params = ReferenceParams;
	using Result = TextDocument_ReferencesResult;
};

/*
 * textDocument/rename
 *
 * A request to rename a symbol.
 */
struct TextDocument_Rename{
	static constexpr auto Method    = std::string_view("textDocument/rename");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = RenameRegistrationOptions;
	using Params = RenameParams;
	using Result = TextDocument_RenameResult;
};

/*
 * textDocument/selectionRange
 *
 * A request to provide selection ranges in a document. The request's
 * parameter is of type {@link SelectionRangeParams}, the
 * response is of type {@link SelectionRange SelectionRange[]} or a Thenable
 * that resolves to such.
 */
struct TextDocument_SelectionRange{
	static constexpr auto Method    = std::string_view("textDocument/selectionRange");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = SelectionRangeRegistrationOptions;
	using PartialResult = TextDocument_SelectionRangePartialResult;
	using Params = SelectionRangeParams;
	using Result = TextDocument_SelectionRangeResult;
};

/*
 * textDocument/semanticTokens/full
 *
 * @since 3.16.0
 */
struct TextDocument_SemanticTokens_Full{
	static constexpr auto Method    = std::string_view("textDocument/semanticTokens/full");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = SemanticTokensRegistrationOptions;
	using PartialResult = SemanticTokensPartialResult;
	using Params = SemanticTokensParams;
	using Result = TextDocument_SemanticTokens_FullResult;
};

/*
 * textDocument/semanticTokens/full/delta
 *
 * @since 3.16.0
 */
struct TextDocument_SemanticTokens_Full_Delta{
	static constexpr auto Method    = std::string_view("textDocument/semanticTokens/full/delta");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = SemanticTokensRegistrationOptions;
	using PartialResult = TextDocument_SemanticTokens_Full_DeltaPartialResult;
	using Params = SemanticTokensDeltaParams;
	using Result = TextDocument_SemanticTokens_Full_DeltaResult;
};

/*
 * textDocument/semanticTokens/range
 *
 * @since 3.16.0
 */
struct TextDocument_SemanticTokens_Range{
	static constexpr auto Method    = std::string_view("textDocument/semanticTokens/range");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using PartialResult = SemanticTokensPartialResult;
	using Params = SemanticTokensRangeParams;
	using Result = TextDocument_SemanticTokens_RangeResult;
};

/*
 * textDocument/signatureHelp
 */
struct TextDocument_SignatureHelp{
	static constexpr auto Method    = std::string_view("textDocument/signatureHelp");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = SignatureHelpRegistrationOptions;
	using Params = SignatureHelpParams;
	using Result = TextDocument_SignatureHelpResult;
};

/*
 * textDocument/typeDefinition
 *
 * A request to resolve the type definition locations of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPositionParams}
 * the response is of type {@link Definition} or a Thenable that resolves to such.
 */
struct TextDocument_TypeDefinition{
	static constexpr auto Method    = std::string_view("textDocument/typeDefinition");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = TypeDefinitionRegistrationOptions;
	using PartialResult = TextDocument_TypeDefinitionPartialResult;
	using Params = TypeDefinitionParams;
	using Result = TextDocument_TypeDefinitionResult;
};

/*
 * textDocument/willSaveWaitUntil
 *
 * A document will save request is sent from the client to the server before
 * the document is actually saved. The request can return an array of TextEdits
 * which will be applied to the text document before it is saved. Please note that
 * clients might drop results if computing the text edits took too long or if a
 * server constantly fails on this request. This is done to keep the save fast and
 * reliable.
 */
struct TextDocument_WillSaveWaitUntil{
	static constexpr auto Method    = std::string_view("textDocument/willSaveWaitUntil");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = TextDocumentRegistrationOptions;
	using Params = WillSaveTextDocumentParams;
	using Result = TextDocument_WillSaveWaitUntilResult;
};

/*
 * typeHierarchy/subtypes
 *
 * A request to resolve the subtypes for a given `TypeHierarchyItem`.
 * 
 * @since 3.17.0
 */
struct TypeHierarchy_Subtypes{
	static constexpr auto Method    = std::string_view("typeHierarchy/subtypes");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using PartialResult = TypeHierarchy_SubtypesPartialResult;
	using Params = TypeHierarchySubtypesParams;
	using Result = TypeHierarchy_SubtypesResult;
};

/*
 * typeHierarchy/supertypes
 *
 * A request to resolve the supertypes for a given `TypeHierarchyItem`.
 * 
 * @since 3.17.0
 */
struct TypeHierarchy_Supertypes{
	static constexpr auto Method    = std::string_view("typeHierarchy/supertypes");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using PartialResult = TypeHierarchy_SupertypesPartialResult;
	using Params = TypeHierarchySupertypesParams;
	using Result = TypeHierarchy_SupertypesResult;
};

/*
 * window/showDocument
 *
 * A request to show a document. This request might open an
 * external program depending on the value of the URI to open.
 * For example a request to open `https://code.visualstudio.com/`
 * will very likely open the URI in a WEB browser.
 * 
 * @since 3.16.0
 */
struct Window_ShowDocument{
	static constexpr auto Method    = std::string_view("window/showDocument");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Params = ShowDocumentParams;
	using Result = ShowDocumentResult;
};

/*
 * window/showMessageRequest
 *
 * The show message request is sent from the server to the client to show a message
 * and a set of options actions to the user.
 */
struct Window_ShowMessageRequest{
	static constexpr auto Method    = std::string_view("window/showMessageRequest");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Params = ShowMessageRequestParams;
	using Result = Window_ShowMessageRequestResult;
};

/*
 * window/workDoneProgress/create
 *
 * The `window/workDoneProgress/create` request is sent from the server to the client to initiate progress
 * reporting from the server.
 */
struct Window_WorkDoneProgress_Create{
	static constexpr auto Method    = std::string_view("window/workDoneProgress/create");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Params = WorkDoneProgressCreateParams;
	using Result = Window_WorkDoneProgress_CreateResult;
};

/*
 * workspace/applyEdit
 *
 * A request sent from the server to the client to modified certain resources.
 */
struct Workspace_ApplyEdit{
	static constexpr auto Method    = std::string_view("workspace/applyEdit");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Params = ApplyWorkspaceEditParams;
	using Result = ApplyWorkspaceEditResult;
};

/*
 * workspace/codeLens/refresh
 *
 * A request to refresh all code actions
 * 
 * @since 3.16.0
 */
struct Workspace_CodeLens_Refresh{
	static constexpr auto Method    = std::string_view("workspace/codeLens/refresh");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Result = Workspace_CodeLens_RefreshResult;
};

/*
 * workspace/configuration
 *
 * The 'workspace/configuration' request is sent from the server to the client to fetch a certain
 * configuration setting.
 * 
 * This pull model replaces the old push model where the client signaled configuration change via an
 * event. If the server still needs to react to configuration changes (since the server caches the
 * result of `workspace/configuration` requests) the server should register for an empty configuration
 * change event and empty the cache if such an event is received.
 */
struct Workspace_Configuration{
	static constexpr auto Method    = std::string_view("workspace/configuration");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Params = ConfigurationParams;
	using Result = Workspace_ConfigurationResult;
};

/*
 * workspace/diagnostic
 *
 * The workspace diagnostic request definition.
 * 
 * @since 3.17.0
 */
struct Workspace_Diagnostic{
	static constexpr auto Method    = std::string_view("workspace/diagnostic");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using PartialResult = WorkspaceDiagnosticReportPartialResult;
	using ErrorData = DiagnosticServerCancellationData;
	using Params = WorkspaceDiagnosticParams;
	using Result = WorkspaceDiagnosticReport;
};

/*
 * workspace/diagnostic/refresh
 *
 * The diagnostic refresh request definition.
 * 
 * @since 3.17.0
 */
struct Workspace_Diagnostic_Refresh{
	static constexpr auto Method    = std::string_view("workspace/diagnostic/refresh");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Result = Workspace_Diagnostic_RefreshResult;
};

/*
 * workspace/executeCommand
 *
 * A request send from the client to the server to execute a command. The request might return
 * a workspace edit which the client will apply to the workspace.
 */
struct Workspace_ExecuteCommand{
	static constexpr auto Method    = std::string_view("workspace/executeCommand");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = ExecuteCommandRegistrationOptions;
	using Params = ExecuteCommandParams;
	using Result = Workspace_ExecuteCommandResult;
};

/*
 * workspace/foldingRange/refresh
 *
 * @since 3.18.0
 * @proposed
 */
struct Workspace_FoldingRange_Refresh{
	static constexpr auto Method    = std::string_view("workspace/foldingRange/refresh");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Result = Workspace_FoldingRange_RefreshResult;
};

/*
 * workspace/inlayHint/refresh
 *
 * @since 3.17.0
 */
struct Workspace_InlayHint_Refresh{
	static constexpr auto Method    = std::string_view("workspace/inlayHint/refresh");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Result = Workspace_InlayHint_RefreshResult;
};

/*
 * workspace/inlineValue/refresh
 *
 * @since 3.17.0
 */
struct Workspace_InlineValue_Refresh{
	static constexpr auto Method    = std::string_view("workspace/inlineValue/refresh");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Result = Workspace_InlineValue_RefreshResult;
};

/*
 * workspace/semanticTokens/refresh
 *
 * @since 3.16.0
 */
struct Workspace_SemanticTokens_Refresh{
	static constexpr auto Method    = std::string_view("workspace/semanticTokens/refresh");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Result = Workspace_SemanticTokens_RefreshResult;
};

/*
 * workspace/symbol
 *
 * A request to list project-wide symbols matching the query string given
 * by the {@link WorkspaceSymbolParams}. The response is
 * of type {@link SymbolInformation SymbolInformation[]} or a Thenable that
 * resolves to such.
 * 
 * @since 3.17.0 - support for WorkspaceSymbol in the returned data. Clients
 *  need to advertise support for WorkspaceSymbols via the client capability
 *  `workspace.symbol.resolveSupport`.
 */
struct Workspace_Symbol{
	static constexpr auto Method    = std::string_view("workspace/symbol");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = WorkspaceSymbolRegistrationOptions;
	using PartialResult = Workspace_SymbolPartialResult;
	using Params = WorkspaceSymbolParams;
	using Result = Workspace_SymbolResult;
};

/*
 * workspace/willCreateFiles
 *
 * The will create files request is sent from the client to the server before files are actually
 * created as long as the creation is triggered from within the client.
 * 
 * The request can return a `WorkspaceEdit` which will be applied to workspace before the
 * files are created. Hence the `WorkspaceEdit` can not manipulate the content of the file
 * to be created.
 * 
 * @since 3.16.0
 */
struct Workspace_WillCreateFiles{
	static constexpr auto Method    = std::string_view("workspace/willCreateFiles");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = FileOperationRegistrationOptions;
	using Params = CreateFilesParams;
	using Result = Workspace_WillCreateFilesResult;
};

/*
 * workspace/willDeleteFiles
 *
 * The did delete files notification is sent from the client to the server when
 * files were deleted from within the client.
 * 
 * @since 3.16.0
 */
struct Workspace_WillDeleteFiles{
	static constexpr auto Method    = std::string_view("workspace/willDeleteFiles");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = FileOperationRegistrationOptions;
	using Params = DeleteFilesParams;
	using Result = Workspace_WillDeleteFilesResult;
};

/*
 * workspace/willRenameFiles
 *
 * The will rename files request is sent from the client to the server before files are actually
 * renamed as long as the rename is triggered from within the client.
 * 
 * @since 3.16.0
 */
struct Workspace_WillRenameFiles{
	static constexpr auto Method    = std::string_view("workspace/willRenameFiles");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using RegistrationOptions = FileOperationRegistrationOptions;
	using Params = RenameFilesParams;
	using Result = Workspace_WillRenameFilesResult;
};

/*
 * workspace/workspaceFolders
 *
 * The `workspace/workspaceFolders` is sent from the server to the client to fetch the open workspace folders.
 */
struct Workspace_WorkspaceFolders{
	static constexpr auto Method    = std::string_view("workspace/workspaceFolders");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Request;

	using Result = Workspace_WorkspaceFoldersResult;
};

/*
 * workspaceSymbol/resolve
 *
 * A request to resolve the range inside the workspace
 * symbol's location.
 * 
 * @since 3.17.0
 */
struct WorkspaceSymbol_Resolve{
	static constexpr auto Method    = std::string_view("workspaceSymbol/resolve");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Request;

	using Params = WorkspaceSymbol;
	using Result = WorkspaceSymbol;
};

} // namespace requests

/*
 * Notification messages
 */
namespace notifications{

/*
 * $/cancelRequest
 */
struct CancelRequest{
	static constexpr auto Method    = std::string_view("$/cancelRequest");
	static constexpr auto Direction = MessageDirection::Bidirectional;
	static constexpr auto Type      = Message::Notification;

	using Params = CancelParams;
};

/*
 * $/logTrace
 */
struct LogTrace{
	static constexpr auto Method    = std::string_view("$/logTrace");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Notification;

	using Params = LogTraceParams;
};

/*
 * $/progress
 */
struct Progress{
	static constexpr auto Method    = std::string_view("$/progress");
	static constexpr auto Direction = MessageDirection::Bidirectional;
	static constexpr auto Type      = Message::Notification;

	using Params = ProgressParams;
};

/*
 * $/setTrace
 */
struct SetTrace{
	static constexpr auto Method    = std::string_view("$/setTrace");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = SetTraceParams;
};

/*
 * exit
 *
 * The exit event is sent from the client to the server to
 * ask the server to exit its process.
 */
struct Exit{
	static constexpr auto Method    = std::string_view("exit");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;
};

/*
 * initialized
 *
 * The initialized notification is sent from the client to the
 * server after the client is fully initialized and the server
 * is allowed to send requests from the server to the client.
 */
struct Initialized{
	static constexpr auto Method    = std::string_view("initialized");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = InitializedParams;
};

/*
 * notebookDocument/didChange
 */
struct NotebookDocument_DidChange{
	static constexpr auto Method    = std::string_view("notebookDocument/didChange");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = DidChangeNotebookDocumentParams;
};

/*
 * notebookDocument/didClose
 *
 * A notification sent when a notebook closes.
 * 
 * @since 3.17.0
 */
struct NotebookDocument_DidClose{
	static constexpr auto Method    = std::string_view("notebookDocument/didClose");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = DidCloseNotebookDocumentParams;
};

/*
 * notebookDocument/didOpen
 *
 * A notification sent when a notebook opens.
 * 
 * @since 3.17.0
 */
struct NotebookDocument_DidOpen{
	static constexpr auto Method    = std::string_view("notebookDocument/didOpen");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = DidOpenNotebookDocumentParams;
};

/*
 * notebookDocument/didSave
 *
 * A notification sent when a notebook document is saved.
 * 
 * @since 3.17.0
 */
struct NotebookDocument_DidSave{
	static constexpr auto Method    = std::string_view("notebookDocument/didSave");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = DidSaveNotebookDocumentParams;
};

/*
 * telemetry/event
 *
 * The telemetry event notification is sent from the server to the client to ask
 * the client to log telemetry data.
 */
struct Telemetry_Event{
	static constexpr auto Method    = std::string_view("telemetry/event");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Notification;

	using Params = LSPAny;
};

/*
 * textDocument/didChange
 *
 * The document change notification is sent from the client to the server to signal
 * changes to a text document.
 */
struct TextDocument_DidChange{
	static constexpr auto Method    = std::string_view("textDocument/didChange");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = TextDocumentChangeRegistrationOptions;
	using Params = DidChangeTextDocumentParams;
};

/*
 * textDocument/didClose
 *
 * The document close notification is sent from the client to the server when
 * the document got closed in the client. The document's truth now exists where
 * the document's uri points to (e.g. if the document's uri is a file uri the
 * truth now exists on disk). As with the open notification the close notification
 * is about managing the document's content. Receiving a close notification
 * doesn't mean that the document was open in an editor before. A close
 * notification requires a previous open notification to be sent.
 */
struct TextDocument_DidClose{
	static constexpr auto Method    = std::string_view("textDocument/didClose");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = TextDocumentRegistrationOptions;
	using Params = DidCloseTextDocumentParams;
};

/*
 * textDocument/didOpen
 *
 * The document open notification is sent from the client to the server to signal
 * newly opened text documents. The document's truth is now managed by the client
 * and the server must not try to read the document's truth using the document's
 * uri. Open in this sense means it is managed by the client. It doesn't necessarily
 * mean that its content is presented in an editor. An open notification must not
 * be sent more than once without a corresponding close notification send before.
 * This means open and close notification must be balanced and the max open count
 * is one.
 */
struct TextDocument_DidOpen{
	static constexpr auto Method    = std::string_view("textDocument/didOpen");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = TextDocumentRegistrationOptions;
	using Params = DidOpenTextDocumentParams;
};

/*
 * textDocument/didSave
 *
 * The document save notification is sent from the client to the server when
 * the document got saved in the client.
 */
struct TextDocument_DidSave{
	static constexpr auto Method    = std::string_view("textDocument/didSave");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = TextDocumentSaveRegistrationOptions;
	using Params = DidSaveTextDocumentParams;
};

/*
 * textDocument/publishDiagnostics
 *
 * Diagnostics notification are sent from the server to the client to signal
 * results of validation runs.
 */
struct TextDocument_PublishDiagnostics{
	static constexpr auto Method    = std::string_view("textDocument/publishDiagnostics");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Notification;

	using Params = PublishDiagnosticsParams;
};

/*
 * textDocument/willSave
 *
 * A document will save notification is sent from the client to the server before
 * the document is actually saved.
 */
struct TextDocument_WillSave{
	static constexpr auto Method    = std::string_view("textDocument/willSave");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = TextDocumentRegistrationOptions;
	using Params = WillSaveTextDocumentParams;
};

/*
 * window/logMessage
 *
 * The log message notification is sent from the server to the client to ask
 * the client to log a particular message.
 */
struct Window_LogMessage{
	static constexpr auto Method    = std::string_view("window/logMessage");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Notification;

	using Params = LogMessageParams;
};

/*
 * window/showMessage
 *
 * The show message notification is sent from a server to a client to ask
 * the client to display a particular message in the user interface.
 */
struct Window_ShowMessage{
	static constexpr auto Method    = std::string_view("window/showMessage");
	static constexpr auto Direction = MessageDirection::ServerToClient;
	static constexpr auto Type      = Message::Notification;

	using Params = ShowMessageParams;
};

/*
 * window/workDoneProgress/cancel
 *
 * The `window/workDoneProgress/cancel` notification is sent from  the client to the server to cancel a progress
 * initiated on the server side.
 */
struct Window_WorkDoneProgress_Cancel{
	static constexpr auto Method    = std::string_view("window/workDoneProgress/cancel");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = WorkDoneProgressCancelParams;
};

/*
 * workspace/didChangeConfiguration
 *
 * The configuration change notification is sent from the client to the server
 * when the client's configuration has changed. The notification contains
 * the changed configuration as defined by the language client.
 */
struct Workspace_DidChangeConfiguration{
	static constexpr auto Method    = std::string_view("workspace/didChangeConfiguration");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = DidChangeConfigurationRegistrationOptions;
	using Params = DidChangeConfigurationParams;
};

/*
 * workspace/didChangeWatchedFiles
 *
 * The watched files notification is sent from the client to the server when
 * the client detects changes to file watched by the language client.
 */
struct Workspace_DidChangeWatchedFiles{
	static constexpr auto Method    = std::string_view("workspace/didChangeWatchedFiles");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = DidChangeWatchedFilesRegistrationOptions;
	using Params = DidChangeWatchedFilesParams;
};

/*
 * workspace/didChangeWorkspaceFolders
 *
 * The `workspace/didChangeWorkspaceFolders` notification is sent from the client to the server when the workspace
 * folder configuration changes.
 */
struct Workspace_DidChangeWorkspaceFolders{
	static constexpr auto Method    = std::string_view("workspace/didChangeWorkspaceFolders");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using Params = DidChangeWorkspaceFoldersParams;
};

/*
 * workspace/didCreateFiles
 *
 * The did create files notification is sent from the client to the server when
 * files were created from within the client.
 * 
 * @since 3.16.0
 */
struct Workspace_DidCreateFiles{
	static constexpr auto Method    = std::string_view("workspace/didCreateFiles");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = FileOperationRegistrationOptions;
	using Params = CreateFilesParams;
};

/*
 * workspace/didDeleteFiles
 *
 * The will delete files request is sent from the client to the server before files are actually
 * deleted as long as the deletion is triggered from within the client.
 * 
 * @since 3.16.0
 */
struct Workspace_DidDeleteFiles{
	static constexpr auto Method    = std::string_view("workspace/didDeleteFiles");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = FileOperationRegistrationOptions;
	using Params = DeleteFilesParams;
};

/*
 * workspace/didRenameFiles
 *
 * The did rename files notification is sent from the client to the server when
 * files were renamed from within the client.
 * 
 * @since 3.16.0
 */
struct Workspace_DidRenameFiles{
	static constexpr auto Method    = std::string_view("workspace/didRenameFiles");
	static constexpr auto Direction = MessageDirection::ClientToServer;
	static constexpr auto Type      = Message::Notification;

	using RegistrationOptions = FileOperationRegistrationOptions;
	using Params = RenameFilesParams;
};

} // namespace notifications
} // namespace lsp
